#include "include/mob.h"
#include "include/drop.h"
#include "include/utils.h"
#include "include/game.h"
#include <string.h>
#include <libdl/stdio.h>
#include <libdl/game.h>
#include <libdl/collision.h>
#include <libdl/graphics.h>
#include <libdl/moby.h>
#include <libdl/random.h>
#include <libdl/radar.h>
#include <libdl/color.h>

int mobInMobMove = 0;
Moby* mobFirstInList = 0;
Moby* mobLastInList = 0;

extern struct SurvivalMapConfig* mapConfig;

#if FIXEDTARGET
extern Moby* FIXEDTARGETMOBY;
#endif

/* 
 * 
 */
SoundDef BaseSoundDef =
{
	0.0,	  // MinRange
	45.0,	  // MaxRange
	0,		  // MinVolume
	1228,		// MaxVolume
	-635,			// MinPitch
	635,			// MaxPitch
	0,			// Loop
	0x10,		// Flags
	0x17D,		// Index
	3			  // Bank
};

Moby* AllMobsSorted[MAX_MOBS_SPAWNED];
int AllMobsSortedFreeSpots = MAX_MOBS_SPAWNED;

extern struct SurvivalState State;

GuberEvent* mobCreateEvent(Moby* moby, u32 eventType);

int mobAmIOwner(Moby* moby)
{
	struct MobPVar* pvars = (struct MobPVar*)moby->PVar;
	return gameGetMyClientId() == pvars->MobVars.Owner;
}

void mobSpawnCorn(Moby* moby, int bangle)
{
#if MOB_CORN
	mobyBlowCorn(
			moby
		, bangle
		, 0
		, 3.0
		, 6.0
		, 3.0
		, 6.0
		, -1
		, -1.0
		, -1.0
		, 255
		, 1
		, 0
		, 1
		, 1.0
		, 0x23
		, 3
		, 1.0
		, NULL
		, 0
		);
#endif
}

void mobSendStateUpdate(Moby* moby)
{
	struct MobStateUpdateEventArgs args;
	struct MobPVar* pvars = (struct MobPVar*)moby->PVar;

	// create event
	GuberEvent * guberEvent = mobCreateEvent(moby, MOB_EVENT_TARGET_UPDATE);
	if (guberEvent) {
		vector_copy(args.Position, moby->Position);
		args.TargetUID = guberGetUID(pvars->MobVars.Target);
		guberEventWrite(guberEvent, &args, sizeof(struct MobStateUpdateEventArgs));
	}
}

void mobSendOwnerUpdate(Moby* moby, char owner)
{
	struct MobPVar* pvars = (struct MobPVar*)moby->PVar;

	// create event
	GuberEvent * guberEvent = mobCreateEvent(moby, MOB_EVENT_OWNER_UPDATE);
	if (guberEvent) {
		guberEventWrite(guberEvent, &owner, sizeof(pvars->MobVars.Owner));
	}
}

void mobSendDamageEvent(Moby* moby, Moby* sourcePlayer, Moby* source, float amount, int damageFlags)
{
	VECTOR delta;
	struct MobDamageEventArgs args;

	// determine knockback
	Player * pDamager = playerGetFromUID(guberGetUID(sourcePlayer));
	if (pDamager)
	{
		int weaponId = getWeaponIdFromOClass(source->OClass);
		if (weaponId >= 0) {
			args.Knockback.Power = (u8)playerGetWeaponAlphaModCount(pDamager, weaponId, ALPHA_MOD_IMPACT);
		}
	}

	// determine angle
	vector_subtract(delta, moby->Position, sourcePlayer->Position);
	float len = vector_length(delta);
	float angle = atan2f(delta[1] / len, delta[0] / len);
	args.Knockback.Angle = (short)(angle * 1000);
	args.Knockback.Ticks = PLAYER_KNOCKBACK_BASE_TICKS;
  args.Knockback.Force = 0;
	
	// create event
	GuberEvent * guberEvent = mobCreateEvent(moby, MOB_EVENT_DAMAGE);
	if (guberEvent) {
		args.SourceUID = guberGetUID(sourcePlayer);
		args.SourceOClass = source->OClass;
		args.DamageQuarters = amount*4;
		guberEventWrite(guberEvent, &args, sizeof(struct MobDamageEventArgs));
	}
}

void mobDestroy(Moby* moby, int hitByUID)
{
	char killedByPlayerId = -1;
	char weaponId = -1;
	struct MobPVar* pvars = (struct MobPVar*)moby->PVar;

	// create event
	GuberEvent * guberEvent = mobCreateEvent(moby, MOB_EVENT_DESTROY);
	if (guberEvent) {

		// get weapon id from source oclass
		weaponId = getWeaponIdFromOClass(pvars->MobVars.LastHitByOClass);

		// get damager player id
		Player* damager = (Player*)playerGetFromUID(hitByUID);
		if (damager)
			killedByPlayerId = (char)damager->PlayerId;

		// 
		guberEventWrite(guberEvent, &killedByPlayerId, sizeof(killedByPlayerId));
		guberEventWrite(guberEvent, &weaponId, sizeof(weaponId));
	}
}

