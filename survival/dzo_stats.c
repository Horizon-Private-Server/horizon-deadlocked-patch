#include <tamtypes.h>
#include <libdl/game.h>
#include <libdl/time.h>
#include <libdl/player.h>
#include <libdl/weapon.h>
#include <libdl/string.h>
#include "config.h"
#include "include/game.h"
#include "include/item.h"
#include "include/interop.h"
#include "include/utils.h"
#include "include/dzo_stats.h"

extern int Initialized;
extern struct SurvivalState State;
extern struct SurvivalMapConfig *mapConfig;

typedef struct DzoPlayerProgress
{
  u32 PerksAcquired;
  u32 PerksHeldPeak;
  u32 V10WeaponsMask;
  u32 MaxPrestigeV10WeaponsMask;
} DzoPlayerProgress_t;

DzoPlayerProgress_t dzoProgress[GAME_MAX_PLAYERS];
int dzoSelectedGambit;
u32 dzoGambitFlags;

//--------------------------------------------------------------------------
int dzoStatsSyncSelectedGambit(void)
{
  PatchInterop_t *patch = PATCH_INTEROP;
  int selected = patch && patch->GameConfig
    ? patch->GameConfig->survivalConfig.gambit : 0;
  if (selected != dzoSelectedGambit) {
    dzoSelectedGambit = selected;
    dzoGambitFlags = 0;
  }
  return selected;
}

//--------------------------------------------------------------------------
void dzoStatsReportGambitState(int gambitIdx, int completed)
{
  int selected = dzoStatsSyncSelectedGambit();
  if (selected <= 0 || gambitIdx != selected
      || (completed != 0 && completed != 1))
    return;

  dzoGambitFlags |= DZO_SURVIVAL_STATS_GAMBIT_AVAILABLE;
  if (completed)
    dzoGambitFlags |= DZO_SURVIVAL_STATS_GAMBIT_COMPLETED;
}

//--------------------------------------------------------------------------
int dzoStatsValidPlayerId(int playerId)
{
  return playerId >= 0 && playerId < GAME_MAX_PLAYERS;
}

//--------------------------------------------------------------------------
int dzoStatsPerksAvailable(void)
{
  return hasMapConfig() && mapConfig->ItemDefs
    && mapConfig->ItemDefCount > 0
    && mapConfig->ItemDefCount <= MAX_ITEM_COUNT;
}

//--------------------------------------------------------------------------
int dzoStatsPrestigeAvailable(void)
{
  return hasMapConfig()
    && mapConfig->Functions.CanPrestigePlayerWeaponFunc != 0;
}

//--------------------------------------------------------------------------
u32 dzoStatsCountPerksHeld(int playerId)
{
  int i;
  u32 count = 0;
  struct SurvivalPlayerState *state;

  if (!dzoStatsValidPlayerId(playerId) || !dzoStatsPerksAvailable())
    return 0;

  state = &State.PlayerStates[playerId].State;
  for (i = 0; i < mapConfig->ItemDefCount; ++i) {
    if (mapConfig->ItemDefs[i].Type == SURVIVAL_ITEM_PASSIVE
        && state->ItemCounts[i] > 0)
      count += (u32)state->ItemCounts[i];
  }

  return count;
}

//--------------------------------------------------------------------------
void dzoStatsUpdatePerkPeak(int playerId, u32 held)
{
  if (held > dzoProgress[playerId].PerksHeldPeak)
    dzoProgress[playerId].PerksHeldPeak = held;
}

//--------------------------------------------------------------------------
void dzoStatsReset(void)
{
  memset(dzoProgress, 0, sizeof(dzoProgress));
  dzoSelectedGambit = 0;
  dzoGambitFlags = 0;
}

//--------------------------------------------------------------------------
void dzoStatsOnItemAcquired(int playerId, int itemId)
{
  if (!dzoStatsValidPlayerId(playerId) || !dzoStatsPerksAvailable())
    return;
  if (itemId < 0 || itemId >= mapConfig->ItemDefCount)
    return;
  if (mapConfig->ItemDefs[itemId].Type != SURVIVAL_ITEM_PASSIVE)
    return;

  /* Existing acquisition events represent one acquired copy/stack. */
  ++dzoProgress[playerId].PerksAcquired;
  dzoStatsUpdatePerkPeak(playerId, dzoStatsCountPerksHeld(playerId));
}

