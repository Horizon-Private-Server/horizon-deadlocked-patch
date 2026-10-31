#include <libdl/game.h>
#include <libdl/net.h>
#include <libdl/pad.h>
#include <libdl/stdio.h>
#include <libdl/time.h>
#include <libdl/utils.h>
#include "config.h"
#include "messageid.h"
#include "include/game.h"
#include "include/menu.h"
#include "include/settings.h"
#include "include/utils.h"

#define SETTINGS_RETRY_TIME              ((long)5 * TIME_SECOND * SYSTEM_TIME_TICKS_PER_MS)
#define SETTINGS_SAVE_DELAY              ((long)500 * SYSTEM_TIME_TICKS_PER_MS)

extern struct RaidsState State;
extern int Initialized;

// defaults until account settings are loaded
RaidsUserSettings_t raidsUserSettings = {
  .Version = RAIDS_USER_SETTINGS_VERSION,
  .MapLod = 2,
  .MobLod = 2,
  .AmmoDropPopups = 0
};

RaidsUserSettings_t raidsSettingsResponse;
RaidsUserSettings_t raidsSettingsPendingSave;
volatile int raidsSettingsHasResponse = 0;
volatile int raidsSettingsAckRevision = 0;
int raidsSettingsLoaded = 0;
int raidsSettingsRevision = 0;
int raidsSettingsSavedRevision = 0;
int raidsSettingsSentRevision = 0;
int raidsSettingsSelectedRow = 0;
int raidsSettingsReturnPending = 0;
long raidsSettingsLastRequest = 0;
long raidsSettingsLastSave = 0;
long raidsSettingsLastEdit = 0;

char* raidsSettingsLodNames[] = {
  "Potato",
  "Low",
  "Normal",
  "High"
};

char* raidsSettingsLabels[] = {
  "Map Level of Detail",
  "Mob Level of Detail",
  "Ammo Pickup Popups"
};

char* raidsSettingsDescriptions[] = {
  "Lower map detail to reduce graphics load on demanding planets.",
  "Lower mob detail to reduce graphics load when many enemies are present.",
  "Show the ammo gained when you collect an ammo pickup."
};

//--------------------------------------------------------------------------
int raidsSettingsShouldCloseMenu(void)
{
  return !Initialized || !isInGame() || !hasMapConfig() || gameHasEnded()
      || hasPendingWorldHop() || gameIsAnyStartMenuOpen() || PATCH_POINTERS_PATCHMENU != 0;
}

//--------------------------------------------------------------------------
void raidsSettingsSave(void)
{
  if (!raidsSettingsLoaded || raidsSettingsSavedRevision == raidsSettingsRevision) return;

  void* connection = netGetLobbyServerConnection();
  if (!connection) return;

  // wait before retrying an unconfirmed save
  long now = timerGetSystemTime();
  if (raidsSettingsSentRevision > raidsSettingsSavedRevision && now - raidsSettingsLastSave < SETTINGS_RETRY_TIME) return;

  // keep the same snapshot until the server confirms it
  if (raidsSettingsSentRevision <= raidsSettingsSavedRevision) {
    raidsSettingsSentRevision = raidsSettingsRevision;
    raidsSettingsPendingSave = raidsUserSettings;
  }

  // send pending settings
  struct RaidsSaveSettingsRequest msg = {
    .Settings = raidsSettingsPendingSave,
    .AckAddress = (u32)&raidsSettingsAckRevision,
    .Revision = raidsSettingsSentRevision
  };

  raidsSettingsLastSave = now;
  netSendCustomAppMessage(NET_DELIVERY_CRITICAL, connection, NET_LOBBY_CLIENT_INDEX,
    CUSTOM_MSG_ID_RAIDS_SAVE_SETTINGS_REQUEST, sizeof(msg), &msg);
}

//--------------------------------------------------------------------------
void raidsSettingsOpen(void)
{
  if (raidsSettingsShouldCloseMenu() || State.MenuOpen != RAIDS_CUSTOM_MENU_INVENTORY) return;

  State.MenuOpen = RAIDS_CUSTOM_MENU_SETTINGS;
  raidsSettingsReturnPending = 0;
  padDisableInput();
}

