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
#include <libdl/net.h>
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
    args.PathCurrentEdgeIdx = pvars->MobVars.MoveVars.PathEdgeCurrent;
    args.PathEndNodeIdx = pvars->MobVars.MoveVars.PathStartEndNodes[0];
    args.PathStartNodeIdx = pvars->MobVars.MoveVars.PathStartEndNodes[1];
    args.PathHasReachedEnd = pvars->MobVars.MoveVars.PathHasReachedEnd;
    args.PathHasReachedStart = pvars->MobVars.MoveVars.PathHasReachedStart;
		args.TargetUID = guberGetUID(pvars->MobVars.Target);
		guberEventWrite(guberEvent, &args, sizeof(struct MobStateUpdateEventArgs));
	}
}

void mobSendStateUpdateUnreliable(Moby* moby)
{
	struct MobUnreliableMsgStateUpdateArgs msg;
	struct MobPVar* pvars = (struct MobPVar*)moby->PVar;

  void * connection = netGetDmeServerConnection();
  if (!connection)
    return;

  // base msg
  msg.Base.MsgId = MOB_UNRELIABLE_MSG_ID_STATE_UPDATE;
  msg.Base.MobUID = guberGetUID(moby);

  // state update
  vector_copy(msg.StateUpdate.Position, moby->Position);
  msg.StateUpdate.PathCurrentEdgeIdx = pvars->MobVars.MoveVars.PathEdgeCurrent;
  msg.StateUpdate.PathEndNodeIdx = pvars->MobVars.MoveVars.PathStartEndNodes[0];
  msg.StateUpdate.PathStartNodeIdx = pvars->MobVars.MoveVars.PathStartEndNodes[1];
  msg.StateUpdate.PathHasReachedEnd = pvars->MobVars.MoveVars.PathHasReachedEnd;
  msg.StateUpdate.PathHasReachedStart = pvars->MobVars.MoveVars.PathHasReachedStart;
  msg.StateUpdate.TargetUID = guberGetUID(pvars->MobVars.Target);

  // broadcast to players unreliably
  netBroadcastCustomAppMessage(0, connection, CUSTOM_MSG_MOB_UNRELIABLE_MSG, sizeof(msg), &msg);
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
	    args.Knockback.Ticks = PLAYER_KNOCKBACK_BASE_TICKS;
		}
	}

	// determine angle
	vector_subtract(delta, moby->Position, sourcePlayer->Position);
	float len = vector_length(delta);
	float angle = atan2f(delta[1] / len, delta[0] / len);
	args.Knockback.Angle = (short)(angle * 1000);
  args.Knockback.Force = 0;

  // full knockback
  if (damageFlags & 0x80000000) {
    args.Knockback.Ticks = 5;
    args.Knockback.Power = 10;
    args.Knockback.Force = 1;
  }

	// create event
	GuberEvent * guberEvent = mobCreateEvent(moby, MOB_EVENT_DAMAGE);
	if (guberEvent) {
		args.SourceUID = guberGetUID(sourcePlayer);
		args.SourceOClass = source->OClass;
		args.DamageQuarters = amount*4;
    args.DamageFlags = damageFlags;
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

int mobIsFrozen(Moby* moby)
{
  if (!moby || !moby->PVar)
    return 0;

	struct MobPVar* pvars = (struct MobPVar*)moby->PVar;
  return State.Freeze && pvars->MobVars.Config.MobAttribute != MOB_ATTRIBUTE_FREEZE;
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
	int isOwner;
	struct MobPVar* pvars = (struct MobPVar*)moby->PVar;
	GameOptions* gameOptions = gameGetOptions();
  int isFrozen = mobIsFrozen(moby);
	if (!pvars || pvars->MobVars.Destroyed || !pvars->VTable)
		return;

  // 
  if (pvars->VTable->PreUpdate)
    pvars->VTable->PreUpdate(moby);

	// handle radar
	if (pvars->MobVars.BlipType >= 0 && pvars->MobVars.Config.MobAttribute != MOB_ATTRIBUTE_GHOST && gameOptions->GameFlags.MultiplayerGameFlags.RadarBlips > 0)
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
				blip->Type = pvars->MobVars.BlipType;
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
	if (!isFrozen) {
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
  if (!isFrozen && pvars->VTable && pvars->VTable->Move) {
    //vector_write(pvars->MobVars.MoveVars.Velocity, 0);
    pvars->VTable->Move(moby);
  }

	//
	if (isOwner && !isFrozen) {
		// set next state
		if (pvars->MobVars.NextAction >= 0 && pvars->MobVars.Knockback.Ticks == 0) {
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
			if (nextAction >= 0 && nextAction != pvars->MobVars.NextAction && nextAction != pvars->MobVars.Action) {
				pvars->MobVars.NextAction = nextAction;
				pvars->MobVars.NextActionDelayTicks = nextAction >= MOB_ACTION_ATTACK ? pvars->MobVars.Config.ReactionTickCount : 0;
			} 

			// get new target
			if (scoutCooldownTicks == 0 && pvars->VTable->GetNextTarget) {
				//mobSetTarget(moby, mobGetNextTarget(moby));
#if FIXEDTARGET
        mobSetTarget(moby, FIXEDTARGETMOBY);
#elif BENCHMARK
        //mobSetTarget(moby, NULL);
        mobSetTarget(moby, pvars->VTable->GetNextTarget(moby));
#else
        mobSetTarget(moby, pvars->VTable->GetNextTarget(moby));
#endif
			}

			pvars->MobVars.NextCheckActionDelayTicks = 2;
		}
	}

	// 
	if (pvars->MobVars.Config.MobAttribute == MOB_ATTRIBUTE_GHOST) {
		u8 targetOpacity = pvars->MobVars.Action > MOB_ACTION_WALK ? 0x80 : 0x10;
		u8 opacity = moby->Opacity;
		if (opacity < targetOpacity)
			opacity = targetOpacity;
		else if (opacity > targetOpacity)
			opacity--;
		moby->Opacity = opacity;
	}

	// update armor
  if (pvars->VTable && pvars->VTable->GetArmor)
	  moby->Bangles = pvars->VTable->GetArmor(moby);

	// process damage
  int damageIndex = moby->CollDamage;
  u32 damageFlags = 0;
  MobyColDamage* colDamage = NULL;
  float damage = 0.0;

  if (damageIndex >= 0) {
    colDamage = mobyGetDamage(moby, 0x80481C40, 0);
    if (colDamage) {
      damage = colDamage->DamageHp;
      damageFlags = colDamage->DamageFlags;
    }
  }
  ((void (*)(Moby*, float*, MobyColDamage*))0x005184d0)(moby, &damage, colDamage);

  if (colDamage && damage > 0) {
    Player * damager = guberMobyGetPlayerDamager(colDamage->Damager);
    if (damager) {
      damage *= 1 + (PLAYER_UPGRADE_DAMAGE_FACTOR * State.PlayerStates[damager->PlayerId].State.Upgrades[UPGRADE_DAMAGE]);

      // surge boost
      if (damager->timers.clankRedEye > 0) {
        damage *= 2;
      }
    }

    if (damager && (isOwner || playerIsLocal(damager))) {
      mobSendDamageEvent(moby, damager->PlayerMoby, colDamage->Damager, damage, damageFlags);	
    } else if (isOwner) {
      mobSendDamageEvent(moby, colDamage->Damager, colDamage->Damager, damage, damageFlags);	
    }

    // 
    moby->CollDamage = -1;
  }

	// handle death stuff
	if (isOwner) {

		// handle falling under map
		if (moby->Position[2] < gameGetDeathHeight()) {
			pvars->MobVars.Respawn = 1;
    }

		// respawn
		if (pvars->MobVars.Respawn) {
			VECTOR p;
			struct MobConfig config;
			if (spawnGetRandomPoint(p)) {
				memcpy(&config, &pvars->MobVars.Config, sizeof(struct MobConfig));
				mobCreate(pvars->MobVars.SpawnParamsIdx, p, 0, guberGetUID(moby), pvars->MobVars.FreeAgent, &config);
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
			mobSendStateUpdateUnreliable(moby);
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
  int freeAgent;
	char random;
	struct MobSpawnEventArgs args;

	// read event
	guberEventRead(event, p, 12);
	guberEventRead(event, &yaw, 4);
	guberEventRead(event, &spawnFromUID, 4);
	guberEventRead(event, &freeAgent, 4);
	guberEventRead(event, &random, 1);
	guberEventRead(event, &args, sizeof(struct MobSpawnEventArgs));

	// set position and rotation
	vector_copy(moby->Position, p);
	moby->Rotation[2] = yaw;

	// set update
	moby->PUpdate = &mobUpdate;

	// 
	moby->ModeBits |= 0x1030;
	moby->Opacity = 0x80;
	moby->CollActive = 1;


	// update pvars
	struct MobPVar* pvars = (struct MobPVar*)moby->PVar;
	memset(pvars, 0, sizeof(struct MobPVar));
	pvars->TargetVarsPtr = &pvars->TargetVars;
	pvars->MoveVarsPtr = NULL; // &pvars->MoveVars;
	pvars->FlashVarsPtr = &pvars->FlashVars;
	pvars->ReactVarsPtr = &pvars->ReactVars;
	
  // free agent data
  pvars->MobVars.FreeAgent = freeAgent;

  // copy spawn params to config
  pvars->MobVars.SpawnParamsIdx = args.SpawnParamsIdx;
  struct MobSpawnParams* params = &mapConfig->DefaultSpawnParams[args.SpawnParamsIdx];
  memcpy(&pvars->MobVars.Config, &params->Config, sizeof(struct MobConfig));

	// initialize mob vars
	pvars->MobVars.Config.MobAttribute = args.MobAttribute;
	pvars->MobVars.Config.Bolts = args.Bolts;
#if PAYDAY
	pvars->MobVars.Config.Bolts = 100000;
#endif
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
  vector_copy(pvars->MobVars.MoveVars.NextPosition, p);
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

	// mob attributes
	switch (pvars->MobVars.Config.MobAttribute)
	{
		case MOB_ATTRIBUTE_GHOST:
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
  if (spawnFromUID == -1 && !gameAmIHost()) {
    ++State.RoundMobSpawnedCount;
  }

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
	DPRINTF("mob created event %08X, %08X, %08X spawnArgsIdx:%d spawnedNum:%d roundTotal:%d)\n", (u32)moby, (u32)event, (u32)moby->GuberMoby, args.SpawnParamsIdx, State.RoundMobCount, State.RoundMobSpawnedCount);
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
				if (p && !playerIsDead(p)) {
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
		pState->State.XP += xp * (pState->IsDoubleXP ? 2 : 1);
		u64 targetForNextToken = getXpForNextToken(pState->State.TotalTokens);

		// handle weapon xp
		if (weaponId > 1 && killedByPlayer) {
			int xpCount = playerGetWeaponAlphaModCount(killedByPlayer, weaponId, ALPHA_MOD_XP);

      pState->State.XP += xpCount * XP_ALPHAMOD_XP;
		}

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
    pvars->MobVars.LastAction = pvars->MobVars.Action;
    
    // pass to mob handler
    if (pvars->VTable && pvars->VTable->ForceLocalAction)
      pvars->VTable->ForceLocalAction(moby, args.Action);
	}
	
	//DPRINTF("mob state update event %08X, %08X, %d\n", (u32)moby, (u32)event, args.Action);
	return 0;
}

int mobHandleEvent_StateUpdateUnreliable(Moby* moby, struct MobStateUpdateEventArgs* args)
{
	struct MobPVar* pvars = (struct MobPVar*)moby->PVar;
	VECTOR t;
	if (!pvars || mobAmIOwner(moby) || !args)
		return 0;

	// teleport position if far away
	vector_subtract(t, moby->Position, args->Position);
	if (vector_sqrmag(t) > 25)
		vector_copy(moby->Position, args->Position);

	// 
	Player* target = (Player*)guberGetObjectByUID(args->TargetUID);
	if (target)
		pvars->MobVars.Target = target->SkinMoby;

  // pass to mob
  if (pvars->VTable && pvars->VTable->OnStateUpdate)
	  pvars->VTable->OnStateUpdate(moby, args);


	//DPRINTF("mob target update event %08X, %d:%08X, %08X\n", (u32)moby, args->TargetUID, (u32)target, (u32)pvars->MobVars.Target);
	return 0;
}

int mobHandleEvent_StateUpdate(Moby* moby, GuberEvent* event)
{
	struct MobStateUpdateEventArgs args;
	struct MobPVar* pvars = (struct MobPVar*)moby->PVar;
	VECTOR t;
	if (!pvars || mobAmIOwner(moby))
		return 0;

	// read event
	guberEventRead(event, &args, sizeof(struct MobStateUpdateEventArgs));
  
  //DPRINTF("mob target update event %08X, %08X, %d:%08X, %08X\n", (u32)moby, (u32)event, args.TargetUID, (u32)target, (u32)pvars->MobVars.Target);
	
  // pass to actual handler
  return mobHandleEvent_StateUpdateUnreliable(moby, &args);
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

int mobOnUnreliableMsgRemote(void * connection, void * data)
{
  struct MobUnreliableBaseMsgArgs baseArgs;
  memcpy(&baseArgs, data, sizeof(struct MobUnreliableBaseMsgArgs));

  switch (baseArgs.MsgId)
  {
    case MOB_UNRELIABLE_MSG_ID_STATE_UPDATE:
    {
      struct MobUnreliableMsgStateUpdateArgs args;
      memcpy(&args, data, sizeof(struct MobUnreliableMsgStateUpdateArgs));

      if (isInGame()) {
        GuberMoby* guber = (GuberMoby*)guberGetObjectByUID(baseArgs.MobUID);
        if (guber) {
          Moby* moby = guber->Moby;
          if (moby && mobyIsMob(moby) && !mobyIsDestroyed(moby)) {
            mobHandleEvent_StateUpdateUnreliable(moby, &args.StateUpdate);
          }
        }
      }

      return sizeof(struct MobUnreliableMsgStateUpdateArgs);
    }
  }

  return sizeof(struct MobUnreliableBaseMsgArgs);
}

int mobCreate(int spawnParamsIdx, VECTOR position, float yaw, int spawnFromUID, int freeAgent, struct MobConfig *config)
{
  if (mapConfig->OnMobCreateFunc)
    return mapConfig->OnMobCreateFunc(spawnParamsIdx, position, yaw, spawnFromUID, freeAgent, config);

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
          args.DamageFlags = 0;
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

#if JOINT_TEST

      static int aaa1 = 0;
      char buf[32];
      MATRIX jointMtx;

      if (padGetButtonDown(0, PAD_L1 | PAD_L3) > 0) {
        aaa1 += 1;
        DPRINTF("%d\n", aaa1);
      }
      else if (padGetButtonDown(0, PAD_L1 | PAD_R3) > 0) {
        aaa1 -= 1;
        DPRINTF("%d\n", aaa1);
      }

      // get position of right hand joint
      mobyGetJointMatrix(m, aaa1, jointMtx);
      
      int x,y;
      if (gfxWorldSpaceToScreenSpace(&jointMtx[12], &x, &y)) {
        sprintf(buf, "%d", aaa1);
        gfxScreenSpaceText(x,y,1,1,0x80FFFFFF, buf, -1, 4);
      }

#endif

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
