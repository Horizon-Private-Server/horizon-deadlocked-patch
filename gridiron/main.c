/***************************************************
 * FILENAME :		main.c
 * 
 * DESCRIPTION :
 * 		PAYLOAD.
 * 		
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */

#include <tamtypes.h>
#include <string.h>

#include <libdl/time.h>
#include <libdl/game.h>
#include <libdl/gamesettings.h>
#include <libdl/player.h>
#include <libdl/weapon.h>
#include <libdl/hud.h>
#include <libdl/cheats.h>
#include <libdl/sha1.h>
#include <libdl/dialog.h>
#include <libdl/ui.h>
#include <libdl/stdio.h>
#include <libdl/graphics.h>
#include <libdl/spawnpoint.h>
#include <libdl/random.h>
#include <libdl/net.h>
#include <libdl/sound.h>
#include <libdl/dl.h>
#include <libdl/utils.h>
#include "module.h"
#include "messageid.h"
#include "include/game.h"
#include "include/utils.h"

int Initialized = 0;
struct CGMState State;

void processPlayer(int pIndex);
void resetRoundState(void);
void initialize(PatchGameConfig_t* gameConfig, PatchStateContainer_t* gameState);
void updateGameState(PatchStateContainer_t * gameState);
void gameTick(PatchStateContainer_t * gameState);
void frameTick(PatchStateContainer_t * gameState);
void setLobbyGameOptions(void);
void setEndGameScoreboard(PatchGameConfig_t * gameConfig);

//--------------------------------------------------------------------------
void gameStart(struct GameModule * module, PatchConfig_t * config, PatchGameConfig_t * gameConfig, PatchStateContainer_t * gameState)
{
	GameSettings * gameSettings = gameGetSettings();
	GameOptions * gameOptions = gameGetOptions();
	Player ** players = playerGetAll();
	int i;
	char buffer[32];
	int gameTime = gameGetTime();

	//
	dlPreUpdate();

	// 
	updateGameState(gameState);

	// Ensure in game
	if (!gameSettings || !isInGame())
		return;

	// Determine if host
	State.IsHost = gameAmIHost();

	if (!Initialized)
	{
		initialize(gameConfig, gameState);
		return;
	}

  // halftime, flip score
  static int lastHtValue = 0;
  if (lastHtValue != gameState->HalfTimeState) {
    if (gameState->HalfTimeState == 2) {
      int score = State.Teams[0].Score;
      State.Teams[0].Score = State.Teams[1].Score;
      State.Teams[1].Score = score;
      DPRINTF("flip score\n");
    } else if (gameState->HalfTimeState == 3) {
      ballReset(State.BallMoby, BALL_RESET_CENTER);
      DPRINTF("reset ball\n");
    }

    lastHtValue = gameState->HalfTimeState;
  }

  // overtime, reset ball
  static int lastOtValue = 0;
  if (lastOtValue != gameState->OverTimeState) {
    if (gameState->OverTimeState == 3) {
      DPRINTF("ot reset ball\n");
      ballReset(State.BallMoby, BALL_RESET_CENTER);
    }

    lastOtValue = gameState->OverTimeState;
  }

  // handle tick
	if (!gameHasEnded())
	{
		frameTick(gameState);
		gameTick(gameState);
	}
  
  // end game
  if (gameHasEnded()) {
    if (State.IsHost) {
      sendTeamScore();
    }

    // set winner
    if (State.Teams[0].Score > State.Teams[1].Score)
      gameSetWinner(State.Teams[0].TeamId, 1);
    else if (State.Teams[0].Score < State.Teams[1].Score)
      gameSetWinner(State.Teams[1].TeamId, 1);
  }

	dlPostUpdate();
	return;
}

//--------------------------------------------------------------------------
void lobbyStart(struct GameModule * module, PatchConfig_t * config, PatchGameConfig_t * gameConfig, PatchStateContainer_t * gameState)
{
	int i;
	int activeId = uiGetActive();
	Player** players = playerGetAll();
	static int initializedScoreboard = 0;

	// send final player stats to others
	if (Initialized == 1) {
		if (State.IsHost) {
			for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
				if (players[i]) {
					sendPlayerStats(i);
				}
			}
		}

    // use team stats for bottom summary
    POKE_U32(0x0073dbf4, 0);

		Initialized = 2;
	}

  // disable ranking
  gameSetIsGameRanked(0);

	// 
	updateGameState(gameState);

	// scoreboard
	switch (activeId)
	{
		case 0x15C:
		{
			if (initializedScoreboard)
				break;

			setEndGameScoreboard(gameConfig);
			initializedScoreboard = 1;

			// patch rank computation to keep rank unchanged for base mode
			POKE_U32(0x0077ACE4, 0x4600BB06);
			break;
		}
		case UI_ID_GAME_LOBBY:
		{
			setLobbyGameOptions();
			break;
		}
	}
}

//--------------------------------------------------------------------------
void loadStart(void)
{
  setLobbyGameOptions();
}
