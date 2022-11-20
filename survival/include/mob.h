#ifndef SURVIVAL_MOB_H
#define SURVIVAL_MOB_H

#include <tamtypes.h>
#include <libdl/moby.h>
#include <libdl/math.h>
#include <libdl/time.h>
#include <libdl/player.h>
#include <libdl/sound.h>

enum ZombieAnimId
{
	ZOMBIE_ANIM_UNDERGROUND,
	ZOMBIE_ANIM_IDLE,
	ZOMBIE_ANIM_2,
	ZOMBIE_ANIM_WALK,
	ZOMBIE_ANIM_AGGRO_WALK,
	ZOMBIE_ANIM_RUN,
	ZOMBIE_ANIM_JUMP,
	ZOMBIE_ANIM_CROUCH,
	ZOMBIE_ANIM_ELECTROCUTED,
	ZOMBIE_ANIM_CRAWL_OUT_OF_GROUND,
	ZOMBIE_ANIM_FLINCH,
	ZOMBIE_ANIM_THROW_HEAD,
	ZOMBIE_ANIM_TAUNT,
	ZOMBIE_ANIM_CAT_PASS,
	ZOMBIE_ANIM_BIG_FLINCH,
	ZOMBIE_ANIM_SLAP,
	ZOMBIE_ANIM_IDLE_2
};

enum MobAction
{
	MOB_ACTION_SPAWN,
	MOB_ACTION_IDLE,
	MOB_ACTION_JUMP,
	MOB_ACTION_WALK,
	MOB_ACTION_FLINCH,
	MOB_ACTION_LOOK_AT_TARGET,
	MOB_ACTION_ATTACK,
	MOB_ACTION_TIME_BOMB,
};

enum MobType
{
	MOB_NORMAL,
	MOB_FREEZE,
	MOB_ACID,
	MOB_EXPLODE,
	MOB_GHOST,
	MOB_TANK
};

enum MobSpecialMutationType
{
	MOB_SPECIAL_MUTATION_NONE = 0,
	MOB_SPECIAL_MUTATION_FREEZE,
	MOB_SPECIAL_MUTATION_ACID,
	MOB_SPECIAL_MUTATION_GHOST,
	MOB_SPECIAL_MUTATION_COUNT
};

enum MobEvent
{
	MOB_EVENT_SPAWN,
	MOB_EVENT_DESTROY,
	MOB_EVENT_DAMAGE,
	MOB_EVENT_STATE_UPDATE,
	MOB_EVENT_TARGET_UPDATE,
	MOB_EVENT_OWNER_UPDATE,
};


enum MobMutateAttribute {
	MOB_MUTATE_DAMAGE,
	MOB_MUTATE_SPEED,
	MOB_MUTATE_HEALTH,
	MOB_MUTATE_COST,
	MOB_MUTATE_COUNT
};

enum ZombieBangles {
	ZOMBIE_BANGLE_HEAD_1 =  	(1 << 0),
	ZOMBIE_BANGLE_HEAD_2 =  	(1 << 1),
	ZOMBIE_BANGLE_HEAD_3 =  	(1 << 2),
	ZOMBIE_BANGLE_HEAD_4 =  	(1 << 3),
	ZOMBIE_BANGLE_HEAD_5 =  	(1 << 4),
	ZOMBIE_BANGLE_TORSO_1 = 	(1 << 5),
	ZOMBIE_BANGLE_TORSO_2 = 	(1 << 6),
	ZOMBIE_BANGLE_TORSO_3 = 	(1 << 7),
	ZOMBIE_BANGLE_TORSO_4 = 	(1 << 8),
	ZOMBIE_BANGLE_HIPS = 			(1 << 9),
	ZOMBIE_BANGLE_LLEG = 			(1 << 10),
	ZOMBIE_BANGLE_RFOOT = 		(1 << 11),
	ZOMBIE_BANGLE_RLEG = 			(1 << 12),
	ZOMBIE_BANGLE_RARM = 			(1 << 13),
	ZOMBIE_BANGLE_LARM = 			(1 << 14),
	ZOMBIE_BANGLE_HIDE_BODY =	(1 << 15)
};

// ordered from least to most probable
enum MobSpawnParamIds {
	MOB_SPAWN_PARAM_TITAN = 0,
	MOB_SPAWN_PARAM_GHOST = 1,
	MOB_SPAWN_PARAM_EXPLOSION = 2,
	MOB_SPAWN_PARAM_ACID = 3,
	MOB_SPAWN_PARAM_FREEZE = 4,
	MOB_SPAWN_PARAM_RUNNER = 5,
	MOB_SPAWN_PARAM_NORMAL = 6,
	MOB_SPAWN_PARAM_COUNT
};

// what kind of spawn a mob can have
enum MobSpawnType {
	SPAWN_TYPE_DEFAULT_RANDOM = 0,
	SPAWN_TYPE_SEMI_NEAR_PLAYER = 1,
	SPAWN_TYPE_NEAR_PLAYER = 2,
	SPAWN_TYPE_ON_PLAYER = 4,
	SPAWN_TYPE_NEAR_HEALTHBOX = 8,
};

