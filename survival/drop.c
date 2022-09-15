#include "include/drop.h"
#include "include/mob.h"
#include "include/utils.h"
#include "include/game.h"
#include <string.h>
#include <libdl/stdio.h>
#include <libdl/game.h>
#include <libdl/color.h>
#include <libdl/collision.h>
#include <libdl/moby.h>
#include <libdl/ui.h>
#include <libdl/graphics.h>
#include <libdl/random.h>
#include <libdl/radar.h>

extern struct SurvivalState State;
extern SoundDef BaseSoundDef;

int dropCount = 0;
int dropThisFrame = 0;

GuberEvent* dropCreateEvent(Moby* moby, u32 eventType);

char DropTexIds[] = {
	[DROP_AMMO] 90,
	[DROP_HEALTH] 94,
	[DROP_NUKE] 19,
	[DROP_FREEZE] 43,
	[DROP_DOUBLE_POINTS] 3,
};

//--------------------------------------------------------------------------
int dropAmIOwner(Moby* moby)
{
	struct DropPVar* pvars = (struct DropPVar**)moby->PVar;
	return gameGetMyClientId() == pvars->Owner;
}

int aaa = 0;

//--------------------------------------------------------------------------
void dropPlayPickupSound(Moby* moby)
{
	BaseSoundDef.Index = 101;
	soundPlay(&BaseSoundDef, 0, moby, 0, 0x400);
}	

//--------------------------------------------------------------------------
void dropDestroy(Moby* moby)
{
	struct DropPVar* pvars = (struct DropPVar*)moby->PVar;

	// create event
	GuberEvent * guberEvent = dropCreateEvent(moby, DROP_EVENT_DESTROY);
}

//--------------------------------------------------------------------------
void dropPickup(Moby* moby, int pickedUpByPlayerId)
{
	struct DropPVar* pvars = (struct DropPVar*)moby->PVar;

	// create event
	GuberEvent * guberEvent = dropCreateEvent(moby, DROP_EVENT_PICKUP);
	if (guberEvent) {
		guberEventWrite(guberEvent, &pickedUpByPlayerId, sizeof(int));
	}
}

//--------------------------------------------------------------------------
void dropPostDraw(Moby* moby)
{
	struct QuadDef quad;
	MATRIX m2;
	VECTOR t;
	VECTOR pTL = {0.5,0,0.5,1};
	VECTOR pTR = {-0.5,0,0.5,1};
	VECTOR pBL = {0.5,0,-0.5,1};
	VECTOR pBR = {-0.5,0,-0.5,1};
	struct DropPVar* pvars = (struct DropPVar*)moby->PVar;
	if (!pvars)
		return;

	// determine color
	u32 color = 0x70FFFFFF;

	// fade as we approach destruction
	int timeUntilDestruction = (pvars->DestroyAtTime - gameGetTime()) / TIME_SECOND;
	if (timeUntilDestruction < 1)
		timeUntilDestruction = 1;

	if (timeUntilDestruction < 10) {
		float speed = timeUntilDestruction < 3 ? 20.0 : 3.0;
		float pulse = (1 + sinf((gameGetTime() / 1000.0) * speed)) * 0.5;
		int opacity = 32 + (pulse * 96);
		//color = (opacity << 24) | (color & 0xFFFFFF);
	}

	// set draw args
	matrix_unit(m2);

	// init
  gfxResetQuad(&quad);

	// color of each corner?
	vector_copy(quad.VertexPositions[0], pTL);
	vector_copy(quad.VertexPositions[1], pTR);
	vector_copy(quad.VertexPositions[2], pBL);
	vector_copy(quad.VertexPositions[3], pBR);
	quad.VertexColors[0] = quad.VertexColors[1] = quad.VertexColors[2] = quad.VertexColors[3] = color;
  quad.VertexUVs[0] = (struct UV){0,0};
  quad.VertexUVs[1] = (struct UV){1,0};
  quad.VertexUVs[2] = (struct UV){0,1};
  quad.VertexUVs[3] = (struct UV){1,1};
	quad.Clamp = 1;
	quad.Tex0 = gfxGetFrameTex(DropTexIds[pvars->Type]);
	quad.Tex1 = 0xFF9000000260;
	quad.Alpha = 0x8000000044;

	GameCamera* camera = cameraGetGameCamera(0);
	if (!camera)
		return;

	// get forward vector
	vector_subtract(t, camera->pos, moby->Position);
	t[2] = 0;
	vector_normalize(&m2[4], t);

	// up vector
	m2[8 + 2] = 1;

	// right vector
	vector_outerproduct(&m2[0], &m2[4], &m2[8]);

	// position
	memcpy(&m2[12], moby->Position, sizeof(VECTOR));

	// draw
	gfxDrawQuad((void*)0x00222590, &quad, m2, 1);
}

