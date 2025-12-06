#include <libdl/utils.h>
#include "../../../include/game.h"
#include "../../../include/mob.h"
#include "../../../include/mysterybox.h"
#include "valix.h"

extern struct SurvivalMapConfig MapConfig;

#define VALIX_MOB_MAX_HEALTH            (100000000)


//--------------------------------------------------------------------------
struct SurvivalSpecialRoundParam specialRoundParams[] = {
	// ROUND 25
	{
    .MinRound = 25,
    .RepeatEveryNRounds = 25,
    .RepeatCount = 0,
    .UnlimitedPostRoundTime = 1,
    .DisableDrops = 1,
		.MaxSpawnedAtOnce = 25,
    .SpawnCountFactor = 1.0,
    .SpawnRateFactor = 1.0,
		.SpawnParamCount = 4,
		.SpawnParamIds = {
			MOB_SPAWN_PARAM_LEVIATHAN,
			MOB_SPAWN_PARAM_LEVIATHAN_COMMON,
			MOB_SPAWN_PARAM_NORMAL,
			MOB_SPAWN_PARAM_SWARMER,
		},
		.Name = "Boss Round"
	},
};
const int specialRoundParamsCount = sizeof(specialRoundParams) / sizeof(struct SurvivalSpecialRoundParam);

