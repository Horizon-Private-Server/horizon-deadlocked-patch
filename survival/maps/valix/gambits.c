#include <libdl/utils.h>
#include <libdl/net.h>
#include <libdl/moby.h>
#include <libdl/random.h>
#include <libdl/stdio.h>
#include "../../../include/game.h"
#include "../../../include/mob.h"
#include "../../../include/mysterybox.h"
#include "valix.h"
#include "maputils.h"
#include "soulcollector.h"

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
void gambitsRandomizeJumpPads(void)
{
  int count = 0;
  Moby* m = mobyListGetStart();
  int cuboids[11];
  Moby* jumpPads[11] = {};

  // collect all jump pads
  while (count < 11 && (m = mobyFindNextByOClass(m, MOBY_ID_JUMP_PAD))) {
    jumpPads[count] = m;
    cuboids[count] = *(int*)(m->PVar + 24);
    ++m;
    ++count;
  }

  // randomize cuboids
  int i;
  for (i = 0; i < 100; ++i) {
    int i0 = randRangeInt(0, count-1);
    int i1 = randRangeInt(0, count-1);
    if (i0 == i1) continue;

    int swap = cuboids[i1];
    cuboids[i1] = cuboids[i0];
    cuboids[i0] = swap;
  }

  // assign
  for (i = 0; i < count; ++i) {
    Moby* jumpPad = jumpPads[i];
    if (!jumpPad || !jumpPad->PVar) continue;

    *(int*)(jumpPad->PVar + 24) = cuboids[i];
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

  // randomize
  if (gambit == GAMBIT_ID_RANDOMIZE_JUMP_PADS) {
    gambitsRandomizeJumpPads();
  }

  // mark complete after X rounds
  if (roundNo == 50) {
    switch (gambit)
    {
      case GAMBIT_ID_IMPOSSIBLE_MODE:
      case GAMBIT_ID_JUMP_PADS_ALWAYS_ACTIVE:
      case GAMBIT_ID_RANDOMIZE_JUMP_PADS:
      case GAMBIT_ID_HOVERBOOTS:
      case GAMBIT_ID_ARBITER_ONLY:
      case GAMBIT_ID_MAGMA_CANNON_ONLY:
      case GAMBIT_ID_MINES_ONLY:
        mapSendSendGambitCompletedMessage(gambit);
        break;
    }
  } else if (roundNo == 100) {
    switch (gambit)
    {
      case GAMBIT_ID_EASY_MODE:
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
      
      // reduce mob health scale
      for (i = 0; i < defaultSpawnParamsCount; ++i) {
        defaultSpawnParams[i].Config.HealthScale *= 0.5;
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

      MapConfig.CreateMobDropFunc = &gambitsDropCreate;
      GambitsState.FinishedSetup = 1;
      break;
    }
    case GAMBIT_ID_JUMP_PADS_ALWAYS_ACTIVE:
    {
      GambitsState.FinishedSetup = 1;
      break;
    }
    case GAMBIT_ID_RANDOMIZE_JUMP_PADS:
    {
      GambitsState.FinishedSetup = 1;
      break;
    }
    case GAMBIT_ID_HOVERBOOTS:
    {
      if (!MapConfig.State) return;

      // set hoverboots perks
      for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
        MapConfig.State->PlayerStates[i].State.ItemStackable[STACKABLE_ITEM_HOVERBOOTS] = 10;
      }

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
    case GAMBIT_ID_MAGMA_CANNON_ONLY:
    {
      gameOptions->WeaponFlags.DualVipers = 0;
      gameOptions->WeaponFlags.MagmaCannon = 1;
      gameOptions->WeaponFlags.Arbiter = 0;
      gameOptions->WeaponFlags.FusionRifle = 0;
      gameOptions->WeaponFlags.MineLauncher = 0;
      gameOptions->WeaponFlags.B6 = 0;
      gameOptions->WeaponFlags.Holoshield = 0;
      gameOptions->WeaponFlags.Flail = 0;
      gambitsSetupSingleWeaponRestriction(WEAPON_ID_MAGMA_CANNON);
      GambitsState.FinishedSetup = 1;
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
    case GAMBIT_ID_EASY_MODE:
    {
      soulcollectorActivateAll();
      break;
    }
    case GAMBIT_ID_IMPOSSIBLE_MODE:
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
    case GAMBIT_ID_JUMP_PADS_ALWAYS_ACTIVE:
    {
      soulcollectorActivateAll();
      break;
    }
    case GAMBIT_ID_ARBITER_ONLY:
    {
      mapEnforceSingleWeaponRestriction(WEAPON_ID_ARBITER);
      break;
    }
    case GAMBIT_ID_MAGMA_CANNON_ONLY:
    {
      mapEnforceSingleWeaponRestriction(WEAPON_ID_MAGMA_CANNON);
      break;
    }
    case GAMBIT_ID_MINES_ONLY:
    {
      mapEnforceSingleWeaponRestriction(WEAPON_ID_MINE_LAUNCHER);
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