//--------------------------------------------------------------------------
void raidsSettingsClose(int returnToInventory)
{
  if (State.MenuOpen != RAIDS_CUSTOM_MENU_SETTINGS) return;

  raidsSettingsSave();
  raidsSettingsReturnPending = 0;
  State.MenuOpen = returnToInventory ? RAIDS_CUSTOM_MENU_INVENTORY : RAIDS_CUSTOM_MENU_NONE;

  // inventory keeps ownership of player input when returning
  if (!returnToInventory) padEnableInput();
}

//--------------------------------------------------------------------------
void raidsSettingsHandleInput(void)
{
  if (State.MenuOpen != RAIDS_CUSTOM_MENU_SETTINGS) return;

  if (raidsSettingsShouldCloseMenu()) {
    raidsSettingsClose(0);
    return;
  }

  // wait for release so inventory does not also handle the back button
  if (raidsSettingsReturnPending) {
    if (padGetButton(0, PAD_TRIANGLE) <= 0) raidsSettingsClose(1);
    return;
  }

  // handle close input
  if (padGetButtonDown(0, PAD_TRIANGLE) > 0) {
    raidsSettingsSave();
    raidsSettingsReturnPending = 1;
    return;
  }

  if (!raidsSettingsLoaded) return;

  // handle navigation
  if (padGetButtonDown(0, PAD_UP) > 0) {
    raidsSettingsSelectedRow = (raidsSettingsSelectedRow + 2) % 3;
  } else if (padGetButtonDown(0, PAD_DOWN) > 0) {
    raidsSettingsSelectedRow = (raidsSettingsSelectedRow + 1) % 3;
  } else {
    int direction = 0;
    if (padGetButtonDown(0, PAD_LEFT) > 0) direction = -1;
    else if (padGetButtonDown(0, PAD_RIGHT) > 0 || padGetButtonDown(0, PAD_CROSS) > 0) direction = 1;
    if (!direction) return;

    // select setting and available values
    int* value = &raidsUserSettings.MapLod;
    int count = 3;
#if DEBUG
    count = 4;
#endif

    if (raidsSettingsSelectedRow == 1) {
      value = &raidsUserSettings.MobLod;
      count = 4;
    } else if (raidsSettingsSelectedRow == 2) {
      value = &raidsUserSettings.AmmoDropPopups;
      count = 2;
    }

    // apply change and mark settings for saving
    *value = (*value + direction + count) % count;
    ++raidsSettingsRevision;
    raidsSettingsLastEdit = timerGetSystemTime();

    Player* player = playerGetFromSlot(0);
    if (player) playEquipSound(player);
  }
}

//--------------------------------------------------------------------------
void raidsSettingsTick(void)
{
  long now = timerGetSystemTime();

  // load account settings
  if (!raidsSettingsLoaded) {
    if (raidsSettingsHasResponse) {
      // copy the response once so late retries cannot overwrite local edits
      // validate values before using them to index menu labels
      if (raidsSettingsResponse.Version == RAIDS_USER_SETTINGS_VERSION
          && raidsSettingsResponse.MapLod >= 0 && raidsSettingsResponse.MapLod <= 3
          && raidsSettingsResponse.MobLod >= 0 && raidsSettingsResponse.MobLod <= 3
          && raidsSettingsResponse.AmmoDropPopups >= 0 && raidsSettingsResponse.AmmoDropPopups <= 1) {
        raidsUserSettings = raidsSettingsResponse;
        raidsSettingsLoaded = 1;
      } else {
        raidsSettingsHasResponse = 0;
      }
    }

    // retry if the account response timed out
    if (!raidsSettingsLoaded && (!raidsSettingsLastRequest || now - raidsSettingsLastRequest >= SETTINGS_RETRY_TIME)) {
      void* connection = netGetLobbyServerConnection();
      if (connection) {
        struct RaidsGetSettingsRequest msg = {
          .DestAddress = (u32)&raidsSettingsResponse,
          .DestHasFlagAddress = (u32)&raidsSettingsHasResponse
        };

        raidsSettingsLastRequest = now;
        netSendCustomAppMessage(NET_DELIVERY_CRITICAL, connection, NET_LOBBY_CLIENT_INDEX,
          CUSTOM_MSG_ID_RAIDS_GET_SETTINGS_REQUEST, sizeof(msg), &msg);
      }
    }
  }

  // accept confirmed saves without moving back to an older revision
  int ack = raidsSettingsAckRevision;
  if (ack > raidsSettingsSavedRevision && ack <= raidsSettingsSentRevision)
    raidsSettingsSavedRevision = ack;

  // wait for input to settle before saving
  if (now - raidsSettingsLastEdit >= SETTINGS_SAVE_DELAY) raidsSettingsSave();
  raidsSettingsHandleInput();
}

