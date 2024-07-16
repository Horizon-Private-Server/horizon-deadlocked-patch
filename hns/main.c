/***************************************************
 * FILENAME :		main.c
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

  // disable quick chat, health boxes, and v2s
  PatchGameConfig_t* gameConfig = gameState->GameConfig;
  gameConfig->grQuickChat = 0;
  gameConfig->grNoHealthBoxes = 2;
  gameConfig->grV2s = 2;

	//
	dlPreUpdate();

	// 
	updateGameState(gameState);

	// Ensure in game
	if (!gameSettings || !isInGame())
		return;

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

  // prevent any stats from saving
  gameSetIsGameRanked(0);

	// 
	updateGameState(gameState);

	switch (activeId)
	{
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
  setLobbyGameOptions();
}

//--------------------------------------------------------------------------
void start(struct GameModule * module, PatchStateContainer_t * gameState, enum GameModuleContext context)
{
  switch (context)
  {
    case GAMEMODULE_LOBBY: lobbyStart(module, gameState); break;
    case GAMEMODULE_LOAD: loadStart(module, gameState); break;
    case GAMEMODULE_GAME: gameStart(module, gameState); break;
  }
}
