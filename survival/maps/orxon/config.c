#include "../../../include/game.h"
#include "../../../include/mob.h"
#include "../../../include/mysterybox.h"
#include "orxon.h"

extern struct SurvivalMapConfig MapConfig;


struct SurvivalSpecialRoundParam specialRoundParams[] = {
	// ROUND 5
	{
		.MaxSpawnedAtOnce = MAX_MOBS_SPAWNED,
    .SpawnCountFactor = 1.0,
    .SpawnRateFactor = 1.0,
		.SpawnParamCount = 2,
		.SpawnParamIds = {
			MOB_SPAWN_PARAM_RUNNER,
			MOB_SPAWN_PARAM_GHOST,
			-1, -1
		},
		.Name = "Ghost Tremor Round"
	},
	// ROUND 10
	{
		.MaxSpawnedAtOnce = MAX_MOBS_SPAWNED,
    .SpawnCountFactor = 1.0,
    .SpawnRateFactor = 1.0,
		.SpawnParamCount = 3,
		.SpawnParamIds = {
			MOB_SPAWN_PARAM_EXPLOSION,
			MOB_SPAWN_PARAM_ACID,
			MOB_SPAWN_PARAM_FREEZE,
			-1
		},
		.Name = "Elemental Round"
	},
	// ROUND 15
	{
		.MaxSpawnedAtOnce = MAX_MOBS_SPAWNED,
    .SpawnCountFactor = 1.0,
    .SpawnRateFactor = 1.0,
		.SpawnParamCount = 2,
		.SpawnParamIds = {
			MOB_SPAWN_PARAM_RUNNER,
			MOB_SPAWN_PARAM_FREEZE,
			-1
		},
		.Name = "Freeze Tremor Round"
	},
	// ROUND 20
	{
		.MaxSpawnedAtOnce = MAX_MOBS_SPAWNED,
    .SpawnCountFactor = 1.0,
    .SpawnRateFactor = 1.0,
		.SpawnParamCount = 3,
		.SpawnParamIds = {
			MOB_SPAWN_PARAM_GHOST,
			MOB_SPAWN_PARAM_EXPLOSION,
			MOB_SPAWN_PARAM_RUNNER,
			-1
		},
		.Name = "Evade Round"
	},
	// ROUND 25
	{
		.MaxSpawnedAtOnce = 20,
    .SpawnCountFactor = 0.1,
    .SpawnRateFactor = 0.2,
		.SpawnParamCount = 3,
		.SpawnParamIds = {
			MOB_SPAWN_PARAM_TITAN,
      MOB_SPAWN_PARAM_NORMAL,
      MOB_SPAWN_PARAM_RUNNER,
			MOB_SPAWN_PARAM_GHOST
		},
		.Name = "Executioner Round"
	},
};

const int specialRoundParamsCount = sizeof(specialRoundParams) / sizeof(struct SurvivalSpecialRoundParam);