void mobSetTarget(Moby* moby, Moby* target)
{
	struct MobPVar* pvars = (struct MobPVar*)moby->PVar;

	if (pvars->MobVars.Target == target)
		return;

	// set target and dirty
	pvars->MobVars.Target = target;
	pvars->MobVars.ScoutCooldownTicks = 60;
	pvars->MobVars.Dirty = 1;
}

void mobSetAction(Moby* moby, int action)
{
	struct MobActionUpdateEventArgs args;
	struct MobPVar* pvars = (struct MobPVar*)moby->PVar;

	// don't set if already action
	if (pvars->MobVars.Action == action)
		return;

	GuberEvent* event = mobCreateEvent(moby, MOB_EVENT_STATE_UPDATE);
	if (event) {
		args.Action = action;
		args.ActionId = ++pvars->MobVars.ActionId;
		guberEventWrite(event, &args, sizeof(struct MobActionUpdateEventArgs));
	}
}

int mobGetLostArmorBangle(short armorStart, short armorEnd)
{
	return (armorStart - armorEnd) & armorStart;
}

void mobHandleDraw(Moby* moby)
{
	int i;
	VECTOR t;
	struct MobPVar* pvars = (struct MobPVar*)moby->PVar;

	// if we aren't in the sorted list, try and find an empty spot
	if (pvars->MobVars.Order < 0 && AllMobsSortedFreeSpots > 0) {
		for (i = 0; i < MAX_MOBS_SPAWNED; ++i) {
			Moby* m = AllMobsSorted[i];
			if (m == NULL) {
				AllMobsSorted[i] = moby;
				pvars->MobVars.Order = i;
				--AllMobsSortedFreeSpots;
				//DPRINTF("set %08X to order %d (free %d)\n", (u32)moby, i, AllMobsSortedFreeSpots);
				break;
			}
		}
	}

	int order = pvars->MobVars.Order;
	moby->DrawDist = 128;
	if (order >= 0) {
		float rank = 1 - clamp(order / (float)State.RoundMaxSpawnedAtOnce, 0, 1);
		//float rankCurve = powf(rank, 6);
		
		if (State.RoundMobCount > 20) {
			if (rank < 0.75) {
        if (pvars->VTable && pvars->VTable->PostDraw) {
          gfxRegisterDrawFunction((void**)0x0022251C, pvars->VTable->PostDraw, moby);
          moby->DrawDist = 0;
        }
			}
		}
	}
}