//--------------------------------------------------------------------------
void dzoStatsObserveWeapons(Player *player)
{
  int playerId;
  int slot;
  int prestigeAvailable;
  struct SurvivalPlayerState *state;
  DzoPlayerProgress_t *progress;

  if (!playerIsValid(player) || !player->GadgetBox)
    return;
  playerId = player->PlayerId;
  if (!dzoStatsValidPlayerId(playerId))
    return;

  state = &State.PlayerStates[playerId].State;
  progress = &dzoProgress[playerId];
  prestigeAvailable = dzoStatsPrestigeAvailable();

  for (slot = 1; slot <= 8; ++slot) {
    int weaponId = weaponSlotToId(slot);
    int level = player->GadgetBox->Gadgets[weaponId].Level;
    int prestige = (unsigned char)state->WeaponPrestige[slot];
    u32 bit = 1u << (slot - 1);

    if (state->BestWeaponLevel[slot] >= VENDOR_MAX_WEAPON_LEVEL
        || level >= VENDOR_MAX_WEAPON_LEVEL)
      progress->V10WeaponsMask |= bit;

    if ((progress->MaxPrestigeV10WeaponsMask & bit)
        || level != VENDOR_MAX_WEAPON_LEVEL || prestige <= 0
        || !prestigeAvailable)
      continue;

    /*
     * Verified Forge callback checks only V10 level and the baked cap.
     * With this callback, allowing current prestige and rejecting the
     * next establishes that current prestige is the maximum.
     * See the custom-map verification gate following this listing.
     */
    if (canPrestigePlayerWeapon(player, weaponId, prestige, NULL)
        && !canPrestigePlayerWeapon(player, weaponId, prestige + 1, NULL))
      progress->MaxPrestigeV10WeaponsMask |= bit;
  }
}

//--------------------------------------------------------------------------
void dzoStatsSend(void)
{
  Player *player;
  int playerId;
  u32 held;
  struct SurvivalPlayerState *state;
  DzoPlayerProgress_t *progress;
  DzoInteropFunctions_t *interop;
  DzoSurvivalStats_t cmd = {0};

  if (!Initialized)
    return;
  dzoStatsSyncSelectedGambit();
  player = playerGetFromSlot(0);
  if (!playerIsValid(player) || !playerIsLocal(player))
    return;
  playerId = player->PlayerId;
  if (!dzoStatsValidPlayerId(playerId))
    return;

  /* Keep milestones even if DZO is temporarily unavailable. */
  dzoStatsObserveWeapons(player);
  held = dzoStatsCountPerksHeld(playerId);
  dzoStatsUpdatePerkPeak(playerId, held);

  interop = PATCH_DZO_INTEROP_FUNCS;
  if (!interop || !interop->SendCustomCommandToClient)
    return;

  state = &State.PlayerStates[playerId].State;
  progress = &dzoProgress[playerId];
  cmd.RunStartTime = (u32)State.InitializedTime;
  cmd.GameTime = (u32)gameGetTime();
  cmd.PlayerId = (u32)playerId;
  cmd.Flags = dzoGambitFlags;
  if (State.Paused)
    cmd.Flags |= DZO_SURVIVAL_STATS_PAUSED;
  if (State.GameOver)
    cmd.Flags |= DZO_SURVIVAL_STATS_GAME_OVER;
  if (dzoStatsPerksAvailable())
    cmd.Flags |= DZO_SURVIVAL_STATS_PERKS_AVAILABLE;
  if (dzoStatsPrestigeAvailable())
    cmd.Flags |= DZO_SURVIVAL_STATS_PRESTIGE_AVAILABLE;
  cmd.Kills = (u32)state->Kills;
  cmd.TeammatesRevived = (u32)state->Revives;
  cmd.MysteryBoxRolls = (u32)state->TimesRolledMysteryBox;
  cmd.TokensEarned = (u32)state->TotalTokens;
  cmd.BoltsEarnedLow = (u32)state->TotalBolts;
  cmd.BoltsEarnedHigh = (u32)(state->TotalBolts >> 32);
  cmd.PerksAcquired = progress->PerksAcquired;
  cmd.PerksHeld = held;
  cmd.PerksHeldPeak = progress->PerksHeldPeak;
  cmd.V10WeaponsMask = progress->V10WeaponsMask;
  cmd.MaxPrestigeV10WeaponsMask = progress->MaxPrestigeV10WeaponsMask;

  interop->SendCustomCommandToClient(
    CUSTOM_DZO_CMD_ID_SURVIVAL_STATS, sizeof(cmd), &cmd);
}
