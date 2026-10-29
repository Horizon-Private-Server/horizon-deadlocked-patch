#include <float.h>
#include <libdl/random.h>
#include "include/game.h"
#include "include/interop.h"
#include "include/utils.h"
#include "include/hop.h"
#include "include/mob.h"
#include "include/bubble.h"
#include "include/loot.h"

void onMissionComplete(int cuboidIdx);
void onMissionFail(void);

//--------------------------------------------------------------------------
void raidsInteropInit(void)
{
  if (!hasMapConfig()) return;

  mapConfig->Functions.ModePushSnackFunc = &pushSnack;
  mapConfig->Functions.ModePushDamageBubbleFunc = &bubblePush;
  mapConfig->Functions.ModeGetAmmoRefillCostFunc = &getAmmoRefillCost;
  mapConfig->Functions.ModeBeginWorldHopFunc = &hopBegin;
  mapConfig->Functions.ModePopulateSpawnArgsFunc = &mobPopulateSpawnArgsFromConfig;
  mapConfig->Functions.ModeRegisterNpcFunc = &mobRegisterNpc;
  mapConfig->Functions.ModeOnGetGuberFunc = &getGuber;
  mapConfig->Functions.ModeOnGuberEventFunc = &handleEvent;
  mapConfig->Functions.ModeTryCreateMobFunc = &mobCreate;
  mapConfig->Functions.ModeRequestPrestigeLootFunc = &lootRequestFromPrestige;
  mapConfig->Functions.ModeOnMissionCompleteFunc = &onMissionComplete;
  mapConfig->Functions.ModeOnMissionFailFunc = &onMissionFail;
}

//--------------------------------------------------------------------------
void raidsModifyPlayerStats(Player* player, struct RaidsPlayerStats* stats)
{
  if (!hasMapConfig() || !stats) return;

  struct RaidsPlayerStats baseline = *stats;
  if (mapConfig->Functions.ModifyPlayerStatsFunc)
    mapConfig->Functions.ModifyPlayerStatsFunc(player, stats);

  if (!(stats->Speed > 0 && stats->Speed <= FLT_MAX)) stats->Speed = baseline.Speed;
  if (!(stats->MaxHealth > 0 && stats->MaxHealth <= FLT_MAX)) stats->MaxHealth = baseline.MaxHealth;
}

//--------------------------------------------------------------------------
void raidsOnPlayerUpdate(Player* player)
{
  if (hasMapConfig() && mapConfig->Functions.OnPlayerUpdateFunc)
    mapConfig->Functions.OnPlayerUpdateFunc(player);
}

//--------------------------------------------------------------------------
void raidsOnPlayerDied(Player* player)
{
  if (hasMapConfig() && mapConfig->Functions.OnPlayerDiedFunc)
    mapConfig->Functions.OnPlayerDiedFunc(player);
}

//--------------------------------------------------------------------------
void raidsOnPlayerRevived(Player* player)
{
  if (hasMapConfig() && mapConfig->Functions.OnPlayerRevivedFunc)
    mapConfig->Functions.OnPlayerRevivedFunc(player);
}

//--------------------------------------------------------------------------
void raidsFinalizePlayerDamage(struct RaidsPlayerDamageContext* context)
{
  if (!context || !context->Attacker || !context->Attacker->IsLocal || !hasMapConfig()) return;

  if (mapConfig->Functions.ModifyPlayerCritChanceFunc)
    mapConfig->Functions.ModifyPlayerCritChanceFunc(context);

  if (!(context->CritChance >= 0 && context->CritChance <= FLT_MAX)) context->CritChance = 0;
  if (context->CritChance > 1) context->CritChance = 1;

  if (mapConfig->Functions.ModifyPlayerCritDamageFunc)
    mapConfig->Functions.ModifyPlayerCritDamageFunc(context);

  if (!(context->CritMultiplier >= 0 && context->CritMultiplier <= FLT_MAX)) context->CritMultiplier = 3;

  context->IsCritical = randRange(0, 1) < context->CritChance;
  if (context->IsCritical) {
    context->Damage *= context->CritMultiplier;
    context->DamageFlags |= 0x20000000;
  }
  if (mapConfig->Functions.FinalizePlayerDamageFunc)
    mapConfig->Functions.FinalizePlayerDamageFunc(context);

  if (!(context->Damage >= 0 && context->Damage <= FLT_MAX)) context->Damage = 0;
}

//--------------------------------------------------------------------------
u32 raidsDamageToQuarters(float damage)
{
  if (!(damage > 0 && damage <= FLT_MAX)) return 0;

  // Use signed conversions: the EE runtime has no double-to-unsigned helper.
  if (damage >= 1073741824.0f) return 0xFFFFFFFFU;
  float quarters = damage * 4.0f;
  if (quarters >= 2147483648.0f)
    return 0x80000000U | (u32)(s32)(quarters - 2147483648.0f);
  return (u32)(s32)quarters;
}

//--------------------------------------------------------------------------
void raidsMobAfterDamage(Moby* moby, struct MobDamageEventArgs* args, float appliedDamage)
{
  if (hasMapConfig() && mapConfig->Functions.MobAfterDamageFunc)
    mapConfig->Functions.MobAfterDamageFunc(moby, args, appliedDamage);
}

//--------------------------------------------------------------------------
float raidsGetAmmoDropChance(Player* player, Moby* moby, float baseChance)
{
  if (!(baseChance >= 0 && baseChance <= FLT_MAX)) baseChance = 0;
  if (baseChance > 1) baseChance = 1;

  float chance = baseChance;
  if (hasMapConfig() && mapConfig->Functions.GetAmmoDropChanceFunc)
    chance = mapConfig->Functions.GetAmmoDropChanceFunc(player, moby, baseChance);

  if (!(chance >= 0 && chance <= FLT_MAX)) chance = baseChance;
  if (chance > 1) chance = 1;
  return chance;
}