void mobUpdate(Moby* moby)
{
	int i;
	struct MobPVar* pvars = (struct MobPVar*)moby->PVar;
	GameOptions* gameOptions = gameGetOptions();
	if (!pvars || pvars->MobVars.Destroyed || !pvars->VTable)
		return;
	int isOwner;

  // 
  if (pvars->VTable->PreUpdate)
    pvars->VTable->PreUpdate(moby);

	// handle radar
	if (pvars->MobVars.Config.MobType != MOB_GHOST && pvars->MobVars.Config.MobSpecialMutation != MOB_SPECIAL_MUTATION_GHOST && gameOptions->GameFlags.MultiplayerGameFlags.RadarBlips > 0)
	{
		if (gameOptions->GameFlags.MultiplayerGameFlags.RadarBlips == 1 || pvars->MobVars.ClosestDist < (20 * 20))
		{
			int blipId = radarGetBlipIndex(moby);
			if (blipId >= 0)
			{
				RadarBlip * blip = radarGetBlips() + blipId;
				blip->X = moby->Position[0];
				blip->Y = moby->Position[1];
				blip->Life = 0x1F;
				blip->Type = 4;
				blip->Team = 1;
			}
		}
	}

	// keep track of number of mobs drawn on screen to try and reduce the lag
	int gameTime = gameGetTime();
	if (gameTime != State.MobsDrawGameTime) {
		State.MobsDrawGameTime = gameTime;
		State.MobsDrawnLast = State.MobsDrawnCurrent;
		State.MobsDrawnCurrent = 0;
	}
	if (moby->Drawn)
		State.MobsDrawnCurrent++;

	// dec timers
	u16 nextCheckActionDelayTicks = decTimerU16(&pvars->MobVars.NextCheckActionDelayTicks);
	u16 nextActionTicks = decTimerU16(&pvars->MobVars.NextActionDelayTicks);
	decTimerU16(&pvars->MobVars.ActionCooldownTicks);
	decTimerU16(&pvars->MobVars.AttackCooldownTicks);
	u16 scoutCooldownTicks = decTimerU16(&pvars->MobVars.ScoutCooldownTicks);
	decTimerU16(&pvars->MobVars.FlinchCooldownTicks);
	decTimerU16(&pvars->MobVars.TimeBombTicks);
	u16 autoDirtyCooldownTicks = decTimerU16(&pvars->MobVars.AutoDirtyCooldownTicks);
	u16 ambientSoundCooldownTicks = decTimerU16(&pvars->MobVars.AmbientSoundCooldownTicks);
	decTimerU8(&pvars->MobVars.Knockback.Ticks);
  
	// validate owner
	Player * ownerPlayer = playerGetAll()[(int)pvars->MobVars.Owner];
	if (!ownerPlayer || !ownerPlayer->PlayerMoby) {
		pvars->MobVars.Owner = gameGetHostId(); // default as host
		//mobSendOwnerUpdate(moby, pvars->MobVars.Owner);
	}
	
	// determine if I'm the owner
	isOwner = mobAmIOwner(moby);

	// change owner to target
	if (isOwner && pvars->MobVars.Target) {
		Player * targetPlayer = (Player*)guberGetObjectByMoby(pvars->MobVars.Target);
		if (targetPlayer && targetPlayer->Guber.Id.GID.HostId != pvars->MobVars.Owner) {
			mobSendOwnerUpdate(moby, targetPlayer->Guber.Id.GID.HostId);
		}
	}

	// 
	mobHandleDraw(moby);

	// 
	if (!State.Freeze) {
		//mobDoAction(moby);
    if (pvars->VTable && pvars->VTable->DoAction)
      pvars->VTable->DoAction(moby);

    // count ticks since last grounded
    if (pvars->MobVars.MoveVars.Grounded)
      pvars->MobVars.TimeLastGroundedTicks = 0;
    else
      pvars->MobVars.TimeLastGroundedTicks++;

    // auto destruct after 15 seconds of falling
    if (pvars->MobVars.TimeLastGroundedTicks > (TPS * 15)) {
      pvars->MobVars.Respawn = 1;
    }
  }

	// 
	if (pvars->FlashVars.type)
		mobyUpdateFlash(moby, 0);

  // move
  if (!State.Freeze && pvars->VTable && pvars->VTable->Move) {
    
    // turn on holos so we can collide with them
    // optimization to only turn on for first mob in moby list (this is expensive)
    if (mobFirstInList == moby)
      weaponTurnOnHoloshields(-1);

    //mobMove(moby);
    pvars->VTable->Move(moby);
  
    // turn off holos for everyone else
    // optimization to only turn off for last mob in moby list (this is expensive)
    if (mobLastInList == moby)
      weaponTurnOffHoloshields();
  }

	//
	if (isOwner && !State.Freeze) {
		// set next state
		if (pvars->MobVars.NextAction >= 0 && pvars->MobVars.ActionCooldownTicks == 0 && pvars->MobVars.Knockback.Ticks == 0) {
			if (nextActionTicks == 0) {
				mobSetAction(moby, pvars->MobVars.NextAction);
				pvars->MobVars.NextAction = -1;
			} else {
				//mobSetAction(moby, MOB_ACTION_LOOK_AT_TARGET);
			}
		}
		
		// 
		if (nextCheckActionDelayTicks == 0 && pvars->VTable && pvars->VTable->GetPreferredAction) {
			//int nextAction = mobGetPreferredAction(moby);
      int nextAction = pvars->VTable->GetPreferredAction(moby);
			if (nextAction >= 0 && nextAction != pvars->MobVars.NextAction && nextAction != pvars->MobVars.Action && nextActionTicks == 0) {
				pvars->MobVars.NextAction = nextAction;
				pvars->MobVars.NextActionDelayTicks = nextAction >= MOB_ACTION_ATTACK ? pvars->MobVars.Config.ReactionTickCount : 0;
			} 

			// get new target
			if (scoutCooldownTicks == 0 && pvars->VTable->GetNextTarget) {
				//mobSetTarget(moby, mobGetNextTarget(moby));
#if BENCHMARK
        mobSetTarget(moby, NULL);
#elif FIXEDTARGET
        mobSetTarget(moby, FIXEDTARGETMOBY);
#else
        mobSetTarget(moby, pvars->VTable->GetNextTarget(moby));
#endif
			}

			pvars->MobVars.NextCheckActionDelayTicks = 2;
		}
	}

	// 
	if (pvars->MobVars.Config.MobType == MOB_GHOST || pvars->MobVars.Config.MobSpecialMutation == MOB_SPECIAL_MUTATION_GHOST) {
		u8 defaultOpacity = pvars->MobVars.Config.MobSpecialMutation ? 0xFF : 0x80;
		u8 targetOpacity = pvars->MobVars.Action > MOB_ACTION_WALK ? defaultOpacity : 0x10;
		u8 opacity = moby->Opacity;
		if (opacity < targetOpacity)
			opacity = targetOpacity;
		else if (opacity > targetOpacity)
			opacity--;
		moby->Opacity = opacity;
	}

	float animSpeed = 0.9 * (pvars->MobVars.Config.Speed / MOB_BASE_SPEED);
	if (State.Freeze || (moby->DrawDist == 0 && pvars->MobVars.Action == MOB_ACTION_WALK)) {
		moby->AnimSpeed = 0;
	} else {
		moby->AnimSpeed = animSpeed;
	}

	// update armor
  if (pvars->VTable && pvars->VTable->GetArmor)
	  moby->Bangles = pvars->VTable->GetArmor(moby);

	// process damage
  int damageIndex = moby->CollDamage;
  MobyColDamage* colDamage = NULL;
  float damage = 0.0;

  if (damageIndex >= 0) {
    colDamage = mobyGetDamage(moby, 0x400C00, 0);
    if (colDamage)
      damage = colDamage->DamageHp;
  }
  ((void (*)(Moby*, float*, MobyColDamage*))0x005184d0)(moby, &damage, colDamage);

  if (colDamage && damage > 0) {
    Player * damager = guberMobyGetPlayerDamager(colDamage->Damager);
    if (damager) {
      damage *= 1 + (0.05 * State.PlayerStates[damager->PlayerId].State.Upgrades[UPGRADE_DAMAGE]);
    }

    if (damager && (isOwner || playerIsLocal(damager))) {
      mobSendDamageEvent(moby, damager->PlayerMoby, colDamage->Damager, damage, colDamage->DamageFlags);	
    } else if (isOwner) {
      mobSendDamageEvent(moby, colDamage->Damager, colDamage->Damager, damage, colDamage->DamageFlags);	
    }

    // 
    moby->CollDamage = -1;
  }

	// handle death stuff
	if (isOwner) {

		// handle falling under map
		if (moby->Position[2] < gameGetDeathHeight())
			pvars->MobVars.Respawn = 1;

		// respawn
		if (pvars->MobVars.Respawn) {
			VECTOR p;
			struct MobConfig config;
			if (spawnGetRandomPoint(p)) {
				memcpy(&config, &pvars->MobVars.Config, sizeof(struct MobConfig));
				mobCreate(p, 0, guberGetUID(moby), &config);
			}	

			pvars->MobVars.Respawn = 0;
			pvars->MobVars.Destroyed = 2;
		}

		// destroy
		else if (pvars->MobVars.Destroy) {
			mobDestroy(moby, pvars->MobVars.LastHitBy);
		}

		// send changes
		else if (pvars->MobVars.Dirty || autoDirtyCooldownTicks == 0) {
			mobSendStateUpdate(moby);
			pvars->MobVars.Dirty = 0;
			pvars->MobVars.AutoDirtyCooldownTicks = MOB_AUTO_DIRTY_COOLDOWN_TICKS;
		}
	}
  
  // 
  if (pvars->VTable->PostUpdate)
    pvars->VTable->PostUpdate(moby);
}

