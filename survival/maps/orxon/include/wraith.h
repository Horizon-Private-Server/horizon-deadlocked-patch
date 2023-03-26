#ifndef _SURVIVAL_ORXON_WRAITH_H_
#define _SURVIVAL_ORXON_WRAITH_H_

#define WRAITH_MOBY_OCLASS                (0x2480)

#define WRAITH_HEIGHT                     (5)
#define WRAITH_HIT_RADIUS                 (3)
#define WRAITH_AGGRO_RANGE                (15)
#define WRAITH_DAMAGE                     (3)
#define WRAITH_WALK_SPEED                 (3)
#define WRAITH_AGGRO_SPEED                (10)
#define WRAITH_OUTSIDE_GAS_SPEED          (5)
#define WRAITH_OUTSIDE_GAS_DAMAGE         (1)
#define WRAITH_OUTSIDE_GAS_SCALE          (0.5)

#define WRAITH_MAX_REMOTE_DIST_TO_TP      (5)

#define WRAITH_AMBIENT_SOUND_COOLDOWN_MIN (TPS * 1)
#define WRAITH_AMBIENT_SOUND_COOLDOWN_MAX (TPS * 4)

#define WRAITH_BASE_PARTICLE_COUNT        (6)
#define WRAITH_MAX_PARTICLES              (24)
#define WRAITH_HEAD_PARTICLE_IDX          (5)
#define WRAITH_PARTICLE_MAX_ACCELERATION  (2.0)
#define WRAITH_EYE_SPACING                (0.6)

enum WraithEventType {
	WRAITH_EVENT_SPAWN,
  WRAITH_EVENT_PATH_UPDATE,
  WRAITH_EVENT_TARGET_UPDATE
};

typedef struct WraithParticle
{
  VECTOR Offset;
  VECTOR PRandomess;
  float Random;
  float Height;
  float Scale;
  float ScaleBreathe;
  float ScaleBreatheSpeed;
  float PRadius;
  float RRadius;
  float PSpeed;
  float RSpeed;
  float PTheta;
  float RTheta;
  u32 Color;
  u32 TexId;
  char Randomness[4];
} WraithParticle_t;

typedef struct WraithPVar
{
  WraithParticle_t Particles[WRAITH_MAX_PARTICLES];
  Player* Target;
  float Speed;
  float Scale;
  int HasPath;
  int CurrentNodeIdx;
  int NextNodeIdx;
  u8 AmbientSoundTicks;
} WraithPVar_t;

int wraithCreate(VECTOR position);
void wraithSpawn(void);
void wraithInit(void);

#endif // _SURVIVAL_ORXON_WRAITH_H_
