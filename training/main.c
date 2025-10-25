/***************************************************
 * FILENAME :		main.c
 * 
 * DESCRIPTION :
 * 		TRAINING.
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
struct TrainingState State;

void processPlayer(int pIndex);
void initialize(PatchGameConfig_t* gameConfig);
void updateGameState(PatchStateContainer_t * gameState);
void gameTick(void);
void frameTick(void);
void modeSetLobbyGameOptions(PatchGameConfig_t * gameConfig);
void modeSetEndGameScoreboard(PatchGameConfig_t * gameConfig);

const char* TRAINING_AGGRO_NAMES[] = {
  [TRAINING_AGGRESSION_AGGRO] "AGGRO",
  [TRAINING_AGGRESSION_AGGRO_NO_DAMAGE] "NO DAMAGE",
  [TRAINING_AGGRESSION_PASSIVE] "PASSIVE",
  [TRAINING_AGGRESSION_IDLE] "IDLE",
};

//--------------------------------------------------------------------------
void correctEndGameData(void)
{
	GameSettings * gameSettings = gameGetSettings();
	int i;

	// fix game over data
	// for some reason adding fake players messes up the data
	// so we need to write the local player's name back and remove all the other names
	u32 gameOverDataAddr = 0x001e0d78;

	strncpy((char*)(gameOverDataAddr + 0x410), gameSettings->PlayerNames[0], 16);
	for (i = 1; i < GAME_MAX_PLAYERS; ++i) {
		*(u32*)(gameOverDataAddr + 0x410 + (i * 0x10)) = 0;
	}

	POKE_U32(gameOverDataAddr + 0x1c, 0);
	POKE_U32(gameOverDataAddr + 0x20, 0);
}

//--------------------------------------------------------------------------
void gameStart(struct GameModule * module, PatchStateContainer_t * gameState)
{
	GameSettings * gameSettings = gameGetSettings();

	//
	dlPreUpdate();

	// 
	updateGameState(gameState);

	// Ensure in game
	if (!gameSettings || !isInGame())
		return;

	// Determine if host
	State.IsHost = gameAmIHost();
  State.AggroMode = gameState->GameConfig->trainingConfig.aggression;

	if (!Initialized)
	{
		initialize(gameState->GameConfig);
		return;
	}

	//
	if (!State.GameOver)
	{
		// handle frame tick
		frameTick();
		gameTick();
	}
	else
	{
		// end game
		if (State.GameOver == 1)
		{
			gameSetWinner(State.WinningTeam, 1);
			gameEnd(4);
			State.GameOver = 2;
		}
	}

	if (gameHasEnded())
		correctEndGameData();

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
					//sendPlayerStats(i);
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

			modeSetEndGameScoreboard(gameState->GameConfig);
			initializedScoreboard = 1;

			// patch rank computation to keep rank unchanged for base mode
			POKE_U32(0x0077ACE4, 0x4600BB06);
			break;
		}
		case UI_ID_GAME_LOBBY:
		{
			modeSetLobbyGameOptions(gameState->GameConfig);
			break;
		}
	}
}

//--------------------------------------------------------------------------
void loadStart(struct GameModule * module, PatchStateContainer_t * gameState)
{
	modeSetLobbyGameOptions(gameState->GameConfig);
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