GuberEvent* mobCreateEvent(Moby* moby, u32 eventType)
{
	GuberEvent * event = NULL;

	// create guber object
	Guber* guber = guberGetObjectByMoby(moby);
	if (guber)
		event = guberEventCreateEvent(guber, eventType, 0, 0);

	return event;
}

int mobHandleEvent_Spawn(Moby* moby, GuberEvent* event)
{
	
	VECTOR p;
	float yaw;
	int i;
	int spawnFromUID;
	char random;
	struct MobSpawnEventArgs args;

	// read event
	guberEventRead(event, p, 12);
	guberEventRead(event, &yaw, 4);
	guberEventRead(event, &spawnFromUID, 4);
	guberEventRead(event, &random, 1);
	guberEventRead(event, &args, sizeof(struct MobSpawnEventArgs));

	// set position and rotation
	vector_copy(moby->Position, p);
	moby->Rotation[2] = yaw;

	// set update
	moby->PUpdate = &mobUpdate;

	// 
	moby->ModeBits |= 0x1030;
	moby->Opacity = args.MobSpecialMutation ? 0xFF : 0x80;
	moby->CollActive = 1;


	// update pvars
	struct MobPVar* pvars = (struct MobPVar*)moby->PVar;
	memset(pvars, 0, sizeof(struct MobPVar));
	pvars->TargetVarsPtr = &pvars->TargetVars;
	pvars->MoveVarsPtr = NULL; // &pvars->MoveVars;
	pvars->FlashVarsPtr = &pvars->FlashVars;
	pvars->ReactVarsPtr = &pvars->ReactVars;
	
	// initialize mob vars
	pvars->MobVars.Config.MobType = args.MobType;
	pvars->MobVars.Config.MobSpecialMutation = args.MobSpecialMutation;
	pvars->MobVars.Config.Bolts = args.Bolts;
	pvars->MobVars.Config.Xp = args.Xp;
	pvars->MobVars.Config.MaxHealth = (float)args.StartHealth;
	pvars->MobVars.Config.Health = (float)args.StartHealth;
	pvars->MobVars.Config.Bangles = args.Bangles;
	pvars->MobVars.Config.Damage = (float)args.Damage;
	pvars->MobVars.Config.AttackRadius = (float)args.AttackRadiusEighths / 8.0;
	pvars->MobVars.Config.HitRadius = (float)args.HitRadiusEighths / 8.0;
	pvars->MobVars.Config.CollRadius = (float)args.CollRadiusEighths / 8.0;
	pvars->MobVars.Config.Speed = (float)args.SpeedEighths / 8.0;
	pvars->MobVars.Config.ReactionTickCount = args.ReactionTickCount;
	pvars->MobVars.Config.AttackCooldownTickCount = args.AttackCooldownTickCount;
	pvars->MobVars.Health = pvars->MobVars.Config.MaxHealth;
	pvars->MobVars.Order = -1;
	pvars->MobVars.TimeLastGroundedTicks = 0;
	pvars->MobVars.Random = random;
#if MOB_NO_MOVE
	pvars->MobVars.Config.Speed = 0;
#endif
#if MOB_NO_DAMAGE
	pvars->MobVars.Config.Damage = 0;
#endif
	//pvars->MobVars.Config.Health = pvars->MobVars.Health = 1;

	// initialize target vars
	pvars->TargetVars.hitPoints = pvars->MobVars.Health;
	pvars->TargetVars.team = 10;
	pvars->TargetVars.targetHeight = 1;

	// 
	Guber* guber = guberGetObjectByMoby(moby);
	if (guber)
	{
		((GuberMoby*)guber)->TeamNum = 10;
	}

	// initialize move vars
	mobySetAnimCache(moby, (void*)0x36f980, 0);
	moby->ModeBits &= ~0x100;

	// initialize react vars
	pvars->ReactVars.acidDamage = 1.0;
	pvars->ReactVars.shieldDamageReduction = 1.0;

	// mob specifc settings
	switch (args.MobType)
	{
		case MOB_GHOST:
		{
			pvars->ReactVars.deathType = 0; // normal

			// disable targeting
			moby->ModeBits &= ~0x1000;
			break;
		}
		case MOB_ACID:
		{
			pvars->ReactVars.deathType = 4; // acid
			break;
		}
		case MOB_FREEZE:
		{
			pvars->ReactVars.deathType = 3; // blue explosion
			break;
		}
		case MOB_EXPLODE:
		{
			pvars->ReactVars.deathType = 2; // explosion
			break;
		}
		case MOB_TANK:
		{
			pvars->ReactVars.deathType = 0; // normal
			break;
		}
	}

	// special mutation settings
	switch (pvars->MobVars.Config.MobSpecialMutation)
	{
		case MOB_SPECIAL_MUTATION_GHOST:
		{
			// disable targeting
			pvars->ReactVars.deathType = 0; // normal
			moby->ModeBits &= ~0x1000;
			break;
		}
	}

	// 
	mobySetState(moby, 0, -1);

	++State.RoundMobCount;

	// destroy spawn from
	if (spawnFromUID != -1) {
		GuberMoby* gm = (GuberMoby*)guberGetObjectByUID(spawnFromUID);
		if (gm && gm->Moby && gm->Moby->PVar && !mobyIsDestroyed(gm->Moby) && mobyIsMob(gm->Moby)) {
			struct MobPVar* spawnFromPVars = (struct MobPVar*)gm->Moby->PVar;
			if (spawnFromPVars->MobVars.Destroyed != 1) {
				guberMobyDestroy(gm->Moby);
				State.RoundMobCount--;
			}
		}
	}

  // pass to map
  if (mapConfig->OnMobSpawnedFunc)
    mapConfig->OnMobSpawnedFunc(moby);

  // pass to mob handler
  if (pvars->VTable && pvars->VTable->OnSpawn)
    pvars->VTable->OnSpawn(moby, p, yaw, spawnFromUID, random, &args);
	
#if LOG_STATS2
	DPRINTF("mob created event %08X, %08X, %08X mobtype:%d spawnedNum:%d roundTotal:%d)\n", (u32)moby, (u32)event, (u32)moby->GuberMoby, args.MobType, State.RoundMobCount, State.RoundMobSpawnedCount);
#endif
	return 0;
}