struct PartInstance * spawnParticle(VECTOR position, u32 color, char opacity, int idx)
{
	u32 a3 = *(u32*)0x002218E8;
	u32 t0 = *(u32*)0x002218E4;
	float f12 = *(float*)0x002218DC;
	float f1 = *(float*)0x002218E0;

	return ((struct PartInstance* (*)(VECTOR, u32, char, u32, u32, int, int, int, float))0x00533308)(position, color, opacity, a3, t0, -1, 0, 0, f12 + (f1 * idx));
}

void destroyParticle(struct PartInstance* particle)
{
	((void (*)(struct PartInstance*))0x005284d8)(particle);
}

//--------------------------------------------------------------------------
void dropUpdate(Moby* moby)
{
	const float rotSpeeds[] = { 0.05, 0.02, -0.03, -0.1 };
	const int opacities[] = { 64, 32, 44, 51 };

	VECTOR t;
	int i;
	struct DropPVar* pvars = (struct DropPVar*)moby->PVar;
	GameOptions* gameOptions = gameGetOptions();
	Player** players = playerGetAll();
	if (!pvars)
		return;

	int isOwner = dropAmIOwner(moby);

	// register draw event
	gfxRegisterDrawFunction((void**)0x0022251C, &dropPostDraw, moby);

	// handle particles
	u32 color = colorLerp(0, TEAM_COLORS[pvars->Team], 1.0 / 4);
	color |= 0x40000000;
	for (i = 0; i < 4; ++i) {
		struct PartInstance * particle = pvars->Particles[i];
		if (!particle) {
			pvars->Particles[i] = particle = spawnParticle(moby->Position, color, opacities[i], i);
		}

		// update
		if (particle) {
			particle->Rot = (int)((gameGetTime() + (i * 100)) / (TIME_SECOND * rotSpeeds[i])) & 0xFF;
		}
	}

	if (!isOwner)
		return;

	// handle pickup
	for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
		Player * player = players[i];
		if (player && pvars->Team == player->Team) {
			vector_subtract(t, player->PlayerPosition, moby->Position);
			if (vector_sqrmag(t) < (DROP_PICKUP_RADIUS * DROP_PICKUP_RADIUS)) {
				dropPickup(moby, i);
				break;
			}
		}
	}

	// handle auto destruct
	if (gameGetTime() > pvars->DestroyAtTime) {
		//dropDestroy(moby);
	}
}

//--------------------------------------------------------------------------
struct GuberMoby* dropGetGuber(Moby* moby)
{
	if (moby->OClass == DROP_MOBY_OCLASS && moby->PVar)
		return moby->GuberMoby;
	
	return 0;
}

//--------------------------------------------------------------------------
GuberEvent* dropCreateEvent(Moby* moby, u32 eventType)
{
	GuberEvent * event = NULL;

	// create guber object
	Guber* guber = guberGetObjectByMoby(moby);
	if (guber)
		event = guberEventCreateEvent(guber, eventType, 0, 0);

	return event;
}

