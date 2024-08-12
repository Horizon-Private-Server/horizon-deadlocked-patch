#include <libdl/utils.h>
#include "../../../include/game.h"
#include "../../../include/mob.h"
#include "../../../include/mysterybox.h"
#include "orxon.h"

extern struct SurvivalMapConfig MapConfig;

#define ORXON_MOB_MAX_HEALTH            (150000)


//--------------------------------------------------------------------------
struct SurvivalSpecialRoundParam specialRoundParams[] = {
	// ROUND 5
	{
    .MinRound = 5,
    .RepeatEveryNRounds = 25,
    .RepeatCount = 0,
    .UnlimitedPostRoundTime = 0,
    .DisableDrops = 0,
		.MaxSpawnedAtOnce = MAX_MOBS_ALIVE_REAL,
    .SpawnCountFactor = 1.0,
    .SpawnRateFactor = 1.0,
		.SpawnParamCount = 2,
		.SpawnParamIds = {
			MOB_SPAWN_PARAM_TREMOR,
			MOB_SPAWN_PARAM_GHOST,
			-1, -1
		},
		.Name = "Ghost Tremor Round"
	},
	// ROUND 10
	{
    .MinRound = 10,
    .RepeatEveryNRounds = 25,
    .RepeatCount = 0,
    .UnlimitedPostRoundTime = 0,
    .DisableDrops = 0,
		.MaxSpawnedAtOnce = MAX_MOBS_ALIVE_REAL,
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
    .MinRound = 15,
    .RepeatEveryNRounds = 25,
    .RepeatCount = 0,
    .UnlimitedPostRoundTime = 0,
    .DisableDrops = 0,
		.MaxSpawnedAtOnce = MAX_MOBS_ALIVE_REAL,
    .SpawnCountFactor = 1.0,
    .SpawnRateFactor = 1.0,
		.SpawnParamCount = 2,
		.SpawnParamIds = {
			MOB_SPAWN_PARAM_TREMOR,
			MOB_SPAWN_PARAM_FREEZE,
			-1
		},
		.Name = "Freeze Tremor Round"
	},
	// ROUND 20
	{
    .MinRound = 20,
    .RepeatEveryNRounds = 25,
    .RepeatCount = 0,
    .UnlimitedPostRoundTime = 0,
    .DisableDrops = 0,
		.MaxSpawnedAtOnce = MAX_MOBS_ALIVE_REAL,
    .SpawnCountFactor = 1.0,
    .SpawnRateFactor = 1.0,
		.SpawnParamCount = 3,
		.SpawnParamIds = {
			MOB_SPAWN_PARAM_GHOST,
			MOB_SPAWN_PARAM_EXPLOSION,
			MOB_SPAWN_PARAM_TREMOR,
			-1
		},
		.Name = "Evade Round"
	},
	// ROUND 25
	{
    .MinRound = 25,
    .RepeatEveryNRounds = 25,
    .RepeatCount = 0,
    .UnlimitedPostRoundTime = 0,
    .DisableDrops = 0,
		.MaxSpawnedAtOnce = MAX_MOBS_ALIVE_REAL,
    .SpawnCountFactor = 0.25,
    .SpawnRateFactor = 1.0,
		.SpawnParamCount = 1,
		.SpawnParamIds = {
			MOB_SPAWN_PARAM_TITAN,
      MOB_SPAWN_PARAM_NORMAL,
      MOB_SPAWN_PARAM_TREMOR,
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
		.Cost = EXECUTIONER_RENDER_COST,
    .MaxSpawnedAtOnce = 0,
    .MaxSpawnedPerRound = 0,
    .SpecialRoundOnly = 0,
		.MinRound = 15,
		.CooldownTicks = TPS * 1,
		.Probability = 0.025,
    .StatId = MOB_STAT_EXECUTIONER,
		.SpawnType = SPAWN_TYPE_DEFAULT_RANDOM,
		.Name = "Executioner",
		.Config = {
			.Xp = 500,
			.Bangles = EXECUTIONER_BANGLE_LEFT_CHEST_PLATE | EXECUTIONER_BANGLE_RIGHT_CHEST_PLATE
                | EXECUTIONER_BANGLE_LEFT_COLLAR_BONE | EXECUTIONER_BANGLE_RIGHT_COLLAR_BONE
                | EXECUTIONER_BANGLE_HELMET | EXECUTIONER_BANGLE_BRAIN,
			.Damage = MOB_BASE_DAMAGE * 1.5,
			.MaxDamage = 0,
      .DamageScale = 1.2,
			.Speed = MOB_BASE_SPEED * 0.85,
			.MaxSpeed = MOB_BASE_SPEED * 2.0,
      .SpeedScale = 0.5,
			.Health = MOB_BASE_HEALTH * 5.0,
			.MaxHealth = ORXON_MOB_MAX_HEALTH * 2.0,
      .HealthScale = 1.0,
			.Bolts = MOB_BASE_BOLTS * 3.0,
			.AttackRadius = EXECUTIONER_MELEE_ATTACK_RADIUS * 2.0,
			.HitRadius = EXECUTIONER_MELEE_HIT_RADIUS * 1.5,
      .CollRadius = EXECUTIONER_BASE_COLL_RADIUS * 4,
			.ReactionTickCount = EXECUTIONER_BASE_REACTION_TICKS * 0.35,
			.AttackCooldownTickCount = EXECUTIONER_BASE_ATTACK_COOLDOWN_TICKS * 1.5,
			.MobAttribute = 0,
		}
	},
	// ghost zombie
	[MOB_SPAWN_PARAM_GHOST]
	{
		.Cost = ZOMBIE_RENDER_COST,
    .MaxSpawnedAtOnce = 0,
    .MaxSpawnedPerRound = 0,
    .SpecialRoundOnly = 0,
		.MinRound = 5,
		.CooldownTicks = TPS * 1,
		.Probability = 0.05,
    .StatId = MOB_STAT_ZOMBIE_GHOST,
		.SpawnType = SPAWN_TYPE_DEFAULT_RANDOM | SPAWN_TYPE_SEMI_NEAR_PLAYER | SPAWN_TYPE_NEAR_HEALTHBOX,
		.Name = "Ghost",
		.Config = {
			.Xp = 25,
			.Bangles = ZOMBIE_BANGLE_HEAD_2 | ZOMBIE_BANGLE_TORSO_2,
			.Damage = MOB_BASE_DAMAGE * 2.0,
			.MaxDamage = 0,
      .DamageScale = 0.2,
			.Speed = MOB_BASE_SPEED * 1.0,
			.MaxSpeed = MOB_BASE_SPEED * 3.0,
      .SpeedScale = 0.5,
			.Health = MOB_BASE_HEALTH * 1.0,
			.MaxHealth = ORXON_MOB_MAX_HEALTH * 1.0,
      .HealthScale = 1.0,
			.Bolts = MOB_BASE_BOLTS * 1.5,
			.AttackRadius = ZOMBIE_MELEE_ATTACK_RADIUS,
			.HitRadius = ZOMBIE_MELEE_HIT_RADIUS,
      .CollRadius = ZOMBIE_BASE_COLL_RADIUS * 1.0,
			.ReactionTickCount = ZOMBIE_BASE_REACTION_TICKS,
			.AttackCooldownTickCount = ZOMBIE_BASE_ATTACK_COOLDOWN_TICKS,
			.MobAttribute = MOB_ATTRIBUTE_GHOST,
		}
	},
	// explode zombie
	[MOB_SPAWN_PARAM_EXPLOSION]
	{
		.Cost = ZOMBIE_RENDER_COST,
    .MaxSpawnedAtOnce = 0,
    .MaxSpawnedPerRound = 0,
    .SpecialRoundOnly = 0,
		.MinRound = 10,
		.CooldownTicks = TPS * 0.5,
		.Probability = 0.08,
    .StatId = MOB_STAT_ZOMBIE_EXPLODE,
		.SpawnType = SPAWN_TYPE_SEMI_NEAR_PLAYER | SPAWN_TYPE_NEAR_PLAYER | SPAWN_TYPE_ON_PLAYER,
		.Name = "Explosion",
		.Config = {
			.Xp = 60,
			.Bangles = ZOMBIE_BANGLE_HEAD_1 | ZOMBIE_BANGLE_TORSO_1,
			.Damage = MOB_BASE_DAMAGE * 2.0,
			.MaxDamage = 0,
      .DamageScale = 0.9,
			.Speed = MOB_BASE_SPEED * 1.0,
			.MaxSpeed = MOB_BASE_SPEED * 1.5,
      .SpeedScale = 0.5,
			.Health = MOB_BASE_HEALTH * 0.5,
			.MaxHealth = ORXON_MOB_MAX_HEALTH * 1.0,
      .HealthScale = 1.0,
			.Bolts = MOB_BASE_BOLTS * 1.6,
			.AttackRadius = ZOMBIE_MELEE_ATTACK_RADIUS,
			.HitRadius = ZOMBIE_MELEE_HIT_RADIUS,
      .CollRadius = ZOMBIE_BASE_COLL_RADIUS * 1.0,
			.ReactionTickCount = ZOMBIE_BASE_REACTION_TICKS,
			.AttackCooldownTickCount = ZOMBIE_BASE_ATTACK_COOLDOWN_TICKS,
			.MobAttribute = MOB_ATTRIBUTE_EXPLODE,
		}
	},
	// acid zombie
	[MOB_SPAWN_PARAM_ACID]
	{
		.Cost = ZOMBIE_RENDER_COST,
    .MaxSpawnedAtOnce = 0,
    .MaxSpawnedPerRound = 0,
    .SpecialRoundOnly = 0,
		.MinRound = 10,
		.Probability = 0.09,
		.CooldownTicks = TPS * 0.5,
    .StatId = MOB_STAT_ZOMBIE_ACID,
		.SpawnType = SPAWN_TYPE_SEMI_NEAR_PLAYER | SPAWN_TYPE_NEAR_PLAYER | SPAWN_TYPE_NEAR_HEALTHBOX,
		.Name = "Acid",
		.Config = {
			.Xp = 50,
			.Bangles = ZOMBIE_BANGLE_HEAD_4 | ZOMBIE_BANGLE_TORSO_4,
			.Damage = MOB_BASE_DAMAGE * 1.0,
			.MaxDamage = 0,
      .DamageScale = 1.0,
			.Speed = MOB_BASE_SPEED * 1.0,
			.MaxSpeed = MOB_BASE_SPEED * 1.8,
      .SpeedScale = 1.0,
			.Health = MOB_BASE_HEALTH * 1.0,
			.Bolts = MOB_BASE_BOLTS * 1.5,
			.MaxHealth = ORXON_MOB_MAX_HEALTH * 1.2,
      .HealthScale = 1.0,
			.AttackRadius = ZOMBIE_MELEE_ATTACK_RADIUS,
			.HitRadius = ZOMBIE_MELEE_HIT_RADIUS,
      .CollRadius = ZOMBIE_BASE_COLL_RADIUS * 1.0,
			.ReactionTickCount = ZOMBIE_BASE_REACTION_TICKS,
			.AttackCooldownTickCount = ZOMBIE_BASE_ATTACK_COOLDOWN_TICKS,
			.MobAttribute = MOB_ATTRIBUTE_ACID,
		}
	},
	// freeze zombie
	[MOB_SPAWN_PARAM_FREEZE]
	{
		.Cost = ZOMBIE_RENDER_COST,
    .MaxSpawnedAtOnce = 0,
    .MaxSpawnedPerRound = 0,
    .SpecialRoundOnly = 0,
		.MinRound = 10,
		.CooldownTicks = TPS * 0.5,
		.Probability = 0.1,
    .StatId = MOB_STAT_ZOMBIE_FREEZE,
		.SpawnType = SPAWN_TYPE_SEMI_NEAR_PLAYER | SPAWN_TYPE_NEAR_PLAYER,
		.Name = "Freeze",
		.Config = {
			.Xp = 50,
			.Bangles = ZOMBIE_BANGLE_HEAD_1 | ZOMBIE_BANGLE_TORSO_1,
			.Damage = MOB_BASE_DAMAGE * 1.0,
			.MaxDamage = 0,
      .DamageScale = 1.0,
			.Speed = MOB_BASE_SPEED * 0.8,
			.MaxSpeed = MOB_BASE_SPEED * 2.5,
      .SpeedScale = 0.5,
			.Health = MOB_BASE_HEALTH * 1.2,
			.MaxHealth = ORXON_MOB_MAX_HEALTH * 1.2,
      .HealthScale = 1.0,
			.Bolts = MOB_BASE_BOLTS * 1.5,
			.AttackRadius = ZOMBIE_MELEE_ATTACK_RADIUS,
			.HitRadius = ZOMBIE_MELEE_HIT_RADIUS,
      .CollRadius = ZOMBIE_BASE_COLL_RADIUS * 1.0,
			.ReactionTickCount = ZOMBIE_BASE_REACTION_TICKS,
			.AttackCooldownTickCount = ZOMBIE_BASE_ATTACK_COOLDOWN_TICKS,
			.MobAttribute = MOB_ATTRIBUTE_FREEZE,
		}
	},
	// runner zombie
	[MOB_SPAWN_PARAM_TREMOR]
	{
		.Cost = TREMOR_RENDER_COST,
    .MaxSpawnedAtOnce = 0,
    .MaxSpawnedPerRound = 0,
    .SpecialRoundOnly = 0,
		.MinRound = 5,
		.CooldownTicks = 0,
		.Probability = 0.1,
    .StatId = MOB_STAT_TREMOR,
		.SpawnType = SPAWN_TYPE_SEMI_NEAR_PLAYER | SPAWN_TYPE_NEAR_PLAYER,
		.Name = "Runner",
		.Config = {
			.Xp = 30,
			.Bangles = TREMOR_BANGLE_HEAD | TREMOR_BANGLE_CHEST | TREMOR_BANGLE_LEFT_ARM,
			.Damage = MOB_BASE_DAMAGE * 0.7,
			.MaxDamage = 0,
      .DamageScale = 0.3,
			.Speed = MOB_BASE_SPEED * 2.0,
			.MaxSpeed = MOB_BASE_SPEED * 3.0,
      .SpeedScale = 0.5,
			.Health = MOB_BASE_HEALTH * 0.6,
			.MaxHealth = ORXON_MOB_MAX_HEALTH * 0.66,
      .HealthScale = 1.0,
			.Bolts = MOB_BASE_BOLTS * 1.5,
			.AttackRadius = TREMOR_MELEE_ATTACK_RADIUS,
			.HitRadius = TREMOR_MELEE_HIT_RADIUS,
      .CollRadius = TREMOR_BASE_COLL_RADIUS * 1.0,
			.ReactionTickCount = TREMOR_BASE_REACTION_TICKS,
			.AttackCooldownTickCount = TREMOR_BASE_ATTACK_COOLDOWN_TICKS,
			.MobAttribute = 0,
		}
	},
	// normal zombie
	[MOB_SPAWN_PARAM_NORMAL]
	{
		.Cost = ZOMBIE_RENDER_COST,
    .MaxSpawnedAtOnce = 0,
    .MaxSpawnedPerRound = 0,
    .SpecialRoundOnly = 0,
		.MinRound = 0,
		.CooldownTicks = 0,
		.Probability = 0.5,
    .StatId = MOB_STAT_ZOMBIE,
		.SpawnType = SPAWN_TYPE_SEMI_NEAR_PLAYER | SPAWN_TYPE_NEAR_PLAYER,
		.Name = "Zombie",
		.Config = {
			.Xp = 15,
			.Bangles = ZOMBIE_BANGLE_HEAD_1 | ZOMBIE_BANGLE_TORSO_1,
			.Damage = MOB_BASE_DAMAGE * 1.0,
			.MaxDamage = 0,
      .DamageScale = 1.05,
			.Speed = MOB_BASE_SPEED * 1.0,
			.MaxSpeed = MOB_BASE_SPEED * 2.0,
      .SpeedScale = 0.5,
			.Health = MOB_BASE_HEALTH * 1.0,
			.MaxHealth = ORXON_MOB_MAX_HEALTH * 1.0,
      .HealthScale = 1.0,
			.Bolts = MOB_BASE_BOLTS * 1.0,
			.AttackRadius = ZOMBIE_MELEE_ATTACK_RADIUS,
			.HitRadius = ZOMBIE_MELEE_HIT_RADIUS,
      .CollRadius = ZOMBIE_BASE_COLL_RADIUS * 1.0,
			.ReactionTickCount = ZOMBIE_BASE_REACTION_TICKS,
			.AttackCooldownTickCount = ZOMBIE_BASE_ATTACK_COOLDOWN_TICKS,
			.MobAttribute = 0,
		}
	},
  // swarmer
  [MOB_SPAWN_PARAM_SWARMER]
	{
		.Cost = SWARMER_RENDER_COST,
    .MaxSpawnedAtOnce = 0,
    .MaxSpawnedPerRound = 0,
    .SpecialRoundOnly = 0,
		.MinRound = 0,
		.CooldownTicks = 0,
		.Probability = 1.0,
    .StatId = MOB_STAT_SWARMER,
		.SpawnType = SPAWN_TYPE_SEMI_NEAR_PLAYER | SPAWN_TYPE_NEAR_PLAYER,
		.Name = "Swarmer",
		.Config = {
			.Xp = 5,
			.Bangles = 0,
			.Damage = MOB_BASE_DAMAGE * 0.2,
			.MaxDamage = 0,
      .DamageScale = 1.0,
			.Speed = MOB_BASE_SPEED * 0.8,
			.MaxSpeed = MOB_BASE_SPEED * 2.0,
      .SpeedScale = 0.5,
			.Health = MOB_BASE_HEALTH * 0.35,
			.MaxHealth = ORXON_MOB_MAX_HEALTH * 0.5,
      .HealthScale = 1.0,
			.Bolts = MOB_BASE_BOLTS * 0.25,
			.AttackRadius = SWARMER_MELEE_ATTACK_RADIUS,
			.HitRadius = SWARMER_MELEE_HIT_RADIUS,
      .CollRadius = SWARMER_BASE_COLL_RADIUS * 1.0,
			.ReactionTickCount = SWARMER_BASE_REACTION_TICKS,
			.AttackCooldownTickCount = SWARMER_BASE_ATTACK_COOLDOWN_TICKS,
			.MobAttribute = 0,
		}
	},
};
const int defaultSpawnParamsCount = sizeof(defaultSpawnParams) / sizeof(struct MobSpawnParams);

//--------------------------------------------------------------------------
SurvivalBakedConfig_t bakedConfig = {
  .Difficulty = 1.5,
  .SpawnDistanceFactor = 0.5,
  .BoltRankMultiplier = 3,
  .BakedSpawnPoints = {
    { .Type = BAKED_SPAWNPOINT_PLAYER_START, .Params = 0, .Position = { 328.6, 544.8498, 433.9998 }, .Rotation = { 0, 0, 0 } },
    { .Type = BAKED_SPAWNPOINT_UPGRADE, .Params = 0, .Position = { 454.05, 566, 429.64 }, .Rotation = { 0, 0, -4.71239 } },
    { .Type = BAKED_SPAWNPOINT_UPGRADE, .Params = 0, .Position = { 478.72, 691.459, 430.73 }, .Rotation = { 0, 0, -3.141592 } },
    { .Type = BAKED_SPAWNPOINT_UPGRADE, .Params = 0, .Position = { 478.9, 508.54, 430.73 }, .Rotation = { 0, 0, 0 } },
    { .Type = BAKED_SPAWNPOINT_UPGRADE, .Params = 0, .Position = { 339.205, 562.895, 431.67 }, .Rotation = { -0.0006243868, 1.079918E-05, -1.660341 } },
    { .Type = BAKED_SPAWNPOINT_UPGRADE, .Params = 0, .Position = { 416.65, 574.96, 431.07 }, .Rotation = { 0, 0, -3.141594 } },
    { .Type = BAKED_SPAWNPOINT_UPGRADE, .Params = 0, .Position = { 335.47, 650.277, 430.623 }, .Rotation = { -6.167562, -7.500661E-09, -1.570796 } },
    { .Type = BAKED_SPAWNPOINT_UPGRADE, .Params = 0, .Position = { 470.02, 600.07, 437.062 }, .Rotation = { 0, 0, -4.712389 } },
    { .Type = BAKED_SPAWNPOINT_UPGRADE, .Params = 0, .Position = { 384.73, 536.032, 437.591 }, .Rotation = { -6.135677, 0, 0 } },
    { .Type = BAKED_SPAWNPOINT_UPGRADE, .Params = 0, .Position = { 426.446, 670.574, 438.23 }, .Rotation = { -0.1282649, 9.390364E-10, -2.937677 } },
    { .Type = BAKED_SPAWNPOINT_MYSTERY_BOX, .Params = 0, .Position = { 465.9, 580.1899, 434.0623 }, .Rotation = { 0, 0, -3.165905 } },
    { .Type = BAKED_SPAWNPOINT_MYSTERY_BOX, .Params = 0, .Position = { 463.8599, 535.0399, 433.9998 }, .Rotation = { 0, 0, -3.230638 } },
    { .Type = BAKED_SPAWNPOINT_MYSTERY_BOX, .Params = 0, .Position = { 464.79, 664.7899, 433.9998 }, .Rotation = { 0, 0, -2.890523 } },
    { .Type = BAKED_SPAWNPOINT_MYSTERY_BOX, .Params = 0, .Position = { 484.9999, 676.71, 427.9998 }, .Rotation = { 0, 0, -3.92699 } },
    { .Type = BAKED_SPAWNPOINT_MYSTERY_BOX, .Params = 0, .Position = { 384.45, 667.6899, 435.9998 }, .Rotation = { 0, 0, -1.37229 } },
    { .Type = BAKED_SPAWNPOINT_MYSTERY_BOX, .Params = 0, .Position = { 426.13, 534.9999, 435.9998 }, .Rotation = { 0, 0, 1.134573 } },
    { .Type = BAKED_SPAWNPOINT_MYSTERY_BOX, .Params = 0, .Position = { 345.3, 625.6999, 435.9999 }, .Rotation = { 0, 0, -0.4388694 } },
    { .Type = BAKED_SPAWNPOINT_MYSTERY_BOX, .Params = 0, .Position = { 422.4, 639.54, 428.3749 }, .Rotation = { 0, 0, -4.074778 } },
    { .Type = BAKED_SPAWNPOINT_MYSTERY_BOX, .Params = 0, .Position = { 445.31, 528.9199, 428.1744 }, .Rotation = { 0, 0, 0.7410109 } },
    { .Type = BAKED_SPAWNPOINT_DEMON_BELL, .Params = 0, .Position = { 324, 566.83, 440.94 }, .Rotation = { 0, 0, -1.570796 } },
    { .Type = BAKED_SPAWNPOINT_DEMON_BELL, .Params = 0, .Position = { 327, 566.83, 440.94 }, .Rotation = { 0, 0, -1.570796 } },
    { .Type = BAKED_SPAWNPOINT_DEMON_BELL, .Params = 0, .Position = { 330, 566.83, 440.94 }, .Rotation = { 0, 0, -1.570796 } },
    { .Type = BAKED_SPAWNPOINT_STACK_BOX, .Params = 0, .Position = { 409.4999, 607, 434.0144 }, .Rotation = { 0, 0, -1.570796 } },
    { .Type = BAKED_SPAWNPOINT_STACK_BOX, .Params = 0, .Position = { 409.4999, 614, 434.0144 }, .Rotation = { 0, 0, -1.570796 } },
    { .Type = BAKED_SPAWNPOINT_STACK_BOX, .Params = 0, .Position = { 409.4999, 621, 434.0144 }, .Rotation = { 0, 0, -1.570796 } }
  }
};

//--------------------------------------------------------------------------
u32 MobPrimaryColors[] = {
  [MOB_SPAWN_PARAM_SWARMER] 0x00464443,
	[MOB_SPAWN_PARAM_NORMAL] 	0x00464443,
	[MOB_SPAWN_PARAM_TREMOR] 	0x00464443,
	[MOB_SPAWN_PARAM_FREEZE] 	0x00804000,
	[MOB_SPAWN_PARAM_ACID] 		0x00464443,
	[MOB_SPAWN_PARAM_EXPLOSION] 0x00202080,
	[MOB_SPAWN_PARAM_GHOST] 	0x00464443,
	[MOB_SPAWN_PARAM_TITAN]		0x00464443,
};

u32 MobSecondaryColors[] = {
	[MOB_SPAWN_PARAM_SWARMER] 0x80202020,
	[MOB_SPAWN_PARAM_NORMAL] 	0x80202020,
	[MOB_SPAWN_PARAM_TREMOR] 	0x80202020,
	[MOB_SPAWN_PARAM_FREEZE] 	0x80FF2000,
	[MOB_SPAWN_PARAM_ACID] 		0x8000FF00,
	[MOB_SPAWN_PARAM_EXPLOSION] 0x000040C0,
	[MOB_SPAWN_PARAM_GHOST] 	0x80202020,
	[MOB_SPAWN_PARAM_TITAN]		0x80FF2020,
};

u32 MobLODColors[] = {
	[MOB_SPAWN_PARAM_SWARMER] 0x00808080,
	[MOB_SPAWN_PARAM_NORMAL] 	0x00808080,
	[MOB_SPAWN_PARAM_TREMOR] 	0x00808080,
	[MOB_SPAWN_PARAM_FREEZE] 	0x00F08000,
	[MOB_SPAWN_PARAM_ACID] 		0x0000F000,
	[MOB_SPAWN_PARAM_EXPLOSION] 0x004040F0,
	[MOB_SPAWN_PARAM_GHOST] 	0x00464443,
	[MOB_SPAWN_PARAM_TITAN]		0x00808080,
};

//--------------------------------------------------------------------------
struct MysteryBoxItemWeight MysteryBoxItemProbabilities[] = {
  { MYSTERY_BOX_ITEM_RESET_GATE, 0.03 },
  { MYSTERY_BOX_ITEM_QUAD, 0.0526 },
  { MYSTERY_BOX_ITEM_SHIELD, 0.0526 },
  { MYSTERY_BOX_ITEM_INVISIBILITY_CLOAK, 0.0526 },
  { MYSTERY_BOX_ITEM_RANDOMIZE_WEAPON_PICKUPS, 0.0526 },
  { MYSTERY_BOX_ITEM_EMP_HEALTH_GUN, 0.0555 },
  { MYSTERY_BOX_ITEM_REVIVE_TOTEM, 0.0555 },
  { MYSTERY_BOX_ITEM_INFINITE_AMMO, 0.0555 },
  //{ MYSTERY_BOX_ITEM_ACTIVATE_POWER, 0.0888 },
  { MYSTERY_BOX_ITEM_UPGRADE_WEAPON, 0.0967 },
  { MYSTERY_BOX_ITEM_TEDDY_BEAR, 0.1428 },
  { MYSTERY_BOX_ITEM_DREAD_TOKEN, 0.33333 },
  { MYSTERY_BOX_ITEM_WEAPON_MOD, 1.0 },
};
const int MysteryBoxItemProbabilitiesCount = sizeof(MysteryBoxItemProbabilities)/sizeof(struct MysteryBoxItemWeight);

struct MysteryBoxItemWeight MysteryBoxItemProbabilitiesLucky[] = {
  { MYSTERY_BOX_ITEM_RESET_GATE, 0.03 },
  { MYSTERY_BOX_ITEM_INVISIBILITY_CLOAK, 0.0526 },
  { MYSTERY_BOX_ITEM_RANDOMIZE_WEAPON_PICKUPS, 0.0526 },
  { MYSTERY_BOX_ITEM_EMP_HEALTH_GUN, 0.0555 },
  { MYSTERY_BOX_ITEM_REVIVE_TOTEM, 0.0555 },
  { MYSTERY_BOX_ITEM_INFINITE_AMMO, 0.0555 },
  //{ MYSTERY_BOX_ITEM_ACTIVATE_POWER, 0.0888 },
  { MYSTERY_BOX_ITEM_UPGRADE_WEAPON, 0.0967 },
  { MYSTERY_BOX_ITEM_TEDDY_BEAR, 0.1428 },
  { MYSTERY_BOX_ITEM_DREAD_TOKEN, 0.33333 },
  { MYSTERY_BOX_ITEM_WEAPON_MOD, 1.0 },
};
const int MysteryBoxItemProbabilitiesLuckyCount = sizeof(MysteryBoxItemProbabilitiesLucky)/sizeof(struct MysteryBoxItemWeight);

//--------------------------------------------------------------------------
int russianDollSpawnParamIdxs[] = {};
const int russianDollSpawnParamIdxsCount = COUNT_OF(russianDollSpawnParamIdxs);

void configInit(void)
{
  MapConfig.DefaultSpawnParams = defaultSpawnParams;
  MapConfig.DefaultSpawnParamsCount = defaultSpawnParamsCount;
  MapConfig.SpecialRoundParams = specialRoundParams;
  MapConfig.SpecialRoundParamsCount = specialRoundParamsCount;
}