int mobHandleEvent_Destroy(Moby* moby, GuberEvent* event)
{
	char killedByPlayerId, weaponId;
	int i;
	Player** players = playerGetAll();
	struct MobPVar* pvars = (struct MobPVar*)moby->PVar;
	if (!pvars || pvars->MobVars.Destroyed)
		return 0;

	// 
	guberEventRead(event, &killedByPlayerId, sizeof(killedByPlayerId));
	guberEventRead(event, &weaponId, sizeof(weaponId));

  // pass to mob handler
  if (pvars->VTable && pvars->VTable->OnDestroy)
    pvars->VTable->OnDestroy(moby, killedByPlayerId, weaponId);
	
	int bolts = pvars->MobVars.Config.Bolts;
	int xp = pvars->MobVars.Config.Xp;

#if DROPS
	if (killedByPlayerId >= 0 && gameAmIHost()) {
		Player * killedByPlayer = players[killedByPlayerId];
		if (killedByPlayer) {
			float randomValue = randRange(0.0, 1.0);
			if (randomValue < MOB_HAS_DROP_PROBABILITY) {
				dropCreate(moby->Position, randRangeInt(0, DROP_COUNT-1), gameGetTime() + DROP_DURATION, killedByPlayer->Team);
			}
		}
	}
#endif

#if SHARED_BOLTS
	if (killedByPlayerId >= 0) {
		Player * killedByPlayer = players[killedByPlayerId];
		if (killedByPlayer) {
			for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
				Player* p = players[i];
				if (p && !playerIsDead(p) && p->Team == killedByPlayer->Team) {
					int multiplier = State.PlayerStates[i].IsDoublePoints ? 2 : 1;
					State.PlayerStates[i].State.Bolts += bolts * multiplier;
					State.PlayerStates[i].State.TotalBolts += bolts * multiplier;
				}
			}
		}
	}
