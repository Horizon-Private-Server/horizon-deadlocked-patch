#include "include/interop.h"
#include "include/game.h"

extern struct SurvivalState State;
extern struct SurvivalMapConfig* mapConfig;

//--------------------------------------------------------------------------
int playerGetRes(Player *player, VECTOR outPos, VECTOR outRot, int firstRes)
{
  if (!mapConfig->Functions.OnPlayerGetResFunc)
    return 0;

  return mapConfig->Functions.OnPlayerGetResFunc(player, outPos, outRot, firstRes);
}

//--------------------------------------------------------------------------
struct SurvivalBakedSpawnpoint *getBakedSpawnPoints(int *count)
{
  if (!mapConfig->Functions.GetBakedSpawnPointsFunc)
  {
    if (count) *count = 0;
    return NULL;
  }

  return mapConfig->Functions.GetBakedSpawnPointsFunc(count);
}

//--------------------------------------------------------------------------
float getDifficultyMultiplier(void)
{
  if (!mapConfig->Functions.GetDifficultyMultiplierFunc)
    return 1;

  return mapConfig->Functions.GetDifficultyMultiplierFunc();
}

//--------------------------------------------------------------------------
float getBoltMultiplier(void)
{
  if (!mapConfig->Functions.GetBoltMultiplierFunc)
    return 1;

  return mapConfig->Functions.GetBoltMultiplierFunc();
}

//--------------------------------------------------------------------------
float getXpMultiplier(void)
{
  if (!mapConfig->Functions.GetXpMultiplierFunc)
    return 1;

  return mapConfig->Functions.GetXpMultiplierFunc();
}

//--------------------------------------------------------------------------
int getBoltRankMultiplier(void)
{
  if (!mapConfig->Functions.GetBoltRankMultiplierFunc)
    return 1;

  return mapConfig->Functions.GetBoltRankMultiplierFunc();
}

//--------------------------------------------------------------------------
float getSpawnDistanceMultiplier(void)
{
  if (!mapConfig->Functions.GetSpawnDistanceMultiplierFunc)
    return 1;

  return mapConfig->Functions.GetSpawnDistanceMultiplierFunc();
}

//--------------------------------------------------------------------------
int canPrestigePlayerWeapon(Player *player, int gadgetId, int prestigeNum, char **outMsg)
{
  if (!mapConfig->Functions.CanPrestigePlayerWeaponFunc)
    return 0;

  return mapConfig->Functions.CanPrestigePlayerWeaponFunc(player, gadgetId, prestigeNum, outMsg);
}

//--------------------------------------------------------------------------
u32 getPrestigePlayerWeaponCost(Player *player, int gadgetId, int prestigeNum)
{
  if (!mapConfig->Functions.GetPrestigePlayerWeaponCostFunc)
    return 0;

  return mapConfig->Functions.GetPrestigePlayerWeaponCostFunc(player, gadgetId, prestigeNum);
}

//--------------------------------------------------------------------------
int canUpgradePlayerWeapon(Player *player, int gadgetId, int levelNum)
{
  if (!mapConfig->Functions.CanUpgradePlayerWeaponFunc)
    return 0;

  return mapConfig->Functions.CanUpgradePlayerWeaponFunc(player, gadgetId, levelNum);
}

//--------------------------------------------------------------------------
u32 getUpgradePlayerWeaponCost(Player *player, int gadgetId, int levelNum)
{
  if (!mapConfig->Functions.GetUpgradePlayerWeaponCostFunc)
    return 0;

  return mapConfig->Functions.GetUpgradePlayerWeaponCostFunc(player, gadgetId, levelNum);
}

//--------------------------------------------------------------------------
u32 getXpForNextToken(Player* player, int token)
{
  if (!mapConfig->Functions.GetXpForNextTokenFunc)
    return 100; // default to 100, shouldn't ever be used

  return mapConfig->Functions.GetXpForNextTokenFunc(player, token);
}

//--------------------------------------------------------------------------
float getCurrentDifficulty(void)
{
  if (!mapConfig->Functions.GetCurrentDifficultyFunc)
    return 1;

  return mapConfig->Functions.GetCurrentDifficultyFunc();
}

//--------------------------------------------------------------------------
int getDropItemOnMobKilled(Player *killedByPlayer, Moby *mob, int gadgetId)
{
  if (!mapConfig->Functions.GetDropItemOnMobKilledFunc)
    return -1;

  return mapConfig->Functions.GetDropItemOnMobKilledFunc(killedByPlayer, mob, gadgetId);
}

//--------------------------------------------------------------------------
int getRoundTransitionTime(int round)
{
  if (!mapConfig->Functions.GetRoundTransitionTimeFunc)
    return ROUND_TRANSITION_DELAY_MS;

  return mapConfig->Functions.GetRoundTransitionTimeFunc(round);
}

//--------------------------------------------------------------------------
void passPlayerOnItemAcquiredToMap(Player* player, int itemId)
{
  if (!mapConfig->Functions.GetOnPlayerItemAcquiredFunc)
    return;

  mapConfig->Functions.GetOnPlayerItemAcquiredFunc(player, itemId);
}

//--------------------------------------------------------------------------
void passPlayerOnItemConsumedToMap(Player* player, int itemId)
{
  if (!mapConfig->Functions.GetOnPlayerItemConsumedFunc)
    return;

  mapConfig->Functions.GetOnPlayerItemConsumedFunc(player, itemId);
}

//--------------------------------------------------------------------------
void passPlayerUpdateToMap(Player* player)
{
  if (!mapConfig->Functions.OnPlayerUpdateFunc)
    return;

  mapConfig->Functions.OnPlayerUpdateFunc(player);
}

//--------------------------------------------------------------------------
void passPlayerDiedToMap(Player* player)
{
  if (!mapConfig->Functions.OnPlayerDiedFunc)
    return;

  mapConfig->Functions.OnPlayerDiedFunc(player);
}

//--------------------------------------------------------------------------
void passPlayerGetVendorRewardToMap(Player* player, int gadgetId, int levelNum)
{
  if (!mapConfig->Functions.OnPlayerGetVendorRewardFunc)
    return;

  mapConfig->Functions.OnPlayerGetVendorRewardFunc(player, gadgetId, levelNum);
}

//--------------------------------------------------------------------------
int mobOnBeforeDamage(Player *player, Moby *sourceMoby, Moby *mobMoby, struct MobDamageEventArgs *args)
{
  if (!mapConfig->Functions.OnBeforeDamageMobFunc)
    return 1; // default is to allow damage

  return mapConfig->Functions.OnBeforeDamageMobFunc(player, sourceMoby, mobMoby, args);
}
