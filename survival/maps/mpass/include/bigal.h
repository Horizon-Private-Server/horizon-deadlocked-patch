#ifndef _SURVIVAL_MPASS_BIGAL_H_
#define _SURVIVAL_MPASS_BIGAL_H_

#include <libdl/player.h>
#include "game.h"

#define BIGAL_HIT_RADIUS                 (1)
#define BIGAL_RUN_SPEED                  (2)

#define BIGAL_MAX_REMOTE_DIST_TO_TP      (5)
#define BIGAL_STUN_DURATION_TICKS        (TPS * 5)

#define BIGAL_AVOID_PLAYER_RADIUS        (5)

#define BIGAL_ATTACHED_POWER_UP_RADIUS   (5)
#define BIGAL_ATTACHED_MOBY_LIST_ITERATE_PER_FRAME_COUNT   (15)

enum BigAlAnimId
{
	BIGAL_ANIM_IDLE,
	BIGAL_ANIM_SCHEMING,
	BIGAL_ANIM_NERVOUS,
	BIGAL_ANIM_THINKING,
	BIGAL_ANIM_TALKING,
	BIGAL_ANIM_WALKING,
};

enum BigAlEventType {
	BIGAL_EVENT_SPAWN,
  BIGAL_EVENT_PATH_UPDATE,
};

enum BigAlStateId {
  BIGAL_STATE_ACTIVE
};

typedef struct BigAlPVar
{
  VECTOR Position;
  float Speed;
  int HasPath;
  int CurrentNodeIdx;
  int NextNodeIdx;
  char StuckForPlayerIdx;
  u16 StuckOnPathTicks;
} BigAlPVar_t;

int bigalCreate(VECTOR position);
void bigalSpawn(void);
void bigalInit(void);

#endif // _SURVIVAL_MPASS_BIGAL_H_
