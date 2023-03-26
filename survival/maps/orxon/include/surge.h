#ifndef _SURVIVAL_ORXON_SURGE_H_
#define _SURVIVAL_ORXON_SURGE_H_

#include <libdl/player.h>
#include "game.h"

#define SURGE_MOBY_OCLASS                (0x201C)

#define SURGE_HIT_RADIUS                 (1)
#define SURGE_RUN_SPEED                  (20)

#define SURGE_MAX_REMOTE_DIST_TO_TP      (5)
#define SURGE_STUN_DURATION_TICKS        (TPS * 5)

#define SURGE_AVOID_PLAYER_RADIUS        (15)

#define SURGE_ARC_TICKER_MIN             (TPS * 1)
#define SURGE_ARC_TICKER_MAX             (TPS * 5)
#define SURGE_ATTACHED_ARC_TICKER_MIN    (TPS * 0.1)
#define SURGE_ATTACHED_ARC_TICKER_MAX    (TPS * 1)
#define SURGE_STUNNED_ARC_TICKER_MIN     (10)
#define SURGE_STUNNED_ARC_TICKER_MAX     (20)
#define SURGE_SURGE_ARC_TICKER_MIN       (10)
#define SURGE_SURGE_ARC_TICKER_MAX       (20)
#define SURGE_ARC_RENDER_FOR             (5)

#define SURGE_LINE_COUNT                 (5)
#define SURGE_BASE_PARTICLE_COUNT        (6)
#define SURGE_MAX_PARTICLES              (10)
#define SURGE_HEAD_PARTICLE_IDX          (5)
#define SURGE_PARTICLE_MAX_ACCELERATION  (2.0)
#define SURGE_EYE_SPACING                (0.6)

#define SURGE_ATTACHED_POWER_UP_RADIUS   (5)
#define SURGE_ATTACHED_MOBY_LIST_ITERATE_PER_FRAME_COUNT   (15)

enum SurgeEventType {
	SURGE_EVENT_SPAWN,
  SURGE_EVENT_PATH_UPDATE,
  SURGE_EVENT_ATTACH_UPDATE
};

enum SurgeStateId {
  SURGE_STATE_HIDDEN,
  SURGE_STATE_ACTIVE
};

typedef struct SurgeParticle
{
  VECTOR StartVelocity;
  Moby* SurgeArc;
  u32 Ticker;
} SurgeParticle_t;

typedef struct SurgePVar
{
  SurgeParticle_t Particles[SURGE_MAX_PARTICLES];
  Moby* AttachedTo;
  Player* AttachedToPlayer;
  Moby* MobyListAt;
  VECTOR Position;
  float Speed;
  int HasPath;
  int CurrentNodeIdx;
  int NextNodeIdx;
  u16 StunnedTicks;
  u16 StuckOnPathTicks;
  char HasSlammed;
} SurgePVar_t;

int surgeCreate(VECTOR position);
void surgeSpawn(void);
void surgeInit(void);

#endif // _SURVIVAL_ORXON_SURGE_H_
