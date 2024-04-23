#include <libdl/utils.h>
#include "../../../include/game.h"
#include "../../../include/mob.h"
#include "../../../include/mysterybox.h"
#include "mpass.h"

extern struct SurvivalMapConfig MapConfig;


//--------------------------------------------------------------------------
struct SurvivalSpecialRoundParam specialRoundParams[] = {
	// BOSS ROUND
	{
    .MinRound = 25,
    .RepeatEveryNRounds = 25,
    .RepeatCount = 0,
    .UnlimitedPostRoundTime = 1,
    .DisableDrops = 1,
		.MaxSpawnedAtOnce = MAX_MOBS_ALIVE_REAL,
    .SpawnCountFactor = 1.0,
    .SpawnRateFactor = 1.0,
		.SpawnParamCount = 1,
		.SpawnParamIds = {
			MOB_SPAWN_PARAM_REACTOR,
			-1, -1, -1
		},
		.Name = "Boss Round"
	},
};
const int specialRoundParamsCount = sizeof(specialRoundParams) / sizeof(struct SurvivalSpecialRoundParam);

// SHOULD NEVER EXCEED MAX_MOB_SPAWN_PARAMS
struct MobSpawnParams defaultSpawnParams[] = {
	// reactor
  [MOB_SPAWN_PARAM_REACTOR]
	{
		.Cost = REACTOR_RENDER_COST,
    .MaxSpawnedAtOnce = 1,
    .MaxSpawnedPerRound = 1,
    .SpecialRoundOnly = 1,
		.MinRound = 0,
		.CooldownTicks = 0,
		.Probability = 1,
    .StatId = MOB_STAT_REACTOR,
		.SpawnType = SPAWN_TYPE_SEMI_NEAR_PLAYER,
		.Name = "Reactor",
		.Config = {
			.Xp = 1000,
      .SharedXp = 1,
			.Bangles = REACTOR_BANGLE_SHOULDER_PLATES,
			.Damage = MOB_BASE_DAMAGE * 1.0,
			.MaxDamage = 0,
      .DamageScale = 2.0,
			.Speed = MOB_BASE_SPEED * 1.5,
			.MaxSpeed = MOB_BASE_SPEED * 5.0,
      .SpeedScale = 0.25,
			.Health = MOB_BASE_HEALTH * 100.0,
			.MaxHealth = 0,
      .HealthScale = 1.0,
			.Bolts = MOB_BASE_BOLTS * 50.0,
			.AttackRadius = REACTOR_MELEE_ATTACK_RADIUS * 1.0,
			.HitRadius = REACTOR_MELEE_HIT_RADIUS * 1.0,
      .CollRadius = REACTOR_BASE_COLL_RADIUS * 1.0,
			.ReactionTickCount = REACTOR_BASE_REACTION_TICKS * 0.35,
			.AttackCooldownTickCount = REACTOR_BASE_ATTACK_COOLDOWN_TICKS * 1.0,
			.MobAttribute = 0,
		}
	},
	// acid zombie
	[MOB_SPAWN_PARAM_ACID]
	{
		.Cost = ZOMBIE_RENDER_COST,
    .MaxSpawnedAtOnce = 20,
    .MaxSpawnedPerRound = 0,
    .SpecialRoundOnly = 1,
		.MinRound = 0,
		.Probability = 0,
		.CooldownTicks = TPS * 1,
    .StatId = MOB_STAT_ZOMBIE_ACID,
		.SpawnType = SPAWN_TYPE_SEMI_NEAR_PLAYER | SPAWN_TYPE_NEAR_PLAYER | SPAWN_TYPE_NEAR_HEALTHBOX,
		.Name = "Acid Minion",
		.Config = {
			.Xp = 75,
			.Bangles = ZOMBIE_BANGLE_HEAD_4 | ZOMBIE_BANGLE_TORSO_4,
			.Damage = MOB_BASE_DAMAGE * 1.0,
			.MaxDamage = 0,
      .DamageScale = 0.5,
			.Speed = MOB_BASE_SPEED * 1.0,
			.MaxSpeed = MOB_BASE_SPEED * 1.8,
      .SpeedScale = 1.0,
			.Health = MOB_BASE_HEALTH * 1.5,
			.MaxHealth = 0,
      .HealthScale = 1.2,
			.Bolts = MOB_BASE_BOLTS * 2.0,
			.AttackRadius = ZOMBIE_MELEE_ATTACK_RADIUS,
			.HitRadius = ZOMBIE_MELEE_HIT_RADIUS,
      .CollRadius = ZOMBIE_BASE_COLL_RADIUS * 1.0,
			.ReactionTickCount = ZOMBIE_BASE_REACTION_TICKS,
			.AttackCooldownTickCount = ZOMBIE_BASE_ATTACK_COOLDOWN_TICKS,
			.MobAttribute = MOB_ATTRIBUTE_ACID,
		}
	},
  // giant swarmer
  [MOB_SPAWN_PARAM_GIANT_SWARMER]
	{
		.Cost = SWARMER_RENDER_COST,
    .MaxSpawnedAtOnce = 5,
    .MaxSpawnedPerRound = 0,
    .SpecialRoundOnly = 0,
		.MinRound = 40,
		.CooldownTicks = 0,
		.Probability = 0.01,
    .StatId = MOB_STAT_SWARMER,
		.SpawnType = SPAWN_TYPE_SEMI_NEAR_PLAYER | SPAWN_TYPE_NEAR_PLAYER,
		.Name = "Swarmer",
		.Config = {
			.Xp = 250,
			.Bangles = 0,
			.Damage = MOB_BASE_DAMAGE * 1.0,
			.MaxDamage = 0,
      .DamageScale = 1.0,
			.Speed = MOB_BASE_SPEED * 2.0,
			.MaxSpeed = MOB_BASE_SPEED * 3.0,
      .SpeedScale = 0.25,
			.Health = MOB_BASE_HEALTH * 5.0,
			.MaxHealth = 0,
      .HealthScale = 1.0,
			.Bolts = MOB_BASE_BOLTS * 2.5,
			.AttackRadius = SWARMER_MELEE_ATTACK_RADIUS * 1.3,
			.HitRadius = SWARMER_MELEE_HIT_RADIUS * 2.0,
      .CollRadius = SWARMER_BASE_COLL_RADIUS * 3.0,
			.ReactionTickCount = SWARMER_BASE_REACTION_TICKS,
			.AttackCooldownTickCount = SWARMER_BASE_ATTACK_COOLDOWN_TICKS,
			.MobAttribute = MOB_ATTRIBUTE_RUSSIAN_DOLL,
		}
	},
  // reaper
  [MOB_SPAWN_PARAM_REAPER]
	{
		.Cost = REAPER_RENDER_COST,
    .MaxSpawnedAtOnce = 20,
    .MaxSpawnedPerRound = 0,
    .SpecialRoundOnly = 0,
		.MinRound = 15,
		.CooldownTicks = 0,
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
	// tremor
  [MOB_SPAWN_PARAM_TREMOR]
	{
		.Cost = TREMOR_RENDER_COST,
    .MaxSpawnedAtOnce = 30,
    .MaxSpawnedPerRound = 0,
    .SpecialRoundOnly = 0,
		.MinRound = 6,
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
		.Probability = 0.5,
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
			.MaxSpeed = MOB_BASE_SPEED * 1.2,
      .SpeedScale = 0.5,
			.Health = MOB_BASE_HEALTH * 0.35,
			.MaxHealth = 0,
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
  .Difficulty = 1.0,
  .BakedSpawnPoints = {
    { .Type = BAKED_SPAWNPOINT_PLAYER_START, .Params = 0, .Position = { 658.3901, 828.0401, 499.7961 }, .Rotation = { 0, 0, -3.141593 } },
    { .Type = BAKED_SPAWNPOINT_UPGRADE, .Params = 0, .Position = { 594.99, 704.85, 507.63 }, .Rotation = { 0, 0, 1.788139E-07 } },
    { .Type = BAKED_SPAWNPOINT_UPGRADE, .Params = 0, .Position = { 503.96, 704.98, 506.32 }, .Rotation = { 0, 0, -7.450579E-07 } },
    { .Type = BAKED_SPAWNPOINT_UPGRADE, .Params = 0, .Position = { 624.6, 801.53, 504.07 }, .Rotation = { 0, 0, -2.283245 } },
    { .Type = BAKED_SPAWNPOINT_UPGRADE, .Params = 0, .Position = { 688.592, 858.57, 514.264 }, .Rotation = { 0, 0, -4.712388 } },
    { .Type = BAKED_SPAWNPOINT_UPGRADE, .Params = 0, .Position = { 504.301, 945.488, 507.915 }, .Rotation = { -5.984646, -7.795392E-09, -2.412499 } },
    { .Type = BAKED_SPAWNPOINT_UPGRADE, .Params = 0, .Position = { 456.14, 791.772, 509.28 }, .Rotation = { 0, 0, -3.979972 } },
    { .Type = BAKED_SPAWNPOINT_UPGRADE, .Params = 0, .Position = { 587.583, 819.116, 502.458 }, .Rotation = { -0.2670273, -2.3173E-08, -5.328631 } },
    { .Type = BAKED_SPAWNPOINT_UPGRADE, .Params = 0, .Position = { 504.936, 821.258, 501.923 }, .Rotation = { -6.177232, 0, -2.660405 } },
    { .Type = BAKED_SPAWNPOINT_UPGRADE, .Params = 0, .Position = { 562.021, 892.229, 508.457 }, .Rotation = { -6.218049, 8.900659E-16, -3.141593 } },
    { .Type = BAKED_SPAWNPOINT_UPGRADE, .Params = 0, .Position = { 448.302, 876.378, 507.799 }, .Rotation = { 0, 0, -5.496722 } },
    { .Type = BAKED_SPAWNPOINT_UPGRADE, .Params = 0, .Position = { 380.211, 826.473, 514.736 }, .Rotation = { -0.3037879, -0.07311971, -2.909856 } },
    { .Type = BAKED_SPAWNPOINT_MYSTERY_BOX, .Params = 0, .Position = { 585.6502, 905.4998, 506.599 }, .Rotation = { 0, 0, 1.260871 } },
    { .Type = BAKED_SPAWNPOINT_MYSTERY_BOX, .Params = 0, .Position = { 683.7198, 807.9098, 501.27 }, .Rotation = { 0, 0, -3.427705 } },
    { .Type = BAKED_SPAWNPOINT_MYSTERY_BOX, .Params = 0, .Position = { 634, 742.02, 507.35 }, .Rotation = { 0, 0, -2.904106 } },
    { .Type = BAKED_SPAWNPOINT_MYSTERY_BOX, .Params = 0, .Position = { 513.0701, 738.4099, 504.54 }, .Rotation = { 0, 0, -0.7853986 } },
    { .Type = BAKED_SPAWNPOINT_MYSTERY_BOX, .Params = 0, .Position = { 397.75, 793.05, 520.38 }, .Rotation = { 0, 0, -0.27788 } },
    { .Type = BAKED_SPAWNPOINT_MYSTERY_BOX, .Params = 0, .Position = { 469.15, 802.01, 504.22 }, .Rotation = { 0, 0, 1.400064 } },
    { .Type = BAKED_SPAWNPOINT_MYSTERY_BOX, .Params = 0, .Position = { 541.81, 778.23, 505.95 }, .Rotation = { 0, 0, -1.747867 } },
    { .Type = BAKED_SPAWNPOINT_MYSTERY_BOX, .Params = 0, .Position = { 547.39, 858.92, 505.86 }, .Rotation = { 0, 0, -4.371555 } },
    { .Type = BAKED_SPAWNPOINT_MYSTERY_BOX, .Params = 0, .Position = { 575.52, 809.11, 500.15 }, .Rotation = { 0, 0, -4.23276 } },
    { .Type = BAKED_SPAWNPOINT_MYSTERY_BOX, .Params = 0, .Position = { 512.43, 951.69, 507.16 }, .Rotation = { 0, 0, -1.327776 } },
    { .Type = BAKED_SPAWNPOINT_MYSTERY_BOX, .Params = 0, .Position = { 370.8, 813.12, 514.0535 }, .Rotation = { 0, 0, -0.06436086 } },
    { .Type = BAKED_SPAWNPOINT_DEMON_BELL, .Params = 0, .Position = { 660.2, 793.97, 507.61 }, .Rotation = { 0, 0, -1.570796 } },
    { .Type = BAKED_SPAWNPOINT_DEMON_BELL, .Params = 0, .Position = { 663.2, 793.97, 507.61 }, .Rotation = { 0, 0, -1.570796 } },
    { .Type = BAKED_SPAWNPOINT_DEMON_BELL, .Params = 0, .Position = { 666.2, 793.97, 507.61 }, .Rotation = { 0, 0, -1.570796 } }
  }
};

//--------------------------------------------------------------------------
u32 MobPrimaryColors[] = {
	[MOB_SPAWN_PARAM_REACTOR] 	0x00464443,
	[MOB_SPAWN_PARAM_ACID] 		  0x00464443,
  [MOB_SPAWN_PARAM_REAPER]    0x00464443,
	[MOB_SPAWN_PARAM_TREMOR] 	  0x00464443,
	[MOB_SPAWN_PARAM_NORMAL] 	  0x00464443,
	[MOB_SPAWN_PARAM_SWARMER] 	0x00464443,
  [MOB_SPAWN_PARAM_GIANT_SWARMER] 0x00464443,
};

u32 MobSecondaryColors[] = {
	[MOB_SPAWN_PARAM_REACTOR]   0x80808080,
	[MOB_SPAWN_PARAM_ACID] 		  0x8000FF00,
  [MOB_SPAWN_PARAM_REAPER]    0x80808080,
	[MOB_SPAWN_PARAM_TREMOR] 	  0x80808080,
	[MOB_SPAWN_PARAM_NORMAL] 	  0x80202020,
	[MOB_SPAWN_PARAM_SWARMER] 	0x80808080,
  [MOB_SPAWN_PARAM_GIANT_SWARMER] 0x80808080,
};

u32 MobLODColors[] = {
	[MOB_SPAWN_PARAM_REACTOR]   0x00808080,
	[MOB_SPAWN_PARAM_ACID] 		  0x0000F000,
  [MOB_SPAWN_PARAM_REAPER]    0x80808080,
	[MOB_SPAWN_PARAM_TREMOR] 	  0x00808080,
	[MOB_SPAWN_PARAM_NORMAL] 	  0x00808080,
	[MOB_SPAWN_PARAM_SWARMER] 	0x00808080,
  [MOB_SPAWN_PARAM_GIANT_SWARMER] 0x80808080,
};

//--------------------------------------------------------------------------
struct MysteryBoxItemWeight MysteryBoxItemProbabilities[] = {
  { MYSTERY_BOX_ITEM_RESET_GATE, 0.03 },
  { MYSTERY_BOX_ITEM_QUAD, 0.0526 },
  { MYSTERY_BOX_ITEM_SHIELD, 0.0526 },
  { MYSTERY_BOX_ITEM_INVISIBILITY_CLOAK, 0.0526 },
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
  { MYSTERY_BOX_ITEM_RESET_GATE, 0.01 },
  { MYSTERY_BOX_ITEM_QUAD, 0.07 },
  { MYSTERY_BOX_ITEM_SHIELD, 0.07 },
  { MYSTERY_BOX_ITEM_TEDDY_BEAR, 0.075 },
  { MYSTERY_BOX_ITEM_INVISIBILITY_CLOAK, 0.10 },
  { MYSTERY_BOX_ITEM_EMP_HEALTH_GUN, 0.10 },
  { MYSTERY_BOX_ITEM_REVIVE_TOTEM, 0.10 },
  { MYSTERY_BOX_ITEM_INFINITE_AMMO, 0.12 },
  { MYSTERY_BOX_ITEM_UPGRADE_WEAPON, 0.15 },
  { MYSTERY_BOX_ITEM_DREAD_TOKEN, 0.33333 },
  { MYSTERY_BOX_ITEM_WEAPON_MOD, 1.0 },
};
const int MysteryBoxItemProbabilitiesLuckyCount = sizeof(MysteryBoxItemProbabilitiesLucky)/sizeof(struct MysteryBoxItemWeight);

//--------------------------------------------------------------------------
// Locations of where to spawn statues
VECTOR statueSpawnPositionRotations[] = {
  { 543.98, 757.12, 505.52, 0 },
  { 0, 0, 180 * MATH_DEG2RAD, 0 },
  { 412.54, 772.13, 515.49, 0 },
  { 0, -3.382 * MATH_DEG2RAD, -215.379 * MATH_DEG2RAD, 0 },
  { 612.77, 940.64, 510.43, 0 },
  { 7.23 * MATH_DEG2RAD, 0, -44.605 * MATH_DEG2RAD, 0 },
};
const int statueSpawnPositionRotationsCount = sizeof(statueSpawnPositionRotations) / (sizeof(VECTOR) * 2);

//--------------------------------------------------------------------------
int zombieHitSoundId = 407;
int zombieAmbientSoundIds[] = { 403, 404 };
int zombieDeathSoundId = -1;
const int zombieAmbientSoundIdsCount = COUNT_OF(zombieAmbientSoundIds);

int tremorHitSoundId = 407;
int tremorAmbientSoundIds[] = {  };
int tremorDeathSoundId = -1;
const int tremorAmbientSoundIdsCount = COUNT_OF(tremorAmbientSoundIds);

int swarmerHitSoundId = 407;
int swarmerAmbientSoundIds[] = { };
int swarmerDeathSoundId = -1;
const int swarmerAmbientSoundIdsCount = COUNT_OF(swarmerAmbientSoundIds);

int reaperHitSoundId = 173;
int reaperAmbientSoundIds[] = { 403, 404 };
int reaperDeathSoundId = -1;
const int reaperAmbientSoundIdsCount = COUNT_OF(reaperAmbientSoundIds);

int reactorMinionSpawnParamIdx = MOB_SPAWN_PARAM_ACID;
int reactorHitSoundId = 407;
int reactorSmashSoundId = 317;
int reactorChargeSoundId = 184;
int reactorKneeDownSoundId = 103;
int reactorAmbientSoundIds[] = { 403, 404 };
int reactorDeathSoundId = 123;
const int reactorAmbientSoundIdsCount = COUNT_OF(reactorAmbientSoundIds);

int trailshotFireSoundId = 416;
int trailshotExplodeSoundId = 0x106;

int russianDollSpawnParamIdxs[] = {
  MOB_SPAWN_PARAM_SWARMER,
  MOB_SPAWN_PARAM_NORMAL,
  MOB_SPAWN_PARAM_TREMOR,
  MOB_SPAWN_PARAM_REAPER,
  MOB_SPAWN_PARAM_ACID,
  MOB_SPAWN_PARAM_GIANT_SWARMER
};
const int russianDollSpawnParamIdxsCount = COUNT_OF(russianDollSpawnParamIdxs);

void configInit(void)
{
  MapConfig.DefaultSpawnParams = defaultSpawnParams;
  MapConfig.DefaultSpawnParamsCount = defaultSpawnParamsCount;
  MapConfig.SpecialRoundParams = specialRoundParams;
  MapConfig.SpecialRoundParamsCount = specialRoundParamsCount;
}