// NOTE
// These must be ordered from least probable to most probable
// SHOULD NEVER EXCEED MAX_MOB_SPAWN_PARAMS
struct MobSpawnParams defaultSpawnParams[] = {
	// executioner
	[MOB_SPAWN_PARAM_TITAN]
	{
		.Cost = 1000,
		.MinRound = 10,
		.CooldownTicks = TPS * 10,
		.Probability = 0.01,
    .StatId = MOB_STAT_EXECUTIONER,
		.SpawnType = SPAWN_TYPE_DEFAULT_RANDOM,
		.Name = "Titan",
		.Config = {
			.Xp = 250,
			.Bangles = EXECUTIONER_BANGLE_LEFT_CHEST_PLATE | EXECUTIONER_BANGLE_RIGHT_CHEST_PLATE
                | EXECUTIONER_BANGLE_LEFT_COLLAR_BONE | EXECUTIONER_BANGLE_RIGHT_COLLAR_BONE
                | EXECUTIONER_BANGLE_HELMET | EXECUTIONER_BANGLE_BRAIN,
			.Damage = MOB_BASE_DAMAGE * 2.5,
			.MaxDamage = MOB_BASE_DAMAGE * 5,
      .DamageScale = 1.0,
			.Speed = MOB_BASE_SPEED * 0.85,
			.MaxSpeed = MOB_BASE_SPEED * 2.0,
      .SpeedScale = 0.5,
			.Health = MOB_BASE_HEALTH * 25.0,
			.MaxHealth = 0,
      .HealthScale = 1.0,
			.Bolts = MOB_BASE_BOLTS * 10,
			.AttackRadius = EXECUTIONER_MELEE_ATTACK_RADIUS * 2.0,
			.HitRadius = EXECUTIONER_MELEE_HIT_RADIUS * 1.5,
      .CollRadius = EXECUTIONER_BASE_COLL_RADIUS * 4,
			.ReactionTickCount = EXECUTIONER_BASE_REACTION_TICKS * 0.35,
			.AttackCooldownTickCount = EXECUTIONER_BASE_ATTACK_COOLDOWN_TICKS * 1.5,
			.MaxCostMutation = 10,
			.MobAttribute = 0,
		}
	},
	// ghost zombie
	[MOB_SPAWN_PARAM_GHOST]
	{
		.Cost = 10,
		.MinRound = 4,
		.CooldownTicks = TPS * 1,
		.Probability = 0.05,
    .StatId = MOB_STAT_ZOMBIE_GHOST,
		.SpawnType = SPAWN_TYPE_DEFAULT_RANDOM | SPAWN_TYPE_SEMI_NEAR_PLAYER | SPAWN_TYPE_NEAR_HEALTHBOX,
		.Name = "Ghost",
		.Config = {
			.Xp = 25,
			.Bangles = ZOMBIE_BANGLE_HEAD_2 | ZOMBIE_BANGLE_TORSO_2,
			.Damage = MOB_BASE_DAMAGE * 1.0,
			.MaxDamage = MOB_BASE_DAMAGE * 5.0,
      .DamageScale = 0.2,
			.Speed = MOB_BASE_SPEED * 1.0,
			.MaxSpeed = MOB_BASE_SPEED * 3.0,
      .SpeedScale = 0.5,
			.Health = MOB_BASE_HEALTH * 1.0,
			.MaxHealth = 0,
      .HealthScale = 1.0,
			.Bolts = MOB_BASE_BOLTS * 1.5,
			.AttackRadius = ZOMBIE_MELEE_ATTACK_RADIUS,
			.HitRadius = ZOMBIE_MELEE_HIT_RADIUS,
      .CollRadius = ZOMBIE_BASE_COLL_RADIUS * 1.0,
			.ReactionTickCount = ZOMBIE_BASE_REACTION_TICKS,
			.AttackCooldownTickCount = ZOMBIE_BASE_ATTACK_COOLDOWN_TICKS,
			.MaxCostMutation = 2,
			.MobAttribute = MOB_ATTRIBUTE_GHOST,
		}
	},
	// explode zombie
	[MOB_SPAWN_PARAM_EXPLOSION]
	{
		.Cost = 10,
		.MinRound = 6,
		.CooldownTicks = TPS * 2,
		.Probability = 0.08,
    .StatId = MOB_STAT_ZOMBIE_EXPLODE,
		.SpawnType = SPAWN_TYPE_SEMI_NEAR_PLAYER | SPAWN_TYPE_NEAR_PLAYER | SPAWN_TYPE_ON_PLAYER,
		.Name = "Explosion",
		.Config = {
			.Xp = 40,
			.Bangles = ZOMBIE_BANGLE_HEAD_1 | ZOMBIE_BANGLE_TORSO_1,
			.Damage = MOB_BASE_DAMAGE * 2.0,
			.MaxDamage = MOB_BASE_DAMAGE * 20.0,
      .DamageScale = 1.0,
			.Speed = MOB_BASE_SPEED * 1.0,
			.MaxSpeed = MOB_BASE_SPEED * 1.5,
      .SpeedScale = 0.5,
			.Health = MOB_BASE_HEALTH * 2.0,
			.MaxHealth = 0,
      .HealthScale = 1.0,
			.Bolts = MOB_BASE_BOLTS * 1.0,
			.AttackRadius = ZOMBIE_MELEE_ATTACK_RADIUS,
			.HitRadius = ZOMBIE_EXPLODE_HIT_RADIUS,
      .CollRadius = ZOMBIE_BASE_COLL_RADIUS * 1.0,
			.ReactionTickCount = ZOMBIE_BASE_REACTION_TICKS,
			.AttackCooldownTickCount = ZOMBIE_BASE_ATTACK_COOLDOWN_TICKS,
			.MaxCostMutation = 2,
			.MobAttribute = MOB_ATTRIBUTE_EXPLODE,
		}
	},
	// acid zombie
	[MOB_SPAWN_PARAM_ACID]
	{
		.Cost = 20,
		.MinRound = 10,
		.Probability = 0.09,
		.CooldownTicks = TPS * 1,
    .StatId = MOB_STAT_ZOMBIE_ACID,
		.SpawnType = SPAWN_TYPE_SEMI_NEAR_PLAYER | SPAWN_TYPE_NEAR_PLAYER | SPAWN_TYPE_NEAR_HEALTHBOX,
		.Name = "Acid",
		.Config = {
			.Xp = 50,
			.Bangles = ZOMBIE_BANGLE_HEAD_4 | ZOMBIE_BANGLE_TORSO_4,
			.Damage = MOB_BASE_DAMAGE * 1.0,
			.MaxDamage = MOB_BASE_DAMAGE * 3.0,
      .DamageScale = 0.5,
			.Speed = MOB_BASE_SPEED * 1.0,
			.MaxSpeed = MOB_BASE_SPEED * 1.8,
      .SpeedScale = 1.0,
			.Health = MOB_BASE_HEALTH * 1.0,
			.Bolts = MOB_BASE_BOLTS * 1.2,
			.MaxHealth = 0,
      .HealthScale = 1.0,
			.AttackRadius = ZOMBIE_MELEE_ATTACK_RADIUS,
			.HitRadius = ZOMBIE_MELEE_HIT_RADIUS,
      .CollRadius = ZOMBIE_BASE_COLL_RADIUS * 1.0,
			.ReactionTickCount = ZOMBIE_BASE_REACTION_TICKS,
			.AttackCooldownTickCount = ZOMBIE_BASE_ATTACK_COOLDOWN_TICKS,
			.MaxCostMutation = 2,
			.MobAttribute = MOB_ATTRIBUTE_ACID,
		}
	},
	// freeze zombie
	[MOB_SPAWN_PARAM_FREEZE]
	{
		.Cost = 20,
		.MinRound = 8,
		.CooldownTicks = TPS * 1,
		.Probability = 0.1,
    .StatId = MOB_STAT_ZOMBIE_FREEZE,
		.SpawnType = SPAWN_TYPE_SEMI_NEAR_PLAYER | SPAWN_TYPE_NEAR_PLAYER,
		.Name = "Freeze",
		.Config = {
			.Xp = 50,
			.Bangles = ZOMBIE_BANGLE_HEAD_1 | ZOMBIE_BANGLE_TORSO_1,
			.Damage = MOB_BASE_DAMAGE * 1.0,
			.MaxDamage = MOB_BASE_DAMAGE * 2.2,
      .DamageScale = 0.5,
			.Speed = MOB_BASE_SPEED * 0.8,
			.MaxSpeed = MOB_BASE_SPEED * 2.5,
      .SpeedScale = 0.5,
			.Health = MOB_BASE_HEALTH * 1.2,
			.MaxHealth = 0,
      .HealthScale = 1.0,
			.Bolts = MOB_BASE_BOLTS * 1.2,
			.AttackRadius = ZOMBIE_MELEE_ATTACK_RADIUS,
			.HitRadius = ZOMBIE_MELEE_HIT_RADIUS,
      .CollRadius = ZOMBIE_BASE_COLL_RADIUS * 1.0,
			.ReactionTickCount = ZOMBIE_BASE_REACTION_TICKS,
			.AttackCooldownTickCount = ZOMBIE_BASE_ATTACK_COOLDOWN_TICKS,
			.MaxCostMutation = 2,
			.MobAttribute = MOB_ATTRIBUTE_FREEZE,
		}
	},
	// runner zombie
	[MOB_SPAWN_PARAM_RUNNER]
	{
		.Cost = 10,
		.MinRound = 0,
		.CooldownTicks = 0,
		.Probability = 0.1,
    .StatId = MOB_STAT_TREMOR,
		.SpawnType = SPAWN_TYPE_SEMI_NEAR_PLAYER | SPAWN_TYPE_NEAR_PLAYER,
		.Name = "Runner",
		.Config = {
			.Xp = 15,
			.Bangles = TREMOR_BANGLE_HEAD | TREMOR_BANGLE_CHEST | TREMOR_BANGLE_LEFT_ARM,
			.Damage = MOB_BASE_DAMAGE * 0.7,
			.MaxDamage = MOB_BASE_DAMAGE * 2.2,
      .DamageScale = 0.3,
			.Speed = MOB_BASE_SPEED * 2.0,
			.MaxSpeed = MOB_BASE_SPEED * 3.0,
      .SpeedScale = 0.5,
			.Health = MOB_BASE_HEALTH * 0.6,
			.MaxHealth = 0,
      .HealthScale = 1.0,
			.Bolts = MOB_BASE_BOLTS * 1.0,
			.AttackRadius = TREMOR_MELEE_ATTACK_RADIUS,
			.HitRadius = TREMOR_MELEE_HIT_RADIUS,
      .CollRadius = TREMOR_BASE_COLL_RADIUS * 1.0,
			.ReactionTickCount = TREMOR_BASE_REACTION_TICKS,
			.AttackCooldownTickCount = TREMOR_BASE_ATTACK_COOLDOWN_TICKS,
			.MaxCostMutation = 2,
			.MobAttribute = 0,
		}
	},
	// normal zombie
	[MOB_SPAWN_PARAM_NORMAL]
	{
		.Cost = 5,
		.MinRound = 0,
		.CooldownTicks = 0,
		.Probability = 1.0,
    .StatId = MOB_STAT_ZOMBIE,
		.SpawnType = SPAWN_TYPE_SEMI_NEAR_PLAYER | SPAWN_TYPE_NEAR_PLAYER,
		.Name = "Zombie",
		.Config = {
			.Xp = 10,
			.Bangles = ZOMBIE_BANGLE_HEAD_1 | ZOMBIE_BANGLE_TORSO_1,
			.Damage = MOB_BASE_DAMAGE * 1.0,
			.MaxDamage = MOB_BASE_DAMAGE * 3.3,
      .DamageScale = 1.0,
			.Speed = MOB_BASE_SPEED * 1.0,
			.MaxSpeed = MOB_BASE_SPEED * 2.0,
      .SpeedScale = 0.5,
			.Health = MOB_BASE_HEALTH * 1.0,
			.MaxHealth = 0,
      .HealthScale = 1.0,
			.Bolts = MOB_BASE_BOLTS * 1.0,
			.AttackRadius = ZOMBIE_MELEE_ATTACK_RADIUS,
			.HitRadius = ZOMBIE_MELEE_HIT_RADIUS,
      .CollRadius = ZOMBIE_BASE_COLL_RADIUS * 1.0,
			.ReactionTickCount = ZOMBIE_BASE_REACTION_TICKS,
			.AttackCooldownTickCount = ZOMBIE_BASE_ATTACK_COOLDOWN_TICKS,
			.MaxCostMutation = 2,
			.MobAttribute = 0,
		}
	},
};

