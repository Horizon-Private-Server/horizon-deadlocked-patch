/***************************************************
 * FILENAME :		game.c
 * 
 * DESCRIPTION :
 * 		TEAM DEFENDER.
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
void onSetPlayerStats(int playerId, struct CGMPlayerStats* stats)
{
	memcpy(&State.PlayerStates[playerId].Stats, stats, sizeof(struct CGMPlayerStats));
}

//--------------------------------------------------------------------------
void onReceiveTeamScore(int teamScores[GAME_MAX_PLAYERS])
{
	int i;
	for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
		if (State.Teams[0].TeamId == i) {
			State.Teams[0].Score = teamScores[i];
		} else if (State.Teams[1].TeamId == i) {
			State.Teams[1].Score = teamScores[i];
		}
	}
}

//--------------------------------------------------------------------------
void onPlayerKill(char * fragMsg)
{
	VECTOR delta;
	Player** players = playerGetAll();

	// call base function
	((void (*)(char*))0x00621CF8)(fragMsg);
}

//--------------------------------------------------------------------------
int gameGetTeamScore(int team, int score)
{
  if (team == State.Teams[0].TeamId)
    return State.Teams[0].Score;
  if (team == State.Teams[1].TeamId)
    return State.Teams[1].Score;

  return score;
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

  // hide waiting for players popup
  hudHidePopup();

	// determine teams
	for (i = 0; i < 2; ++i) {
		State.Teams[i].TeamId = -1;
		State.Teams[i].Score = 0;
	}

	int teamCount = 0;
	for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
		char team = gameSettings->PlayerTeams[i];
		if (team >= 0) {
			if (team != State.Teams[0].TeamId && team != State.Teams[1].TeamId) {
				State.Teams[teamCount++].TeamId = team;

				if (teamCount >= 2)
					break;
			}
		}
	}

	// 
	if (teamCount == 1)
		State.Teams[1].TeamId = (State.Teams[0].TeamId + 1) % 10;

	// initialize player states
	State.LocalPlayerState = NULL;
	for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
		Player * p = players[i];
		State.PlayerStates[i].PlayerIndex = i;
		State.PlayerStates[i].Stats.Points = 0;
		State.PlayerStates[i].Stats.TimeWithFlag = 0;

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
	if (gameState->UpdateCustomGameStats)
	{
    gameState->CustomGameStatsSize = sizeof(struct CGMGameData);
		struct CGMGameData* sGameData = (struct CGMGameData*)gameState->CustomGameStats.Payload;
		sGameData->Rounds = 0;
		sGameData->Version = 0x00000001;

		for (i = 0; i < GAME_MAX_PLAYERS; ++i)
		{
			sGameData->TeamScore[i] = 0;
      sGameData->TimeWithFlag[i] = State.PlayerStates[i].Stats.TimeWithFlag;
			sGameData->Points[i] = State.PlayerStates[i].Stats.Points;
		}

		for (i = 0; i < 2; ++i)
		{
			sGameData->TeamScore[State.Teams[i].TeamId] = State.Teams[i].Score;
		}
	}
}

//--------------------------------------------------------------------------
void setLobbyGameOptions(void)
{
	// set game options
	GameOptions * gameOptions = gameGetOptions();
	GameSettings* gameSettings = gameGetSettings();
	if (!gameOptions || !gameSettings || gameSettings->GameLoadStartTime <= 0)
		return;
	
	// apply options
	gameOptions->GameFlags.MultiplayerGameFlags.Juggernaut = 0;
	gameOptions->GameFlags.MultiplayerGameFlags.Survivor = 0;
	gameOptions->GameFlags.MultiplayerGameFlags.Teamplay = 1;
}
