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
struct PayloadState State;

void processPlayer(int pIndex);
void resetRoundState(void);
void initialize(PatchGameConfig_t* gameConfig);
void updateGameState(PatchStateContainer_t * gameState);
void gameTick(void);
void frameTick(void);
void setLobbyGameOptions(void);
void setEndGameScoreboard(PatchGameConfig_t * gameConfig);

//--------------------------------------------------------------------------
float computePlayerRank(int playerIdx)
{
	// current stats are stored in the calling function's stack
	int * sp;
	int * gameOverData = (int*)0x001E0D78;

	asm __volatile__ (
		"move %0, $sp"
		: : "r" (sp)
	);

	// get index of gamemode rank's stat in widestats
	// and return that as a float
	int currentRank = sp[8 + ((int (*)(int))0x0077a8f8)(gameOverData[2])];

	// uninstall hook
	*(u32*)0x0077b0d4 = 0x0C1DEAE6;

	// return current rank
	// don't compute new rank for base gamemode
	return (float)currentRank;
	//return ((float (*)(int))0x0077AB98)(playerIdx);
}

int updateHook(void)
{

	return ((int (*)(void))0x005986A8)();
}

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
		initialize(gameConfig);
		return;
	}

	// hook nwUpdate (runs each game tick)
	//HOOK_JAL(0x005986C8, &updateHook);

	//
	if (!State.GameOver)
	{
		if (State.RoundEndTime && gameTime > State.RoundEndTime)
		{
			State.RoundNumber++;

			if (State.RoundNumber < State.RoundLimit)
				resetRoundState();
			else
				State.GameOver = 1;
		}

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

			setEndGameScoreboard(gameConfig);
			initializedScoreboard = 1;

			// hook compute rank
			*(u32*)0x0077b0d4 = 0x0C000000 | ((u32)&computePlayerRank >> 2);
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
	
}
