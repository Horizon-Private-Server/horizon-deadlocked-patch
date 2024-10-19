/***************************************************
 * FILENAME :		spawner.c
 * 
 * DESCRIPTION :
 * 		Handles logic for the spawners.
 * 		
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */

#include <tamtypes.h>

#include <libdl/dl.h>
#include <libdl/player.h>
#include <libdl/pad.h>
#include <libdl/time.h>
#include <libdl/net.h>
#include "module.h"
#include "messageid.h"
#include <libdl/game.h>
#include <libdl/string.h>
#include <libdl/math.h>
#include <libdl/math3d.h>
#include <libdl/stdio.h>
#include <libdl/gamesettings.h>
#include <libdl/dialog.h>
#include <libdl/sound.h>
#include <libdl/patch.h>
#include <libdl/ui.h>
#include <libdl/graphics.h>
#include <libdl/spawnpoint.h>
#include <libdl/color.h>
#include <libdl/utils.h>
#include <libdl/random.h>
#include "include/maputils.h"
#include "include/shared.h"
#include "../../include/spawner.h"
#include "../../include/mob.h"
#include "../../include/game.h"

extern struct RaidsMapConfig MapConfig;

//--------------------------------------------------------------------------
int spawnerIsValidCuboidSpawnIdx(Moby* moby, int index)
{
  struct SpawnerPVar* pvars = (struct SpawnerPVar*)moby->PVar;
  return pvars->SpawnCuboidIds[index] >= 0;
}

//--------------------------------------------------------------------------
int spawnerIsValidMobSpawnIdx(Moby* moby, int index)
{
  struct SpawnerPVar* pvars = (struct SpawnerPVar*)moby->PVar;
  return pvars->SpawnableMobParam[index].MobParamIdx >= 0 && pvars->SpawnableMobParam[index].Probability > 0;
}

//--------------------------------------------------------------------------
int spawnerGetRandomSpawnPoint(Moby* moby, int mobParamsIdx, VECTOR outPos, float* outYaw)
{
  VECTOR pos = {0,0,3,0};
  struct SpawnerPVar* pvars = (struct SpawnerPVar*)moby->PVar;
  int i;

  int selSpawnIdx = selectRandomIndex(SPAWNER_MAX_SPAWN_CUBOIDS, moby, spawnerIsValidCuboidSpawnIdx);
  if (selSpawnIdx < 0) return 0;

  int cuboidIdx = pvars->SpawnCuboidIds[selSpawnIdx];
  if (cuboidIdx < 0) return 0;

  // get cuboid
  SpawnPoint* cuboid = spawnPointGet(cuboidIdx);

  // determine where to spawn mob
  pos[0] = randRange(-1, 1);
  pos[1] = randRange(-1, 1);
  pos[2] = 0;
  vector_apply(pos, pos, cuboid->M0);
  pos[2] += 3;

  if (outPos) vector_copy(outPos, pos);
  if (outYaw) *outYaw = cuboid->M1[14] + randRadian();
  return 1;
}

//--------------------------------------------------------------------------
int spawnerSpawn(Moby* moby, int mobParamsIdx, int fromUid)
{
  if (!MapConfig.TryCreateMobFunc) return 0;
  struct SpawnerPVar* pvars = (struct SpawnerPVar*)moby->PVar;
  struct SpawnerMobParams* mobParams = &pvars->SpawnableMobParam[mobParamsIdx];
  struct MobSpawnParams* mobSpawnParams = &MapConfig.MobSpawnParams[mobParams->MobParamIdx];

  struct MobCreateArgs args = {
    .SpawnParamsIdx = mobParams->MobParamIdx,
    .Parent = moby,
    .Userdata = mobParamsIdx,
    .DifficultyMult = mobParams->DifficultyMultiplier,
    .Config = &mobSpawnParams->Config,
    .SpawnFromUID = fromUid,
  };

  if (spawnerGetRandomSpawnPoint(moby, mobParamsIdx, args.Position, &args.Yaw)) {
    if (MapConfig.TryCreateMobFunc(&args)) {
      pvars->State.NumSpawned[mobParamsIdx]++;
      pvars->State.NumTotalSpawned++;
      return 1;
    }
  }

  return 0;
}

