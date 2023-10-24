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

const int patches[][3] = {
	// patch
	{ 0, 0x0072C3FC, 0x0C1CBBDE }, // GAMESETTINGS_LOAD_PATCH
	{ 0, 0x004B882C, 0x00712BF0 }, // GAMESETTINGS_BUILD_PTR
	{ 0, 0x0072E5B4, 0x0C1C2D50 }, // GAMESETTINGS_CREATE_PATCH
	{ 0, 0x00759580, 0x0C1D5D98 }, // start game hook
	{ 0, 0x00123D38, 0x0C049C5A }, // printf hook
	{ -1, 0x01EAAB10, 0x03E00008 }, // GET_MEDIUS_APP_HANDLER_HOOK
	{ -1, 0x00211E64, 0x00000000 }, // net global callbacks ptr
	{ -1, 0x00212164, 0x00000000 }, // dme callback table custom msg handler ptr
	{ -1, 0x00157D38, 0x0C055E68 }, // process level jal
	{ -1, 0x0015C0EC, 0x03E00008 }, // MediusGetLadderStatsWide hook
	{ 0, 0x00594CB8, 0x0C1C1FCA }, // menu loop hook
	{ 0, 0x0015b290, 0x03E00008 }, // nwUpdate hook
	{ 1, 0x004C3A94, 0x0C130C90 }, // draw hook
	{ 1, 0x004a84b0, 0x0C1661AC }, // update hook
  { 1, 0x005281F0, 0x0100F809 }, // update pad RawPadInputCallback
	{ 1, 0x004a84a8, 0x0C05873C }, // 
	{ 0, 0x0075AC3C, 0x0C1DEE28 }, // get rank numbers
	{ 0, 0x0075AC40, 0xC44C0124 }, // get rank numbers
  { -1, 0x00138d7c, 0x0C04E138 }, // sceGsGetGParam
	// maploader
	{ 0, 0x005CFB48, 0x0C058E10 }, // hookLoadAddr
	{ 0, 0x00705554, 0x0C058E02 }, // hookLoadingScreenAddr
	{ -1, 0x00163814, 0x0C058E10 }, // hookLoadCdvdAddr
	{ 0, 0x005CF9B0, 0x0C058E4A }, // hookCheckAddr
	{ -1, 0x00159B20, 0x0C056680 }, // hookTableAddr
	{ 1, 0x00557580, 0x0C058E02 }, // hookMapAddr
	{ 1, 0x0053F970, 0x0C058E02 }, // hookAudioAddr
	{ 0, 0x007055B4, 0x0C046A7B }, // hook loading screen map name strcpy
	{ 0, 0x0070583C, 0x0C046A7B }, // hook loading screen map name strcpy
	// comp
	{ 0, 0x004BD1B8, 0x0073BA08 }, // end game scoreboard vtable function
	{ 0, 0x004BB6C8, 0x007280F0 }, // clan room menu vtable function
	{ 0, 0x004BFA70, 0x00759220 }, // staging menu vtable function
	{ 0, 0x004BE5A0, 0x00749250 }, // quick play vtable function
	{ 0, 0x0074B428, 0x0C1C671A }, // quick play select dialog open
	{ 0, 0x00728620, 0x24070001 }, // clan room logout
	{ 0, 0x0072862C, 0xAFA70148 }, // clan room logout
	{ 0, 0x0075A150, 0x11000005 }, // enable add buddy/ignored/kick in staging 
	{ 0, 0x00763DC0, 0x24020004 }, // enable changing team in staging
	{ 0, 0x0075a7ec, 0x0C1D6E80 }, // leave staging hook
	{ 0, 0x00759448, 0x0C1C668A }, // enable Game Cancelled popup
	{ 0, 0x0071C168, 0x03E00008 }, // reset get return to menu id
	// in game
	{ 1, 0x005930B8, 0x02C3B020 }, // lod patch
	{ 1, 0x005605D4, 0x0C15803E }, // start menu back callback
	{ 1, 0x0060eb80, 0x0C18784C }, // player state update
	{ 1, 0x005E07C8, 0x0C177FC2 }, // who hit me hook (kill steal patch)
	{ 1, 0x005E11B0, 0x0C177FC2 }, // who hit me hook #2
	{ 1, 0x005e2b2c, 0x0C178B9A }, // resurrect WeaponStripMe hook
	{ 1, 0x005e2b48, 0x0C17DD44 }, // resurrect GiveMeRandomWeapons hook
	{ 1, 0x003FC66C, 0x0C12DF94 }, // fusion shot collision check hook
	{ 1, 0x003fc670, 0x0000302D }, // fusion shot collision check a2 arg
	{ 1, 0x0062ac60, 0x0C17C0C6 }, // weapon shot timebase delay hook
  { 1, 0x004aea90, 0x24020470 }, // SetFov patch
  { 1, 0x004aea94, 0x3C030023 }, // SetFov patch
  { 1, 0x005282d8, 0x3C020022 }, // Pad_MappedPad patch
  { 1, 0x005282dc, 0x8C42E694 }, // Pad_MappedPad patch
  { 1, 0x0060684c, 0x0C1833B0 }, // player sync wrench patch
  { 1, 0x00608BC4, 0x0040F809 }, // player sync stop cboot state update
  { 1, 0x006265a4, 0x0C189544 }, // ComputePoints patch
	// spectator
	{ 1, 0x0054F46C, 0x0C1734F4 }, // healthbar
	{ 1, 0x0054f898, 0x0C1734F4 }, // healthbar
	{ 1, 0x00541708, 0x0C1734F4 }, // get current gadget
	{ 1, 0x005418c4, 0x0C1734F4 }, // get gadget version name
	{ 1, 0x00541f3c, 0x0C1734F4 }, // get player hud team
	{ 1, 0x0054209c, 0x0C1734F4 }, // get gadget color
	{ 1, 0x00552bd0, 0x0C1734F4 }, // ammo update ammo
	{ 1, 0x00555904, 0x0C1734F4 }, // radar update map
	{ 1, 0x0055615c, 0x0C1734F4 }, // radar update blip
	// debug
	//{ 1, 0x0015B290, 0x03E00008 }, 
	//{ 1, 0x0060ed9c, 0x0C135C12 },
	//{ 1, 0x004189ec, 0x0C105DF2 },
	//{ 1, 0x005ce330, 0x0C173D64 },
};