struct MobConfig {
	int MobType;
	int Bolts;
	float Damage;
	float MaxDamage;
	float Speed;
	float MaxSpeed;
	float Health;
	float MaxHealth;
	float AttackRadius;
	float HitRadius;
	u16 Bangles;
	u8 MaxCostMutation;
	u8 ReactionTickCount;
	u8 AttackCooldownTickCount;
	char MobSpecialMutation;
};

struct MobSpawnParams {
	int Cost;
	int MinRound;
	int CooldownTicks;
	float Probability;
	enum MobSpawnType SpawnType;
	char Name[32];
	struct MobConfig Config;
};

struct Knockback {
	short Angle;
	u8 Power;
	u8 Ticks;
};

struct MobVars {
	struct MobConfig Config;
	struct Knockback Knockback;
	int Action;
	int NextAction;
	float Health;
	float ClosestDist;
	float LastSpeed;
	Moby * Target;
	int LastHitBy;
	u16 LastHitByOClass;
	u16 NextCheckActionDelayTicks;
	u16 NextActionDelayTicks;
	u16 ActionCooldownTicks;
	u16 AttackCooldownTicks;
	u16 ScoutCooldownTicks;
	u16 FlinchCooldownTicks;
	u16 AutoDirtyCooldownTicks;
	u16 AmbientSoundCooldownTicks;
	u16 TimeBombTicks;
	u16 StuckTicks;
	u16 MovingTicks;
	u16 TimeLastGroundedTicks;
	u8 ActionId;
	u8 LastActionId;
	u8 MoveStep;
	u8 MoveStepCooldownTicks;
	char Owner;
	char IsTraversing;
	char AnimationLooped;
	char AnimationReset;
	char OpacityFlickerDirection;
	char Destroy;
	char Respawn;
	char Dirty;
	char Destroyed;
	char Order;
	char Random;
};

// warning: multiple differing types with the same name, only one recovered
struct ReactVars {
	/*   0 */ int flags;
	/*   4 */ int lastReactFrame;
	/*   8 */ Moby* pInfectionMoby;
	/*   c */ float acidDamage;
	/*  10 */ char eternalDeathCount;
	/*  11 */ char doHotSpotChecks;
	/*  12 */ char unchainable;
	/*  13 */ char isShielded;
	/*  14 */ char state;
	/*  15 */ char deathState;
	/*  16 */ char deathEffectState;
	/*  17 */ char deathStateType;
	/*  18 */ signed char knockbackRes;
	/*  19 */ char padC;
	/*  1a */ char padD;
	/*  1b */ char padE;
	/*  1c */ char padF;
	/*  1d */ char padG;
	/*  1e */ char padH;
	/*  1f */ char padI;
	/*  20 */ float minorReactPercentage;
	/*  24 */ float majorReactPercentage;
	/*  28 */ float deathHeight;
	/*  2c */ float bounceDamp;
	/*  30 */ int deadlyHotSpots;
	/*  34 */ float curUpGravity;
	/*  38 */ float curDownGravity;
	/*  3c */ float shieldDamageReduction;
	/*  40 */ float damageReductionSameOClass;
	/*  44 */ int deathCorn;
	/*  48 */ int deathType;
	/*  4c */ short int deathSound;
	/*  4e */ short int deathSound2;
	/*  50 */ float peakFrame;
	/*  54 */ float landFrame;
	/*  58 */ float drag;
	/*  5c */ short unsigned int effectStates;
	/*  5e */ short unsigned int effectPrimMask;
	/*  60 */ short unsigned int effectTimers[16];
};

struct MobPVar {
  struct TargetVars * TargetVarsPtr;
	char _pad0[0x0C];
	struct ReactVars * ReactVarsPtr;
	char _pad1[0x08];
  struct MoveVars_V2 * MoveVarsPtr;
	char _pad2[0x14];
  struct FlashVars * FlashVarsPtr;
	char _pad3[0x18];

	struct TargetVars TargetVars;
	struct ReactVars ReactVars;
	struct MoveVars_V2 MoveVars;
	struct FlashVars FlashVars;
	struct MobVars MobVars;
};

struct MobDamageEventArgs
{
	struct Knockback Knockback;
	int SourceUID;
	float Angle;
	u16 DamageQuarters;
	u16 SourceOClass;
};

struct MobActionUpdateEventArgs
{
	int Action;
	u8 ActionId;
};

struct MobStateUpdateEventArgs
{
	VECTOR Position;
	int TargetUID;
};

struct MobSpawnEventArgs
{
	int Bolts;
	u16 StartHealth;
	u16 Bangles;
	char MobType;
	char MobSpecialMutation;
	u8 Damage;
	u8 AttackRadiusEighths;
	u8 HitRadiusEighths;
	u8 SpeedHundredths;
	u8 ReactionTickCount;
	u8 AttackCooldownTickCount;
};

void mobNuke(int killedByPlayerId);
void mobMutate(struct MobSpawnParams* spawnParams, enum MobMutateAttribute attribute);
int mobCreate(VECTOR position, float yaw, int spawnFromUID, struct MobConfig *config);
void mobInitialize(void);
void mobTick(void);

#endif // SURVIVAL_MOB_H