#else
	if (killedByPlayerId >= 0) {
		int multiplier = State.PlayerStates[killedByPlayerId].IsDoublePoints ? 2 : 1;
		State.PlayerStates[(int)killedByPlayerId].State.Bolts += bolts * multiplier;
		State.PlayerStates[(int)killedByPlayerId].State.TotalBolts += bolts * multiplier;
	}
#endif

	if (killedByPlayerId >= 0) {
		Player * killedByPlayer = players[killedByPlayerId];
		struct SurvivalPlayer* pState = &State.PlayerStates[(int)killedByPlayerId];
		GameData * gameData = gameGetData();

		// give xp
		pState->State.XP += xp;
		u64 targetForNextToken = getXpForNextToken(pState->State.TotalTokens);

		// give tokens
		while (pState->State.XP >= targetForNextToken)
		{
			pState->State.XP -= targetForNextToken;
			pState->State.TotalTokens += 1;
			pState->State.CurrentTokens += 1;
			targetForNextToken = getXpForNextToken(pState->State.TotalTokens);

			if (killedByPlayer->IsLocal) {
				sendPlayerStats(killedByPlayerId);
			}
		} 

		// handle weapon jackpot
		if (weaponId > 1 && killedByPlayer) {
			int jackpotCount = playerGetWeaponAlphaModCount(killedByPlayer, weaponId, ALPHA_MOD_JACKPOT);

			pState->State.Bolts += jackpotCount * JACKPOT_BOLTS;
			pState->State.TotalBolts += jackpotCount * JACKPOT_BOLTS;
		}

		// handle stats
		pState->State.Kills++;
		gameData->PlayerStats.Kills[killedByPlayerId]++;
		int weaponSlotId = weaponIdToSlot(weaponId);
		if (weaponId > 0 && (weaponId != WEAPON_ID_WRENCH || weaponSlotId == WEAPON_SLOT_WRENCH))
			gameData->PlayerStats.WeaponKills[killedByPlayerId][weaponSlotId]++;
	}

	// limit corn spawning to prevent freezing/framelag
	if (State.RoundMobCount < 30) {
		mobSpawnCorn(moby, ZOMBIE_BANGLE_LARM | ZOMBIE_BANGLE_RARM | ZOMBIE_BANGLE_LLEG | ZOMBIE_BANGLE_RLEG | ZOMBIE_BANGLE_RFOOT | ZOMBIE_BANGLE_HIPS);
	}

	if (pvars->MobVars.Order >= 0) {
		AllMobsSorted[(int)pvars->MobVars.Order] = NULL;
		++AllMobsSortedFreeSpots;
	}
	guberMobyDestroy(moby);
	moby->ModeBits &= ~0x30;
	--State.RoundMobCount;
	pvars->MobVars.Destroyed = 1;

#if LOG_STATS2
	DPRINTF("mob destroy event %08X, %08X, by client %d, (%d)\n", (u32)moby, (u32)event, killedByPlayerId, State.RoundMobCount);
#endif
	return 0;
}

int mobHandleEvent_Damage(Moby* moby, GuberEvent* event)
{
	VECTOR t;
	struct MobDamageEventArgs args;
	struct MobPVar* pvars = (struct MobPVar*)moby->PVar;
  
	if (!pvars)
		return 0;

	// update armor
	int armorStart = 0;
  if (pvars->VTable && pvars->VTable->GetArmor)
	  armorStart = pvars->VTable->GetArmor(moby);

	// read event
	guberEventRead(event, &args, sizeof(struct MobDamageEventArgs));

  // pass to mob handler
  if (pvars->VTable && pvars->VTable->OnDamage)
    pvars->VTable->OnDamage(moby, &args);
	
	// get damager
	GuberMoby* damager = (GuberMoby*)guberGetObjectByUID(args.SourceUID);

	// flash
	mobyStartFlash(moby, FT_HIT, 0x800000FF, 0);

	// decrement health
	float damage = args.DamageQuarters / 4.0;
	float newHp = pvars->MobVars.Health - damage;
	pvars->MobVars.Health = newHp;
	pvars->TargetVars.hitPoints = newHp;

	// drop armor bangle
	/*
	int armorNew = mobGetArmor(pvars);
	if (armorNew != armorStart) {
		int b = mobGetLostArmorBangle(armorStart, armorNew);
		mobSpawnCorn(moby, b);
	}
	*/

	return 0;
}

