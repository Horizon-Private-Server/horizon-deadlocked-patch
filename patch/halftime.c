#include <tamtypes.h>
#include <string.h>

#include <libdl/time.h>
#include <libdl/game.h>
#include <libdl/gamesettings.h>
#include <libdl/map.h>
#include <libdl/player.h>
#include <libdl/cheats.h>
#include <libdl/stdio.h>
#include <libdl/pad.h>
#include <libdl/dl.h>
#include <libdl/spawnpoint.h>
#include <libdl/ui.h>
#include "module.h"


/*
 * When non-zero, it refreshes the in-game scoreboard.
 */
#define GAME_SCOREBOARD_REFRESH_FLAG        (*(u32*)0x00310248)

/*
 * Target scoreboard value.
 */
#define GAME_SCOREBOARD_TARGET              (*(u32*)0x002FA084)

/*
 * Collection of scoreboard items.
 */
#define GAME_SCOREBOARD_ARRAY               ((ScoreboardItem**)0x002FA04C)

/*
 * Number of items in the scoreboard.
 */
#define GAME_SCOREBOARD_ITEM_COUNT          (*(u32*)0x002F9FCC)



enum HalfTimeStates
{
	HT_WAITING,
	HT_INTERMISSION,
	HT_SWITCH,
	HT_INTERMISSION2,
	HT_COMPLETED
};

/* 
 * 
 */
int HalfTimeState = 0;

/*
 * Indicates when the half time grace period should end.
 */
int HalfTimeEnd = -1;

/*
 * Flag moby by team id.
 */
Moby * CtfFlags[4] = {0,0,0,0};


/*
 * NAME :		htReset
 * 
 * DESCRIPTION :
 * 			Resets the state.
 * 
 * NOTES :
 * 
 * ARGS : 
 * 
 * RETURN :
 * 
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */
void htReset(void)
{
    HalfTimeState = 0;
	HalfTimeEnd = -1;
	CtfFlags[0] = CtfFlags[1] = CtfFlags[2] = CtfFlags[3] = 0;
}

/*
 * NAME :		getFlags
 * 
 * DESCRIPTION :
 * 			Grabs all four flags and stores their moby pointers in CtfFlags.
 * 
 * NOTES :
 * 
 * ARGS : 
 * 
 * RETURN :
 * 
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */
void getFlags(void)
{
	Moby * moby = mobyListGetStart();
	Moby * mEnd = mobyListGetEnd();

	// reset
	CtfFlags[0] = CtfFlags[1] = CtfFlags[2] = CtfFlags[3] = 0;

	// grab flags
	while (moby < mEnd)
	{
		if (!mobyIsDestroyed(moby) &&
		 (moby->OClass == MOBY_ID_BLUE_FLAG ||
			moby->OClass == MOBY_ID_RED_FLAG ||
			moby->OClass == MOBY_ID_GREEN_FLAG ||
			moby->OClass == MOBY_ID_ORANGE_FLAG))
		{
			CtfFlags[*(u16*)(moby->PVar + 0x14)] = moby;
		}

		++moby;
	}
}

/*
 * NAME :		htCtfBegin
 * 
 * DESCRIPTION :
 * 			Performs all the operations needed at the start of half time.
 * 
 * NOTES :
 * 
 * ARGS : 
 * 
 * RETURN :
 * 
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */
void htCtfBegin(void)
{
	// Make sure we have the flag mobies
	getFlags();

	// Indicate when the intermission should end
	HalfTimeEnd = gameGetTime() + (TIME_SECOND * 5);
	HalfTimeState = HT_INTERMISSION;

	// Disable saving or pickup up flag
	gameFlagSetPickupDistance(0);

	// Show popup
	uiShowPopup(0, "Halftime");
	uiShowPopup(1, "Halftime");
}


