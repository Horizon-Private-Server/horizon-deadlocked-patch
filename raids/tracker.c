#include <string.h>
#include <libdl/stdio.h>
#include <libdl/game.h>
#include <libdl/pad.h>
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
  int LastPlayerLevel;
  char HasPlayerLevelSnapshot;
  u32 LootDrops;
  u32 Kills;
  u64 Bolts;
  u64 PlayerXp;
  u64 ClassXp[BANK_MAX_CLASSES];
  u32 ClassLevels[BANK_MAX_CLASSES];
  u64 LastClassXp[BANK_MAX_CLASSES];
  char HasClassXpSnapshot;
  double WeaponExperience;
  double LastWeaponExperience;
  u32 WeaponLevels;
  char HasWeaponXpSnapshot;
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
  if (!TrackerState.Enabled || !isInGame()) return;
  const u32 textColor = 0x80FFFFFF;
  double dt = trackerGetTimeElapsed();
  Player* localPlayer = playerGetFromSlot(0);
  if (!localPlayer) return;
  char strBuf[128];
  Window_t drawWindow;

  const float left = 10;
  const float bottom = SCREEN_HEIGHT - 10;
  const float height = 70;
  windowCreate(&drawWindow, left, bottom, 0, 0, 200, height, TEXT_ALIGN_BOTTOMLEFT, 1);
  //windowFill(&drawWindow, 0x40000000);

  // draw additional map-owned tracker hud
  if (hasMapConfig() && mapConfig->Functions.OnTrackerDrawHudFunc)
    mapConfig->Functions.OnTrackerDrawHudFunc(left, bottom - height - 8);

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
  snprintf(strBuf, sizeof(strBuf), "Hero XP: (+%dLVL) %'ld (%'d /min)", TrackerState.PlayerLevels, TrackerState.PlayerXp, (int)((TrackerState.PlayerXp * 60) / dt));
  windowDrawText(&drawWindow, TEXT_ALIGN_TOPLEFT, 0, 0, 0.7, textColor, strBuf, -1, TEXT_ALIGN_TOPLEFT);
  windowMove(&drawWindow, 0, 10);

  // class XP
  strcpy(strBuf, "Class XP: --");
  if (hasMapConfig()) {
    struct BankVTable* bank = mapConfig->BankVTable;
    if (bank->GetHasAccount() && !bank->HasPendingAccountRequest()) {
      RaidsPlayerBank_t* localBank = bank->GetLocalBank();
      int classType = localBank ? localBank->Account.EquippedClass : -1;
      const RaidsClassDef_t* def = classType >= 0 && classType < BANK_MAX_CLASSES ? bank->GetClassDefinition(classType) : NULL;
      if (def) {
        double classXpPerMinute = dt > 0 ? (double)TrackerState.ClassXp[classType] * 60 / dt : 0;
        snprintf(strBuf, sizeof(strBuf), "%s (+%dLVL) Class XP: %'llu (%.0f /min)", def->Name, TrackerState.ClassLevels[classType], (unsigned long long)TrackerState.ClassXp[classType], classXpPerMinute);
      }
    }
  }
  windowDrawText(&drawWindow, TEXT_ALIGN_TOPLEFT, 0, 0, 0.7, textColor, strBuf, -1, TEXT_ALIGN_TOPLEFT);
  windowMove(&drawWindow, 0, 10);

  snprintf(strBuf, sizeof(strBuf), "Weapon XP: (+%uP) %.0f (%.0f /min)",
    TrackerState.WeaponLevels, TrackerState.WeaponExperience,
    dt > 0 ? TrackerState.WeaponExperience * 60 / dt : 0);
  windowDrawText(&drawWindow, TEXT_ALIGN_TOPLEFT, 0, 0, 0.7, textColor, strBuf, -1, TEXT_ALIGN_TOPLEFT);
}

//--------------------------------------------------------------------------
void trackerLogKill(int killedByPlayerId, u64 bolts, u64 playerXp)
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

}

//--------------------------------------------------------------------------
void trackerLogLootDrop(void)
{
  if (!TrackerState.Enabled) return;

  TrackerState.LootDrops++;
}

