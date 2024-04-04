#ifndef _GRIDIRON_BALL_H_
#define _GRIDIRON_BALL_H_

#include <libdl/moby.h>
#include <libdl/math3d.h>

#define BALL_MOBY_OCLASS          (MOBY_ID_BETA_BOX)

void ballReset(Moby* mobyy, int resetType);
int ballGetCarrierIdx(Moby* moby);
void ballPickup(Moby * moby, int playerIdx);
void ballResendPickup(Moby * moby);
void ballThrow(Moby * moby, float power);
void ballUpdate(Moby * moby);
void ballCreate(VECTOR position);
void ballInitialize(void);

typedef struct BallPVars
{
  VECTOR Velocity;
  VECTOR SyncPosition;
  VECTOR SyncVelocity;
  int CarrierIdx;
  int DieTime;
  int SyncTicks;
} BallPVars_t;

enum BallEventType {
	BALL_EVENT_SPAWN,
	BALL_EVENT_RESET,
  BALL_EVENT_PICKUP,
  BALL_EVENT_DROP,
};

#endif // _GRIDIRON_BALL_H_