//--------------------------------------------------------------------------
int spawnerSpawnRandom(Moby* moby)
{
  struct SpawnerPVar* pvars = (struct SpawnerPVar*)moby->PVar;

  int i;
  int spawnerMobIdx = selectRandomIndex(SPAWNER_MAX_MOB_TYPES, moby, spawnerIsValidMobSpawnIdx);
  if (spawnerMobIdx < 0) return 0;
  
  struct SpawnerMobParams* mobParams = &pvars->SpawnableMobParam[spawnerMobIdx];
  if (mobParams->MobParamIdx < 0 || mobParams->Probability <= 0)
    return 0;

  if (mobParams->MaxCanSpawnOrUnlimited > 0 && pvars->State.NumSpawned[spawnerMobIdx] >= mobParams->MaxCanSpawnOrUnlimited)
    return 0;

  if (MapConfig.State && mobParams->MaxCanAliveAtOnce > 0 && mobParams->MaxCanAliveAtOnce <= MapConfig.State->MobStats.NumAlive[mobParams->MobParamIdx])
    return 0;

  if (pvars->State.Cooldown[spawnerMobIdx] > 0)
    return 0;

  if (randRange(0, 1) > mobParams->Probability)
    return 0;

  // spawn
  if (spawnerSpawn(moby, spawnerMobIdx, -1)) {
    pvars->State.Cooldown[spawnerMobIdx] = mobParams->CooldownTicks;
    return 1;
  }

  return 0;
}

//--------------------------------------------------------------------------
int spawnerIsCompleted(Moby* moby)
{
  struct SpawnerPVar* pvars = (struct SpawnerPVar*)moby->PVar;

  return pvars->State.NumTotalKilled >= pvars->NumMobsToSpawn;
}

//--------------------------------------------------------------------------
int spawnerAnyTriggerActivated(Moby* moby)
{
  struct SpawnerPVar* pvars = (struct SpawnerPVar*)moby->PVar;
  
  return pvars->State.TriggersActivated;
}

//--------------------------------------------------------------------------
void spawnerUpdateTriggers(Moby* moby)
{
  struct SpawnerPVar* pvars = (struct SpawnerPVar*)moby->PVar;
  int i,j;
  Player** players = playerGetAll();

  for (i = 0; i < SPAWNER_MAX_TRIGGER_CUBOIDS; ++i) {
    int cuboidIdx = pvars->TriggerCuboids[i].CuboidIdx;
    enum SpawnerCuboidInteractType interactType = pvars->TriggerCuboids[i].InteractType;
    if (cuboidIdx < 0) continue;

    SpawnPoint* triggerCuboid = spawnPointGet(cuboidIdx);
    for (j = 0; j < GAME_MAX_PLAYERS; ++j) {
      Player* p = players[j];
      if (!p || !p->SkinMoby || !playerIsConnected(p)) continue;

      // check if player is inside the cuboid
      int isInside = spawnPointIsPointInside(triggerCuboid, p->PlayerPosition, NULL);
      int activate = !pvars->TriggerCuboids[i].Invert;

      switch (interactType)
      {
        case SPAWNER_CUBOID_INTERACT_ON_ENTER:
        {
          if (isInside && !pvars->State.LastInsideTriggers[i]) {
            pvars->State.TriggersActivated = activate;
          }
          break;
        }
        case SPAWNER_CUBOID_INTERACT_ON_EXIT:
        {
          if (!isInside && pvars->State.LastInsideTriggers[i]) {
            pvars->State.TriggersActivated = activate;
          }
          break;
        }
        case SPAWNER_CUBOID_INTERACT_WHEN_INSIDE:
        {
          if (isInside) {
            pvars->State.TriggersActivated = activate;
          } else {
            pvars->State.TriggersActivated = !activate;
          }
          break;
        }
        case SPAWNER_CUBOID_INTERACT_WHEN_OUTSIDE:
        {
          if (!isInside) {
            pvars->State.TriggersActivated = activate;
          } else {
            pvars->State.TriggersActivated = !activate;
          }
          break;
        }
      }
      
      pvars->State.LastInsideTriggers[i] = isInside;
    }
  }

  return 0;
}

//--------------------------------------------------------------------------
int spawnerCanSpawn(Moby* moby)
{
  struct SpawnerPVar* pvars = (struct SpawnerPVar*)moby->PVar;

  if (MapConfig.State) {
    int totalAlive = MapConfig.State->MobStats.TotalAlive + MapConfig.State->MobStats.TotalSpawning;
    if (totalAlive >= MAX_MOBS_ALIVE_REAL) return 0;
  }

  int total = pvars->State.NumTotalSpawned + pvars->State.NumTotalKilled;
  return moby->State == SPAWNER_STATE_ACTIVATED && !spawnerIsCompleted(moby) && total < pvars->NumMobsToSpawn;
}