//--------------------------------------------------------------------------
void trackerUpdateClassXp(void)
{
  if (!hasMapConfig()) return;

  struct BankVTable* bank = mapConfig->BankVTable;
  if (!bank->GetHasAccount() || bank->HasPendingAccountRequest()) return;

  int i;
  for (i = 0; i < BANK_MAX_CLASSES; ++i) {
    u64 xp = bank->GetClassXP(i);
    if (TrackerState.Enabled && TrackerState.HasClassXpSnapshot && xp > TrackerState.LastClassXp[i]) {
      TrackerState.ClassXp[i] += xp - TrackerState.LastClassXp[i];
      RaidsPlayerClassProgress_t playerClass = { .ClassType = i };
      int levels = bank->GetClassLevel(&playerClass, xp) - bank->GetClassLevel(&playerClass, TrackerState.LastClassXp[i]);
      if (levels > 0) TrackerState.ClassLevels[i] += levels;
    }
    TrackerState.LastClassXp[i] = xp;
  }
  TrackerState.HasClassXpSnapshot = 1;
}

//--------------------------------------------------------------------------
void trackerUpdatePlayerLevel(void)
{
  if (!hasMapConfig() || !mapConfig->BankVTable || !State.LocalPlayerState) return;
  struct BankVTable* bank = mapConfig->BankVTable;
  if (!bank->GetHasAccount() || bank->HasPendingAccountRequest()) return;

  int level = State.LocalPlayerState->State.Level;
  if (TrackerState.Enabled && TrackerState.HasPlayerLevelSnapshot && level > TrackerState.LastPlayerLevel)
    TrackerState.PlayerLevels += level - TrackerState.LastPlayerLevel;
  TrackerState.LastPlayerLevel = level;
  TrackerState.HasPlayerLevelSnapshot = 1;
}

//--------------------------------------------------------------------------
void trackerUpdateWeaponXp(void)
{
  if (!hasMapConfig() || !mapConfig->BankVTable) return;
  struct BankVTable* bank = mapConfig->BankVTable;
  if (!bank->GetHasAccount() || bank->HasPendingAccountRequest()) return;
  RaidsPlayerBank_t* localBank = bank->GetLocalBank();
  if (!localBank) return;

  double xp = localBank->Account.WeaponExperience;
  if (TrackerState.Enabled && TrackerState.HasWeaponXpSnapshot && xp > TrackerState.LastWeaponExperience) {
    TrackerState.WeaponExperience += xp - TrackerState.LastWeaponExperience;
    int levels = getProficiencyFromXp(xp) - getProficiencyFromXp(TrackerState.LastWeaponExperience);
    if (levels > 0) TrackerState.WeaponLevels += levels;
  }
  TrackerState.LastWeaponExperience = xp;
  TrackerState.HasWeaponXpSnapshot = 1;
}

//--------------------------------------------------------------------------
void trackerReset(void)
{
  TrackerState.Kills = 0;
  TrackerState.Bolts = 0;
  TrackerState.LootDrops = 0;
  TrackerState.PlayerXp = 0;
  memset(TrackerState.ClassXp, 0, sizeof(TrackerState.ClassXp));
  memset(TrackerState.ClassLevels, 0, sizeof(TrackerState.ClassLevels));
  TrackerState.HasClassXpSnapshot = 0;
  trackerUpdateClassXp();
  TrackerState.WeaponExperience = 0;
  TrackerState.WeaponLevels = 0;
  TrackerState.HasWeaponXpSnapshot = 0;
  trackerUpdateWeaponXp();
  TrackerState.PlayerLevels = 0;
  TrackerState.HasPlayerLevelSnapshot = 0;
  trackerUpdatePlayerLevel();
  TrackerState.TimeStarted = timerGetSystemTime();
}

//--------------------------------------------------------------------------
void trackerTick(void)
{
  if (!isInGame()) return;

  trackerUpdateClassXp();
  trackerUpdateWeaponXp();
  trackerUpdatePlayerLevel();


  if (!TrackerState.Enabled) {
    if (State.MenuOpen == RAIDS_CUSTOM_MENU_NONE && !gameIsAnyStartMenuOpen() && !PATCH_POINTERS_PATCHMENU
        && localPlayerHasInput() && padGetButtonDown(0, PAD_DOWN) > 0) {
      TrackerState.Enabled = 1;
      trackerReset();
    }
    return;
  } else {
    if (State.MenuOpen == RAIDS_CUSTOM_MENU_NONE && !gameIsAnyStartMenuOpen() && !PATCH_POINTERS_PATCHMENU
        && localPlayerHasInput() && padGetButtonDown(0, PAD_DOWN) > 0) {
      TrackerState.Enabled = 0;
      trackerReset();
    }
  }
}

//--------------------------------------------------------------------------
void trackerInit(void)
{
  //memset(&TrackerState, 0, sizeof(TrackerState));
}
