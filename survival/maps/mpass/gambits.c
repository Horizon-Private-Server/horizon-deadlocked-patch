#include <libdl/utils.h>
#include <libdl/net.h>
#include <libdl/moby.h>
#include <libdl/random.h>
#include <libdl/stdio.h>
#include "../../../include/game.h"
#include "../../../include/mob.h"
#include "../../../include/mysterybox.h"
#include "mpass.h"
#include "maputils.h"

extern struct SurvivalMapConfig MapConfig;
extern SurvivalBakedConfig_t bakedConfig;
extern struct MysteryBoxItemWeight MysteryBoxItemProbabilities[];
extern const int MysteryBoxItemProbabilitiesCount;
extern struct MobSpawnParams defaultSpawnParams[];
extern const int defaultSpawnParamsCount;

struct {
  char FinishedSetup;
  char PrintedGambit;
} GambitsState;

//--------------------------------------------------------------------------
enum GambitIds gambitsGetActive(void)
{
  return (enum GambitIds)PATCH_INTEROP->GameConfig->survivalConfig.gambit;
}

//--------------------------------------------------------------------------
void gambitsSetupSingleWeaponRestriction(int weaponId)
{
  int i;
  for (i = 0; i < GAME_MAX_LOCALS; ++i) {
    Player* player = playerGetFromSlot(i);
    if (!playerIsValid(player)) continue;

    playerGiveWeapon(player->GadgetBox, weaponId, 0, 1);
  }
}

//--------------------------------------------------------------------------
void gambitsDropCreate(VECTOR position, enum DropType dropType, int destroyAtTime, int team)
{
  // intercept health drops
  if (gambitsGetActive() == GAMBIT_ID_IMPOSSIBLE_MODE && dropType == DROP_HEALTH) return;

  dropCreate(position, dropType, destroyAtTime, team);
}

//--------------------------------------------------------------------------
void gambitsOnRoundComplete(int roundNo)
{
  int gambit = gambitsGetActive();
  if (!gambit) return;

  // mark complete after X rounds
  if (roundNo == 50) {
    switch (gambit)
    {
      case GAMBIT_ID_NO_SPEED:
      case GAMBIT_ID_NO_VENDOR:
      case GAMBIT_ID_MINES_ONLY:
      case GAMBIT_ID_ARBITER_ONLY:
      case GAMBIT_ID_FLAIL_ONLY:
        mapSendSendGambitCompletedMessage(gambit);
        break;
    }
  } else if (roundNo == 100) {
    switch (gambit)
    {
      case GAMBIT_ID_EASY_MODE:
      case GAMBIT_ID_IMPOSSIBLE_MODE:
        mapSendSendGambitCompletedMessage(gambit);
        break;
    }
  }
}