// NOTE
// These must be ordered from least probable to most probable
// SHOULD NEVER EXCEED MAX_MOB_SPAWN_PARAMS
struct MobSpawnParams defaultSpawnParams[] = {
  // king leviathan
  [MOB_SPAWN_PARAM_KING_LEVIATHAN]
  {
		.RenderCost = LEVIATHAN_RENDER_COST,
    .MaxSpawnedAtOnce = 1,
    .MaxSpawnedPerRound = 1,
    .SpecialRoundOnly = 1,
		.MinRound = 0,
		.CooldownTicks = 0,
    .CooldownOffsetPerRoundFactor = 0,
		.Probability = 1,
    .StatId = MOB_STAT_LEVIATHAN,
		.SpawnType = SPAWN_TYPE_DEFAULT_RANDOM,
		.Name = "King Leviathan",
		.Config = {
			.Xp = 10000,
      .SharedXp = 1,
			.Bolts = 500000,
			.Bangles = 0x1FFF,
			.Damage = MOB_BASE_DAMAGE * 1.0,
			.MaxDamage = 0,
      .DamageScale = 1.1,
			.Speed = MOB_BASE_SPEED * 1.5,
			.MaxSpeed = MOB_BASE_SPEED * 4.0,
      .SpeedScale = 0.2,
			.Health = MOB_BASE_HEALTH * 50.0,
			.MaxHealth = VALIX_MOB_MAX_HEALTH * 10.0,
      .HealthScale = 1.1,
			.AttackRadius = LEVIATHAN_MELEE_ATTACK_RADIUS * 1.5,
			.HitRadius = LEVIATHAN_MELEE_HIT_RADIUS * 1.5,
      .CollRadius = LEVIATHAN_BASE_COLL_RADIUS * 2.0,
			.ReactionTickCount = LEVIATHAN_BASE_REACTION_TICKS * 1.0,
			.AttackCooldownTickCount = LEVIATHAN_BASE_ATTACK_COOLDOWN_TICKS * 1.0,
			.MobAttribute = MOB_ATTRIBUTE_BOSS,
		}
  },
  // leviathan common
  [MOB_SPAWN_PARAM_LEVIATHAN_COMMON]
  {
		.RenderCost = LEVIATHAN_RENDER_COST,
    .MaxSpawnedAtOnce = 15,
    .MaxSpawnedPerRound = 0,
    .SpecialRoundOnly = 1,
		.MinRound = 25,
		.CooldownTicks = TPS * 0.5,
    .CooldownOffsetPerRoundFactor = (1 / 25.0) * -(TPS * 0.25),
		.Probability = 0.25,
    .StatId = MOB_STAT_LEVIATHAN,
		.SpawnType = SPAWN_TYPE_DEFAULT_RANDOM,
		.Name = "Leviathan",
		.Config = {
			.Xp = 200,
			.Bangles = 0x1C1F,
			.Damage = MOB_BASE_DAMAGE * 1.0,
			.MaxDamage = 0,
      .DamageScale = 1.0,
			.Speed = MOB_BASE_SPEED * 1.5,
			.MaxSpeed = MOB_BASE_SPEED * 3.0,
      .SpeedScale = 0.1,
			.Health = MOB_BASE_HEALTH * 3.0,
			.MaxHealth = VALIX_MOB_MAX_HEALTH * 1.0,
      .HealthScale = 1.1,
			.Bolts = MOB_BASE_BOLTS * 2.0,
			.AttackRadius = LEVIATHAN_MELEE_ATTACK_RADIUS * 1.0,
			.HitRadius = LEVIATHAN_MELEE_HIT_RADIUS * 1.0,
      .CollRadius = LEVIATHAN_BASE_COLL_RADIUS * 1,
			.ReactionTickCount = LEVIATHAN_BASE_REACTION_TICKS * 1.0,
			.AttackCooldownTickCount = LEVIATHAN_BASE_ATTACK_COOLDOWN_TICKS * 1.0,
			.MobAttribute = MOB_ATTRIBUTE_RANGED_ATTACK,
		}
  },
  // leviathan
  [MOB_SPAWN_PARAM_LEVIATHAN]
  {
		.RenderCost = LEVIATHAN_RENDER_COST,
    .MaxSpawnedAtOnce = 15,
    .MaxSpawnedPerRound = 0,
    .SpecialRoundOnly = 0,
		.MinRound = 25,
		.CooldownTicks = TPS * 2,
    .CooldownOffsetPerRoundFactor = (1 / 25.0) * -(TPS * 0.25),
		.Probability = 0.025,
    .StatId = MOB_STAT_LEVIATHAN,
		.SpawnType = SPAWN_TYPE_DEFAULT_RANDOM,
		.Name = "Leviathan",
		.Config = {
			.Xp = 200,
			.Bangles = 0x1C1F,
			.Damage = MOB_BASE_DAMAGE * 1.0,
			.MaxDamage = 0,
      .DamageScale = 1.0,
			.Speed = MOB_BASE_SPEED * 1.5,
			.MaxSpeed = MOB_BASE_SPEED * 3.0,
      .SpeedScale = 0.1,
			.Health = MOB_BASE_HEALTH * 3.0,
			.MaxHealth = VALIX_MOB_MAX_HEALTH * 1.0,
      .HealthScale = 1.1,
			.Bolts = MOB_BASE_BOLTS * 2.0,
			.AttackRadius = LEVIATHAN_MELEE_ATTACK_RADIUS * 1.0,
			.HitRadius = LEVIATHAN_MELEE_HIT_RADIUS * 1.0,
      .CollRadius = LEVIATHAN_BASE_COLL_RADIUS * 1,
			.ReactionTickCount = LEVIATHAN_BASE_REACTION_TICKS * 1.0,
			.AttackCooldownTickCount = LEVIATHAN_BASE_ATTACK_COOLDOWN_TICKS * 1.0,
			.MobAttribute = MOB_ATTRIBUTE_RANGED_ATTACK,
		}
  },
	// reaper
	[MOB_SPAWN_PARAM_REAPER]
	{
		.RenderCost = REAPER_RENDER_COST,
    .MaxSpawnedAtOnce = 10,
    .MaxSpawnedPerRound = 0,
    .SpecialRoundOnly = 0,
		.MinRound = 18,
		.CooldownTicks = 0,
    .CooldownOffsetPerRoundFactor = 0,
		.Probability = 0.03,
    .StatId = MOB_STAT_REAPER,
		.SpawnType = SPAWN_TYPE_SEMI_NEAR_PLAYER,
		.Name = "Reaper",
		.Config = {
			.Xp = 150,
			.Bangles = REAPER_BANGLE_SHOULDER_PAD_LEFT | REAPER_BANGLE_SHOULDER_PAD_RIGHT,
			.Damage = MOB_BASE_DAMAGE * 1.0,
			.MaxDamage = 0,
      .DamageScale = 1.2,
			.Speed = MOB_BASE_SPEED * 0.5,
			.MaxSpeed = MOB_BASE_SPEED * 1.0,
      .SpeedScale = 0.5,
			.Health = MOB_BASE_HEALTH * 3.0,
			.MaxHealth = VALIX_MOB_MAX_HEALTH * 1.0,
      .HealthScale = 1.0,
			.Bolts = MOB_BASE_BOLTS * 2.0,
			.AttackRadius = REAPER_MELEE_ATTACK_RADIUS * 1.0,
			.HitRadius = REAPER_MELEE_HIT_RADIUS * 1.0,
      .CollRadius = REAPER_BASE_COLL_RADIUS * 1.0,
			.ReactionTickCount = REAPER_BASE_REACTION_TICKS * 1.0,
			.AttackCooldownTickCount = REAPER_BASE_ATTACK_COOLDOWN_TICKS * 1.0,
			.MobAttribute = 0,
		}
	},
	// runner zombie
	[MOB_SPAWN_PARAM_TREMOR]
	{
		.RenderCost = TREMOR_RENDER_COST,
    .MaxSpawnedAtOnce = 0,
    .MaxSpawnedPerRound = 0,
    .SpecialRoundOnly = 0,
		.MinRound = 8,
		.CooldownTicks = 0,
    .CooldownOffsetPerRoundFactor = 0,
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
			.MaxHealth = VALIX_MOB_MAX_HEALTH * 0.66,
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
		.RenderCost = ZOMBIE_RENDER_COST,
    .MaxSpawnedAtOnce = 0,
    .MaxSpawnedPerRound = 0,
    .SpecialRoundOnly = 0,
		.MinRound = 2,
		.CooldownTicks = 0,
    .CooldownOffsetPerRoundFactor = 0,
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
			.MaxHealth = VALIX_MOB_MAX_HEALTH * 1.0,
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
		.RenderCost = SWARMER_RENDER_COST,
    .MaxSpawnedAtOnce = 0,
    .MaxSpawnedPerRound = 0,
    .SpecialRoundOnly = 0,
		.MinRound = 0,
		.CooldownTicks = 0,
    .CooldownOffsetPerRoundFactor = 0,
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
			.MaxHealth = VALIX_MOB_MAX_HEALTH * 0.5,
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
  .Difficulty = 1.25,
  .BoltMultiplier = 1.0,
  .XpMultiplier = 1.0,
  .SpawnDistanceFactor = 0.3,
  .BoltRankMultiplier = 1,
  .StackboxBaseCost = 250000,
  .StackboxCostPerPerk = 250000,
  .BakedSpawnPoints = {

    { .Type = BAKED_SPAWNPOINT_PLAYER_START, .Params = 0, .Position = { 402.1, 419.5, 325.46 }, .Rotation = { 0, 0, -2.617994 } },
    { .Type = BAKED_SPAWNPOINT_UPGRADE, .Params = 0, .Position = { 338.24, 443.66, 331.51 }, .Rotation = { 0, 0, -2.617993 } },
    { .Type = BAKED_SPAWNPOINT_UPGRADE, .Params = 0, .Position = { 318.877, 578.931, 330.625 }, .Rotation = { 0, 0, -3.67426 } },
    { .Type = BAKED_SPAWNPOINT_UPGRADE, .Params = 0, .Position = { 504.07, 700.06, 322.85 }, .Rotation = { -0.06846029, -0.003104056, -2.243457 } },
    { .Type = BAKED_SPAWNPOINT_UPGRADE, .Params = 0, .Position = { 709.951, 656.019, 325.151 }, .Rotation = { -5.969765, -3.13285E-08, -5.240595 } },
    { .Type = BAKED_SPAWNPOINT_UPGRADE, .Params = 0, .Position = { 687.38, 442.79, 316.92 }, .Rotation = { 0, 0, -0.3425906 } },
    { .Type = BAKED_SPAWNPOINT_UPGRADE, .Params = 0, .Position = { 675.23, 383.8, 316.472 }, .Rotation = { 0, 0, -0.393218 } },
    { .Type = BAKED_SPAWNPOINT_UPGRADE, .Params = 0, .Position = { 652.51, 278.36, 341.75 }, .Rotation = { -5.855405, 1.63769E-08, -4.083116 } },
    { .Type = BAKED_SPAWNPOINT_UPGRADE, .Params = 0, .Position = { 641.84, 150.9, 341.28 }, .Rotation = { 0, 0, -4.782935 } },
    { .Type = BAKED_SPAWNPOINT_UPGRADE, .Params = 0, .Position = { 591.3, 422.13, 354.5 }, .Rotation = { -6.274548, -3.725429E-09, -4.630847 } },
    { .Type = BAKED_SPAWNPOINT_UPGRADE, .Params = 0, .Position = { 490.49, 554.49, 304.65 }, .Rotation = { -6.275942, -0.004705583, -4.054676 } },
    { .Type = BAKED_SPAWNPOINT_DEMON_BELL, .Params = 0, .Position = { 381.25, 422.6, 331.24 }, .Rotation = { 0, 0, -4.233414 } },
    { .Type = BAKED_SPAWNPOINT_DEMON_BELL, .Params = 0, .Position = { 378.5874, 421.2173, 331.24 }, .Rotation = { 0, 0, -4.233414 } },
    { .Type = BAKED_SPAWNPOINT_DEMON_BELL, .Params = 0, .Position = { 375.9248, 419.8346, 331.24 }, .Rotation = { 0, 0, -4.233414 } },
    { .Type = BAKED_SPAWNPOINT_STACK_BOX, .Params = 0, .Position = { 511.5981, 526.1113, 372.8475 }, .Rotation = { 0, 0, -0.8545017 } },
    { .Type = BAKED_SPAWNPOINT_STACK_BOX, .Params = 0, .Position = { 524.3442, 544.6609, 372.8475 }, .Rotation = { 0, 0, -3.510569 } },
    { .Type = BAKED_SPAWNPOINT_STACK_BOX, .Params = 0, .Position = { 516.9033, 545.4879, 372.8475 }, .Rotation = { 0, 0, -2.885621 } },
    { .Type = BAKED_SPAWNPOINT_STACK_BOX, .Params = 0, .Position = { 510.8462, 540.9991, 372.8475 }, .Rotation = { 0, 0, -2.227496 } },
    { .Type = BAKED_SPAWNPOINT_STACK_BOX, .Params = 0, .Position = { 508.58, 533.47, 372.8475 }, .Rotation = { 0, 0, -1.555972 } },
    { .Type = BAKED_SPAWNPOINT_MYSTERY_BOX, .Params = 0, .Position = { 289.3901, 476.7001, 331.4836 }, .Rotation = { 0, 0, 0.07918823 } },
    { .Type = BAKED_SPAWNPOINT_MYSTERY_BOX, .Params = 0, .Position = { 513.2501, 564.1602, 302.0029 }, .Rotation = { 0, 0, -4.370725 } },
    { .Type = BAKED_SPAWNPOINT_MYSTERY_BOX, .Params = 0, .Position = { 523.9602, 694.0902, 318.9662 }, .Rotation = { 0, 0, -2.405586 } },
    { .Type = BAKED_SPAWNPOINT_MYSTERY_BOX, .Params = 0, .Position = { 608.6002, 660.6802, 319.5969 }, .Rotation = { 0, 0, -3.352686 } },
    { .Type = BAKED_SPAWNPOINT_MYSTERY_BOX, .Params = 0, .Position = { 736.7205, 655.2805, 323.4836 }, .Rotation = { 0, 0, -1.727037 } },
    { .Type = BAKED_SPAWNPOINT_MYSTERY_BOX, .Params = 0, .Position = { 684.3101, 456.0502, 314.8038 }, .Rotation = { 0, 0, -1.216549 } },
    { .Type = BAKED_SPAWNPOINT_MYSTERY_BOX, .Params = 0, .Position = { 531.5202, 532.9302, 372.2292 }, .Rotation = { 0, 0, -3.21687 } },
    { .Type = BAKED_SPAWNPOINT_MYSTERY_BOX, .Params = 0, .Position = { 587.3002, 416.8101, 350.0135 }, .Rotation = { 0, 0, -3.049145 } },
    { .Type = BAKED_SPAWNPOINT_MYSTERY_BOX, .Params = 0, .Position = { 626.5102, 176.14, 338.7231 }, .Rotation = { 0, 0, -3.049145 } }
    
  }
};

//--------------------------------------------------------------------------
int StackboxItems[] = {
  STACKABLE_ITEM_LOW_HEALTH_DMG_BUF,
  STACKABLE_ITEM_EXTRA_JUMP,
  STACKABLE_ITEM_EXTRA_SHOT,
  STACKABLE_ITEM_HOVERBOOTS,
  STACKABLE_ITEM_ALPHA_MOD_SPEED,
  STACKABLE_ITEM_ALPHA_MOD_IMPACT,
  STACKABLE_ITEM_ALPHA_MOD_AREA,
  STACKABLE_ITEM_ALPHA_MOD_AMMO,
  STACKABLE_ITEM_VAMPIRE,
  STACKABLE_ITEM_EXPLODING_ENEMIES,
};
const int StackboxItemsCount = sizeof(StackboxItems)/sizeof(int);

//--------------------------------------------------------------------------
struct MysteryBoxItemWeight MysteryBoxItemProbabilities[] = {
  // { MYSTERY_BOX_ITEM_RESET_GATE, 0.03 },
  { MYSTERY_BOX_ITEM_TEDDY_BEAR, 0.0526 },
  { MYSTERY_BOX_ITEM_QUAD, 0.0526 },
  { MYSTERY_BOX_ITEM_SHIELD, 0.0526 },
  { MYSTERY_BOX_ITEM_INVISIBILITY_CLOAK, 0.0526 },
  { MYSTERY_BOX_ITEM_RANDOMIZE_WEAPON_PICKUPS, 0.0526 },
  { MYSTERY_BOX_ITEM_EMP_HEALTH_GUN, 0.0555 },
  { MYSTERY_BOX_ITEM_REVIVE_TOTEM, 0.0555 },
  { MYSTERY_BOX_ITEM_INFINITE_AMMO, 0.0555 },
  //{ MYSTERY_BOX_ITEM_ACTIVATE_POWER, 0.0888 },
  { MYSTERY_BOX_ITEM_UPGRADE_WEAPON, 0.0967 },
  { MYSTERY_BOX_ITEM_DREAD_TOKEN, 0.33333 },
  { MYSTERY_BOX_ITEM_WEAPON_MOD, 1.0 },
};
const int MysteryBoxItemProbabilitiesCount = sizeof(MysteryBoxItemProbabilities)/sizeof(struct MysteryBoxItemWeight);

struct MysteryBoxItemWeight MysteryBoxItemProbabilitiesLucky[] = {
  // { MYSTERY_BOX_ITEM_RESET_GATE, 0.03 },
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