//--------------------------------------------------------------------------
void spawnerBroadcastNewState(Moby* moby, enum SpawnerState state)
{
	// create event
	GuberEvent * guberEvent = guberCreateEvent(moby, SPAWNER_EVENT_SET_STATE);
  if (guberEvent) {
    guberEventWrite(guberEvent, &state, 4);
  }
}

//--------------------------------------------------------------------------
void spawnerUpdate(Moby* moby)
{
  int i;
  struct SpawnerPVar* pvars = (struct SpawnerPVar*)moby->PVar;

  for (i = 0; i < SPAWNER_MAX_MOB_TYPES; ++i) {
    decTimerU32(&pvars->State.Cooldown[i]);
  }

  if (!gameAmIHost()) return;

  // update triggers
  spawnerUpdateTriggers(moby);

  // check if we should exit idle
  if (moby->State == SPAWNER_STATE_IDLE && spawnerAnyTriggerActivated(moby)) {
    DPRINTF("spawner %08X exit idle\n", moby);
    spawnerBroadcastNewState(moby, SPAWNER_STATE_ACTIVATED);
    return;
  }

  // check if completed
  if (moby->State != SPAWNER_STATE_COMPLETED && spawnerIsCompleted(moby)) {
    DPRINTF("spawner %08X completed\n", moby);
    spawnerBroadcastNewState(moby, SPAWNER_STATE_COMPLETED);
    return;
  }

  if (moby->State != SPAWNER_STATE_ACTIVATED) return;

  // check if we should go into idle mode
  if (!spawnerAnyTriggerActivated(moby)) {
    DPRINTF("spawner %08X idle\n", moby);
    spawnerBroadcastNewState(moby, SPAWNER_STATE_IDLE);
    return;
  }

  // spawn
  if (spawnerCanSpawn(moby)) {
    if (spawnerSpawnRandom(moby)) {
      
    }
  }
}

//--------------------------------------------------------------------------
void spawnerOnChildMobUpdate(Moby* moby, Moby* childMoby, u32 userdata)
{
	struct MobPVar* childPVars = (struct MobPVar*)childMoby->PVar;
  struct SpawnerPVar* pvars = (struct SpawnerPVar*)moby->PVar;
  int i;

  // check if mob has left habitable cuboids
  // if no habitable cuboids are defined, then this will do nothing
  int notInside = 0;
  for (i = 0; i < SPAWNER_MAX_HABITABLE_CUBOIDS; ++i) {
    int cuboidIdx = pvars->HabitableCuboidIds[i];
    if (cuboidIdx < 0) continue;

    SpawnPoint* cuboid = spawnPointGet(cuboidIdx);
    if (spawnPointIsPointInside(cuboid, childMoby->Position, NULL)) {
      notInside = 0;
      break;
    }

    notInside = 1;
  }

  // if spawner is idled and we're not inside a habitable cuboid, despawn
  if (moby->State == SPAWNER_STATE_IDLE && notInside) {
    if (!childPVars->MobVars.Destroyed) {
      childPVars->MobVars.Destroy = 2;
    }
    return;
  }

  // handle respawn
  if (childPVars->MobVars.Respawn || notInside) {
    // pass to mob
    // let mob override respawn logic
    if (!childPVars->VTable->OnRespawn || childPVars->VTable->OnRespawn(childMoby)) {
      if (spawnerSpawn(moby, userdata, guberGetUID(childMoby))) {
        childPVars->MobVars.Destroyed = 2;
      }
    }

    childPVars->MobVars.Respawn = 0;
  }

}

//--------------------------------------------------------------------------
void spawnerOnChildMobKilled(Moby* moby, Moby* childMoby, u32 userdata, int killedByPlayerId, int weaponId)
{
  struct SpawnerPVar* pvars = (struct SpawnerPVar*)moby->PVar;

  // if spawner is activated, save kill
  if (killedByPlayerId >= 0) {
    pvars->State.NumTotalKilled++;
    pvars->State.NumKilled[userdata]++;
  }

  pvars->State.NumTotalSpawned--;
  pvars->State.NumSpawned[userdata]--;

  DPRINTF("MOB%d: spawned:%d killed:%d\n", userdata, pvars->State.NumSpawned[userdata], pvars->State.NumKilled[userdata]);
}

//--------------------------------------------------------------------------
int spawnerOnChildConsiderTarget(Moby* moby, Moby* childMoby, u32 userdata, Moby* target)
{
  int i;
  struct SpawnerPVar* pvars = (struct SpawnerPVar*)moby->PVar;

  // check if target has left aggro cuboids
  // if no aggro cuboids are defined, then return 1
  int inside = 1;
  for (i = 0; i < SPAWNER_MAX_AGGRO_CUBOIDS; ++i) {
    int cuboidIdx = pvars->AggroCuboidIds[i];
    if (cuboidIdx < 0) continue;

    SpawnPoint* cuboid = spawnPointGet(cuboidIdx);
    if (spawnPointIsPointInside(cuboid, target->Position, NULL)) {
      inside = 1;
      break;
    }

    inside = 0;
  }

  return inside;
}

