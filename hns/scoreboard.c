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
#include "include/utils.h"

extern int Initialized;

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
}

//--------------------------------------------------------------------------
void setEndGameScoreboard(PatchGameConfig_t * gameConfig)
{
  // do nothing
}
