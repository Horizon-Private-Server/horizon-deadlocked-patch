#include <string.h>
#include <libdl/stdio.h>
#include <libdl/game.h>
#include <libdl/collision.h>
#include <libdl/stdlib.h>
#include <libdl/color.h>
#include <libdl/moby.h>
#include <libdl/radar.h>
#include <libdl/sound.h>
#include <libdl/random.h>
#include <libdl/utils.h>
#include <libdl/net.h>
#include <libdl/ui.h>
#include <libdl/graphics.h>
#include "include/utils.h"
#include "include/game.h"
#include "include/bank.h"
#include "include/mob.h"
#include "include/loot.h"
#include "config.h"
#include "common.h"
#include "window.h"

struct TrackerState {
  long TimeStarted;
  u32 PlayerLevels;
  u32 WeaponLevels[WEAPON_SLOT_COUNT-1];
  u32 LootDrops;
  u32 Kills;
  u64 Bolts;
  u64 PlayerXp;
  u64 WeaponXp[WEAPON_SLOT_COUNT-1];
  char Enabled;
} TrackerState;

//--------------------------------------------------------------------------
double trackerGetTimeElapsed(void)
{
  long currentTime = timerGetSystemTime();
  return (currentTime - TrackerState.TimeStarted) / (double)(SYSTEM_TIME_TICKS_PER_MS * 1000.0); // convert to seconds
}

//--------------------------------------------------------------------------
void trackerDraw(void)
{
  const u32 textColor = 0x80FFFFFF;
  double dt = trackerGetTimeElapsed();
  Player* localPlayer = playerGetFromSlot(0);
  int weaponId = localPlayer->WeaponHeldId;
  char strBuf[64];
  Window_t drawWindow;

  windowCreate(&drawWindow, 10, SCREEN_HEIGHT-10, 0, 0, 200, 60, TEXT_ALIGN_BOTTOMLEFT, 1);
  //windowFill(&drawWindow, 0x40000000);

  // time
  snprintf(strBuf, sizeof(strBuf), "Time: %d:%02d:%02d", (int)dt / 3600, ((int)dt / 60) % 60, (int)dt % 60);
  windowDrawText(&drawWindow, TEXT_ALIGN_TOPLEFT, 0, 0, 0.7, textColor, strBuf, -1, TEXT_ALIGN_TOPLEFT);
  windowMove(&drawWindow, 0, 10);

  // kills
  snprintf(strBuf, sizeof(strBuf), "Kills: %'d (%.2f /min)", TrackerState.Kills, (float)((TrackerState.Kills * 60) / dt));
  windowDrawText(&drawWindow, TEXT_ALIGN_TOPLEFT, 0, 0, 0.7, textColor, strBuf, -1, TEXT_ALIGN_TOPLEFT);
  windowMove(&drawWindow, 0, 10);

  // bolts
  snprintf(strBuf, sizeof(strBuf), "Bolts: %'ld (%'d /min)", TrackerState.Bolts, (int)((TrackerState.Bolts * 60) / dt));
  windowDrawText(&drawWindow, TEXT_ALIGN_TOPLEFT, 0, 0, 0.7, textColor, strBuf, -1, TEXT_ALIGN_TOPLEFT);
  windowMove(&drawWindow, 0, 10);

  // loot drops
  snprintf(strBuf, sizeof(strBuf), "Loot Drops: %'d (%.1f /min)", TrackerState.LootDrops, (float)((TrackerState.LootDrops * 60) / dt));
  windowDrawText(&drawWindow, TEXT_ALIGN_TOPLEFT, 0, 0, 0.7, textColor, strBuf, -1, TEXT_ALIGN_TOPLEFT);
  windowMove(&drawWindow, 0, 10);

  // xp
  snprintf(strBuf, sizeof(strBuf), "XP: (+%dLVL) %'ld (%'d /min)", TrackerState.PlayerLevels, TrackerState.PlayerXp, (int)((TrackerState.PlayerXp * 60) / dt));
  windowDrawText(&drawWindow, TEXT_ALIGN_TOPLEFT, 0, 0, 0.7, textColor, strBuf, -1, TEXT_ALIGN_TOPLEFT);
  windowMove(&drawWindow, 0, 10);

  // weapon stats
  int slot = weaponIdToSlot(weaponId) - 1;
  if (slot >= 0) {
    struct GadgetDef* gadgetDef = weaponGetDef(weaponId, 0);
    char* weaponName = uiMsgString(gadgetDef->quickSelectTag);
    snprintf(strBuf, sizeof(strBuf), "%s (+%dP) XP: %'ld (%.0f /min)", weaponName, TrackerState.WeaponLevels[slot], TrackerState.WeaponXp[slot], (float)((TrackerState.WeaponXp[slot] * 60) / dt));
    windowDrawText(&drawWindow, TEXT_ALIGN_TOPLEFT, 0, 0, 0.7, textColor, strBuf, -1, TEXT_ALIGN_TOPLEFT);
    windowMove(&drawWindow, 0, 10);
  }
}