//--------------------------------------------------------------------------
int spawnerOnChildConsiderRoamTarget(Moby* moby, Moby* childMoby, u32 userdata, VECTOR targetPosition)
{
  int i;
  struct SpawnerPVar* pvars = (struct SpawnerPVar*)moby->PVar;

  // check if target is in a habitable cuboids
  // if no habitable cuboids are defined, then return 1
  int inside = 1;
  for (i = 0; i < SPAWNER_MAX_HABITABLE_CUBOIDS; ++i) {
    int cuboidIdx = pvars->HabitableCuboidIds[i];
    if (cuboidIdx < 0) continue;

    SpawnPoint* cuboid = spawnPointGet(cuboidIdx);
    if (spawnPointIsPointInside(cuboid, targetPosition, NULL)) {
      inside = 1;
      break;
    }

    inside = 0;
  }

  return inside;
}

//--------------------------------------------------------------------------
void spawnerOnGuberCreated(Moby* moby)
{
  int i;
  struct SpawnerPVar* pvars = (struct SpawnerPVar*)moby->PVar;

  // mark initialized
  pvars->Init = 1;

  // set update
  moby->PUpdate = &spawnerUpdate;

  // don't render
  moby->ModeBits = MOBY_MODE_BIT_HIDDEN | MOBY_MODE_BIT_NO_POST_UPDATE;

  // 
  struct Guber* guber = guberGetObjectByMoby(moby);
  ((GuberMoby*)guber)->TeamNum = 10;

  // print pvars
  #if PRINT_SPAWNER_PVARS
    DPRINTF("NumMobsToSpawn=%d\n", pvars->NumMobsToSpawn);
    DPRINTF("PathGraphIdx=%d\n", pvars->PathGraphIdx);
    DPRINTF("TriggerCuboids\n");
    for (i = 0; i < SPAWNER_MAX_TRIGGER_CUBOIDS; ++i) {
      DPRINTF(" [%d].CuboidIdx=%d\n", i, pvars->TriggerCuboids[i].CuboidIdx);
      DPRINTF(" [%d].InteractType=%d\n", i, pvars->TriggerCuboids[i].InteractType);
    }
    DPRINTF("SpawnCuboidIds\n");
    for (i = 0; i < SPAWNER_MAX_SPAWN_CUBOIDS; ++i) { DPRINTF(" [%d]=%d\n", i, pvars->SpawnCuboidIds[i]); }
    DPRINTF("SpawnableMobParam\n");
    for (i = 0; i < SPAWNER_MAX_MOB_TYPES; ++i) {
      DPRINTF(" [%d].MobParamIdx=%d\n", i, pvars->SpawnableMobParam[i].MobParamIdx);
      DPRINTF(" [%d].Probability=%f\n", i, pvars->SpawnableMobParam[i].Probability);
      DPRINTF(" [%d].DifficultyMultiplier=%f\n", i, pvars->SpawnableMobParam[i].DifficultyMultiplier);
      DPRINTF(" [%d].MaxCanSpawnOrUnlimited=%d\n", i, pvars->SpawnableMobParam[i].MaxCanSpawnOrUnlimited);
    }
  #endif

  // set default state
  memset(&pvars->State, 0, sizeof(pvars->State));
  mobySetState(moby, pvars->DefaultState, -1);  
}

//--------------------------------------------------------------------------
int spawnerHandleEvent_Spawned(Moby* moby, GuberEvent* event)
{
	int i;
  int spawnFromMobyIdx;
  
  DPRINTF("spawner spawned: %08X\n", (u32)moby);
  struct SpawnerPVar* pvars = (struct SpawnerPVar*)moby->PVar;
  if (!pvars)
    return 0;
  
	// read event
	guberEventRead(event, &spawnFromMobyIdx, 4);

  // copy pvars from source moby
  Moby* spawnFromMoby = mobyListGetStart() + spawnFromMobyIdx;
  vector_copy(moby->Position, spawnFromMoby->Position);
  memcpy(moby->M0_03, spawnFromMoby->M0_03, 0x40);
  moby->Scale = spawnFromMoby->Scale;
  memcpy(pvars, spawnFromMoby->PVar, OFFSET_OF(struct SpawnerPVar, State));
  if (!mobyIsDestroyed(spawnFromMoby))
    mobyDestroy(spawnFromMoby);

  spawnerOnGuberCreated(moby);
  return 0;
}

