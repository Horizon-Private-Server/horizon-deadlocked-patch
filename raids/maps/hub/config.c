#include <libdl/utils.h>
#include "../../../include/game.h"
#include "../../../include/mob.h"
#include "../../../include/mysterybox.h"
#include "veldin.h"

extern struct SurvivalMapConfig MapConfig;


//--------------------------------------------------------------------------
struct SurvivalSpecialRoundParam specialRoundParams[] = {};
const int specialRoundParamsCount = sizeof(specialRoundParams) / sizeof(struct SurvivalSpecialRoundParam);

// NOTE
// These must be ordered from least probable to most probable
// SHOULD NEVER EXCEED MAX_MOB_SPAWN_PARAMS
struct MobSpawnParams defaultSpawnParams[] = {
	// reaper
  [MOB_SPAWN_PARAM_REAPER]
	{
		.Cost = REAPER_RENDER_COST,
    .MaxSpawnedAtOnce = 20,
    .MaxSpawnedPerRound = 0,
    .SpecialRoundOnly = 0,
		.MinRound = 15,
		.CooldownTicks = 0,
    .CooldownOffsetPerRoundFactor = 0,
		.Probability = 0.05,
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
			.MaxHealth = 0,
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
		.Cost = TREMOR_RENDER_COST,
    .MaxSpawnedAtOnce = 0,
    .MaxSpawnedPerRound = 0,
    .SpecialRoundOnly = 0,
		.MinRound = 0,
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
			.MaxHealth = 0,
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
    .CooldownOffsetPerRoundFactor = 0,
		.Probability = 1.0,
    .StatId = MOB_STAT_ZOMBIE,
		.SpawnType = SPAWN_TYPE_SEMI_NEAR_PLAYER | SPAWN_TYPE_NEAR_PLAYER,
		.Name = "Zombie",
		.Config = {
			.Xp = 15,
			.Bangles = ZOMBIE_BANGLE_HEAD_1 | ZOMBIE_BANGLE_TORSO_1,
			.Damage = MOB_BASE_DAMAGE * 1.0,
			.MaxDamage = 0,
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
			.MobAttribute = 0,
		}
	},
};
const int defaultSpawnParamsCount = sizeof(defaultSpawnParams) / sizeof(struct MobSpawnParams);

//--------------------------------------------------------------------------
SurvivalBakedConfig_t bakedConfig = {
  .Difficulty = 1.0,
  .SpawnDistanceFactor = 1.0,
  .BoltRankMultiplier = 1,
  .BakedSpawnPoints = {
    { .Type = BAKED_SPAWNPOINT_PLAYER_START, .Params = 0, .Position = { 188.15, 450.96, 85.93756 }, .Rotation = { 0, 0, -1.570796 } },
    { .Type = BAKED_SPAWNPOINT_UPGRADE, .Params = 0, .Position = { 192.334, 374.214, 83.09 }, .Rotation = { 0, 0, -3.025695 } },
    { .Type = BAKED_SPAWNPOINT_UPGRADE, .Params = 0, .Position = { 193.86, 359.946, 83.09 }, .Rotation = { 0, 0, -6.143251 } },
    { .Type = BAKED_SPAWNPOINT_UPGRADE, .Params = 0, .Position = { 166.62, 455.08, 88.27 }, .Rotation = { 0, 0, -2.430318 } },
    { .Type = BAKED_SPAWNPOINT_UPGRADE, .Params = 0, .Position = { 174.508, 443.266, 88.27 }, .Rotation = { 0, 0, -5.878619 } },
    { .Type = BAKED_SPAWNPOINT_UPGRADE, .Params = 0, .Position = { 214.57, 416.072, 81.993 }, .Rotation = { -6.155164, 0, -3.141593 } },
    { .Type = BAKED_SPAWNPOINT_UPGRADE, .Params = 0, .Position = { 186.34, 398.56, 81.36 }, .Rotation = { -5.629477, -6.281482, -1.320548 } },
    { .Type = BAKED_SPAWNPOINT_UPGRADE, .Params = 0, .Position = { 186.522, 438.843, 82.383 }, .Rotation = { -5.942438, -5.533558E-08, -2.430318 } },
    { .Type = BAKED_SPAWNPOINT_UPGRADE, .Params = 0, .Position = { 207.508, 388.318, 81.686 }, .Rotation = { -5.7795, -6.23334, -4.782942 } },
    { .Type = BAKED_SPAWNPOINT_MYSTERY_BOX, .Params = 0, .Position = { 184.1991, 439.6303, 86.28809 }, .Rotation = { 0, 0, -4.262914 } },
    { .Type = BAKED_SPAWNPOINT_MYSTERY_BOX, .Params = 0, .Position = { 169.8801, 367.5203, 80.50705 }, .Rotation = { 0, 0, -0.07412183 } },
    { .Type = BAKED_SPAWNPOINT_DEMON_BELL, .Params = 0, .Position = { 220.27, 410.4701, 86.02 }, .Rotation = { 0, 0, -3.141592 } },
    { .Type = BAKED_SPAWNPOINT_DEMON_BELL, .Params = 0, .Position = { 220.27, 407.47, 86.02 }, .Rotation = { 0, 0, -3.141592 } },
    { .Type = BAKED_SPAWNPOINT_DEMON_BELL, .Params = 0, .Position = { 220.27, 404.4699, 86.02 }, .Rotation = { 0, 0, -3.141592 } }
  }
};