//--------------------------------------------------------------------------
void trackerLogKill(int killedByPlayerId, u64 bolts, u64 playerXp, u64 weaponXp, int weaponId)
{
  if (!TrackerState.Enabled) return;

  // log local kills
  Player* player = playerGetAll()[killedByPlayerId];
  if (player && player->IsLocal) {
    TrackerState.Kills++;
  }

  // log rewards
  TrackerState.PlayerXp += playerXp;
  TrackerState.Bolts += bolts;

  // log weapon XP
  int slot = weaponIdToSlot(weaponId) - 1;
  if (slot >= 0)
    TrackerState.WeaponXp[slot] += weaponXp;
}

//--------------------------------------------------------------------------
void trackerLogPlayerLevelUp(void)
{
  if (!TrackerState.Enabled) return;

  TrackerState.PlayerLevels++;
}

//--------------------------------------------------------------------------
void trackerLogWeaponLevelUp(int weaponId)
{
  if (!TrackerState.Enabled) return;

  int slot = weaponIdToSlot(weaponId) - 1;
  if (slot < 0) return;

  TrackerState.WeaponLevels[slot]++;
}

//--------------------------------------------------------------------------
void trackerLogLootDrop(void)
{
  if (!TrackerState.Enabled) return;

  TrackerState.LootDrops++;
}

//--------------------------------------------------------------------------
void trackerReset(void)
{
  TrackerState.Kills = 0;
  TrackerState.Bolts = 0;
  TrackerState.LootDrops = 0;
  TrackerState.PlayerXp = 0;
  TrackerState.PlayerLevels = 0;
  memset(TrackerState.WeaponLevels, 0, sizeof(TrackerState.WeaponLevels));
  memset(TrackerState.WeaponXp, 0, sizeof(TrackerState.WeaponXp));
  TrackerState.TimeStarted = timerGetSystemTime();
}

//--------------------------------------------------------------------------
void trackerTick(void)
{
  if (!isInGame()) return;

  // track level ups
  static int lastPlayerLevel = 0;
  int currPlayerLevel = State.LocalPlayerState->State.Level;
  if (currPlayerLevel > lastPlayerLevel) {
    trackerLogPlayerLevelUp();
    lastPlayerLevel = currPlayerLevel;
  }

  // track level ups
  static int lastWeaponLevel[WEAPON_SLOT_COUNT-1] = {};
  int i;
  for (i = 0; i < WEAPON_SLOT_COUNT-1; ++i) {
    int weaponId = weaponSlotToId(i + 1);
    double currWeaponXp = hasMapConfig() ? mapConfig->BankVTable->GetWeaponXP(weaponId) : 0;
    int currWeaponLevel = getProficiencyFromXp(currWeaponXp);
    if (currWeaponLevel > lastWeaponLevel[i]) {
      trackerLogWeaponLevelUp(weaponId);
      lastWeaponLevel[i] = currWeaponLevel;
    }
  }

  if (!TrackerState.Enabled) {
    if (State.MenuOpen == RAIDS_CUSTOM_MENU_NONE && !gameIsAnyStartMenuOpen() && !PATCH_POINTERS_PATCHMENU && padGetButtonDown(0, PAD_DOWN)) {
      TrackerState.Enabled = 1;
      trackerReset();
    }
    return;
  } else {
    if (State.MenuOpen == RAIDS_CUSTOM_MENU_NONE && !gameIsAnyStartMenuOpen() && !PATCH_POINTERS_PATCHMENU && padGetButtonDown(0, PAD_DOWN)) {
      TrackerState.Enabled = 0;
      trackerReset();
    }
  }

  // draw
  trackerDraw();
}

//--------------------------------------------------------------------------
void trackerInit(void)
{
  //memset(&TrackerState, 0, sizeof(TrackerState));
}
