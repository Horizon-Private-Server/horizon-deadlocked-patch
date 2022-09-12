#include "include/drop.h"
#include "include/mob.h"
#include "include/utils.h"
#include "include/game.h"
#include <string.h>
#include <libdl/stdio.h>
#include <libdl/game.h>
#include <libdl/collision.h>
#include <libdl/moby.h>
#include <libdl/ui.h>
#include <libdl/graphics.h>
#include <libdl/random.h>
#include <libdl/radar.h>

extern struct SurvivalState State;

int dropCount = 0;
int dropThisFrame = 0;

GuberEvent* dropCreateEvent(Moby* moby, u32 eventType);

char DropTexIds[] = {
	[DROP_AMMO] 90,
	[DROP_HEALTH] 94,
	[DROP_NUKE] 96,
	[DROP_FREEZE] 43,
	[DROP_DOUBLE_POINTS] 3,
};

u32 DropColors[] = {
	[DROP_AMMO] 0x70FFFFFF,
	[DROP_HEALTH] 0x708080FF,
	[DROP_NUKE] 0x8040C0C0,
	[DROP_FREEZE] 0x80FF8080,
	[DROP_DOUBLE_POINTS] 0x8040C040,
};

//--------------------------------------------------------------------------
int dropAmIOwner(Moby* moby)
{
	struct DropPVar* pvars = (struct DropPVar**)moby->PVar;
	return gameGetMyClientId() == pvars->Owner;
}

/*
void mobPlayHitSound(Moby* moby)
{
	BaseSoundDef.Index = 0x17D;
	soundPlay(&BaseSoundDef, 0, moby, 0, 0x400);
}	

void mobPlayAmbientSound(Moby* moby)
{
	BaseSoundDef.Index = AmbientSoundIds[rand(AmbientSoundIdsCount)];
	soundPlay(&BaseSoundDef, 0, moby, 0, 0x400);
}

void mobPlayDeathSound(Moby* moby)
{
	BaseSoundDef.Index = 0x171;
	soundPlay(&BaseSoundDef, 0, moby, 0, 0x400);
}
*/

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
	struct DropPVar* pvars = (struct DropPVar*)moby->PVar;
	if (!pvars)
		return;

	// determine color
	u32 color = DropColors[pvars->Type];

	// fade as we approach destruction
	int timeUntilDestruction = (pvars->DestroyAtTime - gameGetTime()) / TIME_SECOND;
	if (timeUntilDestruction < 1)
		timeUntilDestruction = 1;

	if (timeUntilDestruction < 10) {
		float speed = timeUntilDestruction < 3 ? 20.0 : 3.0;
		float pulse = (1 + sinf((gameGetTime() / 1000.0) * speed)) * 0.5;
		int opacity = 32 + (pulse * 96);
		color = (opacity << 24) | (color & 0xFFFFFF);
	}

	// set draw args
	matrix_unit(m2);

	// init
  gfxResetQuad(&quad);

	// color of each corner?
	quad.VertexColors[0] = quad.VertexColors[1] = quad.VertexColors[2] = quad.VertexColors[3] = color;
  quad.VertexUVs[0] = (struct UV){0,0};
  quad.VertexUVs[1] = (struct UV){1,0};
  quad.VertexUVs[2] = (struct UV){0,1};
  quad.VertexUVs[3] = (struct UV){1,1};
	quad.Clamp = 0;
	quad.Tex0 = gfxGetFrameTex(DropTexIds[pvars->Type]);
	quad.Tex1 = 0xFF9000000260;
	quad.Alpha = 0x8000000044;

	GameCamera* camera = cameraGetGameCamera(0);
	if (!camera)
		return;
	
	// get right vector
	vector_subtract(t, camera->pos, moby->Position);
	t[2] = 0;
	vector_normalize(&m2[0], t);

	// up vector
	m2[10] = 1;

	// right vector
	vector_outerproduct(&m2[4], &m2[0], &m2[8]);

	// position
	memcpy(&m2[12], moby->Position, sizeof(VECTOR));
	m2[14] += 1.5;

	// draw
	gfxDrawQuad((void*)0x00222590, &quad, m2, 1);
}

//--------------------------------------------------------------------------
void dropUpdate(Moby* moby)
{
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
		dropDestroy(moby);
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

	// read event
	guberEventRead(event, p, 12);
	guberEventRead(event, &args, sizeof(struct DropSpawnEventArgs));

	// set position
	vector_copy(moby->Position, p);

	// set update
	moby->PUpdate = &dropUpdate;

	// 
	moby->ModeBits |= 0x30;
	//moby->GlowRGBA = MobSecondaryColors[(int)args.MobType];
	//moby->PrimaryColor = MobPrimaryColors[(int)args.MobType];
	moby->CollData = NULL;
	moby->DrawDist = 0;
	//moby->PClass = NULL;

	// update pvars
	struct DropPVar* pvars = (struct DropPVar*)moby->PVar;
	memcpy(pvars, &args.PvarData, sizeof(struct DropPVar));

	// set team
	Guber* guber = guberGetObjectByMoby(moby);
	if (guber)
		((GuberMoby*)guber)->TeamNum = args.PvarData.Team;
	
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
					else {
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
	// prevent too many from spawning
	if (dropCount > DROP_MAX_SPAWNED)
		return 0;

	GameSettings* gs = gameGetSettings();
	struct DropSpawnEventArgs args;

	// create guber object
	GuberEvent * guberEvent = 0;
	guberMobyCreateSpawned(DROP_MOBY_OCLASS, sizeof(struct DropPVar), &guberEvent, NULL);
	if (guberEvent)
	{
		// owner is always host
		args.PvarData.Owner = gameGetHostId();
		args.PvarData.Type = dropType;
		args.PvarData.DestroyAtTime = destroyAtTime;
		args.PvarData.Team = team;
		
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
	dropThisFrame = 0;
}
