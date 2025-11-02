#include <libdl/utils.h>
#include "../../../include/game.h"
#include "../../../include/mob.h"
#include "../../../include/mysterybox.h"
#include "torval.h"

extern struct SurvivalMapConfig MapConfig;

#define TORVAL_MOB_MAX_HEALTH            (100000000)
#define TORVAL_MOB_BASE_BOLTS            (MOB_BASE_BOLTS * 1.5)
#define TORVAL_MOB_XP_MULT               (1.0)


//--------------------------------------------------------------------------
struct SurvivalSpecialRoundParam specialRoundParams[] = {
};
const int specialRoundParamsCount = sizeof(specialRoundParams) / sizeof(struct SurvivalSpecialRoundParam);

// NOTE
// These must be ordered from least probable to most probable
// SHOULD NEVER EXCEED MAX_MOB_SPAWN_PARAMS
struct MobSpawnParams defaultSpawnParams[] = {
  // reactor
  [MOB_SPAWN_PARAM_REACTOR]
	{
		.Cost = REACTOR_RENDER_COST,
    .MaxSpawnedAtOnce = 1,
    .MaxSpawnedPerRound = 0,
    .SpecialRoundOnly = 0,
		.MinRound = 16,
		.CooldownTicks = 0,
    .CooldownOffsetPerRoundFactor = 0,
		.Probability = 0.0005,
    .StatId = MOB_STAT_REACTOR,
		.SpawnType = SPAWN_TYPE_DEFAULT_RANDOM,
		.Name = "Reactor",
		.Config = {
			.Xp = 10000 * TORVAL_MOB_XP_MULT,
      .SharedXp = 1,
			.Bangles = REACTOR_BANGLE_SHOULDER_PLATES,
			.Damage = MOB_BASE_DAMAGE * 1.2,
			.MaxDamage = 0,
      .DamageScale = 1.1,
			.Speed = MOB_BASE_SPEED * 1.5,
			.MaxSpeed = MOB_BASE_SPEED * 3.0,
      .SpeedScale = 0.25,
			.Health = MOB_BASE_HEALTH * 10.0,
			.MaxHealth = TORVAL_MOB_MAX_HEALTH * 10.0,
      .HealthScale = 1.05,
			.Bolts = TORVAL_MOB_BASE_BOLTS * 50.0,
			.AttackRadius = REACTOR_MELEE_ATTACK_RADIUS * 1.0,
			.HitRadius = REACTOR_MELEE_HIT_RADIUS * 1.0,
      .CollRadius = REACTOR_BASE_COLL_RADIUS * 1.0,
			.ReactionTickCount = REACTOR_BASE_REACTION_TICKS * 0.35,
			.AttackCooldownTickCount = REACTOR_BASE_ATTACK_COOLDOWN_TICKS * 1.0,
			.MobAttribute = 0,
		}
	},
  // king leviathan
  [MOB_SPAWN_PARAM_KING_LEVIATHAN]
  {
		.Cost = LEVIATHAN_RENDER_COST,
    .MaxSpawnedAtOnce = 1,
    .MaxSpawnedPerRound = 0,
    .SpecialRoundOnly = 0,
		.MinRound = 21,
		.CooldownTicks = 0,
    .CooldownOffsetPerRoundFactor = 0,
		.Probability = 0.00050025,
    .StatId = MOB_STAT_LEVIATHAN,
		.SpawnType = SPAWN_TYPE_DEFAULT_RANDOM,
		.Name = "King Leviathan",
		.Config = {
			.Xp = 10000 * TORVAL_MOB_XP_MULT,
      .SharedXp = 1,
			.Bolts = TORVAL_MOB_BASE_BOLTS * 50.0,
			.Bangles = 0x1FFF,
			.Damage = MOB_BASE_DAMAGE * 1.0,
			.MaxDamage = 0,
      .DamageScale = 1.1,
			.Speed = MOB_BASE_SPEED * 1.5,
			.MaxSpeed = MOB_BASE_SPEED * 4.0,
      .SpeedScale = 0.2,
			.Health = MOB_BASE_HEALTH * 20.0,
			.MaxHealth = TORVAL_MOB_MAX_HEALTH * 10.0,
      .HealthScale = 1.1,
			.AttackRadius = LEVIATHAN_MELEE_ATTACK_RADIUS * 1.5,
			.HitRadius = LEVIATHAN_MELEE_HIT_RADIUS * 1.5,
      .CollRadius = LEVIATHAN_BASE_COLL_RADIUS * 2.0,
			.ReactionTickCount = LEVIATHAN_BASE_REACTION_TICKS * 1.0,
			.AttackCooldownTickCount = LEVIATHAN_BASE_ATTACK_COOLDOWN_TICKS * 1.0,
			.MobAttribute = MOB_ATTRIBUTE_BOSS,
		}
  },
  // executioner
	[MOB_SPAWN_PARAM_EXECUTIONER]
	{
		.Cost = EXECUTIONER2_RENDER_COST,
    .MaxSpawnedAtOnce = 4,
    .MaxSpawnedPerRound = 0,
    .SpecialRoundOnly = 0,
		.MinRound = 7,
		.CooldownTicks = TPS * 1,
    .CooldownOffsetPerRoundFactor = (1 / 25.0) * -(TPS * 0.25),
		.Probability = 0.01,
    .StatId = MOB_STAT_EXECUTIONER,
		.SpawnType = SPAWN_TYPE_DEFAULT_RANDOM,
		.Name = "Executioner",
		.Config = {
			.Xp = 250 * TORVAL_MOB_XP_MULT,
      .Bangles = EXECUTIONER2_BANGLE_LEFT_CHEST_PLATE | EXECUTIONER2_BANGLE_RIGHT_CHEST_PLATE
                | EXECUTIONER2_BANGLE_LEFT_COLLAR_BONE | EXECUTIONER2_BANGLE_RIGHT_COLLAR_BONE
                | EXECUTIONER2_BANGLE_HELMET | EXECUTIONER2_BANGLE_BRAIN,
      .Damage = MOB_BASE_DAMAGE * 1.0,
			.MaxDamage = 0,
      .DamageScale = 1.1,
			.Speed = MOB_BASE_SPEED * 0.85,
			.MaxSpeed = MOB_BASE_SPEED * 2.0,
      .SpeedScale = 0.5,
			.Health = MOB_BASE_HEALTH * 2.0,
			.MaxHealth = TORVAL_MOB_MAX_HEALTH * 2.0,
      .HealthScale = 1.0,
			.Bolts = TORVAL_MOB_BASE_BOLTS * 3.0,
			.AttackRadius = EXECUTIONER2_MELEE_ATTACK_RADIUS * 2.0,
			.HitRadius = EXECUTIONER2_MELEE_HIT_RADIUS * 1.5,
      .CollRadius = EXECUTIONER2_BASE_COLL_RADIUS * 4,
			.ReactionTickCount = EXECUTIONER2_BASE_REACTION_TICKS * 0.35,
			.AttackCooldownTickCount = EXECUTIONER2_BASE_ATTACK_COOLDOWN_TICKS * 1.5,
			.MobAttribute = MOB_ATTRIBUTE_RANGED_ATTACK,
		}
	},
  // leviathan
  [MOB_SPAWN_PARAM_LEVIATHAN]
  {
		.Cost = LEVIATHAN_RENDER_COST,
    .MaxSpawnedAtOnce = 10,
    .MaxSpawnedPerRound = 0,
    .SpecialRoundOnly = 0,
		.MinRound = 11,
		.CooldownTicks = TPS * 1,
    .CooldownOffsetPerRoundFactor = (1 / 25.0) * -(TPS * 0.25),
		.Probability = 0.025,
    .StatId = MOB_STAT_LEVIATHAN,
		.SpawnType = SPAWN_TYPE_DEFAULT_RANDOM,
		.Name = "Leviathan",
		.Config = {
			.Xp = 200 * TORVAL_MOB_XP_MULT,
			.Bangles = 0x1C1F,
			.Damage = MOB_BASE_DAMAGE * 1.0,
			.MaxDamage = 0,
      .DamageScale = 1.0,
			.Speed = MOB_BASE_SPEED * 1.5,
			.MaxSpeed = MOB_BASE_SPEED * 3.0,
      .SpeedScale = 0.1,
			.Health = MOB_BASE_HEALTH * 3.0,
			.MaxHealth = TORVAL_MOB_MAX_HEALTH * 1.0,
      .HealthScale = 1.1,
			.Bolts = TORVAL_MOB_BASE_BOLTS * 2.0,
			.AttackRadius = LEVIATHAN_MELEE_ATTACK_RADIUS * 1.0,
			.HitRadius = LEVIATHAN_MELEE_HIT_RADIUS * 1.0,
      .CollRadius = LEVIATHAN_BASE_COLL_RADIUS * 1,
			.ReactionTickCount = LEVIATHAN_BASE_REACTION_TICKS * 1.0,
			.AttackCooldownTickCount = LEVIATHAN_BASE_ATTACK_COOLDOWN_TICKS * 1.0,
			.MobAttribute = 0,
		}
  },
  // reaper
	[MOB_SPAWN_PARAM_REAPER]
	{
		.Cost = REAPER_RENDER_COST,
    .MaxSpawnedAtOnce = 10,
    .MaxSpawnedPerRound = 0,
    .SpecialRoundOnly = 0,
		.MinRound = 5,
		.CooldownTicks = 0,
    .CooldownOffsetPerRoundFactor = 0,
		.Probability = 0.04,
    .StatId = MOB_STAT_REAPER,
		.SpawnType = SPAWN_TYPE_SEMI_NEAR_PLAYER,
		.Name = "Reaper",
		.Config = {
			.Xp = 150 * TORVAL_MOB_XP_MULT,
			.Bangles = REAPER_BANGLE_SHOULDER_PAD_LEFT | REAPER_BANGLE_SHOULDER_PAD_RIGHT,
			.Damage = MOB_BASE_DAMAGE * 1.0,
			.MaxDamage = 0,
      .DamageScale = 1.2,
			.Speed = MOB_BASE_SPEED * 0.5,
			.MaxSpeed = MOB_BASE_SPEED * 1.0,
      .SpeedScale = 0.5,
			.Health = MOB_BASE_HEALTH * 3.0,
			.MaxHealth = TORVAL_MOB_MAX_HEALTH * 1.0,
      .HealthScale = 1.0,
			.Bolts = TORVAL_MOB_BASE_BOLTS * 2.0,
			.AttackRadius = REAPER_MELEE_ATTACK_RADIUS * 1.0,
			.HitRadius = REAPER_MELEE_HIT_RADIUS * 1.0,
      .CollRadius = REAPER_BASE_COLL_RADIUS * 1.0,
			.ReactionTickCount = REAPER_BASE_REACTION_TICKS * 1.0,
			.AttackCooldownTickCount = REAPER_BASE_ATTACK_COOLDOWN_TICKS * 1.0,
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
		.MinRound = 2,
		.CooldownTicks = 0,
    .CooldownOffsetPerRoundFactor = 0,
		.Probability = 0.5,
    .StatId = MOB_STAT_ZOMBIE,
		.SpawnType = SPAWN_TYPE_SEMI_NEAR_PLAYER | SPAWN_TYPE_NEAR_PLAYER,
		.Name = "Zombie",
		.Config = {
			.Xp = 20 * TORVAL_MOB_XP_MULT,
			.Bangles = ZOMBIE_BANGLE_HEAD_1 | ZOMBIE_BANGLE_TORSO_1,
			.Damage = MOB_BASE_DAMAGE * 1.0,
			.MaxDamage = 0,
      .DamageScale = 1.05,
			.Speed = MOB_BASE_SPEED * 1.0,
			.MaxSpeed = MOB_BASE_SPEED * 2.0,
      .SpeedScale = 0.5,
			.Health = MOB_BASE_HEALTH * 1.0,
			.MaxHealth = TORVAL_MOB_MAX_HEALTH * 1.0,
      .HealthScale = 1.0,
			.Bolts = TORVAL_MOB_BASE_BOLTS * 1.0,
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
    .CooldownOffsetPerRoundFactor = 0,
		.Probability = 1.0,
    .StatId = MOB_STAT_SWARMER,
		.SpawnType = SPAWN_TYPE_SEMI_NEAR_PLAYER | SPAWN_TYPE_NEAR_PLAYER,
		.Name = "Swarmer",
		.Config = {
			.Xp = 10 * TORVAL_MOB_XP_MULT,
			.Bangles = 0,
			.Damage = MOB_BASE_DAMAGE * 0.2,
			.MaxDamage = 0,
      .DamageScale = 1.0,
			.Speed = MOB_BASE_SPEED * 0.8,
			.MaxSpeed = MOB_BASE_SPEED * 2.0,
      .SpeedScale = 0.5,
			.Health = MOB_BASE_HEALTH * 0.35,
			.MaxHealth = TORVAL_MOB_MAX_HEALTH * 0.5,
      .HealthScale = 1.0,
			.Bolts = TORVAL_MOB_BASE_BOLTS * 0.5,
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
  .Difficulty = 2.0,
  .BoltMultiplier = 1.0,
  .XpMultiplier = 1.0,
  .SpawnDistanceFactor = 0.5,
  .BoltRankMultiplier = 1,
  .StackboxBaseCost = 250000,
  .StackboxCostPerPerk = 250000,
  .BakedSpawnPoints = {

    { .Type = BAKED_SPAWNPOINT_PLAYER_START, .Params = 0, .Position = { 384.9, 321.76, 101.0166 }, .Rotation = { 0, 0, -3.140336 } },
    { .Type = BAKED_SPAWNPOINT_DEMON_BELL, .Params = 0, .Position = { 385.0935, 338.5183, 105.84 }, .Rotation = { 0, 0, -5.252155 } },
    { .Type = BAKED_SPAWNPOINT_DEMON_BELL, .Params = 0, .Position = { 382.52, 340.06, 105.84 }, .Rotation = { 0, 0, -5.252155 } },
    { .Type = BAKED_SPAWNPOINT_DEMON_BELL, .Params = 0, .Position = { 379.9465, 341.6017, 105.84 }, .Rotation = { 0, 0, -5.252155 } },
    { .Type = BAKED_SPAWNPOINT_STACK_BOX, .Params = 0, .Position = { 383.44, 425.85, 100.7485 }, .Rotation = { 0, 0, 0 } },
    { .Type = BAKED_SPAWNPOINT_STACK_BOX, .Params = 0, .Position = { 381.29, 414.58, 100.7639 }, .Rotation = { 0, 0, -0.7853981 } },
    { .Type = BAKED_SPAWNPOINT_STACK_BOX, .Params = 0, .Position = { 375.21, 402.15, 100.7147 }, .Rotation = { 0, 0, -1.570796 } },
    { .Type = BAKED_SPAWNPOINT_STACK_BOX, .Params = 0, .Position = { 373.48, 388.95, 100.6285 }, .Rotation = { 0, 0, -2.356194 } },
    { .Type = BAKED_SPAWNPOINT_STACK_BOX, .Params = 0, .Position = { 374.06, 344.28, 100.6097 }, .Rotation = { 0, 0, -3.141593 } },
    { .Type = BAKED_SPAWNPOINT_STACK_BOX, .Params = 0, .Position = { 386.41, 337.46, 100.5199 }, .Rotation = { 0, 0, -3.92699 } },
    { .Type = BAKED_SPAWNPOINT_STACK_BOX, .Params = 0, .Position = { 394.67, 328.74, 100.6955 }, .Rotation = { 0, 0, -4.712389 } },
    { .Type = BAKED_SPAWNPOINT_STACK_BOX, .Params = 0, .Position = { 404.41, 317.41, 100.6955 }, .Rotation = { 0, 0, -5.497787 } },
    { .Type = BAKED_SPAWNPOINT_MYSTERY_BOX, .Params = 0, .Position = { 377.8201, 429.5302, 100.6408 }, .Rotation = { 0, 0, -2.19704 } },
    { .Type = BAKED_SPAWNPOINT_MYSTERY_BOX, .Params = 0, .Position = { 303.9201, 344.9801, 107.18 }, .Rotation = { 0, 0, -4.606381 } },
    { .Type = BAKED_SPAWNPOINT_UPGRADE, .Params = 0, .Position = { 356.7, 312, 102.14 }, .Rotation = { 0, 0, 0 } },
    { .Type = BAKED_SPAWNPOINT_UPGRADE, .Params = 0, .Position = { 365.6, 363.09, 102.21 }, .Rotation = { -5.906532, -0.03035478, -4.555718 } },
    { .Type = BAKED_SPAWNPOINT_UPGRADE, .Params = 0, .Position = { 367.9, 435.65, 102.82 }, .Rotation = { 0, 0, -3.141593 } },
    { .Type = BAKED_SPAWNPOINT_UPGRADE, .Params = 0, .Position = { 322.527, 404.8, 101.861 }, .Rotation = { -5.96934, -6.280801, -1.650588 } },
    { .Type = BAKED_SPAWNPOINT_UPGRADE, .Params = 0, .Position = { 328.679, 370.721, 101.857 }, .Rotation = { -0.01053229, 4.656871E-10, -1.570796 } },
    { .Type = BAKED_SPAWNPOINT_UPGRADE, .Params = 0, .Position = { 294.33, 343.81, 110.45 }, .Rotation = { -5.798351, 0, 0 } },
    { .Type = BAKED_SPAWNPOINT_UPGRADE, .Params = 0, .Position = { 299.76, 423.91, 107.96 }, .Rotation = { 0, 0, -3.141593 } }
  }
};

//--------------------------------------------------------------------------
u32 MobPrimaryColors[] = {
  [MOB_SPAWN_PARAM_SWARMER] 0x00464443,
	[MOB_SPAWN_PARAM_NORMAL] 	0x00464443,
	[MOB_SPAWN_PARAM_EXECUTIONER] 	0x00464443,
	[MOB_SPAWN_PARAM_REAPER]  0x00464443,
	[MOB_SPAWN_PARAM_LEVIATHAN]	0x00464443,
	[MOB_SPAWN_PARAM_REACTOR]	0x00464443,
	[MOB_SPAWN_PARAM_KING_LEVIATHAN]	0x00464443,
};

u32 MobSecondaryColors[] = {
	[MOB_SPAWN_PARAM_SWARMER] 0x80808080,
	[MOB_SPAWN_PARAM_NORMAL] 	0x80202020,
	[MOB_SPAWN_PARAM_EXECUTIONER] 	0x80202020,
	[MOB_SPAWN_PARAM_REAPER]	0x80FF2020,
	[MOB_SPAWN_PARAM_LEVIATHAN]	0x80FF2020,
	[MOB_SPAWN_PARAM_REACTOR]	0x8020C020,
	[MOB_SPAWN_PARAM_KING_LEVIATHAN]	0x80FF2020,
};

u32 MobLODColors[] = {
	[MOB_SPAWN_PARAM_SWARMER] 0x00808080,
	[MOB_SPAWN_PARAM_NORMAL] 	0x00808080,
	[MOB_SPAWN_PARAM_EXECUTIONER] 	0x000000FF,
	[MOB_SPAWN_PARAM_REAPER]	0x00202020,
	[MOB_SPAWN_PARAM_LEVIATHAN]	0x0080FF80,
	[MOB_SPAWN_PARAM_REACTOR]	0x00FF0000,
	[MOB_SPAWN_PARAM_KING_LEVIATHAN]	0x0000FF00,
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
int reactorMinionSpawnParamIdx = MOB_SPAWN_PARAM_NORMAL;

void configInit(void)
{
  MapConfig.DefaultSpawnParams = defaultSpawnParams;
  MapConfig.DefaultSpawnParamsCount = defaultSpawnParamsCount;
  MapConfig.SpecialRoundParams = specialRoundParams;
  MapConfig.SpecialRoundParamsCount = specialRoundParamsCount;
}
