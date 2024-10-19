#ifndef RAIDS_MOVER_H
#define RAIDS_MOVER_H

#include <tamtypes.h>
#include <libdl/moby.h>
#include <libdl/math.h>
#include <libdl/time.h>
#include <libdl/player.h>
#include <libdl/math3d.h>

#define MOVER_OCLASS                        (0x4002)
#define MOVER_MAX_TARGETS                   (4)

enum MoverEventType {
	MOVER_EVENT_SPAWN,
  MOVER_EVENT_SET_STATE,
};

enum MoverState {
	MOVER_STATE_DEACTIVATED,
	MOVER_STATE_ACTIVATED,
};

enum MoverSplineLoopType {
	MOVER_MOTION_NONE,
	MOVER_MOTION_LOOP,
	MOVER_MOTION_PING_PONG,
};

struct MoverRuntimeState
{
  VECTOR LastAppliedPositionDelta;
  VECTOR LastAppliedRotationDelta;
  int TimeStarted;
  int CurrentSplineDir;
  float CurrentSplineLen;
};

struct MoverPVar
{
  // linear motion
  VECTOR Velocity;
  VECTOR Acceleration;
  VECTOR AngularVelocity;
  VECTOR AngularAcceleration;

  // cyclic motion
  VECTOR FrequencyStart;
  VECTOR AngularFrequencyStart;
  
  // general
  int Init;
  enum MoverState DefaultState;

  // linear motion
  float Speed;
  float AngularSpeed;

  // cyclic motion
  int Frequency;
  int AngularFrequency;

  // spline
  int SplineIdx;
  char SplineInitSnapTo;
  float SplineSpeed;
  enum MoverSplineLoopType SplineLoop;

  // 
  float RuntimeSeconds;

  // targets
  Moby* MobyTargets[MOVER_MAX_TARGETS];
  int CuboidTargets[MOVER_MAX_TARGETS];

  struct MoverRuntimeState State;
};

void moverBroadcastNewState(Moby* moby, enum MoverState state);
struct GuberMoby* moverGetGuber(Moby* moby);
int moverHandleEvent(Moby* moby, GuberEvent* event);
void moverStart(void);
void moverInit(void);

#endif // RAIDS_MOVER_H
