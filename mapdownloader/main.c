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

int showResultPopup = 0;
int confirmUpdatePopup = 0;
int confirmUpdatePopupResult = 0;
int numMapsUpdated = 0;
int numMapsToUpdate = 0;

int count_updates(struct DbIndex* db)
{
  int count = 0;
  int i;

  for ( i = 0; i < db->ItemCount; ++i ) {
    if ( db->Items[i].LocalVersion != db->Items[i].RemoteVersion ) {
      count += 1;
    }
  }

  return count;
}

int update_all(struct DbIndex* db)
{
  int i;
  char path[256];

  dbStateActive = 1;
  for (i = 0; i < db->ItemCount; ++i) {
    struct DbIndexItem* item = &db->Items[i];
    if (item->RemoteVersion != item->LocalVersion) {

      DPRINTF("downloading %s...\n", item->Name);
      if (dbStateCancel)
        break;

      snprintf(dbStateTitleStr, sizeof(dbStateTitleStr), "%s (%d/%d)", item->Name, numMapsUpdated+1, numMapsToUpdate);
      int result = download_db_item(item);
      if (result < 0) {
        dbStateActive = 0;
        return -1;
      } else {
        ++numMapsUpdated;
      }
    }
  }

  // successfully updated
  // update global version to latest
  db_write_global_maps_version(db->GlobalVersion);

  dbStateActive = 0;
  return 0;
}

/*
 * NAME :		drawDownloadPopup
 * 
 * DESCRIPTION :
 * 			
 * 
 * NOTES :
 * 
 * ARGS : 
 * 
 * RETURN :
 * 
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */
void drawDownloadPopup(void)
{
  char etaStr[64];
	u32 bgColorDownload = 0x80000000;
	u32 textColor = 0x80C0C0C0;
	u32 barBgColor = 0x80202020;
	u32 barFgColor = 0x80000040;

  if (!dbStateActive)
    return;

  if (dbStateTimeStarted > 0) {
    float secondsElapsed = (gameGetTime() - dbStateTimeStarted) / 1000.0;
    float downloaded = dbStateDownloadAmount / (float)dbStateDownloadTotal;
    float kbps = (dbStateDownloadAmount / secondsElapsed) / 1024.0;
    int estimatedSecondsLeft = (int)((1 - downloaded) / (downloaded / secondsElapsed));
    int m = estimatedSecondsLeft / 60;
    int s = estimatedSecondsLeft % 60;
    snprintf(etaStr, sizeof(etaStr), "%02d:%02d remaining.. (%.2f kbps)", m, s, kbps);
  } else {
    etaStr[0] = 0;
  }

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

/*
 * NAME :		downloadPatch
 * 
 * DESCRIPTION :
 * 			
 * 
 * NOTES :
 * 
 * ARGS : 
 * 
 * RETURN :
 * 
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */
void downloadPatch(void)
{
  int i;

  // send request
  void * lobbyConnection = netGetLobbyServerConnection();
  if (lobbyConnection)
    netSendCustomAppMessage(NET_DELIVERY_CRITICAL, lobbyConnection, NET_LOBBY_CLIENT_INDEX, CUSTOM_MSG_ID_CLIENT_REQUEST_PATCH, 0, &i);
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
  char buf[64];

	// call normal draw routine
	((void (*)(void))0x00707F28)();

  if ( confirmUpdatePopup == 1 ) {

    snprintf( buf, sizeof( buf ), "You have %d map update(s). Would you like to download them?", numMapsToUpdate );
    if ( uiShowYesNoDialog("Custom Maps", buf) == 1 ) {
      confirmUpdatePopupResult = 1;
    } else {
      downloadPatch();
    }

    confirmUpdatePopup = 0;
  }
  else if ( showResultPopup == 1 ) {

    uiShowOkDialog("Custom Maps", "Download stopped abruptly. Please double check the USB drive is not corrupt.");

    downloadPatch();
    showResultPopup = 0;
  } else if ( showResultPopup == 2 ) {
    
    snprintf( buf, sizeof( buf ), "%d maps updated.", numMapsUpdated );
    uiShowOkDialog("Custom Maps", numMapsUpdated == 0 ? "Maps already up to date." : buf);
    
    downloadPatch();
    showResultPopup = 0;
  } else if ( showResultPopup == 3 ) {
    
    uiShowOkDialog("Custom Maps", "Unable to access USB drive. Try plugging it into another slot.");
    
    downloadPatch();
    showResultPopup = 0;
  }

	// only show on main menu
	if (uiGetActive() != UI_ID_ONLINE_MAIN_MENU)
		return;
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
  static struct DbIndex db;

  dlPreUpdate();

	// Hook menu loop
	*(u32*)0x00594CB8 = 0x0C000000 | ((u32)(&onOnlineMenu) / 4);

  if (!initialized) {
    initialized = 1;
      
    // stop server select
    //*(u32*)0x00138D7C = 0x0C04E138;

    // reset global net callbacks table ptr
    *(u32*)0x00211E64 = 0;

    rpcUSBInit();
    padEnableInput();

    // ensures that the USB drive is accessible
    if ( db_check_mass_dir() < 0 ) {
      DPRINTF("unable to open usb drive\n");
      showResultPopup = 3;
      dbStateCancel = 1;
      return;
    }

    client_init();
    int parsed = parse_db(&db);
    if (parsed < 0) {
      DPRINTF("db parse error %d\n", parsed);
    } else {
      numMapsToUpdate = count_updates( &db );
      if ( numMapsToUpdate > 0 ) {
        confirmUpdatePopup = 1;
        confirmUpdatePopupResult = 0;
      } else {
        showResultPopup = 2;
      }
    }
  }

  // run update
  if (confirmUpdatePopupResult) {
    confirmUpdatePopupResult = 0;
    padDisableInput();
    if (update_all(&db) < 0) {
      showResultPopup = 1;
    } else {
      showResultPopup = 2;
    }
    padEnableInput();
  }
  
  drawDownloadPopup();
  if (dbStateActive && !dbStateCancel && padGetButtonDown(0, PAD_TRIANGLE) > 0) {
    dbStateCancel = 1;
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

  dlPostUpdate();
	return 0;
}
