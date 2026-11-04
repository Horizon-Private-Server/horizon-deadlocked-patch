#include <libdl/game.h>
#include <libdl/hud.h>
#include <libdl/math.h>
#include <libdl/stdio.h>
#include <libdl/string.h>
#include <libdl/time.h>
#include "config.h"
#include "common.h"
#include "include/game.h"
#include "include/settings.h"
#include "include/utils.h"
#include "include/xp.h"

#define XP_POPUP_LIFETIME_MS (1250)
#define XP_POPUP_POP_MS      (150)
#define XP_POPUP_FADE_MS     (350)

extern struct RaidsState State;

struct {
  u64 Xp;
  int LastKillTime;
  int Active;
} raidsXpPopup;

//--------------------------------------------------------------------------
void raidsXpInit(void)
{
  memset(&raidsXpPopup, 0, sizeof(raidsXpPopup));
}

//--------------------------------------------------------------------------
void raidsXpNotifyKill(u64 xp)
{
  if (!(raidsUserSettings.Flags & RAIDS_USER_SETTINGS_KILL_XP_POPUPS)) return;
  if (!xp) return;

  int now = gameGetTime();
  int age = now - raidsXpPopup.LastKillTime;
  if (!raidsXpPopup.Active || age < 0 || age >= XP_POPUP_LIFETIME_MS)
    raidsXpInit();

  // roll rapid kills into the same popup and restart its animation
  raidsXpPopup.Xp += xp;
  raidsXpPopup.LastKillTime = now;
  raidsXpPopup.Active = 1;
}

//--------------------------------------------------------------------------
void raidsXpDraw(void)
{
  if (!raidsXpPopup.Active) return;

  int age = gameGetTime() - raidsXpPopup.LastKillTime;
  PlayerHUDFlags* hudFlags = hudGetPlayerFlags(0);
  if (age < 0 || age >= XP_POPUP_LIFETIME_MS
      || !(raidsUserSettings.Flags & RAIDS_USER_SETTINGS_KILL_XP_POPUPS)
      || State.GameOver || State.MenuOpen != RAIDS_CUSTOM_MENU_NONE
      || gameIsAnyStartMenuOpen() || PATCH_POINTERS_PATCHMENU != 0
      || hasPendingWorldHop() || !hudFlags || !hudFlags->Flags.Raw) {
    raidsXpInit();
    return;
  }

  float pop = 1 - clamp(age / (float)XP_POPUP_POP_MS, 0, 1);
  float scale = 0.75f + 0.15f * pop;
  float y = -30 - 8 * (age / (float)XP_POPUP_LIFETIME_MS);
  u32 alpha = (u32)(128 * clamp((XP_POPUP_LIFETIME_MS - age) / (float)XP_POPUP_FADE_MS, 0, 1));
  char buf[64];

  snprintf(buf, sizeof(buf), "+%llu XP", (unsigned long long)raidsXpPopup.Xp);
  gfxHelperDrawText(SCREEN_WIDTH * 0.5f, SCREEN_HEIGHT * 0.5f, 40, y, scale,
    (alpha << 24) | 0x0080E0FF, buf, -1, TEXT_ALIGN_MIDDLELEFT, COMMON_DZO_DRAW_NORMAL);
}
