/***************************************************
 * FILENAME :		scoreboard.c
 * 
 * DESCRIPTION :
 * 		Handles gamemode scoreboard.
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
#include "module.h"
#include "messageid.h"
#include "include/game.h"

/*
 * Target scoreboard value.
 */
#define GAME_SCOREBOARD_TARGET              (*(u32*)0x002FA084)

extern int Initialized;
extern struct State State;

//--------------------------------------------------------------------------
void setTeamScore(int team, int score)
{
	gameScoreboardSetTeamScore(team, gameGetTeamScore(team, score));
}

//--------------------------------------------------------------------------
void updateTeamScore(int team)
{
	setTeamScore(team, 0);
}

void initializeScoreboard(void)
{
	*(u32*)0x005404f0 = 0x0C000000 | ((u32)&setTeamScore >> 2);
	GAME_SCOREBOARD_TARGET = 0;
}

//--------------------------------------------------------------------------
void setEndGameScoreboard(PatchGameConfig_t * gameConfig)
{
	u32 * uiElements = (u32*)(*(u32*)(0x011C7064 + 4*18) + 0xB0);
	int i;
	char buf[32];

	// column headers start at 17
	strncpy((char*)(uiElements[18] + 0x60), "DISTANCE", 9);
	strncpy((char*)(uiElements[19] + 0x60), "TIME", 5);

	// rows
	int* pids = (int*)(uiElements[0] - 0x9C);
	for (i = 0; i < GAME_MAX_PLAYERS; ++i)
	{
		// match scoreboard player row to their respective dme id
		int pid = pids[i];
		char* name = (char*)(uiElements[7 + i] + 0x18);
		if (pid < 0 || name[0] == 0)
			continue;

		// set distance
		sprintf(buf, "%.2f", State.PlayerBestHeight[pid]);
		strncpy((char*)(uiElements[22 + (i*4) + 0] + 0x60), buf, strlen(buf) + 1);

		// set time alive
		int pTime = State.TimePlayerDied[pid] - State.StartTime;
		if (pTime < 0)
			pTime = State.EndTime - State.StartTime;

		sprintf(buf, "%02d:%02d", pTime / TIME_MINUTE, (pTime % TIME_MINUTE) / TIME_SECOND);
		strncpy((char*)(uiElements[22 + (i*4) + 1] + 0x60), buf, strlen(buf) + 1);
	}
}