/*
 * NAME :		htCtfSwitch
 * 
 * DESCRIPTION :
 * 			
 * 
 * NOTES :
 * 
 * ARGS : 
 * 
 * RETURN :
 * 
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */
void htCtfSwitch(void)
{
	int i, j;
	Player ** players = playerGetAll();
	GameSettings* gameSettings = gameGetSettings();
	Player * player;
	Moby * moby;
	ScoreboardItem * scoreboardItem;
	VECTOR rVector, pVector;
	u8 * teamCaps = (u8*)0x0036DC4C;
	int teams = 0;
	u8 teamChangeMap[4] = {0,1,2,3};
	u8 backupTeamCaps[4];
	int scoreboardItemCount = GAME_SCOREBOARD_ITEM_COUNT;

	// 
	memset(rVector, 0, sizeof(VECTOR));

	// backup
	memcpy(backupTeamCaps, teamCaps, 4);
	
	// Determine teams
	for (i = 0; i < GAME_MAX_PLAYERS; ++i)
	{
		player = players[i];
		if (!player)
			continue;
			
		teams |= (1 << (player->Team+1));
	}

	// If all four teams then just swap
	if (teams == 0xF)
	{
		teamChangeMap[TEAM_BLUE] = TEAM_RED;
		teamChangeMap[TEAM_RED] = TEAM_BLUE;
		teamChangeMap[TEAM_GREEN] = TEAM_ORANGE;
		teamChangeMap[TEAM_ORANGE] = TEAM_GREEN;
	}
	// Otherwise rotate the teams
	else
	{
		for (i = 0; i < 4; ++i)
		{
			if (!(teams & (1 << (i+1))))
				continue;

			j = i;
			do
			{
				++j;
				if (j >= 4)
					j = 0;

				if (!(teams & (1 << (j+1))))
					continue;

				teamChangeMap[i] = j;
				break;
			} while (j != i);
		}
	}

	// Switch player teams
	for (i = 0; i < GAME_MAX_PLAYERS; ++i)
	{
		player = players[i];
		if (!player)
			continue;

		if (playerIsLocal(player))
		{
			// Update local scoreboard
			for (j = 0; j < scoreboardItemCount; ++j)
			{
				scoreboardItem = GAME_SCOREBOARD_ARRAY[j];
				if (!scoreboardItem)
					continue;

				// Swap
				if (scoreboardItem->TeamId == teamChangeMap[player->Team])
				{
					ScoreboardItem * temp = GAME_SCOREBOARD_ARRAY[player->LocalPlayerIndex];
					GAME_SCOREBOARD_ARRAY[player->LocalPlayerIndex] = scoreboardItem;
					GAME_SCOREBOARD_ARRAY[j] = temp;
					break;
				}
			}
		}

		// Kick from vehicle
		if (player->Vehicle)
			vehicleRemovePlayer(player->Vehicle, player);

		// Change to new team
		playerSetTeam(player, teamChangeMap[player->Team]);

		// 
		gameSettings->PlayerTeams[player->PlayerId] = player->Team;
		
		// Respawn
		playerRespawn(player);

		// Teleport player to base
		moby = CtfFlags[player->Team];
		if (moby)
		{
			vector_copy(pVector, (float*)moby->PVar);
			float theta = (player->PlayerId / (float)GAME_MAX_PLAYERS) * MATH_TAU;
			pVector[0] += cosf(theta) * 2.5;
			pVector[1] += sinf(theta) * 2.5;
			playerSetPosRot(player, pVector, rVector);
		}
	}

	// Switch team scores
	for (i = 0; i < 4; ++i)
		teamCaps[i] = backupTeamCaps[teamChangeMap[i]];

	// reset flags
	for (i = 0; i < 4; ++i)
	{
		if (CtfFlags[i])
		{
			*(u16*)(CtfFlags[i]->PVar + 0x10) = 0xFFFF;
			vector_copy((float*)&CtfFlags[i]->Position, (float*)CtfFlags[i]->PVar);
		}
	}
}

/*
 * NAME :		htCtfEnd
 * 
 * DESCRIPTION :
 * 			
 * 
 * NOTES :
 * 
 * ARGS : 
 * 
 * RETURN :
 * 
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */
void htCtfEnd(void)
{
	// Enable flag pickup
	gameFlagSetPickupDistance(2);
}


/*
 * NAME :		htCtfTick
 * 
 * DESCRIPTION :
 * 			Performs all the operations needed while in intermission.
 * 
 * NOTES :
 * 
 * ARGS : 
 * 
 * RETURN :
 * 
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */
void htCtfTick(void)
{
	int i;
	Player ** players = playerGetAll();
	int gameTime = gameGetTime();

	// Prevent input
	padResetInput(0);
	padResetInput(1);

	switch (HalfTimeState)
	{
		case HT_INTERMISSION:
		{
			if (gameTime > (HalfTimeEnd - (TIME_SECOND * 2)))
				HalfTimeState = HT_SWITCH;
			break;
		}
		case HT_SWITCH:
		{
			// Show popup
			if (gameTime > (HalfTimeEnd - (TIME_SECOND * 2)))
			{
				uiShowPopup(0, "switching sides...");
				uiShowPopup(1, "switching sides...");

				htCtfSwitch();
				HalfTimeState = HT_INTERMISSION2;
			}
			break;
		}
		case HT_INTERMISSION2:
		{
			// Drop held items
			for (i = 0; i < GAME_MAX_PLAYERS; ++i)
			{
				if (!players[i])
					continue;

				players[i]->HeldMoby = 0;
			}

			// reset flags
			for (i = 0; i < 4; ++i)
			{
				if (CtfFlags[i])
				{
					*(u16*)(CtfFlags[i]->PVar + 0x10) = 0xFFFF;
					vector_copy((float*)&CtfFlags[i]->Position, (float*)CtfFlags[i]->PVar);
				}
			}

			if (gameTime > HalfTimeEnd)
			{
				htCtfEnd();
				HalfTimeState = HT_COMPLETED;
			}
			break;
		}
	}
	
}

/*
 * NAME :		halftimeLogic
 * 
 * DESCRIPTION :
 * 			Checks if half the game has passed, and then flips the sides if possible.
 * 
 * NOTES :
 * 
 * ARGS : 
 * 
 * RETURN :
 * 
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */
void halftimeLogic(void)
{
	int timeLimit = gameGetRawTimeLimit();
	int gameTime = gameGetTime();
	GameSettings * gameSettings = gameGetSettings();

	// Check we're in game and that it is compatible
	if (!gameSettings || gameSettings->GameRules != GAMERULE_CTF)
		return;

	// 
	switch (HalfTimeState)
	{
		case HT_WAITING:
		{
			if (timeLimit <= 0)
				break;

			// Trigger halfway through game
			u32 gameHalfTime = gameSettings->GameStartTime + (timeLimit / 2);
			if (gameTime > gameHalfTime)
			{
				htCtfBegin();
				HalfTimeState = HT_INTERMISSION;
			}
			break;
		}
		case HT_COMPLETED:
		{
			break;
		}
		default:
		{
			htCtfTick();
			break;
		}
	}
}
