#include "include/mob.h"
#include "include/drop.h"
#include "include/utils.h"
#include "include/game.h"
#include "include/bubble.h"
#include "config.h"
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
extern PatchConfig_t* playerConfig;

int mobOrderedDrawUpToIndex = MAX_MOBS_ALIVE;
int mobComplexitySum = 0;

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

int MobComplexityValueByOClass[MAX_MOB_SPAWN_PARAMS][2] = {};

Moby* AllMobsSorted[MAX_MOBS_ALIVE] = {};
int AllMobsSortedFreeSpots = MAX_MOBS_ALIVE;

extern struct SurvivalState State;

GuberEvent* mobCreateEvent(Moby* moby, u32 eventType);
void sendPlayerStats(int playerId);
int spawnGetRandomPoint(VECTOR out, struct MobSpawnParams* mob);
void playerRewardXp(int playerId, int weaponId, int xp);
int playerHasBlessing(int playerId, int blessing);

//--------------------------------------------------------------------------
int mobAmIOwner(Moby* moby)
{
	struct MobPVar* pvars = (struct MobPVar*)moby->PVar;
	return gameGetMyClientId() == pvars->MobVars.Owner;
}

//--------------------------------------------------------------------------
void mobStatsOnNewMobCreated(int spawnParamIdx, int spawnFromUID)
{
  State.MobStats.TotalSpawning++;
  if (!mapConfig) return;

  if (spawnFromUID == -1 && spawnParamIdx >= 0 && spawnParamIdx < mapConfig->DefaultSpawnParamsCount) {
    State.MobStats.NumAlive[spawnParamIdx]++;
    State.MobStats.NumSpawnedThisRound[spawnParamIdx]++;
    //State.MobStats.TotalAlive++;
    State.MobStats.TotalSpawned++;
    State.MobStats.TotalSpawnedThisRound++;
  }
}

//--------------------------------------------------------------------------
void mobStatsOnNewMobSpawned(Moby* moby, int spawnFromUID, int fromThisClient)
{
  if (fromThisClient) { State.MobStats.TotalSpawning--; }
  if (!moby) return;
  if (!mapConfig) return;
	struct MobPVar* pvars = (struct MobPVar*)moby->PVar;
  if (!pvars) return;

  // increment stats if we haven't already run mobStatsOnNewMobCreated
  int spIdx = pvars->MobVars.SpawnParamsIdx;
  if (spawnFromUID == -1 && !fromThisClient && spIdx >= 0 && spIdx < mapConfig->DefaultSpawnParamsCount) {
    State.MobStats.NumAlive[spIdx]++;
    State.MobStats.NumSpawnedThisRound[spIdx]++;
    State.MobStats.TotalAlive++;
    State.MobStats.TotalSpawned++;
    State.MobStats.TotalSpawnedThisRound++;
  }
}

//--------------------------------------------------------------------------
void mobStatsOnMobDestroyed(Moby* moby)
{
  if (!moby) return;
  if (!mapConfig) return;
	struct MobPVar* pvars = (struct MobPVar*)moby->PVar;
  if (!pvars) return;

  int spIdx = pvars->MobVars.SpawnParamsIdx;
  if (spIdx >= 0 && spIdx < mapConfig->DefaultSpawnParamsCount) {
    State.MobStats.NumAlive[spIdx]--;
    State.MobStats.TotalAlive--;
  }
}

//--------------------------------------------------------------------------
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

//--------------------------------------------------------------------------
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

//--------------------------------------------------------------------------
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

  // action update
  msg.StateUpdate.Action = pvars->MobVars.Action;
  msg.StateUpdate.ActionId = pvars->MobVars.ActionId;
  msg.StateUpdate.Random = pvars->MobVars.DynamicRandom;

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

//--------------------------------------------------------------------------
void mobSendOwnerUpdate(Moby* moby, char owner)
{
	struct MobPVar* pvars = (struct MobPVar*)moby->PVar;

	// create event
	GuberEvent * guberEvent = mobCreateEvent(moby, MOB_EVENT_OWNER_UPDATE);
	if (guberEvent) {
		guberEventWrite(guberEvent, &owner, sizeof(pvars->MobVars.Owner));
	}
}

