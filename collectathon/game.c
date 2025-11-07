/***************************************************
 * FILENAME :		game.c
 * 
 * DESCRIPTION :
 * 		COLLECTATHON.
 * 		
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */

#include <tamtypes.h>
#include <string.h>

#include <libdl/time.h>
#include <libdl/game.h>
#include <libdl/gamesettings.h>
#include <libdl/player.h>
#include <libdl/camera.h>
#include <libdl/weapon.h>
#include <libdl/radar.h>
#include <libdl/hud.h>
#include <libdl/cheats.h>
#include <libdl/sha1.h>
#include <libdl/collision.h>
#include <libdl/dialog.h>
#include <libdl/ui.h>
#include <libdl/stdio.h>
#include <libdl/graphics.h>
#include <libdl/spawnpoint.h>
#include <libdl/random.h>
#include <libdl/net.h>
#include <libdl/sound.h>
#include <libdl/color.h>
#include <libdl/dl.h>
#include <libdl/utils.h>
#include "common.h"
#include "module.h"
#include "messageid.h"
#include "include/collectible.h"
#include "include/game.h"
#include "include/utils.h"

struct ServerConfig ServerConfig __attribute__((section(".config"))) = {
  
};

extern int Initialized;

//--------------------------------------------------------------------------
int whoKilledMeHook(void)
{
	return 0;
}

//--------------------------------------------------------------------------
void drawTimer(int time, u32 color)
{
  char buf[32];

  if (time < 0) time = 0;

  snprintf(buf, sizeof(buf), "%d:%02d.%03d", time / TIME_MINUTE, (time % TIME_MINUTE) / TIME_SECOND, time % TIME_SECOND);
  gfxHelperDrawText(15, SCREEN_HEIGHT - 15, 0, 0, 0.9, color, buf, -1, TEXT_ALIGN_BOTTOMLEFT, COMMON_DZO_DRAW_NORMAL);
}

//--------------------------------------------------------------------------
void frameTick(void)
{
  // drawTimer(ms, 0x80E0E0E0);
}

//--------------------------------------------------------------------------
void gameTick(void)
{
  
}

//--------------------------------------------------------------------------
void initialize(PatchStateContainer_t* gameState)
{
	GameSettings* gameSettings = gameGetSettings();
	Player** players = playerGetAll();
	GameData* gameData = gameGetData();
	int i;

	// hook messages
	netHookMessages();

  // vehicle respawn ticks
  POKE_U32(0x00386C00, 8);
  POKE_U32(0x00386720, 8);
  POKE_U32(0x00387550, 8);
  POKE_U32(0x00386F30, 8);

	// patch who killed me to prevent damaging others
	//*(u32*)0x005E07C8 = 0x0C000000 | ((u32)&whoKilledMeHook >> 2);
	//*(u32*)0x005E11B0 = *(u32*)0x005E07C8;

  // disable targeting players
  //*(u32*)0x005F8A80 = 0x10A20002;
  //*(u32*)0x005F8A84 = 0x0000102D;
  //*(u32*)0x005F8A88 = 0x24440001;

  if (State.StartDelay) {
    --State.StartDelay;
    return;
  }
  
  // wait for all clients to be ready
  // or for 5 seconds
  if (!gameState->AllClientsReady && State.WaitingForClientsReady < (5 * 60)) {
    uiShowPopup(0, "Waiting For Players...");
    ++State.WaitingForClientsReady;
    return;
  }

  // hide waiting for players popup
  hudHidePopup();

  // 
  collectibleInit();

	// initialize state
	State.InitializedTime = gameGetTime();
	Initialized = 1;
}

//--------------------------------------------------------------------------
void updateGameState(PatchStateContainer_t * gameState)
{
	int i;

	// game state update
	if (gameState->UpdateGameState)
	{
    // 
	}

	// stats
	if (gameState->UpdateCustomGameStats && gameState->CustomGameStats)
	{
    GameData* gameData = gameGetData();
    gameState->CustomGameStatsSize = sizeof(struct CGMGameData);
		struct CGMGameData* sGameData = (struct CGMGameData*)gameState->CustomGameStats->Payload;
		sGameData->Version = 0x00000001;
	}
}

//--------------------------------------------------------------------------
void setLobbyGameOptions(PatchStateContainer_t * gameState)
{
	// set game options
	GameOptions * gameOptions = gameGetOptions();
	GameSettings* gameSettings = gameGetSettings();
	if (!gameOptions || !gameSettings || gameSettings->GameLoadStartTime <= 0)
		return;

  // force same teams
  int i;
  for (i = 1; i < GAME_MAX_PLAYERS; ++i) {
   if (gameSettings->PlayerClients[i] >= 0) {
     gameSettings->PlayerTeams[i] = gameSettings->PlayerTeams[0];
   }
  }
	
  // read custom map settings
  if (!State.HasMapData) {
    memset(&State.MapData, 0, sizeof(State.MapData));
    if (gameState->ReadExtraDataFunc(&State.MapData, sizeof(State.MapData)) > 0) {
      State.BoltCount = State.MapData.EasyBoltsCount + State.MapData.MediumBoltsCount + State.MapData.HardBoltsCount + State.MapData.VeryHardBoltsCount;
      State.HasMapData = 1;
    }
  }

	// apply options
	gameOptions->GameFlags.MultiplayerGameFlags.UnlimitedAmmo = 1;
	gameOptions->GameFlags.MultiplayerGameFlags.AutospawnWeapons = 1;
	gameOptions->GameFlags.MultiplayerGameFlags.SpawnWithChargeboots = 1;
	gameOptions->GameFlags.MultiplayerGameFlags.RespawnTime = 0;
	gameOptions->GameFlags.MultiplayerGameFlags.Timelimit = 0;
	gameOptions->GameFlags.MultiplayerGameFlags.Juggernaut = 0;
	gameOptions->GameFlags.MultiplayerGameFlags.Survivor = 0;
	gameOptions->GameFlags.MultiplayerGameFlags.Teamplay = 1;

  // prevent players from cheating
  gameState->GameConfig->grRespawnOverride = 0;
  gameState->GameConfig->grCqDisableTurrets = 0;
  gameState->GameConfig->grHalfTime = 0;
  gameState->GameConfig->grOvertime = 0;
  gameState->GameConfig->drFreecam = 0;
  //gameState->GameConfig->drLevelReload = 0;
  gameState->GameConfig->prChargebootForever = 0;
  gameState->GameConfig->prRotatingWeapons = 0;
  gameState->GameConfig->prHeadbutt = 0;
}
