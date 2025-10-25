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
void setLobbyGameOptions(void);
void setEndGameScoreboard(PatchGameConfig_t * gameConfig);

//--------------------------------------------------------------------------
void gameStart(struct GameModule * module, PatchStateContainer_t * gameState)
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
		initialize(gameState);
		return;
	}

  // handle tick
	if (!gameHasEnded())
	{
		frameTick();
		gameTick();
	}
  
  // end game
  if (gameHasEnded() && State.IsHost) {
    sendTeamScore();
  }

	dlPostUpdate();
	return;
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

    // use team stats for bottom summary
    POKE_U32(0x0073dbf4, 0);

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
			setLobbyGameOptions();
			break;
		}
	}
}

//--------------------------------------------------------------------------
void loadStart(struct GameModule * module, PatchStateContainer_t * gameState)
{
  voiceEnableGlobalChat(0); // team chat
  setLobbyGameOptions();
}

//--------------------------------------------------------------------------
void start(struct GameModule * module, PatchStateContainer_t * gameState, enum GameModuleContext context)
{
  switch (context)
  {
    case GAMEMODULE_LOBBY: lobbyStart(module, gameState); break;
    case GAMEMODULE_LOAD: loadStart(module, gameState); break;
    case GAMEMODULE_GAME_FRAME: gameStart(module, gameState); break;
    case GAMEMODULE_GAME_UPDATE: break;
  }
}
