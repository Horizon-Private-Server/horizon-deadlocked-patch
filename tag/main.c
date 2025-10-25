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
void initialize(PatchStateContainer_t* gameState);
void updateGameState(PatchStateContainer_t * gameState);
void gameTick(void);
void frameTick(void);
void setLobbyGameOptions(PatchStateContainer_t * gameState);
void setEndGameScoreboard(PatchGameConfig_t * gameConfig);

int getTeamCount(void)
{
	int i;
	int count = 0;
	char teams[GAME_MAX_PLAYERS];
	Player** players = playerGetAll();

	memset(teams, 0, sizeof(teams));
	for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
		if (players[i] && !teams[players[i]->Team]) {
			++count;
			teams[players[i]->Team] = 1;
		}
	}

	return count;
}

//--------------------------------------------------------------------------
void gameUpdateTick(struct GameModule * module, PatchStateContainer_t * gameState)
{
	GameSettings * gameSettings = gameGetSettings();
	GameOptions * gameOptions = gameGetOptions();
	Player ** players = playerGetAll();
	int i;
	char buffer[32];
	int gameTime = gameGetTime();
	static int teamsAtStart = 0;

	dlPreUpdate();
	updateGameState(gameState);

	// Ensure in game
	if (!gameSettings || !isInGame())
		return;

	// determine if host
	State.IsHost = gameAmIHost();

  // initialize
	if (!Initialized)
	{
		initialize(gameState);
		teamsAtStart = getTeamCount();
		return;
	}

  if (!State.GameOver)
  {
		// end if all but one team left
		int teamsLeft = getTeamCount();
		if (teamsLeft <= 1 && teamsLeft < teamsAtStart) {
      State.GameOver = 9; // enemies left
    }

    // invoke custom mode game update logic
    gameTick();
  }
  else if (State.GameOver > 0)
  {
    gameSetWinner(State.WinningTeam, 1);
    gameEnd(State.GameOver);
    State.GameOver = -1;
  }

	dlPostUpdate();
}

//--------------------------------------------------------------------------
void gameFrameTick(struct GameModule * module, PatchStateContainer_t * gameState)
{
	if (!gameGetSettings() || !isInGame())
		return;

  // invoke custom mode frame update logic
	if (!State.GameOver)
		frameTick();

}

//--------------------------------------------------------------------------
void lobbyStart(struct GameModule * module, PatchStateContainer_t * gameState)
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

		Initialized = 2;
	}

	// 
	updateGameState(gameState);

	// scoreboard
	switch (activeId)
	{
		case 0x15C:
		{
			if (initializedScoreboard)
				break;

			setEndGameScoreboard(gameState->GameConfig);
			initializedScoreboard = 1;

			// patch rank computation to keep rank unchanged for base mode
			POKE_U32(0x0077ACE4, 0x4600BB06);
			break;
		}
		case UI_ID_GAME_LOBBY:
		{
			setLobbyGameOptions(gameState);
			break;
		}
	}
}

//--------------------------------------------------------------------------
void loadStart(struct GameModule * module, PatchStateContainer_t * gameState)
{
  setLobbyGameOptions(gameState);
}

//--------------------------------------------------------------------------
void start(struct GameModule * module, PatchStateContainer_t * gameState, enum GameModuleContext context)
{
  switch (context)
  {
    case GAMEMODULE_LOBBY: lobbyStart(module, gameState); break;
    case GAMEMODULE_LOAD: loadStart(module, gameState); break;
    case GAMEMODULE_GAME_FRAME: gameFrameTick(module, gameState); break;
    case GAMEMODULE_GAME_UPDATE: gameUpdateTick(module, gameState); break;
  }
}