//--------------------------------------------------------------------------
void mobSendDamageEvent(Moby* moby, Moby* sourcePlayer, Moby* source, float amount, int damageFlags)
{
	VECTOR delta;
	struct MobDamageEventArgs args;
  memset(&args, 0, sizeof(args));

	// determine knockback
	Player * pDamager = playerGetFromUID(guberGetUID(sourcePlayer));
	if (pDamager)
	{
		int weaponId = getWeaponIdFromOClass(source->OClass);
		if (weaponId >= 0) {
			args.Knockback.Power = (u8)playerGetWeaponAlphaModCount(pDamager, weaponId, ALPHA_MOD_IMPACT);
	    args.Knockback.Ticks = PLAYER_KNOCKBACK_BASE_TICKS;
		}

    if (weaponId == WEAPON_ID_FLAIL) {
      args.Knockback.Ticks = PLAYER_KNOCKBACK_BASE_TICKS;
      args.Knockback.Power += pDamager->GadgetBox->Gadgets[WEAPON_ID_FLAIL].Level + 1;
    } else if (weaponId == WEAPON_ID_OMNI_SHIELD) {
      damageFlags |= 0x40000000; // short slowdown
      amount *= 2; // damage buff
      amount *= pDamager->DamageMultiplier; // quad doesn't seem to affect holos
    } else if (weaponId == WEAPON_ID_ARBITER) {
      amount *= 2; // damage buff
    } else if (weaponId == WEAPON_ID_VIPERS && pDamager->GadgetBox->Gadgets[weaponId].Level == 9) {
      amount *= 0.75; // damage nerf
    }

    // prestige damage 2x per prestige
    int weaponSlot = weaponIdToSlot(weaponId);
    amount *= 1 + (2 * State.PlayerStates[pDamager->PlayerId].State.WeaponPrestige[weaponSlot]);
    
    // crit
    if (pDamager->IsLocal) {
      int critCount = State.PlayerStates[pDamager->PlayerId].State.Upgrades[UPGRADE_CRIT];
      float critProbability = critCount * PLAYER_UPGRADE_CRIT_FACTOR;
      float r = randRange(0, 1);
      if (r < critProbability) {
        amount *= 3;
        damageFlags |= 0x20000000;
        //mobyPlaySoundByClass(4, 0, moby, 0x10A9);
        //mobyPlaySoundByClass(0, 0, moby, MOBY_ID_WRENCH);
      }
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

//--------------------------------------------------------------------------
int getMaxComplexity(void)
{
  int maxComplexity = MAX_MOB_COMPLEXITY_DRAWN;
  int i = 0;
  Player** players = playerGetAll();

  // reduce by lod
  // maxComplexity -= MOB_COMPLEXITY_LOD_FACTOR * (playerConfig ? playerConfig->levelOfDetail : 2);

  // dzo bypasses max complexity
  if (PATCH_INTEROP->Client == CLIENT_TYPE_DZO)
    maxComplexity = MAX_MOB_COMPLEXITY_DRAWN_DZO;

  // reduce by map complexity
  maxComplexity -= State.MapBaseComplexity;

  // reduce by number of visible players
  for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
    Player* p = players[i];
    if (!p) continue;
    if (!p->SkinMoby) continue;

    if (p->SkinMoby->Drawn) {
      maxComplexity -= MOB_COMPLEXITY_SKIN_FACTOR;
    }
  }

	if (maxComplexity < MAX_MOB_COMPLEXITY_MIN)
    return MAX_MOB_COMPLEXITY_MIN;
  return maxComplexity;
}

//--------------------------------------------------------------------------
int mobyComputeComplexity(Moby * moby)
{
  int complexity = 0;
  int i;

  if (mobyIsMob(moby) && mapConfig) {
    struct MobPVar* pvars = (struct MobPVar*)moby->PVar;

    // pull from spawn params
    if (pvars->MobVars.SpawnParamsIdx >= 0 && pvars->MobVars.SpawnParamsIdx < mapConfig->DefaultSpawnParamsCount) {
      return mapConfig->DefaultSpawnParams[pvars->MobVars.SpawnParamsIdx].Cost;
    }
  }

  // baked by oclass
  switch (moby->OClass)
  {
    case ZOMBIE_MOBY_OCLASS: return ZOMBIE_RENDER_COST;
    case TREMOR_MOBY_OCLASS: return TREMOR_RENDER_COST;
    case SWARMER_MOBY_OCLASS: return SWARMER_RENDER_COST;
    case REACTOR_MOBY_OCLASS: return REACTOR_RENDER_COST;
    case REAPER_MOBY_OCLASS: return REAPER_RENDER_COST;
    case EXECUTIONER_MOBY_OCLASS: return EXECUTIONER_RENDER_COST;
  }

  if (!moby || !moby->PClass)
    return 0;

  void * pclass =moby->PClass;
  int highPacketCnt = *(char*)(pclass + 0x04);
  int lowPacketCnt = *(char*)(pclass + 0x05);
  int metalPacketCnt = *(char*)(pclass + 0x0A);
  //int jointCnt = *(char*)(pclass + 0x08);
  void * packets = *(void**)pclass;

  if (packets) {

    int count = highPacketCnt + lowPacketCnt + metalPacketCnt;
    for (i = 0; i < count; ++i) {
      // int vertexDataSize = *(char*)(packets + 0x0C);
      int vifListSize = *(short*)(packets + 0x04);
      // int vertexCount = *(u8*)(packets + 0x0f);
      complexity += vifListSize * 0x10;
      packets += 0x10;
    }
  }

  //complexity += jointCnt * 0x40;

  return complexity/0x10;
}

//--------------------------------------------------------------------------
int mobyGetComplexity(Moby* moby)
{
  if (!moby)
    return 0;

  int i;
  int freeIdx = -1;
  float factor = 1;

  if (playerGetNumLocals() > 1)
    factor *= 2;
  
  VECTOR dt;
  Player* p0 = playerGetFromSlot(0);
  if (p0) {
    vector_subtract(dt, moby->Position, p0->CameraPos);
    if (vector_sqrmag(dt) < (5*5))
      factor *= 2;
  }

  // ensure we don't already have
  for (i = 0; i < MAX_MOB_SPAWN_PARAMS; ++i) {
    int oclass = MobComplexityValueByOClass[i][0];
    if (oclass == moby->OClass)
      return MobComplexityValueByOClass[i][1] * factor;
    else if (oclass == 0 && freeIdx < 0)
      freeIdx = i;
  }

  if (freeIdx < 0)
    return 0;

  // disable lod low
  /*
  int lowPacketCnt = *(char*)(moby->PClass + 0x05);
  if (lowPacketCnt > 0) {

    int highPacketCnt = *(char*)(moby->PClass + 0x04);
    int copyPacketCnt = lowPacketCnt + *(char*)(moby->PClass + 0x0A);
    void * packets = *(void**)moby->PClass;

    memmove(packets, packets + highPacketCnt*0x10, copyPacketCnt * 0x10);

    *(char*)(moby->PClass + 0x04) = lowPacketCnt;
    *(char*)(moby->PClass + 0x05) = 0;
    *(char*)(moby->PClass + 0x0A) = 0;
    *(char*)(moby->PClass + 0x0E) = -1;
    *(char*)(moby->PClass + 0x06) = 0;
    DPRINTF("%04X -> %08X (%d)\n", moby->OClass, (u32)moby->PClass, mobyComputeComplexity(moby));
  }
  */

  int complexity = mobyComputeComplexity(moby);

#if LOG_STATS2
  DPRINTF("%04X -> %08X (%d) (max %d)\n", moby->OClass, (u32)moby->PClass, complexity, (getMaxComplexity() / complexity) + 1);
#endif

  // compute and save
  MobComplexityValueByOClass[freeIdx][0] = moby->OClass;
  MobComplexityValueByOClass[freeIdx][1] = complexity;
  return complexity * factor;
}

//--------------------------------------------------------------------------
void mobDestroy(Moby* moby, int hitByUID)
{
	char killedByPlayerId = -1;
	char weaponId = -1;

  if (!moby)
    return;

	struct MobPVar* pvars = (struct MobPVar*)moby->PVar;
  if (!pvars)
    return;

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

//--------------------------------------------------------------------------
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

//--------------------------------------------------------------------------
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
    args.Random = (char)rand(255);
		guberEventWrite(event, &args, sizeof(struct MobActionUpdateEventArgs));
	}
}

//--------------------------------------------------------------------------
int mobGetLostArmorBangle(short armorStart, short armorEnd)
{
	return (armorStart - armorEnd) & armorStart;
}

//--------------------------------------------------------------------------
int mobIsFrozen(Moby* moby)
{
  if (!moby || !moby->PVar)
    return 0;

	struct MobPVar* pvars = (struct MobPVar*)moby->PVar;
  return State.Freeze && pvars->MobVars.Config.MobAttribute != MOB_ATTRIBUTE_FREEZE && pvars->MobVars.Health > 0;
}

//--------------------------------------------------------------------------
int mobHasVelocity(struct MobPVar* pvars)
{
	VECTOR t;
	vector_projectonhorizontal(t, pvars->MobVars.MoveVars.Velocity);
	return vector_sqrmag(t) >= 0.0001;
}

//--------------------------------------------------------------------------
void mobHandleDraw(Moby* moby)
{
	int i;
	struct MobPVar* pvars = (struct MobPVar*)moby->PVar;
  int minMobsForHiding = 20;
  float minRankForDrawCutoff = 0.75;

	// if we aren't in the sorted list, try and find an empty spot
	if (pvars->MobVars.Order < 0 && AllMobsSortedFreeSpots > 0) {
		for (i = 0; i < MAX_MOBS_ALIVE; ++i) {
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

  // dzo clients don't need draw optimization as much
  // unfortunately the cost of each mob can cause problems on the VU
  // so we are forced to stop animating some
  if (PATCH_INTEROP->Client == CLIENT_TYPE_DZO) {
    minMobsForHiding = 30;
    minRankForDrawCutoff = 0.25;
  }

  if (1) {
    moby->DrawDist = 128;
    if (pvars->MobVars.Order >= mobOrderedDrawUpToIndex) {
      if (pvars->VTable && pvars->VTable->PostDraw) {
        gfxRegisterDrawFunction((void**)0x0022251C, (gfxDrawFuncDef*)pvars->VTable->PostDraw, moby);
        moby->DrawDist = 0;
      }
    }
  } else {
    int order = pvars->MobVars.Order;
    moby->DrawDist = 128;
    if (order >= 0) {
      float rank = 1 - clamp(order / (float)State.RoundMaxSpawnedAtOnce, 0, 1);
      //float rankCurve = powf(rank, 6);
      
      if (State.MobStats.TotalAlive > minMobsForHiding) {
        if (rank < minRankForDrawCutoff) {
          if (pvars->VTable && pvars->VTable->PostDraw) {
            gfxRegisterDrawFunction((void**)0x0022251C, (gfxDrawFuncDef*)pvars->VTable->PostDraw, moby);
            moby->DrawDist = 0;
          }
        }
      }
    }
  }
}

//--------------------------------------------------------------------------
void mobUpdate(Moby* moby)
{
	int i;
	int isOwner;
	struct MobPVar* pvars = (struct MobPVar*)moby->PVar;
	GameOptions* gameOptions = gameGetOptions();
  GameSettings* gameSettings = gameGetSettings();
  Player** players = playerGetAll();
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
	if (gameTime != State.MobStats.MobsDrawGameTime) {
		State.MobStats.MobsDrawGameTime = gameTime;
		State.MobStats.MobsDrawnLast = State.MobStats.MobsDrawnCurrent;
		State.MobStats.MobsDrawnCurrent = 0;
	}
	if (moby->Drawn)
		State.MobStats.MobsDrawnCurrent++;

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
	Player * ownerPlayer = NULL;
  for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
    Player* p = players[i];
    if (p && p->PlayerMoby && p->pNetPlayer && gameSettings->PlayerClients[i] == pvars->MobVars.Owner) {
      ownerPlayer = p;
      break;
    }
  }

  // default owner to host
	if (!ownerPlayer && gameAmIHost()) {
		pvars->MobVars.Owner = gameGetHostId();
		mobSendOwnerUpdate(moby, pvars->MobVars.Owner);
  }
	
	// determine if I'm the owner
	isOwner = mobAmIOwner(moby);

	// change owner to target
	if (isOwner && pvars->MobVars.Target) {
		Player * targetPlayer = (Player*)guberGetObjectByMoby(pvars->MobVars.Target);
		if (targetPlayer && targetPlayer->PlayerMoby && targetPlayer->pNetPlayer && targetPlayer->Guber.Id.GID.HostId != pvars->MobVars.Owner) {
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
			}
		}
		
		// 
		if (nextCheckActionDelayTicks == 0 && pvars->VTable && pvars->VTable->GetPreferredAction) {
      int delayTicks = 0;
      int nextAction = pvars->VTable->GetPreferredAction(moby, &delayTicks);
			if (nextAction >= 0 && nextAction != pvars->MobVars.NextAction && nextAction != pvars->MobVars.Action) {
				pvars->MobVars.NextAction = nextAction;
				pvars->MobVars.NextActionDelayTicks = delayTicks; //nextAction >= MOB_ACTION_ATTACK ? pvars->MobVars.Config.ReactionTickCount : 0;
			} 

			// get new target
			if (scoutCooldownTicks == 0 && pvars->VTable->GetNextTarget) {
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
		u8 targetOpacity = pvars->VTable->IsAttacking(moby) ? 0x80 : 0x10;
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

  if (colDamage && damage > 0 && colDamage->Damager) {
    Player * damager = guberMobyGetPlayerDamager(colDamage->Damager);
    if (damager) {
      damage *= 1 + (PLAYER_UPGRADE_DAMAGE_FACTOR * State.PlayerStates[damager->PlayerId].State.Upgrades[UPGRADE_DAMAGE]);

      // deal extra damage after being hit
      if (damager->timers.postHitInvinc > 0) {
        damage *= 2;
      }
    }

    // give mob final say before damage is applied
    // we also want the local players to register their own damage
    // or if damage came from non-player, have mob owner register damage
    struct MobLocalDamageEventArgs e = {
      .Damage = damage,
      .DamageFlags = damageFlags,
      .Damager = colDamage->Damager,
      .PlayerDamager = damager
    };

    if (!pvars->VTable || pvars->VTable->OnLocalDamage(moby, &e)) {
      if (e.PlayerDamager && playerIsLocal(e.PlayerDamager)) {
        mobSendDamageEvent(moby, e.PlayerDamager->PlayerMoby, e.Damager, e.Damage, e.DamageFlags);	
      } else if (!e.PlayerDamager && isOwner) {
        mobSendDamageEvent(moby, e.Damager, e.Damager, e.Damage, e.DamageFlags);	
      }
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

    // auto destruct after 15 seconds of falling
    else if (pvars->MobVars.TimeLastGroundedTicks > (TPS * 15)) {
      pvars->MobVars.Respawn = 1;
    }

    // auto destruct after 15 seconds of being stuck
    else if (pvars->MobVars.MoveVars.StuckCounter > 5) {
      pvars->MobVars.Respawn = 1;
    }
    
    // destroy
    if (pvars->MobVars.Destroy) {
			mobDestroy(moby, pvars->MobVars.LastHitBy);
		}

		// respawn
		else if (pvars->MobVars.Respawn) {

      // pass to mob
      // let mob override respawn logic
      if (!pvars->VTable->OnRespawn || pvars->VTable->OnRespawn(moby)) {
        VECTOR p;
        struct MobConfig config;
        struct MobSpawnParams* spawnParams = &mapConfig->DefaultSpawnParams[pvars->MobVars.SpawnParamsIdx];
        if (spawnGetRandomPoint(p, spawnParams)) {
          memcpy(&config, &pvars->MobVars.Config, sizeof(struct MobConfig));
          mobCreate(pvars->MobVars.SpawnParamsIdx, p, 0, guberGetUID(moby), pvars->MobVars.SpawnFlags, &config);
        }
        
        pvars->MobVars.Destroyed = 2;
      }

			pvars->MobVars.Respawn = 0;
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

//--------------------------------------------------------------------------
GuberEvent* mobCreateEvent(Moby* moby, u32 eventType)
{
	GuberEvent * event = NULL;

	// create guber object
	Guber* guber = guberGetObjectByMoby(moby);
	if (guber)
		event = guberEventCreateEvent(guber, eventType, 0, 0);

	return event;
}

//--------------------------------------------------------------------------
int mobHandleEvent_Spawn(Moby* moby, GuberEvent* event)
{
	VECTOR p;
	float yaw;
  int i;
	int spawnFromUID;
  int spawnFlags;
	char random;
	struct MobSpawnEventArgs args;

  // 
  int fromThisClient = event->NetEvent.OriginClientIdx == gameGetMyClientId();

	// read event
	guberEventRead(event, p, 12);
	guberEventRead(event, &yaw, 4);
	guberEventRead(event, &spawnFromUID, 4);
	guberEventRead(event, &spawnFlags, 4);
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
  moby->UpdateDist = -1;


	// update pvars
	struct MobPVar* pvars = (struct MobPVar*)moby->PVar;
	memset(pvars, 0, sizeof(struct MobPVar));
	pvars->TargetVarsPtr = &pvars->TargetVars;
	pvars->MoveVarsPtr = NULL; // &pvars->MoveVars;
	pvars->FlashVarsPtr = &pvars->FlashVars;
	pvars->ReactVarsPtr = &pvars->ReactVars;
  pvars->AdditionalMobVarsPtr = pvars + 1;
	
  // free agent data
  pvars->MobVars.SpawnFlags = spawnFlags;

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
  pvars->MobVars.DynamicRandom = random;
  vector_copy(pvars->MobVars.MoveVars.NextPosition, p);
#if MOB_NO_MOVE
	pvars->MobVars.Config.Speed = 0.001;
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

  // spawn flags
  if (spawnFlags & MOB_SPAWN_FLAG_RUSSIAN_DOLL) {
    // give velocity on spawn
    float theta = randRadian();
    vector_fromyaw(pvars->MobVars.MoveVars.AddVelocity, theta);
    vector_scale(pvars->MobVars.MoveVars.AddVelocity, pvars->MobVars.MoveVars.AddVelocity, 6 * MATH_DT);
    pvars->MobVars.MoveVars.AddVelocity[2] = 6 * MATH_DT;
  }

	// 
	mobySetState(moby, 0, -1);
  mobStatsOnNewMobSpawned(moby, spawnFromUID, fromThisClient);

	// destroy spawn from
	if (spawnFromUID != -1) {
		GuberMoby* gm = (GuberMoby*)guberGetObjectByUID(spawnFromUID);
		if (gm && gm->Moby && gm->Moby->PVar && !mobyIsDestroyed(gm->Moby) && mobyIsMob(gm->Moby)) {
			struct MobPVar* spawnFromPVars = (struct MobPVar*)gm->Moby->PVar;
			if (spawnFromPVars->MobVars.Destroyed != 1) {
				guberMobyDestroy(gm->Moby);
			}
		}
	}

	// if we aren't in the sorted list, try and find an empty spot
	if (pvars->MobVars.Order < 0 && AllMobsSortedFreeSpots > 0) {
		for (i = 0; i < MAX_MOBS_ALIVE; ++i) {
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

  // pass to map
  if (mapConfig->OnMobSpawnedFunc)
    mapConfig->OnMobSpawnedFunc(moby);

  // pass to mob handler
  if (pvars->VTable && pvars->VTable->OnSpawn)
    pvars->VTable->OnSpawn(moby, p, yaw, spawnFromUID, random, &args);
	
#if LOG_STATS2
	DPRINTF("mob created event %08X, %08X, %08X spawnArgsIdx:%d spawnedNum:%d roundTotal:%d)\n", (u32)moby, (u32)event, (u32)moby->GuberMoby, args.SpawnParamsIdx, State.MobStats.TotalAlive, State.MobStats.TotalSpawnedThisRound);
#endif
	return 0;
}

//--------------------------------------------------------------------------
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
  if (mapConfig && mapConfig->CreateMobDropFunc && (!State.RoundIsSpecial || !mapConfig->SpecialRoundParams[State.RoundSpecialIdx].DisableDrops)) {
    if (killedByPlayerId >= 0 && gameAmIHost()) {
      Player * killedByPlayer = players[(int)killedByPlayerId];
      if (killedByPlayer) {
        float randomValue = randRange(0.0, 1.0);
        float probability = playerHasBlessing(killedByPlayerId, BLESSING_ITEM_LUCK) ? MOB_HAS_DROP_PROBABILITY_LUCKY : MOB_HAS_DROP_PROBABILITY;
        if (randomValue < probability) {
          mapConfig->CreateMobDropFunc(moby->Position, randRangeInt(0, DROP_COUNT-1), gameGetTime() + DROP_DURATION, killedByPlayer->Team);
        }
      }
    }
  }
#endif  

#if SHARED_BOLTS
	if (killedByPlayerId >= 0) {
		Player * killedByPlayer = players[(int)killedByPlayerId];
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
		int multiplier = State.PlayerStates[(int)killedByPlayerId].IsDoublePoints ? 2 : 1;
		State.PlayerStates[(int)killedByPlayerId].State.Bolts += bolts * multiplier;
		State.PlayerStates[(int)killedByPlayerId].State.TotalBolts += bolts * multiplier;
	}
#endif

  // shared xp
  if (mapConfig && mapConfig->DefaultSpawnParams[pvars->MobVars.SpawnParamsIdx].Config.SharedXp) {
    for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
      Player* p = players[i];
      if (p && !playerIsDead(p) && i != killedByPlayerId) {
        playerRewardXp(i, 0, xp);
      }
    }
  }

	if (killedByPlayerId >= 0) {
		Player * killedByPlayer = players[(int)killedByPlayerId];
		struct SurvivalPlayer* pState = &State.PlayerStates[(int)killedByPlayerId];
		GameData * gameData = gameGetData();

    // give xp
    playerRewardXp(killedByPlayerId, weaponId, xp);

		// handle weapon jackpot
		if (weaponId > 1 && killedByPlayer) {
			int jackpotCount = playerGetWeaponAlphaModCount(killedByPlayer, weaponId, ALPHA_MOD_JACKPOT);

			pState->State.Bolts += jackpotCount * JACKPOT_BOLTS;
			pState->State.TotalBolts += jackpotCount * JACKPOT_BOLTS;
		}

		// handle stats
		pState->State.Kills++;
    pState->State.KillsPerMob[pvars->MobVars.SpawnParamsIdx]++;
		gameData->PlayerStats.Kills[(int)killedByPlayerId]++;
		int weaponSlotId = weaponIdToSlot(weaponId);
		if (weaponId > 0 && (weaponId != WEAPON_ID_WRENCH || weaponSlotId == WEAPON_SLOT_WRENCH))
			gameData->PlayerStats.WeaponKills[(int)killedByPlayerId][weaponSlotId]++;
	}

	if (pvars->MobVars.Order >= 0) {
		AllMobsSorted[(int)pvars->MobVars.Order] = NULL;
		++AllMobsSortedFreeSpots;
	}

  mobStatsOnMobDestroyed(moby);
	guberMobyDestroy(moby);
	moby->ModeBits &= ~0x30;
	pvars->MobVars.Destroyed = 1;

#if LOG_STATS2
	DPRINTF("mob destroy event %08X, %08X, by client %d, (%d)\n", (u32)moby, (u32)event, killedByPlayerId, State.MobStats.TotalAlive);
#endif
	return 0;
}

//--------------------------------------------------------------------------
int mobHandleEvent_Damage(Moby* moby, GuberEvent* event)
{
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

#if MOB_INVINCIBLE
  pvars->MobVars.Config.MaxHealth = (args.DamageQuarters / 4.0) + 1;
  pvars->MobVars.Config.Health = pvars->MobVars.Config.MaxHealth;
  pvars->MobVars.Health = pvars->MobVars.Config.MaxHealth;
#endif

  // pass to mob handler
  if (pvars->VTable && pvars->VTable->OnDamage)
    pvars->VTable->OnDamage(moby, &args);
	
	// flash
	mobyStartFlash(moby, FT_HIT, 0x800000FF, 0);

	// decrement health
	float damage = args.DamageQuarters / 4.0;
  float appliedDamage = maxf(0, minf(damage, pvars->MobVars.Health));
	float newHp = pvars->MobVars.Health - damage;
#if !MOB_INVINCIBLE
	pvars->MobVars.Health = newHp;
	pvars->TargetVars.hitPoints = newHp;
#endif

	// get damager
	Player* damager = playerGetFromUID(args.SourceUID);
  if (appliedDamage > 0) { // && damager && damager->IsLocal) {
    VECTOR mobCenter = {0,0,pvars->TargetVars.targetHeight,0};
    vector_add(mobCenter, mobCenter, moby->Position);

    int isLocal = 0;
    if (damager) isLocal = damager->IsLocal;
    bubblePush(mobCenter, pvars->MobVars.Config.CollRadius, appliedDamage, isLocal, (args.DamageFlags & 0x20000000) ? 1 : 0);
  }

  // 

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

//--------------------------------------------------------------------------
int mobHandleEvent_ActionUpdate(Moby* moby, GuberEvent* event)
{
	struct MobActionUpdateEventArgs args;
	struct MobPVar* pvars = (struct MobPVar*)moby->PVar;
	if (!pvars)
		return 0;

	// read event
	guberEventRead(event, &args, sizeof(struct MobActionUpdateEventArgs));

	// 
	if (SEQ_DIFF_U8(pvars->MobVars.LastActionId, args.ActionId) > 0 && pvars->MobVars.Action != args.Action) {
		pvars->MobVars.LastActionId = args.ActionId;
    pvars->MobVars.LastAction = pvars->MobVars.Action;
    
    // pass to mob handler
    if (pvars->VTable && pvars->VTable->ForceLocalAction)
      pvars->VTable->ForceLocalAction(moby, args.Action);
	}

  pvars->MobVars.DynamicRandom = args.Random;
	
#if LOG_STATS2
	DPRINTF("mob state update event %08X, %08X, %d\n", (u32)moby, (u32)event, args.Action);
#endif
	return 0;
}

//--------------------------------------------------------------------------
int mobHandleEvent_StateUpdateUnreliable(Moby* moby, struct MobStateUpdateEventArgs* args)
{
	struct MobPVar* pvars = (struct MobPVar*)moby->PVar;
	VECTOR t;
	if (!pvars || mobAmIOwner(moby) || !args)
		return 0;

	// teleport position if far away
	vector_subtract(t, moby->Position, args->Position);
	if (vector_sqrmag(t) > 25) {
		vector_copy(moby->Position, args->Position);
		vector_copy(pvars->MobVars.MoveVars.NextPosition, args->Position);
  }

	// 
	if (0 && SEQ_DIFF_U8(pvars->MobVars.LastActionId, args->ActionId) > 0 && pvars->MobVars.Action != args->Action) {
		pvars->MobVars.LastActionId = args->ActionId;
    pvars->MobVars.LastAction = pvars->MobVars.Action;
    pvars->MobVars.DynamicRandom = args->Random;
    
    // pass to mob handler
    if (pvars->VTable && pvars->VTable->ForceLocalAction)
      pvars->VTable->ForceLocalAction(moby, args->Action);
	}

	// 
	Player* target = (Player*)guberGetObjectByUID(args->TargetUID);
	if (target) {
    Moby* targetMoby = playerGetTargetMoby(target);
    if (targetMoby != pvars->MobVars.Target) {
      pvars->MobVars.Target = targetMoby;
    }
  }

#if FIXEDTARGET
  pvars->MobVars.Target = FIXEDTARGETMOBY;
#endif

  // pass to mob
  if (pvars->VTable && pvars->VTable->OnStateUpdate)
	  pvars->VTable->OnStateUpdate(moby, args);

	//DPRINTF("mob target update event %08X, %d:%08X, %08X\n", (u32)moby, args->TargetUID, (u32)target, (u32)pvars->MobVars.Target);
	return 0;
}

//--------------------------------------------------------------------------
int mobHandleEvent_StateUpdate(Moby* moby, GuberEvent* event)
{
	struct MobStateUpdateEventArgs args;
	struct MobPVar* pvars = (struct MobPVar*)moby->PVar;
	if (!pvars || mobAmIOwner(moby))
		return 0;

	// read event
	guberEventRead(event, &args, sizeof(struct MobStateUpdateEventArgs));
  
  //DPRINTF("mob target update event %08X, %08X, %d:%08X, %08X\n", (u32)moby, (u32)event, args.TargetUID, (u32)target, (u32)pvars->MobVars.Target);
	
  // pass to actual handler
  return mobHandleEvent_StateUpdateUnreliable(moby, &args);
}

//--------------------------------------------------------------------------
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

//--------------------------------------------------------------------------
int mobHandleEvent_Custom(Moby* moby, GuberEvent* event)
{
	struct MobPVar* pvars = (struct MobPVar*)moby->PVar;
	if (!pvars)
		return 0;

  if (pvars->VTable && pvars->VTable->OnCustomEvent)
    pvars->VTable->OnCustomEvent(moby, event);
	return 0;
}

//--------------------------------------------------------------------------
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
      case MOB_EVENT_CUSTOM: return mobHandleEvent_Custom(moby, event);
			default:
			{
				DPRINTF("unhandle mob event %d\n", mobEvent);
				break;
			}
		}
	}

	return 0;
}

//--------------------------------------------------------------------------
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

//--------------------------------------------------------------------------
int mobCreate(int spawnParamsIdx, VECTOR position, float yaw, int spawnFromUID, int spawnFlags, struct MobConfig *config)
{
  // log
  mobStatsOnNewMobCreated(spawnParamsIdx, spawnFromUID);

  if (mapConfig->OnMobCreateFunc)
    return mapConfig->OnMobCreateFunc(spawnParamsIdx, position, yaw, spawnFromUID, spawnFlags, config);

  return 0;
}

//--------------------------------------------------------------------------
void mobInitialize(void)
{
	// set vtable callbacks
	*(u32*)0x003A0A84 = (u32)&getGuber;
	*(u32*)0x003A0A94 = (u32)&handleEvent;

	// collision hit type
	//*(u32*)0x004bd1f0 = 0x08000000 | ((u32)&colHotspot / 4);
	//*(u32*)0x004bd1f4 = 0x00402021;

	// 
  memset(MobComplexityValueByOClass, 0, sizeof(MobComplexityValueByOClass));
	memset(AllMobsSorted, 0, sizeof(AllMobsSorted));
}

//--------------------------------------------------------------------------
void mobNuke(int killedByPlayerId)
{
	Player** players = playerGetAll();
	u32 playerUid = 0;

  // only let host destroy
  if (!gameAmIHost())
    return;

  // get uid of player
	if (killedByPlayerId >= 0 && killedByPlayerId < GAME_MAX_PLAYERS) {
		Player* p = players[killedByPlayerId];
		if (p) {
			playerUid = p->Guber.Id.UID;
		}
	}

  Moby* m = mobyListGetStart();
  Moby* mEnd = mobyListGetEnd();

  while (m < mEnd) {
    if (!mobyIsDestroyed(m) && mobyIsMob(m)) {
      mobDestroy(m, playerUid);
    }

    ++m;
  }
}

//--------------------------------------------------------------------------
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

	for (i = 0; i < MAX_MOBS_ALIVE; ++i) {
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

//--------------------------------------------------------------------------
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

  // reset
  State.MobStats.TotalAlive = 0;
  memset(State.MobStats.NumAlive, 0, sizeof(State.MobStats.NumAlive));
  mobComplexitySum = 0;
  mobOrderedDrawUpToIndex = MAX_MOBS_ALIVE;
  int maxComplexity = getMaxComplexity();

	// run single pass on sort
	for (i = 0; i < MAX_MOBS_ALIVE; ++i)
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

      State.MobStats.TotalAlive++;
      if (mapConfig && pvars->MobVars.SpawnParamsIdx >= 0 && pvars->MobVars.SpawnParamsIdx < mapConfig->DefaultSpawnParamsCount)
        State.MobStats.NumAlive[pvars->MobVars.SpawnParamsIdx]++;

      int complexity = mobyGetComplexity(m);
      mobComplexitySum += complexity;
      if (i < mobOrderedDrawUpToIndex && mobComplexitySum > maxComplexity) {
        mobOrderedDrawUpToIndex = i-1;
      }

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
      //mobyComputeJointWorldMatrix(m, aaa1, jointMtx);
      
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

#if PRINT_MOB_COMPLEXITY
  //DPRINTF("MOB COMPLEXITY %d\n", mobComplexitySum);
  char buf[64];
  snprintf(buf, 64, "%d/%d -> %d", mobComplexitySum, maxComplexity, State.MobStats.MobsDrawnCurrent);
  gfxScreenSpaceText(0, 0, 1, 1, 0x80FFFFFF, buf, -1, 0);
#endif
}
