#ifndef ZOMBIES_MOB_H
#define ZOMBIES_MOB_H

#include <tamtypes.h>
#include <libdl/moby.h>
#include <libdl/math.h>
#include <libdl/time.h>
#include <libdl/player.h>
#include <libdl/sound.h>


struct MobConfig {
	int MobType;
	float Damage;
	float Speed;
	float MaxHealth;
	float AttackRadius;
	float HitRadius;
	u8 ReactionTickCount;
	u8 AttackCooldownTickCount;
};

struct MobVars {
	struct MobConfig Config;
	int Action;
	int NextAction;
	float Health;
	Moby * Target;
	u16 NextActionDelayTicks;
	u16 ActionCooldownTicks;
	u16 AttackCooldownTicks;
	u16 ScoutCooldownTicks;
	u16 FlinchCooldownTicks;
	u16 TimeBombTicks;
	u16 StuckTicks;
	u16 MovingTicks;
	char IsTraversing;
	char AnimationLooped;
	char AnimationReset;
	char OpacityFlickerDirection;
	char Destroy;
	char Respawn;
};

struct MobPVar {
  struct TargetVars * TargetVarsPtr;
	char _pad0[0x18];
  struct MoveVars_V2 * MoveVarsPtr;
	char _pad1[0x14];
  struct FlashVars * FlashVarsPtr;
	char _pad2[0x18];

	struct TargetVars TargetVars;
	struct MoveVars_V2 MoveVars;
	struct FlashVars FlashVars;
	struct MobVars MobVars;
};

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
	MOB_GHOST
};


GuberMoby * mobCreate(VECTOR position, float yaw, struct MobConfig *config);
void mobInitialize(void);

#endif // ZOMBIES_MOB_H
