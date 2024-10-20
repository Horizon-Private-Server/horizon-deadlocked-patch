/***************************************************
 * FILENAME :		controller.c
 * 
 * DESCRIPTION :
 * 		Handles logic for the controllers.
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
#include "../../include/gate.h"
#include "../../include/mover.h"
#include "../../include/controller.h"
#include "../../include/mob.h"
#include "../../include/game.h"

extern struct RaidsMapConfig MapConfig;

int controllerInitialized = 0;

//--------------------------------------------------------------------------
int controllerAnyTriggerActivated(Moby* moby)
{
  struct ControllerPVar* pvars = (struct ControllerPVar*)moby->PVar;
  
  return pvars->State.TriggersActivated;
}

//--------------------------------------------------------------------------
int controllerIsMobyStateConditionTrue(Moby* moby, int conditionIdx)
{
  struct ControllerPVar* pvars = (struct ControllerPVar*)moby->PVar;
  struct ControllerCondition* condition = &pvars->Conditions[conditionIdx];
  
  // invalid moby
  Moby* target = condition->Moby;
  if (!target || mobyIsDestroyed(target)) {
    condition->Moby = NULL;
    return 0;
  }

  switch (condition->MobyState.StateInteractType) {
    case CONTROLLER_CUBOID_INTERACT_EQUAL: return target->State == condition->MobyState.State;
    case CONTROLLER_CUBOID_INTERACT_NOTEQUAL: return target->State != condition->MobyState.State;
    case CONTROLLER_CUBOID_INTERACT_LESS: return target->State < condition->MobyState.State;
    case CONTROLLER_CUBOID_INTERACT_LEQUAL: return target->State <= condition->MobyState.State;
    case CONTROLLER_CUBOID_INTERACT_GREATER: return target->State > condition->MobyState.State;
    case CONTROLLER_CUBOID_INTERACT_GEQUAL: return target->State >= condition->MobyState.State;
  }

  return 0;
}

//--------------------------------------------------------------------------
int controllerIsCuboidConditionTrue(Moby* moby, int conditionIdx, char validPlayers[GAME_MAX_PLAYERS])
{
  struct ControllerPVar* pvars = (struct ControllerPVar*)moby->PVar;
  struct ControllerCondition* condition = &pvars->Conditions[conditionIdx];
  Player** players = playerGetAll();
  int j;
  int cuboidIdx = condition->Cuboid.CuboidIdx;
  int succeeded = 0, count = 0;

  SpawnPoint* triggerCuboid = spawnPointGet(cuboidIdx);
  for (j = 0; j < GAME_MAX_PLAYERS; ++j) {
    Player* p = players[j];
    if (!p || !p->SkinMoby || !playerIsConnected(p)) continue;

    // check if player is inside the cuboid
    int isInside = spawnPointIsPointInside(triggerCuboid, p->PlayerPosition, NULL);
    if (isInside != condition->Cuboid.InteractType && validPlayers[j]) {
      ++succeeded;
    } else {
      validPlayers[j] = 0;
    }

    ++count;
  }

  return (succeeded > 0 && condition->Cuboid.PlayerInteractType == CONTROLLER_CUBOID_PLAYER_ANY)
      || (succeeded > 0 && succeeded == count && condition->Cuboid.PlayerInteractType == CONTROLLER_CUBOID_PLAYER_ALL)
      || (succeeded == 0 && count > 0 && condition->Cuboid.PlayerInteractType == CONTROLLER_CUBOID_PLAYER_NONE);
}

//--------------------------------------------------------------------------
int controllerIsPlayerButtonConditionTrue(Moby* moby, int conditionIdx, char validPlayers[GAME_MAX_PLAYERS])
{
  struct ControllerPVar* pvars = (struct ControllerPVar*)moby->PVar;
  struct ControllerCondition* condition = &pvars->Conditions[conditionIdx];
  Player** players = playerGetAll();
  int j;
  int succeeded = 0;

  for (j = 0; j < GAME_MAX_PLAYERS; ++j) {
    Player* p = players[j];
    if (!p || !p->SkinMoby || !playerIsConnected(p)) continue;

    // check if player is inside the cuboid
    int hasButtonMask = playerPadGetButton(p, condition->PlayerButtons.PadMask);
    if (hasButtonMask && validPlayers[j]) {
      ++succeeded;
    } else {
      validPlayers[j] = 0;
    }
  }

  return succeeded > 0;
}

//--------------------------------------------------------------------------
int controllerIsDelayConditionTrue(Moby* moby, int conditionIdx)
{
  struct ControllerPVar* pvars = (struct ControllerPVar*)moby->PVar;
  struct ControllerCondition* condition = &pvars->Conditions[conditionIdx];
  
  int startTime = pvars->State.DelayStartTime[conditionIdx];
  if (startTime <= 0) pvars->State.DelayStartTime[conditionIdx] = startTime = gameGetTime();

  if (condition->Delay.Milliseconds < (gameGetTime() - startTime)) {
    //pvars->State.DelayStartTime[conditionIdx] = startTime = gameGetTime();
    return 1;
  }

  return 0;
}

//--------------------------------------------------------------------------
int controllerIsXORConditionTrue(Moby* moby, int conditionIdx)
{
  struct ControllerPVar* pvars = (struct ControllerPVar*)moby->PVar;

  return !pvars->State.TriggersActivated;
}

//--------------------------------------------------------------------------
void controllerUpdateTriggers(Moby* moby)
{
  struct ControllerPVar* pvars = (struct ControllerPVar*)moby->PVar;
  int i;
  int succeeded = 0;
  int count = 0;
  Player** players = playerGetAll();
  char validPlayers[GAME_MAX_PLAYERS];
  memset(validPlayers, 1, sizeof(validPlayers));

  for (i = 0; i < CONTROLLER_MAX_CONDITIONS; ++i) {
    int success = 0;
    switch (pvars->Conditions[i].ConditionType) {
      case CONTROLLER_CONDITION_TYPE_CUBOID: count += 1; succeeded += controllerIsCuboidConditionTrue(moby, i, validPlayers); break;
      case CONTROLLER_CONDITION_TYPE_MOBY_STATE: count += 1; succeeded += controllerIsMobyStateConditionTrue(moby, i); break;
      case CONTROLLER_CONDITION_TYPE_PLAYER_BUTTON: count += 1; succeeded += controllerIsPlayerButtonConditionTrue(moby, i, validPlayers); break;
      case CONTROLLER_CONDITION_TYPE_DELAY: count += 1; succeeded += controllerIsDelayConditionTrue(moby, i); break;
      case CONTROLLER_CONDITION_TYPE_XOR: count += 1; succeeded += controllerIsXORConditionTrue(moby, i); break;
    }

    // stop when condition fails
    if (pvars->TriggerIfAllTrue && succeeded < count) break;
  }

  // update activate
  pvars->State.TriggersActivated = (succeeded > 0 && pvars->TriggerIfAllTrue == 0)
                                || (succeeded > 0 && succeeded == count && pvars->TriggerIfAllTrue == 1);
}

//--------------------------------------------------------------------------
int controllerControlMobyState(Moby* moby, struct ControllerTarget* target)
{
  int state = target->State;
  Moby* targetMoby = target->Moby;
  if (!targetMoby) return 0;

  if (target->TargetUpdateType == CONTROLLER_TARGET_UPDATE_TYPE_MOBY_STATE_ADDITIVE)
    state += targetMoby->State;

  // only when state changes
  if (targetMoby->State == state) return 0;

  // negative is destroyed
  if (state < 0) {
    guberMobyDestroy(targetMoby);
    return 1;
  }

  DPRINTF("controller set %08X state %d=>%d\n", targetMoby, targetMoby->State, state);

  // handle special cases
  switch (targetMoby->OClass) {
    case SPAWNER_OCLASS: if (gameAmIHost()) { spawnerBroadcastNewState(targetMoby, state); }; break;
    case MOVER_OCLASS: if (gameAmIHost()) { moverBroadcastNewState(targetMoby, state); } break;
    case CONTROLLER_OCLASS: if (gameAmIHost()) { controllerBroadcastNewState(targetMoby, state); } break;
#if GATE
    case GATE_OCLASS: if (gameAmIHost()) { gateBroadcastNewState(targetMoby, state); } break;
#endif
    default: mobySetState(targetMoby, state, -1); break;
  }

  return 1;
}

//--------------------------------------------------------------------------
int controllerControlMobyAnimation(Moby* moby, struct ControllerTarget* target)
{
  int animId = target->AnimId;
  Moby* targetMoby = target->Moby;
  if (!targetMoby || !targetMoby->PClass) return 0;

  // bad anim id
  int seqCount = *(char*)(targetMoby->PClass + 0x0C);
  if (animId >= seqCount) return 0;
  if (targetMoby->AnimSeqId == animId) return 0;

  DPRINTF("set anim %08X => %d\n", targetMoby, animId);
  mobyAnimTransition(targetMoby, animId, 0, 0);
  return 1;
}

//--------------------------------------------------------------------------
int controllerControlMobyEnabled(Moby* moby, struct ControllerTarget* target)
{
  Moby* targetMoby = target->Moby;
  if (!targetMoby || !targetMoby->PClass) return 0;
  
  if (target->Enabled) {
    if (targetMoby->CollActive == 0 && (targetMoby->ModeBits & MOBY_MODE_BIT_DISABLED) == 0) return 0;
    targetMoby->CollActive = 0;
    targetMoby->ModeBits &= ~MOBY_MODE_BIT_DISABLED;
  } else {
    if (targetMoby->CollActive && (targetMoby->ModeBits & MOBY_MODE_BIT_DISABLED)) return 0;
    targetMoby->CollActive = -1;
    targetMoby->ModeBits |= MOBY_MODE_BIT_DISABLED;
  }

  return 1;
}

//--------------------------------------------------------------------------
int controllerControlCuboidMove(Moby* moby, struct ControllerTarget* target)
{
  int srcIdx = target->CuboidSrcIdx;
  int dstIdx = target->CuboidDestIdx;
  if (srcIdx < 0 || dstIdx < 0) return 0;
  
  SpawnPoint* src = spawnPointGet(srcIdx);
  SpawnPoint* dst = spawnPointGet(dstIdx);
  if (memcmp(src, dst, sizeof(SpawnPoint)) == 0) return 0;

  memcpy(dst, src, sizeof(SpawnPoint));
  return 1;
}

//--------------------------------------------------------------------------
int controllerIterate(Moby* moby)
{
  int i;
  struct ControllerPVar* pvars = (struct ControllerPVar*)moby->PVar;
  int changed = 0;

  for (i = 0; i < CONTROLLER_MAX_TARGETS; ++i) {

    // clear destroyed moby
    Moby* target = pvars->Targets[i].Moby;
    if (target && mobyIsDestroyed(target)) { pvars->Targets[i].Moby = NULL; }

    switch (pvars->Targets[i].TargetUpdateType) {
      case CONTROLLER_TARGET_UPDATE_TYPE_MOBY_STATE: changed += controllerControlMobyState(moby, &pvars->Targets[i]); break;
      case CONTROLLER_TARGET_UPDATE_TYPE_MOBY_STATE_ADDITIVE: changed += controllerControlMobyState(moby, &pvars->Targets[i]); break;
      case CONTROLLER_TARGET_UPDATE_TYPE_MOBY_ANIMATION: changed += controllerControlMobyAnimation(moby, &pvars->Targets[i]); break;
      case CONTROLLER_TARGET_UPDATE_TYPE_MOBY_ENABLED: changed += controllerControlMobyEnabled(moby, &pvars->Targets[i]); break;
      case CONTROLLER_TARGET_UPDATE_TYPE_MOVE_CUBOID: changed += controllerControlCuboidMove(moby, &pvars->Targets[i]); break;
    }
  }
  
  pvars->State.Iterations++;
  return changed;
}

//--------------------------------------------------------------------------
void controllerBroadcastNewState(Moby* moby, enum ControllerState state)
{
	// create event
	GuberEvent * guberEvent = guberCreateEvent(moby, CONTROLLER_EVENT_SET_STATE);
  if (guberEvent) {
    guberEventWrite(guberEvent, &state, 4);
  }
}

//--------------------------------------------------------------------------
void controllerBroadcastIterate(Moby* moby)
{
  struct ControllerPVar* pvars = (struct ControllerPVar*)moby->PVar;

	// create event
	GuberEvent * guberEvent = guberCreateEvent(moby, CONTROLLER_EVENT_ITERATE);
  if (guberEvent) {
    guberEventWrite(guberEvent, &pvars->State.Iterations, 4);
  }
}

//--------------------------------------------------------------------------
void controllerUpdate(Moby* moby)
{
  int i;
  struct ControllerPVar* pvars = (struct ControllerPVar*)moby->PVar;

  // initialize by sending first state
  if (!pvars->Init) {
    if (gameAmIHost() && controllerInitialized) {
      controllerBroadcastNewState(moby, pvars->DefaultState);
    }
    
    return;
  }

  if (!gameAmIHost()) return;
  if (moby->State == CONTROLLER_STATE_DEACTIVATED) return;
  if (moby->State == CONTROLLER_STATE_COMPLETED) return;

  // update triggers
  controllerUpdateTriggers(moby);

  // check if we've reached the iteration count
  if (pvars->Repeat > 0 && pvars->State.Iterations >= pvars->Repeat) {
    DPRINTF("controller %08X complete\n", moby);
    controllerBroadcastNewState(moby, CONTROLLER_STATE_COMPLETED);
    return;
  }

  // check if we should exit idle
  if (moby->State == CONTROLLER_STATE_IDLE && controllerAnyTriggerActivated(moby)) {
    DPRINTF("controller %08X exit idle\n", moby);
    controllerBroadcastNewState(moby, CONTROLLER_STATE_ACTIVATED);
  } else if (moby->State == CONTROLLER_STATE_ACTIVATED && !controllerAnyTriggerActivated(moby)) {
    DPRINTF("controller %08X idle\n", moby);
    controllerBroadcastNewState(moby, CONTROLLER_STATE_IDLE);
    return;
  } else if (moby->State != CONTROLLER_STATE_ACTIVATED) {
    return;
  }

  // each iteration is broadcasted to everyone
  // but only if the iteration changed any target(s)
  // otherwise we assume the same would be true on other clients and skip broadcasting
  // this guarantees sync (for the most part)
  // at the cost of network bandwidth if configured to run every tick
  if (controllerIterate(moby)) {
    controllerBroadcastIterate(moby);
  }
}

//--------------------------------------------------------------------------
void controllerOnGuberCreated(Moby* moby)
{
  int i;
  struct ControllerPVar* pvars = (struct ControllerPVar*)moby->PVar;

  moby->PUpdate = &controllerUpdate;
  moby->ModeBits = MOBY_MODE_BIT_HIDDEN | MOBY_MODE_BIT_NO_POST_UPDATE;

  memset(&pvars->State, 0, sizeof(pvars->State));
  pvars->Init = 0;
  
  // update pvars target references
  Moby* mobyStart = mobyListGetStart();
  for (i = 0; i < CONTROLLER_MAX_TARGETS; ++i) {
    int mobyIdx = (int)pvars->Targets[i].Moby;
    if (mobyIdx < 0) {
      pvars->Targets[i].Moby = NULL;
      continue;
    }

    Moby* targetMoby = mobyStart + mobyIdx;
    if (mobyIsDestroyed(targetMoby)) {
      pvars->Targets[i].Moby = NULL;
      continue;
    }

    DPRINTF("controller %08X found target %d %08X\n", moby, i, targetMoby);
    pvars->Targets[i].Moby = targetMoby;
  }
  
  for (i = 0; i < CONTROLLER_MAX_CONDITIONS; ++i) {
    int mobyIdx = (int)pvars->Conditions[i].Moby;
    if (mobyIdx < 0) {
      pvars->Conditions[i].Moby = NULL;
      continue;
    }

    Moby* targetMoby = mobyStart + mobyIdx;
    if (mobyIsDestroyed(targetMoby)) {
      pvars->Conditions[i].Moby = NULL;
      continue;
    }

    DPRINTF("controller %08X found condition moby %d %08X\n", moby, i, targetMoby);
    pvars->Conditions[i].Moby = targetMoby;
  }
}

//--------------------------------------------------------------------------
int controllerHandleEvent_SetState(Moby* moby, GuberEvent* event)
{
  int state;
  if (!moby || !moby->PVar)
    return 0;

  struct ControllerPVar* pvars = (struct ControllerPVar*)moby->PVar;
  
	// read event
	guberEventRead(event, &state, 4);

  switch (state)
  {
    case CONTROLLER_STATE_DEACTIVATED:
    case CONTROLLER_STATE_COMPLETED:
    {
      // reset runtime stats when deactivating
      memset(&pvars->State, 0, sizeof(pvars->State));
      break;
    }
    case CONTROLLER_STATE_IDLE:
    {
      break;
    }
    case CONTROLLER_STATE_ACTIVATED:
    {
      break;
    }
  }

  pvars->Init = 1;
  mobySetState(moby, state, -1);
  return 0;
}

//--------------------------------------------------------------------------
int controllerHandleEvent_Iterate(Moby* moby, GuberEvent* event)
{
  if (!moby || !moby->PVar)
    return 0;

  struct ControllerPVar* pvars = (struct ControllerPVar*)moby->PVar;
  
	// iterate
  if (!gameAmIHost()) {
    controllerIterate(moby);
  }

	guberEventRead(event, &pvars->State.Iterations, 4);
  DPRINTF("controller iterate %d\n", pvars->State.Iterations);
  return 0;
}

//--------------------------------------------------------------------------
struct GuberMoby* controllerGetGuber(Moby* moby)
{
	if (moby->OClass == CONTROLLER_OCLASS && moby->PVar)
		return moby->GuberMoby;
	
	return 0;
}

//--------------------------------------------------------------------------
int controllerHandleEvent(Moby* moby, GuberEvent* event)
{
	if (!moby || !event)
		return 0;

	if (isInGame() && !mobyIsDestroyed(moby) && moby->OClass == CONTROLLER_OCLASS && moby->PVar) {
		u32 upgradeEvent = event->NetEvent.EventID;

		switch (upgradeEvent)
		{
      case CONTROLLER_EVENT_SET_STATE: { return controllerHandleEvent_SetState(moby, event); }
      case CONTROLLER_EVENT_ITERATE: { return controllerHandleEvent_Iterate(moby, event); }
			default:
			{
				DPRINTF("unhandle controller event %d\n", upgradeEvent);
				break;
			}
		}
	}

	return 0;
}

//--------------------------------------------------------------------------
void controllerStart(void)
{
  controllerInitialized = 1;
}

//--------------------------------------------------------------------------
void controllerInit(void)
{
  Moby* temp = mobySpawn(CONTROLLER_OCLASS, 0);
  if (!temp)
    return;

  // set vtable callbacks
  MobyFunctions* mobyFunctionsPtr = mobyGetFunctions(temp);
  if (mobyFunctionsPtr) {
    mapInstallMobyFunctions(mobyFunctionsPtr);
    DPRINTF("CONTROLLER oClass:%04X mClass:%02X func:%08X getGuber:%08X handleEvent:%08X\n", temp->OClass, temp->MClass, mobyFunctionsPtr, *(u32*)(mobyFunctionsPtr + 0x04), *(u32*)(mobyFunctionsPtr + 0x14));
  }
  mobyDestroy(temp);
  
  // create gubers for controllers
  Moby* moby = mobyListGetStart();
	while ((moby = mobyFindNextByOClass(moby, CONTROLLER_OCLASS)))
	{
		if (!mobyIsDestroyed(moby) && moby->PVar) {
      struct GuberMoby* guber = guberGetOrCreateObjectByMoby(moby, -1, 1);
      DPRINTF("found controller %08X %08X\n", moby, guber);
      if (guber) {
        controllerOnGuberCreated(moby);
      }
    }

		++moby;
	}
}
