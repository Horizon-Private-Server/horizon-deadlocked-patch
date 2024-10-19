#ifndef RAIDS_CONTROLLER_H
#define RAIDS_CONTROLLER_H

#include <tamtypes.h>
#include <libdl/moby.h>
#include <libdl/math.h>
#include <libdl/time.h>
#include <libdl/player.h>
#include <libdl/math3d.h>

#define CONTROLLER_OCLASS                         (0x4003)
#define CONTROLLER_MAX_CONDITIONS                 (8)
#define CONTROLLER_MAX_TARGETS                    (8)

enum ControllerEventType {
	CONTROLLER_EVENT_SPAWN,
  CONTROLLER_EVENT_SET_STATE,
};

enum ControllerState {
	CONTROLLER_STATE_DEACTIVATED,
	CONTROLLER_STATE_IDLE,
	CONTROLLER_STATE_ACTIVATED,
};

enum ControllerConditionType {
  CONTROLLER_CONDITION_TYPE_NONE,
  CONTROLLER_CONDITION_TYPE_MOBY_STATE,
  CONTROLLER_CONDITION_TYPE_CUBOID,
  CONTROLLER_CONDITION_TYPE_PLAYER_BUTTON,
};

enum ControllerMobyStateInteractType {
	CONTROLLER_CUBOID_INTERACT_EQUAL,
	CONTROLLER_CUBOID_INTERACT_NOTEQUAL,
	CONTROLLER_CUBOID_INTERACT_LESS,
	CONTROLLER_CUBOID_INTERACT_LEQUAL,
	CONTROLLER_CUBOID_INTERACT_GREATER,
	CONTROLLER_CUBOID_INTERACT_GEQUAL,
};

enum ControllerCuboidInteractType {
	CONTROLLER_CUBOID_INTERACT_WHEN_INSIDE,
	CONTROLLER_CUBOID_INTERACT_WHEN_OUTSIDE,
};

enum ControllerTargetUpdateType {
  CONTROLLER_TARGET_UPDATE_TYPE_NONE,
  CONTROLLER_TARGET_UPDATE_TYPE_MOBY_STATE,
  CONTROLLER_TARGET_UPDATE_TYPE_MOBY_ANIMATION,
  CONTROLLER_TARGET_UPDATE_TYPE_MOBY_ENABLED,
};

struct ControllerRuntimeState
{
  int TriggersActivated;
};

struct ControllerCondition
{
  enum ControllerConditionType ConditionType;

  // trigger if moby state
  Moby* Moby;
  short MobyStateInteractType;
  short MobyState;

  // trigger if cuboid
  int CuboidIdx;
  short CuboidInteractType;
  short CuboidAllPlayers;

  // trigger if player button
  int PlayerButtonMask;
};

struct ControllerTarget
{
  Moby* Moby;
  char State;
  char AnimId;
  char Enabled;
  char TargetUpdateType;
};

struct ControllerPVar
{
  int Init;
  enum ControllerState DefaultState;
  struct ControllerTarget Targets[CONTROLLER_MAX_TARGETS];
  int TriggerIfAllTrue;
  struct ControllerCondition Conditions[CONTROLLER_MAX_CONDITIONS];
  struct ControllerRuntimeState State;
};

void controllerBroadcastNewState(Moby* moby, enum ControllerState state);
struct GuberMoby* controllerGetGuber(Moby* moby);
int controllerHandleEvent(Moby* moby, GuberEvent* event);
void controllerStart(void);
void controllerInit(void);

#endif // RAIDS_CONTROLLER_H