//--------------------------------------------------------------------------
void gambitsSetup(void)
{
  int i;
  GameOptions* gameOptions = gameGetOptions();
  Moby* mStart = mobyListGetStart();

  int gambit = gambitsGetActive();
  if (!gambit) return;

  switch (gambit)
  {
    case GAMBIT_ID_EASY_MODE:
    {
      bakedConfig.Difficulty *= 0.5;
      bakedConfig.BoltMultiplier = 2;
      bakedConfig.XpMultiplier = 2;
      GambitsState.FinishedSetup = 1;
      break;
    }
    case GAMBIT_ID_IMPOSSIBLE_MODE:
    {
      bakedConfig.Difficulty *= 2;
      bakedConfig.BoltMultiplier = 0.5;
      bakedConfig.XpMultiplier = 0.5;

      // disable revive items from mystery box
      for (i = 0; i < MysteryBoxItemProbabilitiesCount; ++i) {
        if (MysteryBoxItemProbabilities[i].Item == MYSTERY_BOX_ITEM_REVIVE_TOTEM) {
          MysteryBoxItemProbabilities[i].Probability = 0;
        }
        if (MysteryBoxItemProbabilities[i].Item == MYSTERY_BOX_ITEM_EMP_HEALTH_GUN) {
          MysteryBoxItemProbabilities[i].Probability = 0;
        }
      }

      MapConfig.CreateMobDropFunc = &gambitsDropCreate;
      GambitsState.FinishedSetup = 1;
      break;
    }
    case GAMBIT_ID_NO_SPEED:
    {
      Moby* m = mStart;
      while ((m = mobyFindNextByOClass(m, UPGRADE_MOBY_OCLASS))) {
        if (m->PVar) {
          struct UpgradePVar* pvars = m->PVar;
          if (pvars->Type == UPGRADE_SPEED) {
            m->Position[2] = 0;
            GambitsState.FinishedSetup = 1;
            DPRINTF("set free %08X\n", m);
          }
        }
        ++m;
      }
      break;
    }
    case GAMBIT_ID_NO_VENDOR:
    {
      Moby* m = mStart;
      while ((m = mobyFindNextByOClass(m, VENDOR_MOBY_OCLASS))) {
        m->DrawDist = 0;
        m->CollActive = -1;
        GambitsState.FinishedSetup = 1;
        ++m;
      }
      break;
    }
    case GAMBIT_ID_MINES_ONLY:
    {
      gameOptions->WeaponFlags.DualVipers = 0;
      gameOptions->WeaponFlags.MagmaCannon = 0;
      gameOptions->WeaponFlags.Arbiter = 0;
      gameOptions->WeaponFlags.FusionRifle = 0;
      gameOptions->WeaponFlags.MineLauncher = 1;
      gameOptions->WeaponFlags.B6 = 0;
      gameOptions->WeaponFlags.Holoshield = 0;
      gameOptions->WeaponFlags.Flail = 0;
      gambitsSetupSingleWeaponRestriction(WEAPON_ID_MINE_LAUNCHER);
      GambitsState.FinishedSetup = 1;
      break;
    }
    case GAMBIT_ID_ARBITER_ONLY:
    {
      gameOptions->WeaponFlags.DualVipers = 0;
      gameOptions->WeaponFlags.MagmaCannon = 0;
      gameOptions->WeaponFlags.Arbiter = 1;
      gameOptions->WeaponFlags.FusionRifle = 0;
      gameOptions->WeaponFlags.MineLauncher = 0;
      gameOptions->WeaponFlags.B6 = 0;
      gameOptions->WeaponFlags.Holoshield = 0;
      gameOptions->WeaponFlags.Flail = 0;
      gambitsSetupSingleWeaponRestriction(WEAPON_ID_ARBITER);
      GambitsState.FinishedSetup = 1;
      break;
    }
    case GAMBIT_ID_FLAIL_ONLY:
    {
      gameOptions->WeaponFlags.DualVipers = 0;
      gameOptions->WeaponFlags.MagmaCannon = 0;
      gameOptions->WeaponFlags.Arbiter = 0;
      gameOptions->WeaponFlags.FusionRifle = 0;
      gameOptions->WeaponFlags.MineLauncher = 0;
      gameOptions->WeaponFlags.B6 = 0;
      gameOptions->WeaponFlags.Holoshield = 0;
      gameOptions->WeaponFlags.Flail = 1;
      gambitsSetupSingleWeaponRestriction(WEAPON_ID_FLAIL);
      GambitsState.FinishedSetup = 1;
      break;
    }
  }
}

//--------------------------------------------------------------------------
void gambitsTick(void)
{
  int gambit = gambitsGetActive();
  if (!gambit) return;

  // make sure we've initialized
  if (!GambitsState.FinishedSetup) gambitsSetup();

  // print ready
  if (GambitsState.FinishedSetup && !GambitsState.PrintedGambit && MapConfig.ClientsReady) {
    GambitsState.PrintedGambit = 1;
    mapPrintGambit(gambit);
  }

  switch (gambit)
  {
    case GAMBIT_ID_IMPOSSIBLE_MODE:
    {
      // disable bank
      if (MapConfig.State->Bankbox) {
        MapConfig.State->Bankbox->DrawDist = 0;
        MapConfig.State->Bankbox->CollActive = -1;
        MapConfig.State->Bankbox = NULL;
      }

      // remove self revive from players (solo)
      int i;
      for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
        if (MapConfig.State->PlayerStates[i].State.Item == MYSTERY_BOX_ITEM_REVIVE_TOTEM) {
          MapConfig.State->PlayerStates[i].State.Item = 0;
        }

        // immediately "kill" downed player
        if (MapConfig.State->PlayerStates[i].ReviveCooldownTicks > 0)
          MapConfig.State->PlayerStates[i].ReviveCooldownTicks = 0;
      }

      break;
    }
    case GAMBIT_ID_NO_VENDOR:
    {
      if (MapConfig.State)
        MapConfig.State->Vendor = NULL;
      break;
    }
    case GAMBIT_ID_MINES_ONLY:
    {
      mapEnforceSingleWeaponRestriction(WEAPON_ID_MINE_LAUNCHER);
      break;
    }
    case GAMBIT_ID_ARBITER_ONLY:
    {
      mapEnforceSingleWeaponRestriction(WEAPON_ID_ARBITER);
      break;
    }
    case GAMBIT_ID_FLAIL_ONLY:
    {
      mapEnforceSingleWeaponRestriction(WEAPON_ID_FLAIL);
      break;
    }
  }
}

//--------------------------------------------------------------------------
void gambitsInit(void)
{
  memset(&GambitsState, 0, sizeof(GambitsState));
  gambitsSetup();
}