//--------------------------------------------------------------------------
int spawnerHandleEvent_SetState(Moby* moby, GuberEvent* event)
{
  int state;
  if (!moby || !moby->PVar)
    return 0;

  struct SpawnerPVar* pvars = (struct SpawnerPVar*)moby->PVar;
  
	// read event
	guberEventRead(event, &state, 4);

  switch (state)
  {
    case SPAWNER_STATE_DEACTIVATED:
    {
      // reset runtime stats when deactivating
      memset(&pvars->State, 0, sizeof(pvars->State));
      break;
    }
    case SPAWNER_STATE_IDLE:
    {
      // when idling, set NumSpawned to NumKilled
      // so that when we start up again NumSpawned accurately represents the # of mobs killed, and # left to go
      //pvars->State.NumTotalSpawned = pvars->State.NumTotalKilled;
      //memcpy(pvars->State.NumSpawned, pvars->State.NumKilled, sizeof(pvars->State.NumSpawned));
      break;
    }
    case SPAWNER_STATE_ACTIVATED:
    {
      break;
    }
    case SPAWNER_STATE_COMPLETED:
    {
      // reset runtime stats when completed
      memset(&pvars->State, 0, sizeof(pvars->State));
      break;
    }
  }

  mobySetState(moby, state, -1);
  return 0;
}

//--------------------------------------------------------------------------
struct GuberMoby* spawnerGetGuber(Moby* moby)
{
	if (moby->OClass == SPAWNER_OCLASS && moby->PVar)
		return moby->GuberMoby;
	
	return 0;
}

//--------------------------------------------------------------------------
int spawnerHandleEvent(Moby* moby, GuberEvent* event)
{
	if (!moby || !event)
		return 0;

	if (isInGame() && !mobyIsDestroyed(moby) && moby->OClass == SPAWNER_OCLASS && moby->PVar) {
		u32 upgradeEvent = event->NetEvent.EventID;

		switch (upgradeEvent)
		{
			case SPAWNER_EVENT_SPAWN: { return spawnerHandleEvent_Spawned(moby, event); }
      case SPAWNER_EVENT_SET_STATE: { return spawnerHandleEvent_SetState(moby, event); }
			default:
			{
				DPRINTF("unhandle spawner event %d\n", upgradeEvent);
				break;
			}
		}
	}

	return 0;
}

//--------------------------------------------------------------------------
int spawnerCreate(int fromMobyIdx)
{
	// create guber object
	GuberEvent * guberEvent = 0;
	guberMobyCreateSpawned(SPAWNER_OCLASS, sizeof(struct SpawnerPVar), &guberEvent, NULL);
	if (guberEvent)
	{
		guberEventWrite(guberEvent, &fromMobyIdx, 4);
	}
	else
	{
		DPRINTF("failed to guberevent spawner\n");
	}
  
  return guberEvent != NULL;
}

//--------------------------------------------------------------------------
void spawnerStart(void)
{

  // find all existing spawners
  // respawn with net
  Moby* moby = mobyListGetStart();
	while ((moby = mobyFindNextByOClass(moby, SPAWNER_OCLASS)))
	{
		if (!mobyIsDestroyed(moby) && moby->PVar) {
      struct SpawnerPVar* pvars = (struct SpawnerPVar*)moby->PVar;
      if (!pvars->Init) {
        DPRINTF("found spawner %08X\n", moby);

        struct GuberMoby* guber = guberGetOrCreateObjectByMoby(moby, -1, 1);
        if (guber) {
          spawnerOnGuberCreated(moby);
        }
      }
    }

		++moby;
	}
}

//--------------------------------------------------------------------------
void spawnerInit(void)
{
  Moby* temp = mobySpawn(SPAWNER_OCLASS, 0);
  if (!temp)
    return;

  // set vtable callbacks
  MobyFunctions* mobyFunctionsPtr = mobyGetFunctions(temp);
  if (mobyFunctionsPtr) {
    mapInstallMobyFunctions(mobyFunctionsPtr);
    DPRINTF("SPAWNER oClass:%04X mClass:%02X func:%08X getGuber:%08X handleEvent:%08X\n", temp->OClass, temp->MClass, mobyFunctionsPtr, *(u32*)(mobyFunctionsPtr + 0x04), *(u32*)(mobyFunctionsPtr + 0x14));
  }
  mobyDestroy(temp);
}