//--------------------------------------------------------------------------
int dropHandleEvent_Spawn(Moby* moby, GuberEvent* event)
{
	VECTOR p;
	int i;
	struct DropSpawnEventArgs args;
	VECTOR offset = {0,0,1.5,0};

	// read event
	guberEventRead(event, p, 12);
	guberEventRead(event, &args, sizeof(struct DropSpawnEventArgs));

	// set position
	vector_add(moby->Position, p, offset);

	// set update
	moby->PUpdate = &dropUpdate;

	// 
	//moby->ModeBits |= 0x30;
	//moby->GlowRGBA = MobSecondaryColors[(int)args.MobType];
	//moby->PrimaryColor = MobPrimaryColors[(int)args.MobType];
	moby->CollData = NULL;
	moby->DrawDist = 0;
	//moby->PClass = NULL;

	// update pvars
	struct DropPVar* pvars = (struct DropPVar*)moby->PVar;
	pvars->Type = args.Type;
	pvars->Team = args.Team;
	pvars->DestroyAtTime = args.DestroyAtTime;
	pvars->Owner = args.Owner;
	memset(pvars->Particles, 0, sizeof(pvars->Particles));

	// set team
	Guber* guber = guberGetObjectByMoby(moby);
	if (guber)
		((GuberMoby*)guber)->TeamNum = args.Team;
	
	// 
	++dropCount;

	// 
	mobySetState(moby, 0, -1);
	printf("drop spawned at %08X type:%d team:%d destroyAt:%d\n", (u32)moby, pvars->Type, pvars->Team, pvars->DestroyAtTime);
	return 0;
}

//--------------------------------------------------------------------------
int dropHandleEvent_Destroy(Moby* moby, GuberEvent* event)
{
	char killedByPlayerId, weaponId;
	int i;
	struct DropPVar* pvars = (struct DropPVar*)moby->PVar;
	if (!pvars)
		return 0;

	// destroy particles
	for (i = 0; i < 4; ++i) {
		if (pvars->Particles[i]) {
			destroyParticle(pvars->Particles[i]);
			pvars->Particles[i] = 0;
		}
	}

	guberMobyDestroy(moby);
	--dropCount;
	return 0;
}

//--------------------------------------------------------------------------
int dropHandleEvent_Pickup(Moby* moby, GuberEvent* event)
{
	struct DropPickupEventArgs args;
	int i,j;
	Player** players = playerGetAll();
	struct DropPVar* pvars = (struct DropPVar*)moby->PVar;

	if (!pvars)
		return 0;

	// read event
	guberEventRead(event, &args, sizeof(struct DropPickupEventArgs));

	// handle effect
	switch (pvars->Type)
	{
		case DROP_AMMO:
		{
			DPRINTF("giving ammo to all players on %d team\n", pvars->Team);
			for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
				Player * p = players[i];
				if (p && p->Team == pvars->Team) {
					for (j = 0; j <= 8; ++j) {
						int gadgetId = weaponSlotToId(j);
						if (p->GadgetBox->Gadgets[gadgetId].Level >= 0)
							p->GadgetBox->Gadgets[gadgetId].Ammo = playerGetWeaponAmmo(p, gadgetId);
					}

					if (p->IsLocal)
						uiShowPopup(p->LocalPlayerIndex, "You got ammo!");
				}
			}
			break;
		}
		case DROP_HEALTH:
		{
			DPRINTF("giving health to all players on %d team\n", pvars->Team);
			for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
				Player * p = players[i];
				if (p && p->Team == pvars->Team) {
					if (!playerIsDead(p)) {
						playerSetHealth(p, 50);
					}
					else if (State.PlayerStates[i].ReviveCooldownTicks) {
						playerRevive(p, args.PickedUpByPlayerId);
					}

					if (p->IsLocal)
						uiShowPopup(p->LocalPlayerIndex, "You got health!");
				}
			}
			break;
		}
		case DROP_DOUBLE_POINTS:
		{
			DPRINTF("giving double points to all players on %d team\n", pvars->Team);
			setDoublePointsForTeam(pvars->Team, 1);
			break;
		}
		case DROP_FREEZE:
		{
			DPRINTF("freezing all mobs\n");
			uiShowPopup(0, "Freeze activated!");
			uiShowPopup(1, "Freeze activated!");
			setFreeze(1);
			break;
		}
		case DROP_NUKE:
		{
			DPRINTF("killing all mobs\n");
			uiShowPopup(0, "Nuke activated!");
			uiShowPopup(1, "Nuke activated!");
			mobNuke(args.PickedUpByPlayerId);
			break;
		}
	}

	// destroy particles
	for (i = 0; i < 4; ++i) {
		if (pvars->Particles[i]) {
			destroyParticle(pvars->Particles[i]);
			pvars->Particles[i] = 0;
		}
	}

	// play pickup sound
	dropPlayPickupSound(moby);

	guberMobyDestroy(moby);
	--dropCount;
	return 0;
}