//--------------------------------------------------------------------------
u32 MobPrimaryColors[] = {
	[MOB_SPAWN_PARAM_NORMAL] 	0x00464443,
	[MOB_SPAWN_PARAM_TREMOR] 	0x00464443,
	[MOB_SPAWN_PARAM_REAPER] 	0x00464443,
};

u32 MobSecondaryColors[] = {
	[MOB_SPAWN_PARAM_NORMAL] 	0x80202020,
	[MOB_SPAWN_PARAM_TREMOR] 	0x80202020,
	[MOB_SPAWN_PARAM_REAPER] 	0x80808080,
};

u32 MobLODColors[] = {
	[MOB_SPAWN_PARAM_NORMAL] 	0x00808080,
	[MOB_SPAWN_PARAM_TREMOR] 	0x00808080,
	[MOB_SPAWN_PARAM_REAPER] 	0x80808080,
};

//--------------------------------------------------------------------------
struct MysteryBoxItemWeight MysteryBoxItemProbabilities[] = {
  { MYSTERY_BOX_ITEM_QUAD, 0.0526 },
  { MYSTERY_BOX_ITEM_SHIELD, 0.0526 },
  { MYSTERY_BOX_ITEM_INVISIBILITY_CLOAK, 0.0526 },
  { MYSTERY_BOX_ITEM_RANDOMIZE_WEAPON_PICKUPS, 0.0526 },
  { MYSTERY_BOX_ITEM_EMP_HEALTH_GUN, 0.0555 },
  { MYSTERY_BOX_ITEM_REVIVE_TOTEM, 0.0555 },
  { MYSTERY_BOX_ITEM_INFINITE_AMMO, 0.0555 },
  { MYSTERY_BOX_ITEM_UPGRADE_WEAPON, 0.0967 },
  { MYSTERY_BOX_ITEM_TEDDY_BEAR, 0.1428 },
  { MYSTERY_BOX_ITEM_DREAD_TOKEN, 0.33333 },
  { MYSTERY_BOX_ITEM_WEAPON_MOD, 1.0 },
};
const int MysteryBoxItemProbabilitiesCount = sizeof(MysteryBoxItemProbabilities)/sizeof(struct MysteryBoxItemWeight);

struct MysteryBoxItemWeight MysteryBoxItemProbabilitiesLucky[] = {
  { MYSTERY_BOX_ITEM_WEAPON_MOD, 1.0 },
};
const int MysteryBoxItemProbabilitiesLuckyCount = sizeof(MysteryBoxItemProbabilitiesLucky)/sizeof(struct MysteryBoxItemWeight);

//--------------------------------------------------------------------------
void configInit(void)
{
  MapConfig.DefaultSpawnParams = defaultSpawnParams;
  MapConfig.DefaultSpawnParamsCount = defaultSpawnParamsCount;
  MapConfig.SpecialRoundParams = specialRoundParams;
  MapConfig.SpecialRoundParamsCount = specialRoundParamsCount;
}
