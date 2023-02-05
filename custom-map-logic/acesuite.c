/***************************************************
 * FILENAME :		acesuite.c
 * 
 * DESCRIPTION :
 * 		Custom map logic for Ace Hardlight's Suite.
 * 		Hides/Shows tunnel barrier mobys depending if Green/Orange flags are spawned.
 * 		
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */

#include <tamtypes.h>

#include <libdl/dl.h>
#include <libdl/player.h>
#include <libdl/pad.h>
#include <libdl/time.h>
#include <libdl/net.h>
#include "module.h"
#include "messageid.h"
#include <libdl/game.h>
#include <libdl/string.h>
#include <libdl/stdio.h>
#include <libdl/gamesettings.h>
#include <libdl/dialog.h>
#include <libdl/patch.h>
#include <libdl/ui.h>
#include <libdl/graphics.h>

/*
 * NAME :		main
 * 
 * DESCRIPTION :
 * 			Entrypoint.
 * 
 * NOTES :
 * 
 * ARGS : 
 * 
 * RETURN :
 * 
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */
int main (void)
{
	static int initialized = 0;
	int i;
	int hasGreenOrangeFlag = 0;

	if (initialized)
		return;

	// determine if green or orange flags are spawned
	GameSettings* gs = gameGetSettings();
	if (!gs)
		return;

	if (gs->GameRules == GAMERULE_CTF) {
		for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
			if (gs->PlayerTeams[i] == TEAM_GREEN || gs->PlayerTeams[i] == TEAM_ORANGE) {
				hasGreenOrangeFlag = 1;
				break;
			}
		}
	}

	// if we don't have then destroy mobys in 0xFE group
	if (!hasGreenOrangeFlag) {
		Moby* moby = mobyListGetStart();
		Moby* mEnd = mobyListGetEnd();

		while (moby < mEnd) {

			if (moby->Group == 0xFE) {
				DPRINTF("destroying %d %04X\n", (u32)moby, moby->OClass);
				mobyDestroy(moby);
			}

			++moby;
		}
	}

	initialized = 1;

	return 0;
}
