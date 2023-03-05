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
#include "module.h"
#include "messageid.h"
#include <libdl/game.h>
#include <libdl/string.h>
#include <libdl/stdio.h>
#include <libdl/gamesettings.h>
#include <libdl/dialog.h>
#include <libdl/patch.h>
#include <libdl/ui.h>
#include <libdl/graphics.h>

int selfDestruct __attribute__((section(".config"))) = 0;

int bytesReceived = 0;
int totalBytes = 0;

int loadelf (u32 loadFromAddress, u32 size);
int doBootElf = 0;
ServerResponseBootElf_t bootElf = {
  .BootElfId = 0,
  .Address = 0,
  .Size = 0
};

int onServerDownloadDataRequest(void * connection, void * data)
{
	ServerDownloadDataRequest_t* request = (ServerDownloadDataRequest_t*)data;


	// copy bytes to target
	totalBytes = request->TotalSize;
	bytesReceived += request->DataSize;
	memcpy((void*)request->TargetAddress, request->Data, request->DataSize);
	DPRINTF("DOWNLOAD: {%d} %d/%d, writing %d to %08X\n", request->Id, bytesReceived, request->TotalSize, request->DataSize, request->TargetAddress);

	// respond
	if (connection)
	{
		ClientDownloadDataResponse_t response;
		response.Id = request->Id;
		response.BytesReceived = bytesReceived;
		netSendCustomAppMessage(NET_DELIVERY_CRITICAL, connection, NET_LOBBY_CLIENT_INDEX, CUSTOM_MSG_ID_CLIENT_DOWNLOAD_DATA_RESPONSE, sizeof(ClientDownloadDataResponse_t), &response);
	}

	return sizeof(ServerDownloadDataRequest_t) - sizeof(request->Data) + request->DataSize;
}

/*
 * NAME :		onBootElfResponse
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
int onBootElfResponse(void * connection, void * data)
{
  memcpy(&bootElf, data, sizeof(ServerResponseBootElf_t));

  DPRINTF("boot elf response id:%d addr:%08X size:%08X\n", bootElf.BootElfId, bootElf.Address, bootElf.Size);
  doBootElf = 1;

	return sizeof(ServerResponseBootElf_t);
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

  if (bootElf.Address && bootElf.Size && doBootElf == 1) {
    doBootElf = 2;
    loadelf(bootElf.Address, bootElf.Size);
  }

	// only show on main menu
	if (uiGetActive() != UI_ID_ONLINE_MAIN_MENU)
		return;

	gfxScreenSpaceBox(0.2, 0.35, 0.6, 0.125, bgColorDownload);
	gfxScreenSpaceBox(0.2, 0.45, 0.6, 0.05, barBgColor);
	gfxScreenSpaceText(SCREEN_WIDTH * 0.35, SCREEN_HEIGHT * 0.4, 1, 1, textColor, "Downloading elf...", 17 + (gameGetTime()/240 % 4), 3);

	if (totalBytes > 0)
	{
		float w = (float)bytesReceived / (float)totalBytes;
		gfxScreenSpaceBox(0.2, 0.45, 0.6 * w, 0.05, barFgColor);
	}
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

  if (!initialized) {
      
    // stop server select
    *(u32*)0x00138D7C = 0x0C04E138;

    // reset global net callbacks table ptr
    *(u32*)0x00211E64 = 0;

    initialized = 1;
  }
  
  if (doBootElf == 2) {
    gfxScreenSpaceBox(0.2, 0.35, 0.6, 0.125, 0x80000000);
    gfxScreenSpaceText(SCREEN_WIDTH * 0.26, SCREEN_HEIGHT * 0.41, 1, 1, 0x80C0C0C0, "Loading... this may take awhile...", 31 + (gameGetTime()/240 % 4), 3);
  }
  
	// just clear if selfDestruct is true
	if (selfDestruct) {
		return;
	}


	// 
	netInstallCustomMsgHandler(CUSTOM_MSG_ID_SERVER_DOWNLOAD_DATA_REQUEST, &onServerDownloadDataRequest);
	netInstallCustomMsgHandler(CUSTOM_MSG_ID_SERVER_RESPONSE_BOOT_ELF, &onBootElfResponse);

	// Hook menu loop
	*(u32*)0x00594CB8 = 0x0C000000 | ((u32)(&onOnlineMenu) / 4);

	// disable pad on online main menu
	if (uiGetActive() == UI_ID_ONLINE_MAIN_MENU)
		padDisableInput();

	return 0;
}
