/***************************************************
 * FILENAME :		game.c
 * 
 * DESCRIPTION :
 * 		HNS.
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
#include "module.h"
#include "messageid.h"
#include "include/game.h"
#include "include/utils.h"

extern int Initialized;

void initializeScoreboard(void);

//--------------------------------------------------------------------------
void onSetPlayerTeam(int playerId, int team)
{
  if (!isInGame()) return;

  DPRINTF("set %d team to %d\n", playerId, team);
  Player* player = playerGetAll()[playerId];
  if (player) playerSetTeam(player, team);
}

//--------------------------------------------------------------------------
void onReceiveRoundStage(enum GameStage stage)
{
  State.Stage = stage;
  State.StageTicks = 0;
}

//--------------------------------------------------------------------------
void onPlayerKill(char * fragMsg)
{
	VECTOR delta;
	Player** players = playerGetAll();

	// call base function
	((void (*)(char*))0x00621CF8)(fragMsg);

	char weaponId = fragMsg[3];
	char killedPlayerId = fragMsg[2];
	char sourcePlayerId = fragMsg[0];
}

//--------------------------------------------------------------------------
int gameGetTeamScore(int team, int score)
{
  if (team == TEAM_HIDERS) return State.NumHiders;
  else if (team == TEAM_SEEKERS && !countRemainingHiders()) return 10;

  return 0;
}

//--------------------------------------------------------------------------
void drawSeekerOverlay(int localPlayerIndex, int time)
{
  char buf[64];
  int localCount = playerGetNumLocals();
  float vertScale = 1.0 / localCount;
  float yOff = localPlayerIndex * 0.5;
  snprintf(buf, sizeof(buf), "It's not rabbit season yet... %02d", time);
  gfxScreenSpaceBox(0, yOff, 1, vertScale, 0x80000000);
  gfxScreenSpaceText(SCREEN_WIDTH/2, (SCREEN_HEIGHT*yOff) + (SCREEN_HEIGHT*vertScale)/2, 1, 1, 0x80FFFFFF, buf, -1, 4);
  
  if (PATCH_DZO_INTEROP_FUNCS) {
    CustomDzoCommandDrawBox_t boxCmd;
    boxCmd.AnchorX = 0;
    boxCmd.AnchorY = localPlayerIndex * vertScale;
    boxCmd.X = 0;
    boxCmd.Y = 0;
    boxCmd.W = 1;
    boxCmd.H = vertScale;
    boxCmd.Color = 0x80000000;
    boxCmd.Alignment = TEXT_ALIGN_TOPLEFT;
    boxCmd.Stretch = 1;
    PATCH_DZO_INTEROP_FUNCS->SendCustomCommandToClient(CUSTOM_DZO_CMD_ID_DRAW_BOX, sizeof(boxCmd), &boxCmd);

    CustomDzoCommandDrawText_t textCmd;
    textCmd.AnchorX = 0.5;
    textCmd.AnchorY = 0.5 + (localPlayerIndex * 0.5 * vertScale);
    textCmd.X = 0;
    textCmd.Y = 0;
    textCmd.Scale = 1;
    textCmd.Color = 0x80FFFFFF;
    textCmd.Alignment = TEXT_ALIGN_MIDDLECENTER;
    strncpy(textCmd.Text, buf, sizeof(textCmd.Text));
    PATCH_DZO_INTEROP_FUNCS->SendCustomCommandToClient(CUSTOM_DZO_CMD_ID_DRAW_TEXT, sizeof(textCmd), &textCmd);
  }
}

//--------------------------------------------------------------------------
int countRemainingHiders(void)
{
  int count = 0;
  Player** players = playerGetAll();
  if (!players) return 0;

  int i;
  for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
    Player* player = players[i];
    if (!player || !player->PlayerMoby || !player->pNetPlayer) continue;

    if (player->Team == TEAM_HIDERS) {
      if (State.Stage == GAME_STAGE_HIDE || !playerIsDead(player)) {
        ++count;
      }
    }
  }

  return count;
}

//--------------------------------------------------------------------------
int countSeekers(void)
{
  int count = 0;
  Player** players = playerGetAll();
  if (!players) return 0;

  int i;
  for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
    Player* player = players[i];
    if (!player || !player->PlayerMoby || !player->pNetPlayer) continue;

    if (player->Team == TEAM_SEEKERS) {
      ++count;
    }
  }

  return count;
}

//--------------------------------------------------------------------------
void stageHideLogic(void)
{
  Player** players = playerGetAll();
  GameSettings* gs = gameGetSettings();
  int i;

  int hideTime = (PATCH_INTEROP->GameConfig->hnsConfig.hideStageTime + 1) * HIDE_STAGE_INCREMENT_SEC;
  int stageTimeSec = State.StageTicks / TPS;

  // disable nametags during hide stage
  if (gs) gs->PlayerNamesOn = 0;

  // disable radar for everyone during initial phase
  POKE_U32(0x00613108, 0x24020001);

  for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
    Player* player = players[i];
    if (!player || !player->PlayerMoby || !player->pNetPlayer) continue;

    // lock seekers in place with no vision
    if (player->Team == TEAM_SEEKERS) {
      player->timers.noCamInputTimer = 5;
      player->timers.noInput = 5;
      player->Health = 50;

      if (player->IsLocal) {
        vector_write(player->CameraPos, 0);
        drawSeekerOverlay(player->LocalPlayerIndex, hideTime - stageTimeSec);
      }
    }

    // respawn hider immediately if they die during hide stage
    if (player->Team == TEAM_HIDERS) {
      if (player->IsLocal && playerIsDead(player)) playerRespawn(player);
      player->MaxHealth = player->Health = 100;
    }
  }

  // switch to seek stage after hide time runs out
  if (gameAmIHost() && stageTimeSec >= hideTime) {
    State.Stage = GAME_STAGE_SEEK;
    State.StageTicks = 0;
    sendRoundStage(State.Stage);
  }
}

//--------------------------------------------------------------------------
void stageSeekLogic(void)
{
  Player** players = playerGetAll();
  GameSettings* gs = gameGetSettings();
  GameOptions* gameOptions = gameGetOptions();
  static int init = 0;

  // enable nametags during seek stage
  if (gs) gs->PlayerNamesOn = !PATCH_INTEROP->GameConfig->grNoNames;

  int i;
  for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
    Player* player = players[i];
    if (!player || !player->PlayerMoby || !player->pNetPlayer) continue;

    // give seekers inf health
    if (player->Team == TEAM_SEEKERS) {
      player->Health = 50;

      // enable team radar for seekers
      if (player->IsLocal) {
        POKE_U32(0x00613108, 0x8E022EF4);
      }

      // give all weapons to seekers
      // alpha seekers have juggy pack
      int level = State.AlphaSeeker[i];
      int w;
      for (w = WEAPON_SLOT_VIPERS; w <= WEAPON_SLOT_OMNI_SHIELD; ++w) {
        int wId = weaponSlotToId(w);
        if (gameOptions->WeaponFlags.Raw & (1 << wId)) {
          if (player->GadgetBox->Gadgets[wId].Level != level) {
            player->GadgetBox->Gadgets[wId].Level = -1;
            playerGiveWeapon(player->GadgetBox, wId, level, 1);
          }
        }
      }
    }

    if (player->IsLocal && player->Team == TEAM_HIDERS) {

      // disable team radar for hiders
      POKE_U32(0x00613108, 0x24020001);

      // switch hider to seeker when dead
      if (playerIsDead(player)) {
        if (player->timers.resurrectWait == 0) {
          player->timers.resurrectWait = 100;
        } else if (player->timers.resurrectWait == 1) {
          playerSetTeam(player, TEAM_SEEKERS);
          sendPlayerTeam(i, TEAM_SEEKERS);
          playerRespawn(player);
        }
      } else {
        if (!init) uiShowPopup(player->LocalPlayerIndex, "Press UP to switch to spectate mode");

        // toggle spectate
        if (!gameIsStartMenuOpen(player->LocalPlayerIndex) && !PATCH_POINTERS_PATCHMENU && padGetButtonDown(player->LocalPlayerIndex, PAD_UP) > 0) {
          PATCH_INTEROP->SetSpectate(player->LocalPlayerIndex, i);
        }
      }
    }
  }

  // end when hiders die
  if (gameAmIHost() && !State.NumHiders) {
    gameSetWinner(TEAM_SEEKERS, 1);
    gameEnd(2);
    gameSetWinner(TEAM_SEEKERS, 1);
  }

  init = 1;
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

  State.NumHiders = countRemainingHiders();

  // end when seekers leave
  int seekersLeft = countSeekers();
  if (gameAmIHost() && seekersLeft <= 0) {
    gameSetWinner(TEAM_HIDERS, 1);
    gameEnd(9);
    gameSetWinner(TEAM_HIDERS, 1);
  }

  // stage logic
  if (State.Stage == GAME_STAGE_HIDE) {
    stageHideLogic();
  } else {
    stageSeekLogic();
  }

  gameSetWinner(State.NumHiders ? TEAM_HIDERS : TEAM_SEEKERS, 1);
  State.StageTicks++;
}

//--------------------------------------------------------------------------
void initialize(PatchGameConfig_t* gameConfig, PatchStateContainer_t* gameState)
{
  static int startDelay = 60 * 0.2;
	static int waitingForClientsReady = 0;

	GameSettings* gameSettings = gameGetSettings();
	Player** players = playerGetAll();
	GameData* gameData = gameGetData();
	int i;

	// hook into player kill event
	*(u32*)0x00621c7c = 0x0C000000 | ((u32)&onPlayerKill >> 2);

  // hook into computePlayerScore (endgame pts)
  //HOOK_J(0x00625510, &getPlayerScore);
  //POKE_U32(0x00625514, 0);

  // change deathmatch points to use playerPoints
  // instead of playerKills - playerSuicides
  //POKE_U16(0x006236B4, 0x7E8); // move to read buffer playerRank (reads are -0x28 from buffer)
  //POKE_U32(0x006236D8, 0x8CA2FFD8); // read points as word
  //POKE_U32(0x006236F4, 0x8CA2FFD8); // read points as word
  POKE_U32(0x00623700, 0); // stop counting suicides
  //POKE_U32(0x00623708, 0); // stop counting deaths (tiebreaker)
  //POKE_U16(0x00623710, 4); // increment by 4 (sizeof(int))

	// hook messages
	netHookMessages();

  // 
  initializeScoreboard();

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

  // save who is first seeker(s)
  for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
    if (gameSettings->PlayerClients[i] >= 0 && gameSettings->PlayerTeams[i] == TEAM_SEEKERS) {
      State.AlphaSeeker[i] = 1;
    } else {
      State.AlphaSeeker[i] = 0;
    }
  }

  // hide waiting for players popup
  hudHidePopup();

	// initialize state
  State.Stage = GAME_STAGE_HIDE;
  State.StageTicks = 0;
	State.InitializedTime = gameGetTime();

  sendRoundStage(State.Stage);
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
	if (gameState->UpdateCustomGameStats)
	{
    // no cgm data
    gameState->CustomGameStatsSize = 0;
	}
}

//--------------------------------------------------------------------------
void setLobbyGameOptions(void)
{
  static int set = 0;

	// deathmatch options
	static char options[] = { 
		0, 0, 				// 0x06 - 0x08
		0, 0, 0, 0, 	// 0x08 - 0x0C
		1, 1, 1, 0,  	// 0x0C - 0x10
		0, 1, 0, 0,		// 0x10 - 0x14
		-1, -1, 0, 1,	// 0x14 - 0x18
	};

	// set game options
	GameOptions * gameOptions = gameGetOptions();
	GameSettings* gameSettings = gameGetSettings();
	if (!gameOptions || !gameSettings || gameSettings->GameLoadStartTime <= 0 || set)
		return;
	
	// apply options
	memcpy((void*)&gameOptions->GameFlags.Raw[6], (void*)options, sizeof(options)/sizeof(char));
	gameOptions->GameFlags.MultiplayerGameFlags.Juggernaut = 0;
	gameOptions->GameFlags.MultiplayerGameFlags.Survivor = 0;
	gameOptions->GameFlags.MultiplayerGameFlags.Teamplay = 1;
  gameOptions->GameFlags.MultiplayerGameFlags.KillsToWin = 0;
  gameOptions->GameFlags.MultiplayerGameFlags.RadarBlips = 0;
  gameOptions->GameFlags.MultiplayerGameFlags.Vehicles = 0;
  gameOptions->GameFlags.MultiplayerGameFlags.SpecialPickups = 0;
  gameOptions->GameFlags.MultiplayerGameFlags.SpecialPickupsRandom = 0;
  gameOptions->GameFlags.MultiplayerGameFlags.SpawnWithChargeboots = 1;
  gameOptions->GameFlags.MultiplayerGameFlags.UnlimitedAmmo = 1;
  gameOptions->GameFlags.MultiplayerGameFlags.AutospawnWeapons = 1;
  gameOptions->GameFlags.MultiplayerGameFlags.RespawnTime = 0;
  gameOptions->GameFlags.MultiplayerGameFlags.Teamplay = 1;
  if (gameOptions->GameFlags.MultiplayerGameFlags.Timelimit == 0)
    gameOptions->GameFlags.MultiplayerGameFlags.Timelimit = 10;

  DPRINTF("set lobby teams\n");

  if (gameAmIHost()) {

    // set everyone not red to blue team
    int i;
    int numPlayers = 0;
    int numHiders = 0;
    int numPreSeekers = 0;
    for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
      if (gameSettings->PlayerClients[i] >= 0) {
        if (gameSettings->PlayerTeams[i] != TEAM_SEEKERS) {
          gameSettings->PlayerTeams[i] = TEAM_HIDERS;
          ++numHiders;
        } else {
          ++numPreSeekers;
        }

        ++numPlayers;
      }
    }

    int seekerTargetCount = gameSettings->PlayerCountAtStart > 6 ? 2 : 1;
    int numSeekers = numPreSeekers;

    // if not enough seekers, choose random seekers and set to red team
    // if too many seekers, choose random seeker and set to hider team
    while (numSeekers != seekerTargetCount)
    {
      int mod = (numSeekers > seekerTargetCount) ? numSeekers : numHiders;
      int r = 1 + (libcRand() % mod);
      i = -1;
      while (r)
      {
        i = (i + 1) % GAME_MAX_PLAYERS;
        if (gameSettings->PlayerClients[i] >= 0) {
          if (numSeekers > seekerTargetCount) {
            if (gameSettings->PlayerTeams[i] == TEAM_SEEKERS) {
              --r;
            }
          } else {
            --r;
          }
        }
      }

      if (numSeekers > seekerTargetCount) {
        --numSeekers;
        gameSettings->PlayerTeams[i] = TEAM_HIDERS;
        ++numHiders;
      } else {
        ++numSeekers;
        gameSettings->PlayerTeams[i] = TEAM_SEEKERS;
        --numHiders;
      }
    }
  }

  set = 1;
}