const int defaultSpawnParamsCount = sizeof(defaultSpawnParams) / sizeof(struct MobSpawnParams);


u32 MobPrimaryColors[] = {
	[MOB_SPAWN_PARAM_NORMAL] 	0x00464443,
	[MOB_SPAWN_PARAM_RUNNER] 	0x00464443,
	[MOB_SPAWN_PARAM_FREEZE] 	0x00804000,
	[MOB_SPAWN_PARAM_ACID] 		0x00464443,
	[MOB_SPAWN_PARAM_EXPLOSION] 0x00202080,
	[MOB_SPAWN_PARAM_GHOST] 	0x00464443,
	[MOB_SPAWN_PARAM_TITAN]		0x00464443,
};

u32 MobSecondaryColors[] = {
	[MOB_SPAWN_PARAM_NORMAL] 	0x80202020,
	[MOB_SPAWN_PARAM_RUNNER] 	0x80202020,
	[MOB_SPAWN_PARAM_FREEZE] 	0x80FF2000,
	[MOB_SPAWN_PARAM_ACID] 		0x8000FF00,
	[MOB_SPAWN_PARAM_EXPLOSION] 0x000040C0,
	[MOB_SPAWN_PARAM_GHOST] 	0x80202020,
	[MOB_SPAWN_PARAM_TITAN]		0x80FF2020,
};

