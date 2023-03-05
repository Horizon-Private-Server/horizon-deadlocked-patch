/***************************************************
 * FILENAME :		main.c
 * 
 * DESCRIPTION :
 * 		Unhooks patch file and spawns menu popup telling user patch is downloading.
 * 
 * NOTES :
 * 		Each offset is determined per app id.
 * 		This is to ensure compatibility between versions of Deadlocked/Gladiator.
 * 		
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */

#include <tamtypes.h>

#include <libdl/dl.h>
#include <libdl/player.h>
#include <libdl/pad.h>
#include <libdl/time.h>
#include <libdl/net.h>
#include <libdl/game.h>
#include <libdl/string.h>
#include <libdl/stdio.h>
#include <libdl/gamesettings.h>
#include <libdl/dialog.h>
#include <libdl/patch.h>
#include <libdl/ui.h>
#include <libdl/graphics.h>

#include "module.h"
#include "messageid.h"
#include "db.h"

int selfDestruct __attribute__((section(".config"))) = 0;

void client_destroy(void);
int client_init(void);
int client_get(char * path, char * output, int output_size);

int update_all(struct DbIndex* db)
{
  int i;
  char path[256];

  dbStateActive = 1;
  for (i = 0; i < db->ItemCount; ++i) {
    struct DbIndexItem* item = &db->Items[i];
    DPRINTF("downloading %s...\n", item->Name);

    if (dbStateCancel)
      break;

    snprintf(dbStateTitleStr, sizeof(dbStateTitleStr), "%s (%d/%d)", item->Name, i+1, db->ItemCount);
    int result = download_db_item(item);
    if (result < 0) {
      dbStateActive = 0;
      return -1;
      //scr_printf("error downloading %s (%d)\n", item->Name, result);
    }
  }
  dbStateActive = 0;

  return 0;
}

/*
 * NAME :		onOnlineMenu
 * 
 * DESCRIPTION :
 * 			Called every ui update.
 * 
 * NOTES :
 * 
 * ARGS : 
 * 
 * RETURN :
 * 
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */
void onOnlineMenu(void)
{
	u32 bgColorDownload = 0x80000000;
	u32 textColor = 0x80C0C0C0;
	u32 barBgColor = 0x80202020;
	u32 barFgColor = 0x80000040;

	// call normal draw routine
	((void (*)(void))0x00707F28)();

	// only show on main menu
	if (uiGetActive() != UI_ID_ONLINE_MAIN_MENU)
		return;

}

void draw(void)
{
  char etaStr[64];
	u32 bgColorDownload = 0x80000000;
	u32 textColor = 0x80C0C0C0;
	u32 barBgColor = 0x80202020;
	u32 barFgColor = 0x80000040;

  if (!dbStateActive)
    return;

  float secondsElapsed = (gameGetTime() - dbStateTimeStarted) / 1000.0;
  float downloaded = dbStateDownloadAmount / (float)dbStateDownloadTotal;
  int estimatedSecondsLeft = (int)((1 - downloaded) / (downloaded / secondsElapsed));
  int m = estimatedSecondsLeft / 60;
  int s = estimatedSecondsLeft % 60;
  snprintf(etaStr, sizeof(etaStr), "%02d:%02d remaining..", m, s);

	gfxScreenSpaceBox(0.2, 0.35, 0.6, 0.225, bgColorDownload);
	gfxScreenSpaceBox(0.2, 0.45, 0.6, 0.05, barBgColor);
	gfxScreenSpaceText(SCREEN_WIDTH * 0.5, SCREEN_HEIGHT * 0.35, 1, 1, textColor, dbStateTitleStr, -1, 1);
	gfxScreenSpaceText(SCREEN_WIDTH * 0.5, SCREEN_HEIGHT * 0.4, 1, 1, textColor, dbStateDescStr, -1, 1);
	gfxScreenSpaceText(SCREEN_WIDTH * 0.5, SCREEN_HEIGHT * 0.55, 1, 1, textColor, "\x12 Cancel", -1, 1 + 3);

	if (dbStateDownloadTotal > 0)
	{
		float w = (float)dbStateDownloadAmount / (float)dbStateDownloadTotal;
		gfxScreenSpaceBox(0.2, 0.45, 0.6 * w, 0.05, barFgColor);
	}
  
	gfxScreenSpaceText(SCREEN_WIDTH * 0.5, SCREEN_HEIGHT * 0.475, 1, 1, textColor, etaStr, -1, 1 + 3);
}

void cancel(void)
{
  int i;
  if (dbStateCancel)
    return;

  dbStateCancel = 1;

  // send request
  void * lobbyConnection = netGetLobbyServerConnection();
  if (lobbyConnection)
    netSendCustomAppMessage(NET_DELIVERY_CRITICAL, lobbyConnection, NET_LOBBY_CLIENT_INDEX, CUSTOM_MSG_ID_CLIENT_REQUEST_PATCH, 0, &i);

}

/*
 * NAME :		main
 * 
 * DESCRIPTION :
 * 			Entrypoint.
 * 
 * NOTES :
 * 
 * ARGS : 
 * 
 * RETURN :
 * 
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */
int main (void)
{
	int i;
  static int initialized = 0;
  static int running = 0;
  static struct DbIndex db;

  dlPreUpdate();

  if (!initialized) {
    initialized = 1;
      
    // stop server select
    //*(u32*)0x00138D7C = 0x0C04E138;

    // reset global net callbacks table ptr
    //*(u32*)0x00211E64 = 0;

    rpcUSBInit();
    padEnableInput();
  }

  if (!running && padGetButtonDown(0, PAD_L1) > 0) {
    running = 1;
    client_init();
    int parsed = parse_db(&db);
    if (parsed < 0) {
      DPRINTF("db parse error %d\n", parsed);
    } else {
		  padDisableInput();
      if (update_all(&db) < 0) {
        cancel();
      }
      padEnableInput();
    }

    running = 0;
  }
  
  draw();
  if (dbStateActive && !dbStateCancel && padGetButtonDown(0, PAD_TRIANGLE) > 0) {
    cancel();
    return 0;
  }
  else if (dbStateCancel) {
    padEnableInput();
    return 0;
  }

	// just clear if selfDestruct is true
	if (selfDestruct) {
    client_destroy();
		return;
	}

	// Hook menu loop
	*(u32*)0x00594CB8 = 0x0C000000 | ((u32)(&onOnlineMenu) / 4);

  dlPostUpdate();
	return 0;
}