//--------------------------------------------------------------------------
void raidsSettingsDraw(void)
{
  if (State.MenuOpen != RAIDS_CUSTOM_MENU_SETTINGS || raidsSettingsShouldCloseMenu()) return;

  const u32 selectedColor = 0x40008080; // yellow
  Window_t window;
  Window_t footer;

  // draw frame
  windowCreate(&window, SCREEN_WIDTH * 0.5, SCREEN_HEIGHT * 0.5, 0, 0, 420, 196, TEXT_ALIGN_MIDDLECENTER, 1);
  menuDrawFrame(&window, "Raids Settings", RAIDS_MENU_TITLE_HEIGHT);

  // draw footer
  windowCreateFrom(&footer, &window, 0, 0, window.Width, RAIDS_MENU_FOOTER_HEIGHT, TEXT_ALIGN_BOTTOMCENTER);
  menuDrawFooter(&footer, "\x1B \x1A CHANGE    \x10 CHANGE    \x12 BACK");

  if (!raidsSettingsLoaded) {
    windowDrawText(&window, TEXT_ALIGN_TOPLEFT, 16, 48, 0.8, RAIDS_MENU_SECONDARY_COLOR,
      "Loading account settings...", -1, TEXT_ALIGN_TOPLEFT);
    return;
  }

  // draw settings
  int i;
  for (i = 0; i < 3; ++i) {
    float y = 36 + 20*i;
    if (i == raidsSettingsSelectedRow)
      windowDrawBox(&window, TEXT_ALIGN_TOPLEFT, 8, y - 4, window.Width - 16, 18, selectedColor, TEXT_ALIGN_TOPLEFT);

    windowDrawText(&window, TEXT_ALIGN_TOPLEFT, 16, y, 0.8, RAIDS_MENU_TEXT_COLOR,
      raidsSettingsLabels[i], -1, TEXT_ALIGN_TOPLEFT);

    char* value = i == 0 ? raidsSettingsLodNames[raidsUserSettings.MapLod]
      : i == 1 ? raidsSettingsLodNames[raidsUserSettings.MobLod]
      : raidsUserSettings.AmmoDropPopups ? "On" : "Off";

    windowDrawText(&window, TEXT_ALIGN_TOPLEFT, window.Width - 16, y, 0.8, RAIDS_MENU_TEXT_COLOR,
      value, -1, TEXT_ALIGN_TOPRIGHT);
  }

  // draw selected setting description above the footer
  windowDrawText(&window, TEXT_ALIGN_BOTTOMLEFT, 16, -RAIDS_MENU_FOOTER_HEIGHT - RAIDS_MENU_PADDING, 0.65, RAIDS_MENU_SECONDARY_COLOR,
    raidsSettingsDescriptions[raidsSettingsSelectedRow], -1, TEXT_ALIGN_BOTTOMLEFT);
}

//--------------------------------------------------------------------------
void raidsSettingsAmmoPopup(int playerId, int messageId, int amount)
{
  if (raidsUserSettings.AmmoDropPopups)
    ((void (*)(int, int, int))0x005401c8)(playerId, messageId, amount);
}

//--------------------------------------------------------------------------
void raidsSettingsMapInit(void)
{
  // hook pickup messages on each map load
  // pickup code already computes the actual ammo gained before this call
  HOOK_JAL(0x003ac400, &raidsSettingsAmmoPopup);
}
