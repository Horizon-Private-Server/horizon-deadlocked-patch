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
  CONTROLLER_EVENT_ITERATE
};

enum ControllerState {
	CONTROLLER_STATE_DEACTIVATED,
	CONTROLLER_STATE_IDLE,
	CONTROLLER_STATE_ACTIVATED,
	CONTROLLER_STATE_COMPLETED = 127,
};

enum ControllerConditionType {
  CONTROLLER_CONDITION_TYPE_NONE,
  CONTROLLER_CONDITION_TYPE_MOBY_STATE,
  CONTROLLER_CONDITION_TYPE_CUBOID,
  CONTROLLER_CONDITION_TYPE_PLAYER_BUTTON,
  CONTROLLER_CONDITION_TYPE_DELAY,
  CONTROLLER_CONDITION_TYPE_XOR,
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

enum ControllerCuboidTriggerBy {
  CONTROLLER_CUBOID_TRIGGER_BY_NONE = 0,
	CONTROLLER_CUBOID_TRIGGER_BY_ANY_PLAYER = 1 << 0,
	CONTROLLER_CUBOID_TRIGGER_BY_ALL_PLAYERS = 1 << 1,
	CONTROLLER_CUBOID_TRIGGER_BY_NO_PLAYERS = 1 << 2,
	CONTROLLER_CUBOID_TRIGGER_BY_ANY_NPC = 1 << 3,
	CONTROLLER_CUBOID_TRIGGER_BY_ALL_NPCS = 1 << 4,
	CONTROLLER_CUBOID_TRIGGER_BY_NO_NPCS = 1 << 5,

	CONTROLLER_CUBOID_TRIGGER_BY_CHECK_PLAYER = CONTROLLER_CUBOID_TRIGGER_BY_ANY_PLAYER | CONTROLLER_CUBOID_TRIGGER_BY_ALL_PLAYERS | CONTROLLER_CUBOID_TRIGGER_BY_NO_PLAYERS,
	CONTROLLER_CUBOID_TRIGGER_BY_CHECK_NPC = CONTROLLER_CUBOID_TRIGGER_BY_ANY_NPC | CONTROLLER_CUBOID_TRIGGER_BY_ALL_NPCS | CONTROLLER_CUBOID_TRIGGER_BY_NO_NPCS,
};

enum ControllerCuboidMobyInteractType {
  CONTROLLER_CUBOID_MOBY_SKIP,
	CONTROLLER_CUBOID_MOBY_MOB,
};

enum ControllerTargetUpdateType {
  CONTROLLER_TARGET_UPDATE_TYPE_NONE,
  CONTROLLER_TARGET_UPDATE_TYPE_MOBY_STATE,
  CONTROLLER_TARGET_UPDATE_TYPE_MOBY_ANIMATION,
  CONTROLLER_TARGET_UPDATE_TYPE_MOBY_ENABLED,
  CONTROLLER_TARGET_UPDATE_TYPE_MOVE_CUBOID,
  CONTROLLER_TARGET_UPDATE_TYPE_MOBY_STATE_ADDITIVE,
};

struct ControllerRuntimeState
{
  char TriggersActivated;
  int Iterations;
  int DelayStartTime[CONTROLLER_MAX_CONDITIONS];
};

struct ControllerCondition
{
  enum ControllerConditionType ConditionType;
  Moby* Moby;

  union {
    
    // trigger if moby state
    struct {
      short StateInteractType;
      short State;
    } MobyState;
    
    // trigger if cuboid
    struct {
      int CuboidIdx;
      short InteractType;
      char TriggerBy;
      char TriggerByAll;
    } Cuboid;
    
    // trigger if player button
    struct {
      int PadMask;
    } PlayerButtons;
    
    // trigger if delay
    struct {
      int Milliseconds;
    } Delay;
  };
};

struct ControllerTarget
{
  Moby* Moby;
  char State;
  char AnimId;
  char Enabled;
  char TargetUpdateType;
  int CuboidDestIdx;
  int CuboidSrcIdx;
};

struct ControllerPVar
{
  int Init;
  enum ControllerState DefaultState;
  struct ControllerTarget Targets[CONTROLLER_MAX_TARGETS];
  char TriggerIfAllTrue;
  short Repeat;
  struct ControllerCondition Conditions[CONTROLLER_MAX_CONDITIONS];
  struct ControllerRuntimeState State;
};

void controllerBroadcastNewState(Moby* moby, enum ControllerState state);
struct GuberMoby* controllerGetGuber(Moby* moby);
int controllerHandleEvent(Moby* moby, GuberEvent* event);
void controllerStart(void);
void controllerInit(void);

#endif // RAIDS_CONTROLLER_H