u32 MobLODColors[] = {
	[MOB_SPAWN_PARAM_NORMAL] 	0x00808080,
	[MOB_SPAWN_PARAM_RUNNER] 	0x00808080,
	[MOB_SPAWN_PARAM_FREEZE] 	0x00F08000,
	[MOB_SPAWN_PARAM_ACID] 		0x0000F000,
	[MOB_SPAWN_PARAM_EXPLOSION] 0x004040F0,
	[MOB_SPAWN_PARAM_GHOST] 	0x00464443,
	[MOB_SPAWN_PARAM_TITAN]		0x00808080,
};

struct MysteryBoxItemWeight MysteryBoxItemProbabilities[] = {
  { MYSTERY_BOX_ITEM_RESET_GATE, 0.05 },
  { MYSTERY_BOX_ITEM_INVISIBILITY_CLOAK, 0.0526 },
  { MYSTERY_BOX_ITEM_REVIVE_TOTEM, 0.0555 },
  { MYSTERY_BOX_ITEM_INFINITE_AMMO, 0.0555 },
  { MYSTERY_BOX_ITEM_ACTIVATE_POWER, 0.0888 },
  { MYSTERY_BOX_ITEM_UPGRADE_WEAPON, 0.0967 },
  { MYSTERY_BOX_ITEM_TEDDY_BEAR, 0.1428 },
  { MYSTERY_BOX_ITEM_DREAD_TOKEN, 0.33333 },
  // { MYSTERY_BOX_ITEM_WEAPON_MOD, 0.4 },
  { MYSTERY_BOX_ITEM_WEAPON_MOD, 1.0 },
};
const int MysteryBoxItemMysteryBoxItemProbabilitiesCount = sizeof(MysteryBoxItemProbabilities)/sizeof(struct MysteryBoxItemWeight);

void configInit(void)
{
  MapConfig.DefaultSpawnParams = defaultSpawnParams;
  MapConfig.DefaultSpawnParamsCount = defaultSpawnParamsCount;
  MapConfig.SpecialRoundParams = specialRoundParams;
  MapConfig.SpecialRoundParamsCount = specialRoundParamsCount;
}
