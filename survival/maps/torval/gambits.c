#include <libdl/utils.h>
#include <libdl/net.h>
#include <libdl/moby.h>
#include <libdl/stdio.h>
#include "../../../include/game.h"
#include "../../../include/mob.h"
#include "../../../include/mysterybox.h"
#include "torval.h"

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
void gambitsOnRoundComplete(int roundNo)
{
  int gambit = gambitsGetActive();
  if (!gambit) return;

  // mark complete after X rounds
  if (roundNo == 50) {
    switch (gambit)
    {
      case GAMBIT_ID_BIG_MOBS_ONLY:
      case GAMBIT_ID_WEAPON_LIFE:
      case GAMBIT_ID_VAMPIRE:
      case GAMBIT_ID_FLAIL_ONLY:
      case GAMBIT_ID_FUSION_ONLY:
      case GAMBIT_ID_B6_ONLY:
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

      GambitsState.FinishedSetup = 1;
      break;
    }
    case GAMBIT_ID_BIG_MOBS_ONLY:
    {
      // disable zombies and swarmers
      defaultSpawnParams[MOB_SPAWN_PARAM_NORMAL].Probability = 0;
      defaultSpawnParams[MOB_SPAWN_PARAM_SWARMER].Probability = 0;

      // have all mobs spawn at start
      for (i = 0; i < defaultSpawnParamsCount; ++i)
        defaultSpawnParams[i].MinRound = 0;

      GambitsState.FinishedSetup = 1;
      break;
    }
    case GAMBIT_ID_WEAPON_LIFE:
    {
      GambitsState.FinishedSetup = 1;
      break;
    }
    case GAMBIT_ID_VAMPIRE:
    {
      // disable perk boxes
      for (i = 0; i < BAKED_SPAWNPOINT_COUNT; ++i) {
        if (bakedConfig.BakedSpawnPoints[i].Type == BAKED_SPAWNPOINT_STACK_BOX) {
          bakedConfig.BakedSpawnPoints[i].Type = BAKED_SPAWNPOINT_NONE;
        }
      }
      
      // disable revive items from mystery box
      for (i = 0; i < MysteryBoxItemProbabilitiesCount; ++i) {
        if (MysteryBoxItemProbabilities[i].Item == MYSTERY_BOX_ITEM_REVIVE_TOTEM) {
          MysteryBoxItemProbabilities[i].Probability = 0;
        }
        if (MysteryBoxItemProbabilities[i].Item == MYSTERY_BOX_ITEM_EMP_HEALTH_GUN) {
          MysteryBoxItemProbabilities[i].Probability = 0;
        }
      }

      if (!MapConfig.State) return;

      // set vampire perks
      for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
        MapConfig.State->PlayerStates[i].State.ItemStackable[STACKABLE_ITEM_VAMPIRE] = 10;
      }

      GambitsState.FinishedSetup = 1;
      break;
    }
    // case GAMBIT_ID_FREE_VOX:
    // {
    //   Moby* m = mStart;
    //   while ((m = mobyFindNextByOClass(m, MYSTERY_BOX_OCLASS))) {
    //     if (m->PVar) {
    //       struct MysteryBoxPVar* pvars = m->PVar;
    //       pvars->BoltCostMultiplier = 0;
    //       GambitsState.FinishedSetup = 1;
    //       DPRINTF("set free %08X\n", m);
    //     }
    //     ++m;
    //   }
    //   break;
    // }
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
    case GAMBIT_ID_FUSION_ONLY:
    {
      gameOptions->WeaponFlags.DualVipers = 0;
      gameOptions->WeaponFlags.MagmaCannon = 0;
      gameOptions->WeaponFlags.Arbiter = 0;
      gameOptions->WeaponFlags.FusionRifle = 1;
      gameOptions->WeaponFlags.MineLauncher = 0;
      gameOptions->WeaponFlags.B6 = 0;
      gameOptions->WeaponFlags.Holoshield = 0;
      gameOptions->WeaponFlags.Flail = 0;
      gambitsSetupSingleWeaponRestriction(WEAPON_ID_FUSION_RIFLE);
      GambitsState.FinishedSetup = 1;
      break;
    }
    case GAMBIT_ID_B6_ONLY:
    {
      gameOptions->WeaponFlags.DualVipers = 0;
      gameOptions->WeaponFlags.MagmaCannon = 0;
      gameOptions->WeaponFlags.Arbiter = 0;
      gameOptions->WeaponFlags.FusionRifle = 0;
      gameOptions->WeaponFlags.MineLauncher = 0;
      gameOptions->WeaponFlags.B6 = 1;
      gameOptions->WeaponFlags.Holoshield = 0;
      gameOptions->WeaponFlags.Flail = 0;
      gambitsSetupSingleWeaponRestriction(WEAPON_ID_B6);
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
    case GAMBIT_ID_WEAPON_LIFE:
    {
      int i;
      for (i = 0; i < GAME_MAX_LOCALS; ++i) { 
        Player* player = playerGetFromSlot(i);
        if (!playerIsValid(player)) continue;

        MobyColDamage* colDamage = mobyGetDamage(player->PlayerMoby, -1, 0);
        if (!colDamage || colDamage->DamageHp >= 100000000 || colDamage->DamageHp <= 0) continue;

        if (player->timers.postHitInvinc == 0) {
          int weaponId = player->WeaponHeldId;
          int weaponLevel = player->GadgetBox->Gadgets[weaponId].Level;
          if (weaponLevel > 0) {
            player->GadgetBox->Gadgets[weaponId].Level--;
            colDamage->DamageHp = 0;
          } else {
            colDamage->DamageHp = 100000000;
          }
        }
      }
      break;
    }
    case GAMBIT_ID_IMPOSSIBLE_MODE:
    case GAMBIT_ID_VAMPIRE:
    {
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
    case GAMBIT_ID_FLAIL_ONLY:
    {
      mapEnforceSingleWeaponRestriction(WEAPON_ID_FLAIL);
      break;
    }
    case GAMBIT_ID_FUSION_ONLY:
    {
      mapEnforceSingleWeaponRestriction(WEAPON_ID_FUSION_RIFLE);
      break;
    }
    case GAMBIT_ID_B6_ONLY:
    {
      mapEnforceSingleWeaponRestriction(WEAPON_ID_B6);
      break;
    }
    // case GAMBIT_ID_NO_WEAPON_PRESTIGE:
    //   {
    //     // disable prestige
    //     if (MapConfig.State->PrestigeMachine) {
    //       MapConfig.State->PrestigeMachine->DrawDist = 0;
    //       MapConfig.State->PrestigeMachine->CollActive = -1;
    //       MapConfig.State->PrestigeMachine = NULL;
    //     }
    //     break;
    //   }
  }
}

//--------------------------------------------------------------------------
void gambitsInit(void)
{
  memset(&GambitsState, 0, sizeof(GambitsState));
  gambitsSetup();
}