const int clears[][2] = {
	{ 0x000D0000, 0x00020000 }, // patch
	{ 0x000F0000, 0x0000F000 }, // game mode
	{ 0x000CF000, 0x00000800 }, // module definitions
	{ 0x000CFFD0, 0x00000020 }, // patch hash
	{ 0x000CFFC0, 0x00000010 }, // patch pointers
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
		netSendCustomAppMessage(NET_DELIVERY_CRITICAL, connection, NET_LOBBY_CLIENT_INDEX, CUSTOM_MSG_ID_CLIENT_DOWNLOAD_DATA_RESPONSE, sizeof(ClientDownloadDataResponse_t), &response);
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
	//if (uiGetActive() != UI_ID_ONLINE_MAIN_MENU)
	//	return;

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

	// state
	// 0 = menus
	// 1 = in game
	// 2 = loading scene
	int state = isInGame() ? 1 : (isInMenus() ? 0 : 2);

	// clear memory
	if (!hasClearedMemory)
	{
		// unhook patch
		for (i = 0; i < patchesSize; ++i)
		{
			int context = patches[i][0];
			if (context < 0 || context == state)
				*(u32*)patches[i][1] = (u32)patches[i][2];
		}

		hasClearedMemory = 1;
		for (i = 0; i < clearsSize; ++i)
		{
			memset((void*)clears[i][0], 0, clears[i][1]);
		}
	}

	// just clear if selfDestruct is true
	if (selfDestruct) {
		return;
	}

	// 
	netInstallCustomMsgHandler(CUSTOM_MSG_ID_SERVER_DOWNLOAD_DATA_REQUEST, &onServerDownloadDataRequest);

	// only continue if in menus
	if (state != 0)
		return;

	// Hook menu loop
	*(u32*)0x00594CB8 = 0x0C000000 | ((u32)(&onOnlineMenu) / 4);

	// disable pad on online main menu
  if (!netGetLobbyServerConnection()) {
    padEnableInput();

	  *(u32*)0x00594CB8 = 0x0C000000 | (0x00707F28 / 4);
  }
	else if (uiGetActive() == UI_ID_ONLINE_MAIN_MENU) {
		padDisableInput();
  }

	return 0;
}
