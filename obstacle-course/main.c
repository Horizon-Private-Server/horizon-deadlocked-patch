/***************************************************
 * FILENAME :		main.c
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

//--------------------------------------------------------------------------
void gameUpdateTick(struct GameModule * module, PatchStateContainer_t * gameState)
{
	GameSettings * gameSettings = gameGetSettings();
	GameOptions * gameOptions = gameGetOptions();
	Player ** players = playerGetAll();
	int i;
	char buffer[32];
	int gameTime = gameGetTime();

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
		return;
	}

  if (!State.GameOver)
  {
		// end if all but one team left
		// int teamsLeft = getTeamCount();
		// if (teamsLeft <= 1 && teamsLeft < teamsAtStart) {
    //   State.GameOver = 9; // enemies left
    // }

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

	// 
	updateGameState(gameState);

  // disable ranking
  gameSetIsGameRanked(0);

	// scoreboard
	switch (activeId)
	{
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
  
	if (!Initialized)
		initialize(gameState);
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