int mobHandleEvent_ActionUpdate(Moby* moby, GuberEvent* event)
{
	struct MobActionUpdateEventArgs args;
	struct MobPVar* pvars = (struct MobPVar*)moby->PVar;
	if (!pvars)
		return 0;

	// read event
	guberEventRead(event, &args, sizeof(struct MobActionUpdateEventArgs));

	// 
	if (pvars->MobVars.LastActionId != args.ActionId && pvars->MobVars.Action != args.Action) {
		pvars->MobVars.LastActionId = args.ActionId;
    
    // pass to mob handler
    if (pvars->VTable && pvars->VTable->ForceLocalAction)
      pvars->VTable->ForceLocalAction(moby, args.Action);
	}
	
	//DPRINTF("mob state update event %08X, %08X, %d\n", (u32)moby, (u32)event, args.Action);
	return 0;
}

int mobHandleEvent_StateUpdate(Moby* moby, GuberEvent* event)
{
	struct MobStateUpdateEventArgs args;
	struct MobPVar* pvars = (struct MobPVar*)moby->PVar;
	VECTOR t;
	if (!pvars)
		return 0;

	// read event
	guberEventRead(event, &args, sizeof(struct MobStateUpdateEventArgs));

	// teleport position if far away
	vector_subtract(t, moby->Position, args.Position);
	if (vector_sqrmag(t) > 25)
		vector_copy(moby->Position, args.Position);

	// 
	Player* target = (Player*)guberGetObjectByUID(args.TargetUID);
	if (target)
		pvars->MobVars.Target = target->SkinMoby;


	//DPRINTF("mob target update event %08X, %08X, %d:%08X, %08X\n", (u32)moby, (u32)event, args.TargetUID, (u32)target, (u32)pvars->MobVars.Target);
	return 0;
}

int mobHandleEvent_OwnerUpdate(Moby* moby, GuberEvent* event)
{
	struct MobPVar* pvars = (struct MobPVar*)moby->PVar;
	char newOwner;
	if (!pvars)
		return 0;

	// read event
	guberEventRead(event, &newOwner, 1);

	// 
	pvars->MobVars.Owner = newOwner;

	if (gameGetMyClientId() == newOwner) {
		pvars->MobVars.NextAction = -1; // indicate we have no new action since we just became owner
	}
	
	//DPRINTF("mob owner update event %08X, %08X, %d\n", (u32)moby, (u32)event, newOwner);
	return 0;
}

int mobHandleEvent(Moby* moby, GuberEvent* event)
{
	struct MobPVar* pvars = (struct MobPVar*)moby->PVar;

	if (isInGame() && mobyIsMob(moby) && pvars) {
		u32 mobEvent = event->NetEvent.EventID;
		int isFromHost = gameIsHost(event->NetEvent.OriginClientIdx);
		if (!isFromHost && mobEvent != MOB_EVENT_SPAWN && mobEvent != MOB_EVENT_DAMAGE && mobEvent != MOB_EVENT_DESTROY && pvars->MobVars.Owner != event->NetEvent.OriginClientIdx)
		{
			DPRINTF("ignoring mob event %d from %d (not owner, %d)\n", mobEvent, event->NetEvent.OriginClientIdx, pvars->MobVars.Owner);
			return 0;
		}

		switch (mobEvent)
		{
			case MOB_EVENT_SPAWN: return mobHandleEvent_Spawn(moby, event);
			case MOB_EVENT_DESTROY: return mobHandleEvent_Destroy(moby, event);
			case MOB_EVENT_DAMAGE: return mobHandleEvent_Damage(moby, event);
			case MOB_EVENT_STATE_UPDATE: return mobHandleEvent_ActionUpdate(moby, event);
			case MOB_EVENT_TARGET_UPDATE: return mobHandleEvent_StateUpdate(moby, event);
			case MOB_EVENT_OWNER_UPDATE: return mobHandleEvent_OwnerUpdate(moby, event);
			default:
			{
				DPRINTF("unhandle mob event %d\n", mobEvent);
				break;
			}
		}
	}

	return 0;
}

int mobCreate(VECTOR position, float yaw, int spawnFromUID, struct MobConfig *config)
{
  if (mapConfig->OnMobCreateFunc)
    return mapConfig->OnMobCreateFunc(position, yaw, spawnFromUID, config);

  return 0;
}

