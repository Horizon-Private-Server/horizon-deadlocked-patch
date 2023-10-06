#ifndef _GRIDIRON_BALL_H_
#define _GRIDIRON_BALL_H_

#include <libdl/moby.h>
#include <libdl/math3d.h>

#define BALL_MOBY_OCLASS          (MOBY_ID_BETA_BOX)

void ballPickup(Moby * moby, int playerIdx);
void ballThrow(Moby * moby);
void ballUpdate(Moby * moby);
void ballCreate(VECTOR position);
void ballInitialize(void);

typedef struct BallPVars
{
  VECTOR Velocity;
  int CarrierIdx;
  int DieTime;
} BallPVars_t;

enum BallEventType {
	BALL_EVENT_SPAWN,
	BALL_EVENT_RESET,
};

#endif // _GRIDIRON_BALL_H_
