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
#include <libdl/color.h>
#include <libdl/utils.h>
#include <libdl/random.h>
#include "include/maputils.h"
#include "include/shared.h"
#include "../../include/spawner.h"
#include "../../include/game.h"

extern struct RaidsMapConfig MapConfig;

//--------------------------------------------------------------------------
void spawnerUpdate(Moby* moby)
{
  struct SpawnerPVar* pvars = (struct SpawnerPVar*)moby->PVar;

  if (!gameAmIHost()) return;
  if (moby->State != SPAWNER_STATE_ACTIVATED) return;

  // spawn


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
  memcpy(pvars, spawnFromMoby->PVar, sizeof(struct SpawnerPVar));
  if (!mobyIsDestroyed(spawnFromMoby))
    mobyDestroy(spawnFromMoby);

	// set update
	moby->PUpdate = &spawnerUpdate;

  // mark initialized
  pvars->Init = 1;

  // don't render
  moby->ModeBits = MOBY_MODE_BIT_HIDDEN | MOBY_MODE_BIT_NO_POST_UPDATE;

  // print pvars
#if PRINT_SPAWNER_PVARS
  DPRINTF("NumMobsToSpawn=%d\n", pvars->NumMobsToSpawn);
  DPRINTF("PathGraphIdx=%d\n", pvars->PathGraphIdx);
  DPRINTF("TriggerCuboidIds\n");
  for (i = 0; i < SPAWNER_MAX_TRIGGER_CUBOIDS; ++i) { DPRINTF(" [%d]=%d\n", i, pvars->TriggerCuboidIds[i]); }
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
	mobySetState(moby, pvars->DefaultState, -1);
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
      case SPAWNER_EVENT_ACTIVATE: { mobySetState(moby, SPAWNER_STATE_ACTIVATED, -1); return 0; }
      case SPAWNER_EVENT_DEACTIVATE: { mobySetState(moby, SPAWNER_STATE_DEACTIVATED, -1); return 0; }
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

  // find all existing spawners
  // respawn with net
  Moby* moby = mobyListGetStart();
	while ((moby = mobyFindNextByOClass(moby, SPAWNER_OCLASS)))
	{
    DPRINTF("found spawner %08X\n", moby);
		if (!mobyIsDestroyed(moby) && moby->PVar) {
      struct SpawnerPVar* pvars = (struct SpawnerPVar*)moby->PVar;
      if (!pvars->Init) {
        int mobyIdx = ((u32)moby - (u32)mobyListGetStart()) / sizeof(Moby);
        spawnerCreate(mobyIdx);
      }
    }

		++moby;
	}
}
