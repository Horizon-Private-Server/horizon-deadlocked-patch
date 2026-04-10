/***************************************************
 * FILENAME :		game.c
 * 
 * DESCRIPTION :
 * 		ONE IN THE CHAMBER.
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
#include "include/game.h"
#include "include/utils.h"

extern int Initialized;

#define OITC_GUN_ID   (WEAPON_ID_FUSION_RIFLE)

//--------------------------------------------------------------------------
void getResurrectPoint(Player* player, VECTOR outPos, VECTOR outRot, int firstRes)
{
  // call base
  ((void (*)(Player*, VECTOR, VECTOR, int))0x006242d0)(player, outPos, outRot, firstRes);

  // force ammo to 1
  if (player->GadgetBox) {
    player->GadgetBox->Gadgets[OITC_GUN_ID].Level = 0;
    player->GadgetBox->Gadgets[OITC_GUN_ID].Ammo = 1;
  }
}

//--------------------------------------------------------------------------
void onAfterFirstRes(Player* player, int gadgetId)
{
  // call base
  ((void (*)(Player*, int))0x005f0208)(player, gadgetId);

  // force ammo to 1
  if (player->GadgetBox) {
    player->GadgetBox->Gadgets[OITC_GUN_ID].Level = 0;
    player->GadgetBox->Gadgets[OITC_GUN_ID].Ammo = 1;
  }
}

//--------------------------------------------------------------------------
void onPlayerKill(char * fragMsg)
{
  Player** players = playerGetAll();

	// call base function
	((void (*)(char*))0x00621CF8)(fragMsg);

  // parse frag msg
	char weaponId = fragMsg[3];
	int killedPlayerId = fragMsg[2];
	int sourcePlayerId = fragMsg[0];

  // award 1 ammo for kill
  if (sourcePlayerId >= 0 && killedPlayerId >= 0) {
    Player* player = players[sourcePlayerId];
    if (player && playerIsConnected(player) && player->GadgetBox) {
      player->GadgetBox->Gadgets[OITC_GUN_ID].Level = 0;
      player->GadgetBox->Gadgets[OITC_GUN_ID].Ammo += 1;
    }
  }
}

//--------------------------------------------------------------------------
void gameOnPlayerUpdate(Player* player)
{
  if (!player || !playerIsConnected(player)) return;

  // force player health
  player->Health = clamp(player->Health, 0, 10);

  // instantly call Hero::Death on death, so that the killer can get their ammo point immediately
  // does break the animation on the local players screen
  if (player->IsLocal && player->Health <= 0 && *(char*)((u32)player + 0x2ed7) == 0) {
    ((void (*)(Player*))0x005e2188)(player);
  }

  // disable respawn invincibility
  player->timers.invincibilityTimer = 0;
}

//--------------------------------------------------------------------------
void frameTick(void)
{
  
}

//--------------------------------------------------------------------------
void gameTick(void)
{
	GameSettings * gameSettings = gameGetSettings();
	GameOptions * gameOptions = gameGetOptions();
  GameData * gameData = gameGetData();
	Player ** players = playerGetAll();
	int i;
	char buffer[32];
	int gameTime = gameGetTime();

  // player updates
  for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
    gameOnPlayerUpdate(players[i]);
  }
}

//--------------------------------------------------------------------------
void initialize(PatchStateContainer_t* gameState)
{
  static int startDelay = 60 * 0.2;
	static int waitingForClientsReady = 0;

	GameSettings* gameSettings = gameGetSettings();
	Player** players = playerGetAll();
	GameData* gameData = gameGetData();
	int i;

	// hook into player kill event
  HOOK_JAL(0x00621c7c, &onPlayerKill);

	// hook get resurrect point
  HOOK_JAL(0x005e2d44, &getResurrectPoint);
  HOOK_JAL(0x00610724, &getResurrectPoint);
  HOOK_JAL(0x00610EB4, &onAfterFirstRes);

  // move ideal spawn distance from 40 to 8
  // spawns players closer to other players
  POKE_U16(0x00624614, 0x4100);

	// hook messages
	netHookMessages();

  if (startDelay) {
    --startDelay;
    return;
  }
  
  // wait for all clients to be ready
  // or for 5 seconds
  if (!gameState->AllClientsReady && waitingForClientsReady < (5 * 60)) {
    uiShowPopup(0, "Waiting For Players...");
    ++waitingForClientsReady;
    return;
  }

  // hide waiting for players popup
  hudHidePopup();

	// initialize player states
	State.LocalPlayerState = NULL;
	for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
		Player * p = players[i];
		State.PlayerStates[i].PlayerIndex = i;

		// is local
		if (p && p->IsLocal && !State.LocalPlayerState) {
      State.LocalPlayerState = &State.PlayerStates[i];
		}
	}

	// initialize state
	State.GameOver = 0;
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
		sGameData->Rounds = 0;
		sGameData->Version = 0x00000001;

		for (i = 0; i < GAME_MAX_PLAYERS; ++i)
		{
      sGameData->Kills[i] = gameData->PlayerStats.Kills;
		}
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

  // force ffa
  //int i;
  //for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
  //  if (gameSettings->PlayerClients[i] >= 0) {
  //    gameSettings->PlayerTeams[i] = i;
  //  }
  //}

  // force deathmatch
  if (gameSettings->GameRules != GAMERULE_DM) {
    gameSettings->GameRules = GAMERULE_DM;
    gameOptions->GameFlags.MultiplayerGameFlags.KillsToWin = 0;
    gameOptions->GameFlags.MultiplayerGameFlags.Timelimit = (int)maxf(5, gameOptions->GameFlags.MultiplayerGameFlags.Timelimit);
  }
	
	// apply options
  gameOptions->GameFlags.MultiplayerGameFlags.Nodes = 0;
  gameOptions->GameFlags.MultiplayerGameFlags.Flags = 0;
  gameOptions->GameFlags.MultiplayerGameFlags.Hills = 0;
  gameOptions->GameFlags.MultiplayerGameFlags.SpawnType = 3; // NORMAL SPAWNS
  gameOptions->GameFlags.MultiplayerGameFlags.UnlimitedAmmo = 0;
	gameOptions->GameFlags.MultiplayerGameFlags.AutospawnWeapons = 0;
	gameOptions->GameFlags.MultiplayerGameFlags.Juggernaut = 0;
	gameOptions->GameFlags.MultiplayerGameFlags.Survivor = 0;
	//gameOptions->GameFlags.MultiplayerGameFlags.Teamplay = 0;
	gameOptions->GameFlags.MultiplayerGameFlags.Vehicles = 0;
	gameOptions->GameFlags.MultiplayerGameFlags.Puma = 0;
	gameOptions->GameFlags.MultiplayerGameFlags.Hoverbike = 0;
	gameOptions->GameFlags.MultiplayerGameFlags.Hovership = 0;
	gameOptions->GameFlags.MultiplayerGameFlags.Landstalker = 0;

  // disable all weapons but fusion
  gameOptions->WeaponFlags.DualVipers = 0;
  gameOptions->WeaponFlags.MagmaCannon = 0;
  gameOptions->WeaponFlags.Arbiter = 0;
  gameOptions->WeaponFlags.FusionRifle = 1;
  gameOptions->WeaponFlags.MineLauncher = 0;
  gameOptions->WeaponFlags.B6 = 0;
  gameOptions->WeaponFlags.Holoshield = 0;
  gameOptions->WeaponFlags.Flail = 0;

  // prevent players from healing & from getting ammo
  gameState->GameConfig->grNoHealthBoxes = 2;
  gameState->GameConfig->grVampire = 0;
  gameState->GameConfig->grNoPickups = 1;
  gameState->GameConfig->grNoPacks = 1;
  gameState->GameConfig->grV2s = 2;
  gameState->GameConfig->grNoInvTimer = 1;
}
