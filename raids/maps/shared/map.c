/***************************************************
 * FILENAME :		map.c
 * 
 * DESCRIPTION :
 * 		
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */

#include <libdl/moby.h>
#include <libdl/stdio.h>
#include "../../include/spawner.h"
#include "../../include/mob.h"
#include "../../include/game.h"
#include "maputils.h"

extern struct RaidsMapConfig MapConfig;

MobyGetGuberObject_func baseGetGuberFunc = NULL;
MobyEventHandler_func baseHandleGuberEventFunc = NULL;

//--------------------------------------------------------------------------
void mapOnMobUpdate(Moby* moby)
{
  if (!moby || !moby->PVar) return;

	struct MobPVar* pvars = (struct MobPVar*)moby->PVar;
  if (moby->PParent && moby->PParent->OClass == SPAWNER_OCLASS) {
    spawnerOnChildMobUpdate(moby->PParent, moby, pvars->MobVars.Userdata);
  }
}

//--------------------------------------------------------------------------
void mapOnMobKilled(Moby* moby, int killedByPlayerId, int weaponId)
{
  if (!moby || !moby->PVar) return;

	struct MobPVar* pvars = (struct MobPVar*)moby->PVar;
  if (moby->PParent && moby->PParent->OClass == SPAWNER_OCLASS) {
    spawnerOnChildMobKilled(moby->PParent, moby, pvars->MobVars.Userdata, killedByPlayerId, weaponId);
  }
}

//--------------------------------------------------------------------------
struct GuberMoby* mapGetGuber(Moby* moby)
{
  if (mobyIsMob(moby)) return moby->GuberMoby;

  switch (moby->OClass)
  {
    case SPAWNER_OCLASS: return spawnerGetGuber(moby);
    default:
    {
      // pass up to mode
      if (MapConfig.OnGetGuberFunc) {
        struct GuberMoby* guberMoby = MapConfig.OnGetGuberFunc(moby);
        if (guberMoby) return guberMoby;
      }

      // pass to overwritten game func
      if (baseGetGuberFunc) { 
        //DPRINTF("base get guber object %08X %04X\n", moby, moby->OClass);
        return baseGetGuberFunc(moby);
      }

      // unhandled
      DPRINTF("unhandled get guber for moby %04X at %08X\n", moby->OClass, moby);
      return  NULL;
    }
  }
	
	return 0;
}

//--------------------------------------------------------------------------
void mapHandleEvent(Moby* moby, GuberEvent* event)
{
	if (!moby || !event)
		return;

	if (isInGame() && !mobyIsDestroyed(moby)) {

    switch (moby->OClass)
    {
      case SPAWNER_OCLASS: spawnerHandleEvent(moby, event); break;
      default:
			{
        // pass up to mode
        if (MapConfig.OnGuberEventFunc && MapConfig.OnGuberEventFunc(moby, event))
          return;
        
        // pass to overwritten game func
        if (baseHandleGuberEventFunc) {
          //DPRINTF("base handle guber event %08X %04X\n", moby, moby->OClass);
          baseHandleGuberEventFunc(moby, event);
          return;
        }

        // unhandled
        DPRINTF("unhandled guber event %d for moby %04X at %08X\n", event->NetEvent.EventID, moby->OClass, moby);
				break;
			}
    }
	}
}

//--------------------------------------------------------------------------
void mapInstallMobyFunctions(MobyFunctions* mobyFunctions)
{
  if (!baseGetGuberFunc) baseGetGuberFunc = mobyFunctions->GetGuberObject;
  if (!baseHandleGuberEventFunc) baseHandleGuberEventFunc = mobyFunctions->MobyEventHandler;

  mobyFunctions->GetGuberObject = &mapGetGuber;
  mobyFunctions->GetMobyInterface = NULL;
  mobyFunctions->MobyEventHandler = &mapHandleEvent;
}