//--------------------------------------------------------------------------
int dropHandleEvent(Moby* moby, GuberEvent* event)
{
	struct DropPVar* pvars = (struct DropPVar*)moby->PVar;

	if (isInGame() && !mobyIsDestroyed(moby) && moby->OClass == DROP_MOBY_OCLASS && pvars) {
		u32 dropEvent = event->NetEvent.EventID;
		int isFromHost = gameIsHost(event->NetEvent.OriginClientIdx);
		if (!isFromHost)
		{
			DPRINTF("ignoring drop event %d from %d (not owner, %d)\n", dropEvent, event->NetEvent.OriginClientIdx, pvars->Owner);
			return 0;
		}

		switch (dropEvent)
		{
			case DROP_EVENT_SPAWN: return dropHandleEvent_Spawn(moby, event);
			case DROP_EVENT_DESTROY: return dropHandleEvent_Destroy(moby, event);
			case DROP_EVENT_PICKUP: return dropHandleEvent_Pickup(moby, event);
			default:
			{
				DPRINTF("unhandle drop event %d\n", dropEvent);
				break;
			}
		}
	}

	return 0;
}

//--------------------------------------------------------------------------
int dropCreate(VECTOR position, enum DropType dropType, int destroyAtTime, int team)
{
	if (State.DropCooldownTicks > 0)
		return 0;

	GameSettings* gs = gameGetSettings();
	struct DropSpawnEventArgs args;

	// set cooldown
	State.DropCooldownTicks = randRangeInt(DROP_COOLDOWN_TICKS_MIN, DROP_COOLDOWN_TICKS_MAX);

	// create guber object
	GuberEvent * guberEvent = 0;
	guberMobyCreateSpawned(DROP_MOBY_OCLASS, sizeof(struct DropPVar), &guberEvent, NULL);
	if (guberEvent)
	{
		// owner is always host
		args.Owner = gameGetHostId();
		args.Type = dropType;
		args.DestroyAtTime = destroyAtTime;
		args.Team = team;
		
		guberEventWrite(guberEvent, position, 12);
		guberEventWrite(guberEvent, &args, sizeof(struct DropSpawnEventArgs));
		dropThisFrame = 1;
	}
	else
	{
		DPRINTF("failed to guberevent drop\n");
	}
  
  return guberEvent != NULL;
}

//--------------------------------------------------------------------------
void dropInitialize(void)
{
	Moby* testMoby = mobySpawn(DROP_MOBY_OCLASS, 0);
	if (testMoby) {
		u32 mobyFunctionsPtr = (u32)mobyGetFunctions(testMoby);
		if (mobyFunctionsPtr) {
			// set vtable callbacks
			*(u32*)(mobyFunctionsPtr + 0x04) = (u32)&dropGetGuber;
			*(u32*)(mobyFunctionsPtr + 0x14) = (u32)&dropHandleEvent;
		}

		mobyDestroy(testMoby);
	}
}

//--------------------------------------------------------------------------
void dropTick(void)
{
	if (padGetButtonDown(0, PAD_LEFT) > 0) {
		--aaa;
		DPRINTF("%d\n", aaa);
	}
	else if (padGetButtonDown(0, PAD_RIGHT) > 0) {
		++aaa;
		DPRINTF("%d\n", aaa);
	}

	dropThisFrame = 0;

	if (State.DropCooldownTicks > 0)
		--State.DropCooldownTicks;
}