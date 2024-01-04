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
int zombieHitSoundId = 0x17D;
int zombieAmbientSoundIds[] = { 0x17A, 0x179 };
int zombieDeathSoundId = -1;
const int zombieAmbientSoundIdsCount = COUNT_OF(zombieAmbientSoundIds);

int tremorHitSoundId = 0x17D;
int tremorAmbientSoundIds[] = { 0x17A, 0x179 };
int tremorDeathSoundId = -1;
const int tremorAmbientSoundIdsCount = COUNT_OF(tremorAmbientSoundIds);

int reaperHitSoundId = 0x17D;
int reaperAmbientSoundIds[] = { 0x17A, 0x179 };
int reaperDeathSoundId = -1;
const int reaperAmbientSoundIdsCount = COUNT_OF(reaperAmbientSoundIds);

void configInit(void)
{
  MapConfig.DefaultSpawnParams = defaultSpawnParams;
  MapConfig.DefaultSpawnParamsCount = defaultSpawnParamsCount;
  MapConfig.SpecialRoundParams = specialRoundParams;
  MapConfig.SpecialRoundParamsCount = specialRoundParamsCount;
}