void mobInitialize(void)
{
	// set vtable callbacks
	*(u32*)0x003A0A84 = (u32)&getGuber;
	*(u32*)0x003A0A94 = (u32)&handleEvent;

	// collision hit type
	//*(u32*)0x004bd1f0 = 0x08000000 | ((u32)&colHotspot / 4);
	//*(u32*)0x004bd1f4 = 0x00402021;

	// 
	memset(AllMobsSorted, 0, sizeof(AllMobsSorted));
}

void mobNuke(int killedByPlayerId)
{
	int i;
	Player** players = playerGetAll();
	u32 playerUid = 0;
	if (killedByPlayerId >= 0 && killedByPlayerId < GAME_MAX_PLAYERS) {
		Player* p = players[killedByPlayerId];
		if (p) {
			playerUid = p->Guber.Id.UID;
		}
	}

	for (i = 0; i < MAX_MOBS_SPAWNED; ++i) {
		Moby* m = AllMobsSorted[i];
		if (m) {
			mobDestroy(m, playerUid);
		}
	}
}

void mobReactToExplosionAt(int byPlayerId, VECTOR position, float damage, float radius)
{
	int i;
  VECTOR delta;
  struct MobDamageEventArgs args;
  float sqrRadius = radius * radius;
	Player** players = playerGetAll();
	u32 playerUid = 0;
	if (byPlayerId >= 0 && byPlayerId < GAME_MAX_PLAYERS) {
		Player* p = players[byPlayerId];
		if (p) {
			playerUid = p->Guber.Id.UID;
		}
	}

	for (i = 0; i < MAX_MOBS_SPAWNED; ++i) {
		Moby* m = AllMobsSorted[i];
		if (m) {
      
      vector_subtract(delta, m->Position, position);
      if (vector_sqrmag(delta) <= sqrRadius) {
	      
        float dist = vector_length(delta);
        float angle = atan2f(delta[1] / dist, delta[0] / dist);
        
        // create event
        GuberEvent * guberEvent = mobCreateEvent(m, MOB_EVENT_DAMAGE);
        if (guberEvent) {
          args.SourceUID = playerUid;
          args.SourceOClass = 0;
          args.DamageQuarters = damage*4;
          args.Knockback.Angle = (short)(angle * 1000);
          args.Knockback.Ticks = 5;
          args.Knockback.Power = 10;
          args.Knockback.Force = 1;
          guberEventWrite(guberEvent, &args, sizeof(struct MobDamageEventArgs));
        }
      }

		}
	}
}

void mobTick(void)
{
	int i, j;
	VECTOR t;
	Player* locals[] = { playerGetFromSlot(0), playerGetFromSlot(1) };
	int localsCount = playerGetNumLocals();

	if (mobFirstInList && (mobyIsDestroyed(mobFirstInList) || !mobyIsMob(mobFirstInList)))
		mobFirstInList = NULL;
	if (mobLastInList && (mobyIsDestroyed(mobLastInList) || !mobyIsMob(mobLastInList)))
		mobLastInList = NULL;
	
	// run single pass on sort
	for (i = 0; i < MAX_MOBS_SPAWNED; ++i)
	{
		Moby* m = AllMobsSorted[i];

		if (m && (!mobFirstInList || m < mobFirstInList))
			mobFirstInList = m;
		if (m && (!mobLastInList || m > mobLastInList))
			mobLastInList = m;

		// remove invalid moby ref
		if (m && (mobyIsDestroyed(m) || !m->PVar || !mobyIsMob(m))) {
			AllMobsSorted[i] = NULL;
			m = NULL;
			++AllMobsSortedFreeSpots;
		}

		if (m) {
			struct MobPVar* pvars = (struct MobPVar*)m->PVar;

			// find closest dist to local
			pvars->MobVars.ClosestDist = 10000000;
			for (j = 0; j < localsCount; ++j) {
				Player * p = locals[j];
				vector_subtract(t, m->Position, p->CameraPos);
				float dist = vector_sqrmag(t);
				if (dist < pvars->MobVars.ClosestDist)
					pvars->MobVars.ClosestDist = dist;
			}

			// if closer than last, swap
			if (i > 0) {
				Moby* last = AllMobsSorted[i-1];
				int swap = 0;

				// swap if previous is empty
				// or if this is closer than previous
				if (!last) {
					swap = 1;
				} else {
					struct MobPVar* lPvars = (struct MobPVar*)last->PVar;
					if (lPvars && pvars->MobVars.ClosestDist < lPvars->MobVars.ClosestDist) {
						swap = 1;
						lPvars->MobVars.Order = i;
					}
				}
				
				// move current to previous
				if (swap) {
					AllMobsSorted[i] = AllMobsSorted[i-1];
					AllMobsSorted[i-1] = m;
					pvars->MobVars.Order = i-1;
				}
			}
		}
	}
}
