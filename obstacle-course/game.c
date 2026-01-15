/***************************************************
 * FILENAME :		game.c
 * 
 * DESCRIPTION :
 * 		OBSTACLE COURSE.
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

struct ServerConfig ServerConfig __attribute__((section(".config"))) = {
  .LastCheckpointUid = 0
};

extern int Initialized;
struct ObstacleMapConfig* mapConfig = (struct ObstacleMapConfig*)(EXTRA_CODE_SEG_PTR + 0x10);

//--------------------------------------------------------------------------
void tryFullRestart(void)
{
  if (hasGameCodeSeg() && PATCH_INTEROP && PATCH_INTEROP->PatchStateContainer && PATCH_INTEROP->PatchStateContainer->SelectedCustomMapId > 0) {
    PatchStateContainer_t* patchStateContainer = PATCH_INTEROP->PatchStateContainer;
    CustomMapDef_t* map = PATCH_INTEROP->GetCustomMapDef(patchStateContainer->SelectedCustomMapId - 1);
    DPRINTF("sel map %d => %s\n", patchStateContainer->SelectedCustomMapId, map->Filename);

    ServerConfig.LastCheckpointUid = 0;

    // send to server
    void* lobbyConnection = netGetLobbyServerConnection();
    if (lobbyConnection) {
      SetPlayerSavedCheckpointRequest_t msg;
      msg.CheckpointUid = 0;
      msg.CheckpointTicks = 0;
      netSendCustomAppMessage(NET_DELIVERY_CRITICAL, lobbyConnection, NET_LOBBY_CLIENT_INDEX, CUSTOM_MSG_CLIENT_SET_LAST_CHECKPOINT, sizeof(msg), &msg);
    }

    PATCH_INTEROP->HopToCustomMap(map);
  }
}

//--------------------------------------------------------------------------
int checkpointSetActive(Moby* checkpointManagerMoby, Moby* checkpointMoby)
{
  if (!checkpointManagerMoby) return 0;

  struct CheckpointManagerPVar* pvars = (struct CheckpointManagerPVar*)checkpointManagerMoby->PVar;

  // get index of checkpoint
  int idx = 0;
  for (idx = 0; idx < CHECKPOINT_MAX_CHECKPOINTS; ++idx) {
    if (pvars->CheckpointMobys[idx] == checkpointMoby) {
      break;
    }
  }

  if (idx >= CHECKPOINT_MAX_CHECKPOINTS) return 0;
  if (checkpointManagerMoby->State == idx) return 1;

  //DLOG_MNGR(checkpointManagerMoby, "Activate checkpoint %08X => %d\n", (u32)checkpointMoby, idx);
  //DLOG_CHPT(checkpointMoby, "Activate checkpoint %08X => %d\n", (u32)checkpointMoby, idx);
  uiShowPopup(0, "Loaded Last Checkpoint");
  
  // update checkpoint locally
  mobySetState(checkpointManagerMoby, idx, -1);
  ((void (*)(Moby*))checkpointManagerMoby->PUpdate)(checkpointManagerMoby);
  ((void (*)(Moby*))checkpointMoby->PUpdate)(checkpointMoby);

  return 1;
}

//--------------------------------------------------------------------------
void checkpointLoadSaved(void)
{
  // find checkpoint manager
  if (ServerConfig.LastCheckpointUid > 0) {
    Moby* mCheckpoint = mobyFindByUID(ServerConfig.LastCheckpointUid);
    Moby* mCheckpointManager = mobyFindNextByOClass(mobyListGetStart(), 0x4007);
    DPRINTF("found checkpoint %08X with uid %d\n", mCheckpoint, ServerConfig.LastCheckpointUid);
    DPRINTF("found checkpoint manager %08X\n", mCheckpointManager);
    if (mCheckpointManager && mCheckpoint && mCheckpoint->OClass == 0x4008) {
      if (!checkpointSetActive(mCheckpointManager, mCheckpoint)) return;

      int i;
      for (i = 0; i < GAME_MAX_LOCALS; ++i) {
        Player* player = playerGetFromSlot(i);
        if (!playerIsValid(player)) continue;
        
        playerRespawn(player);
        State.LocalPlayerState->TotalTicks = ServerConfig.CheckpointTicks;
        State.HasLoadedLast = 1;
      }
    }
  } else {
    State.HasLoadedLast = 1;
  }
}

//--------------------------------------------------------------------------
void onReachedEnd(void)
{
  if (!State.LocalPlayerState) return;
  if (State.LocalPlayerState->TimeCompleted) return;

  DPRINTF("set complete\n");
  int pidx = State.LocalPlayerState->PlayerIndex;
  uiShowPopup(0, "Obstacle Course Complete!");
  sendPlayerReachedEnd(pidx, State.LocalPlayerState->TotalTicks);

	// send to server
  void* lobbyConnection = netGetLobbyServerConnection();
  if (!lobbyConnection) return;

  SetPlayerCompleteTimeRequest_t msg;
  msg.TotalTicks = State.LocalPlayerState->TotalTicks;
  netSendCustomAppMessage(NET_DELIVERY_CRITICAL, lobbyConnection, NET_LOBBY_CLIENT_INDEX, CUSTOM_MSG_CLIENT_SET_COMPLETE_TIME, sizeof(msg), &msg);
}

//--------------------------------------------------------------------------
void onReachedCheckpoint(Moby* checkpoint)
{
  if (!checkpoint) return;
  if (!State.LocalPlayerState) return;
  if (State.LocalPlayerState->TimeCompleted) return;

  DPRINTF("save checkpoint %d\n", checkpoint->UID);
  ServerConfig.LastCheckpointUid = checkpoint->UID;
  ServerConfig.CheckpointTicks = State.LocalPlayerState->TotalTicks;

	// send to server
  void* lobbyConnection = netGetLobbyServerConnection();
  if (!lobbyConnection) return;

  SetPlayerSavedCheckpointRequest_t msg;
  msg.CheckpointUid = checkpoint->UID;
  msg.CheckpointTicks = State.LocalPlayerState->TotalTicks;
  netSendCustomAppMessage(NET_DELIVERY_CRITICAL, lobbyConnection, NET_LOBBY_CLIENT_INDEX, CUSTOM_MSG_CLIENT_SET_LAST_CHECKPOINT, sizeof(msg), &msg);
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
  if (!State.LocalPlayerState) return;

  int i;
  for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
    if (!State.PlayerStates[i].TimeCompleted) {
      ++State.PlayerStates[i].TotalTicks;
    }
  }

  int ms = (double)State.LocalPlayerState->TotalTicks * 16.66666666;

  // draw timer
  if (State.LocalPlayerState && State.LocalPlayerState->TimeCompleted) {
    //drawTimer(State.LocalPlayerState->TimeCompleted - State.InitializedTime, 0x8000E000);
    drawTimer(ms, 0x8000E000);
  } else {
    //drawTimer(gameGetTime() - State.InitializedTime, 0x80E0E0E0);
    drawTimer(ms, 0x80E0E0E0);
  }
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

	// initialize player states
	//State.LocalPlayerState = NULL;
	for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
		Player * p = players[i];
		State.PlayerStates[i].PlayerIndex = i;

		// is local
		if (p && p->IsLocal && !State.LocalPlayerState) {
      State.LocalPlayerState = &State.PlayerStates[i];
		}
	}

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

  if (mapConfig) {
    mapConfig->SetLocalPlayerReachedEnd = &onReachedEnd;
    mapConfig->SetLocalPlayerReachedCheckpoint = &onReachedCheckpoint;
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

  // force same teams
  int i;
  for (i = 1; i < GAME_MAX_PLAYERS; ++i) {
   if (gameSettings->PlayerClients[i] >= 0) {
     gameSettings->PlayerTeams[i] = gameSettings->PlayerTeams[0];
   }
  }
	
  // read custom map settings
  if (!State.HasMapData) {
    if (gameState->ReadExtraDataFunc(&State.MapData, sizeof(State.MapData)) > 0) {
      State.HasMapData = 1;
    }
  }

	// apply options
	gameSettings->GameRules = State.MapData.GameRule;
	gameOptions->GameFlags.MultiplayerGameFlags.UnlimitedAmmo = State.MapData.UnlimitedAmmo;
	gameOptions->GameFlags.MultiplayerGameFlags.AutospawnWeapons = State.MapData.AutospawnWeapons;
	gameOptions->GameFlags.MultiplayerGameFlags.SpawnWithChargeboots = State.MapData.SpawnWithChargeboots;
	gameOptions->GameFlags.MultiplayerGameFlags.RespawnTime = State.MapData.RespawnTime;
	gameOptions->GameFlags.MultiplayerGameFlags.NodeType = !State.MapData.CQBoltCranks;
	gameOptions->GameFlags.MultiplayerGameFlags.Lockdown = State.MapData.CQLockdown;
	gameOptions->GameFlags.MultiplayerGameFlags.Homenodes = State.MapData.CQHomenodes;
	gameOptions->GameFlags.MultiplayerGameFlags.Timelimit = State.MapData.Timelimit;
	gameOptions->GameFlags.MultiplayerGameFlags.Juggernaut = 0;
	gameOptions->GameFlags.MultiplayerGameFlags.Survivor = State.MapData.Survivor;
	gameOptions->GameFlags.MultiplayerGameFlags.Teamplay = 1;
	gameOptions->GameFlags.MultiplayerGameFlags.Vehicles = State.MapData.Vehicles;
	gameOptions->GameFlags.MultiplayerGameFlags.Puma = State.MapData.Vehicles;
	gameOptions->GameFlags.MultiplayerGameFlags.Hoverbike = State.MapData.Vehicles;
	gameOptions->GameFlags.MultiplayerGameFlags.Hovership = State.MapData.Vehicles;
	gameOptions->GameFlags.MultiplayerGameFlags.Landstalker = State.MapData.Vehicles;
  gameOptions->GameFlags.MultiplayerGameFlags.Nodes = State.MapData.GameRule == GAMERULE_CQ;
  gameOptions->GameFlags.MultiplayerGameFlags.Hills = State.MapData.GameRule == GAMERULE_KOTH;
  gameOptions->GameFlags.MultiplayerGameFlags.Flags = State.MapData.GameRule == GAMERULE_CTF;
  gameOptions->WeaponFlags.Raw = State.MapData.GadgetsMask;

  // prevent players from healing & from getting ammo
  gameState->GameConfig->grNoPickups = 0;
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
