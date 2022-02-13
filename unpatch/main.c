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

const int patches[][3] = {
	// patch
	{ 0, 0x0072C3FC, 0x0C1CBBDE }, // GAMESETTINGS_LOAD_PATCH
	{ 0, 0x004B882C, 0x00712BF0 }, // GAMESETTINGS_BUILD_PTR
	{ 0, 0x0072E5B4, 0x0C1C2D50 }, // GAMESETTINGS_CREATE_PATCH
	{ -1, 0x01EAAB10, 0x03E00008 }, // GET_MEDIUS_APP_HANDLER_HOOK
	{ -1, 0x00211E64, 0x00000000 }, // net global callbacks ptr
	{ -1, 0x00212164, 0x00000000 }, // dme callback table custom msg handler ptr
	{ -1, 0x00157D38, 0x0C055E68 }, // process level jal
	// maploader
	{ 0, 0x005CFB48, 0x0C058E10 }, // hookLoadAddr
	{ 0, 0x00705554, 0x0C058E02 }, // hookLoadingScreenAddr
	{ -1, 0x00163814, 0x0C058E10 }, // hookLoadCdvdAddr
	{ 0, 0x005CF9B0, 0x0C058E4A }, // hookCheckAddr
	{ -1, 0x00159B20, 0x0C056680 }, // hookTableAddr
	{ -1, 0x00159B20, 0x0C056680 }, // hookTableAddr
	{ 0, 0x007055B4, 0x0C046A7B }, // hook loading screen map name strcpy
	// in game
	{ 1, 0x005930B8, 0x02C3B020 }, // lod patch
};

const int clears[][2] = {
	{ 0x000E0000, 0x0000C000 }, // 
	{ 0x000EC000, 0x00004000 }, // gamerules
	{ 0x000F0000, 0x0000F000 }, // game mode
	{ 0x000CF000, 0x00000800 }, // module definitions
};

int hasClearedMemory = 0;
int bytesReceived = 0;
int totalBytes = 0;

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
		netSendCustomAppMessage(connection, NET_LOBBY_CLIENT_INDEX, CUSTOM_MSG_ID_CLIENT_DOWNLOAD_DATA_RESPONSE, sizeof(ClientDownloadDataResponse_t), &response);
	}

	return sizeof(ServerDownloadDataRequest_t) - sizeof(request->Data) + request->DataSize;
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

	gfxScreenSpaceBox(0.2, 0.35, 0.6, 0.125, bgColorDownload);
	gfxScreenSpaceBox(0.2, 0.45, 0.6, 0.05, barBgColor);
	gfxScreenSpaceText(SCREEN_WIDTH * 0.35, SCREEN_HEIGHT * 0.4, 1, 1, textColor, "Downloading patch...", 17 + (gameGetTime()/240 % 4), 3);

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
	const int patchesSize =  sizeof(patches) / (3 * sizeof(int));
	const int clearsSize =  sizeof(clears) / (2 * sizeof(int));
	int inGame = gameIsIn();

	// clear memory
	if (!hasClearedMemory)
	{
		// unhook patch
		for (i = 0; i < patchesSize; ++i)
		{
			int context = patches[i][0];
			if (context < 0 || context == inGame)
				*(u32*)patches[i][1] = (u32)patches[i][2];
		}

		hasClearedMemory = 1;
		for (i = 0; i < clearsSize; ++i)
		{
			memset((void*)clears[i][0], 0, clears[i][1]);
		}
	}

	// 
	netInstallCustomMsgHandler(CUSTOM_MSG_ID_SERVER_DOWNLOAD_DATA_REQUEST, &onServerDownloadDataRequest);

	// 
	if (inGame)
		return;

	// Hook menu loop
	*(u32*)0x00594CB8 = 0x0C000000 | ((u32)(&onOnlineMenu) / 4);

	// disable pad on online main menu
	if (uiGetActive() == UI_ID_ONLINE_MAIN_MENU)
		padDisableInput();

	return 0;
}
