#include <libdl/utils.h>
#include "../../../include/game.h"
#include "../../../include/mob.h"
#include "hub.h"

extern struct RaidsMapConfig MapConfig;

// NOTE
// These must be ordered from least probable to most probable
// SHOULD NEVER EXCEED MAX_MOB_SPAWN_PARAMS
struct MobSpawnParams mobSpawnParams[] = {
	// normal zombie
	[MOB_SPAWN_PARAM_NORMAL]
	{
		.RenderCost = ZOMBIE_RENDER_COST,
    .MaxSpawnedAtOnce = 0,
    .SpecialRoundOnly = 0,
		.CooldownTicks = 0,
    .CooldownOffsetPerRoundFactor = 0,
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
		}
	},
};

//--------------------------------------------------------------------------
RaidsBakedConfig_t bakedConfig = {
  .Difficulty = 1.0,
  .SpawnDistanceFactor = 1.0,
  .BoltRankMultiplier = 1
};

//--------------------------------------------------------------------------
void configInit(void)
{
  MapConfig.MobSpawnParams = mobSpawnParams;
  MapConfig.MobSpawnParamsCount = COUNT_OF(mobSpawnParams);
}
