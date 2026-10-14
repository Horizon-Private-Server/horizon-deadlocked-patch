#ifndef SURVIVAL_DZO_STATS_H
#define SURVIVAL_DZO_STATS_H

#include <stddef.h>
#include <tamtypes.h>
#include <libdl/player.h>

/* Reserved locally; mirror in DZO's DzoPatchCustomCommandIds. */
#define CUSTOM_DZO_CMD_ID_SURVIVAL_STATS (15)

#define DZO_SURVIVAL_STATS_PAUSED              (1u << 0)
#define DZO_SURVIVAL_STATS_GAME_OVER           (1u << 1)
#define DZO_SURVIVAL_STATS_PERKS_AVAILABLE     (1u << 2)
#define DZO_SURVIVAL_STATS_PRESTIGE_AVAILABLE  (1u << 3)
#define DZO_SURVIVAL_STATS_GAMBIT_AVAILABLE    (1u << 4)
#define DZO_SURVIVAL_STATS_GAMBIT_COMPLETED    (1u << 5)

typedef struct DzoSurvivalStats
{
  u32 RunStartTime;
  u32 GameTime;
  u32 PlayerId;
  u32 Flags;
  u32 Kills;
  u32 TeammatesRevived;
  u32 MysteryBoxRolls;
  u32 TokensEarned;
  u32 BoltsEarnedLow;
  u32 BoltsEarnedHigh;
  u32 PerksAcquired;
  u32 PerksHeld;
  u32 PerksHeldPeak;
  u32 V10WeaponsMask;
  u32 MaxPrestigeV10WeaponsMask;
} DzoSurvivalStats_t;

typedef char DzoSurvivalStatsSizeCheck[
  sizeof(DzoSurvivalStats_t) == 60 ? 1 : -1];

typedef char DzoSurvivalStatsOffset0[offsetof(DzoSurvivalStats_t, RunStartTime) == 0 ? 1 : -1];
typedef char DzoSurvivalStatsOffset1[offsetof(DzoSurvivalStats_t, GameTime) == 4 ? 1 : -1];
typedef char DzoSurvivalStatsOffset2[offsetof(DzoSurvivalStats_t, PlayerId) == 8 ? 1 : -1];
typedef char DzoSurvivalStatsOffset3[offsetof(DzoSurvivalStats_t, Flags) == 12 ? 1 : -1];
typedef char DzoSurvivalStatsOffset4[offsetof(DzoSurvivalStats_t, Kills) == 16 ? 1 : -1];
typedef char DzoSurvivalStatsOffset5[offsetof(DzoSurvivalStats_t, TeammatesRevived) == 20 ? 1 : -1];
typedef char DzoSurvivalStatsOffset6[offsetof(DzoSurvivalStats_t, MysteryBoxRolls) == 24 ? 1 : -1];
typedef char DzoSurvivalStatsOffset7[offsetof(DzoSurvivalStats_t, TokensEarned) == 28 ? 1 : -1];
typedef char DzoSurvivalStatsOffset8[offsetof(DzoSurvivalStats_t, BoltsEarnedLow) == 32 ? 1 : -1];
typedef char DzoSurvivalStatsOffset9[offsetof(DzoSurvivalStats_t, BoltsEarnedHigh) == 36 ? 1 : -1];
typedef char DzoSurvivalStatsOffset10[offsetof(DzoSurvivalStats_t, PerksAcquired) == 40 ? 1 : -1];
typedef char DzoSurvivalStatsOffset11[offsetof(DzoSurvivalStats_t, PerksHeld) == 44 ? 1 : -1];
typedef char DzoSurvivalStatsOffset12[offsetof(DzoSurvivalStats_t, PerksHeldPeak) == 48 ? 1 : -1];
typedef char DzoSurvivalStatsOffset13[offsetof(DzoSurvivalStats_t, V10WeaponsMask) == 52 ? 1 : -1];
typedef char DzoSurvivalStatsOffset14[offsetof(DzoSurvivalStats_t, MaxPrestigeV10WeaponsMask) == 56 ? 1 : -1];

void dzoStatsReset(void);
void dzoStatsOnItemAcquired(int playerId, int itemId);
void dzoStatsObserveWeapons(Player *player);
void dzoStatsReportGambitState(int gambitIdx, int completed);
void dzoStatsSend(void);

#endif
