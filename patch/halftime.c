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
#include <libdl/utils.h>
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


enum OvertimeStates
{
	OT_WAITING,
	OT_INTERMISSION,
	OT_SWITCH,
	OT_INTERMISSION2,
	OT_COMPLETED,
  OT_GAMEOVER
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
 *
 */
int OvertimeState = 0;

/*
 * Indicates when the overtime grace period should end.
 */
int OvertimeEnd = -1;

/*
 * Flag moby by team id.
 */
Moby * CtfFlags[4] = {0,0,0,0};
VECTOR CtfHtOtFreezePositions[GAME_MAX_PLAYERS];

extern PatchStateContainer_t patchStateContainer;

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
  patchStateContainer.HalfTimeState = 0;
  memset(CtfHtOtFreezePositions, 0, sizeof(CtfHtOtFreezePositions));
}

/*
 * NAME :		htDestroyPlayerObjects
 * 
 * DESCRIPTION :
 * 			Destroys all player owned/created mobys.
 *        * Mines
 * 
 * NOTES :
 * 
 * ARGS : 
 * 
 * RETURN :
 * 
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */
void htDestroyPlayerObjects(void)
{
  Moby* moby = mobyListGetStart();
	while ((moby = mobyFindNextByOClass(moby, MOBY_ID_MINE_LAUNCHER_MINE)))
	{
		if (!mobyIsDestroyed(moby)) {
      moby->State = 3; // destroy
    }

		++moby;
	}
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
  int i;
  Player** players = playerGetAll();

	// Make sure we have the flag mobies
	getFlags();

  // destroy player objects
  htDestroyPlayerObjects();

	// Indicate when the intermission should end
	HalfTimeEnd = gameGetTime() + (TIME_SECOND * 3);
	HalfTimeState = HT_INTERMISSION;

	// Disable saving or pickup up flag
	gameFlagSetPickupDistance(0);

  // Freeze players
  for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
    Player* player = players[i];
    if (player) {
      vector_copy(CtfHtOtFreezePositions[i], player->PlayerPosition);
    }
  }

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
		
		// Respawn (first res patch)
    POKE_U32(0x205e2d48, 0x24070001);
		playerRespawn(player);
    POKE_U32(0x205e2d48, 0x0000382d);
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

  // prevent pad input
  padResetInput(0);
  padResetInput(1);
  
  patchStateContainer.HalfTimeState = HalfTimeState;
	switch (HalfTimeState)
	{
		case HT_INTERMISSION:
		{
      // lock players to positions
      for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
        Player* player = players[i];
        if (player && player->PlayerMoby && player->IsLocal) {
          vector_copy(player->PlayerPosition, CtfHtOtFreezePositions[i]);
          vector_copy(player->PlayerMoby->Position, CtfHtOtFreezePositions[i]);
        }
      }

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
			// Drop held items and reset health
			for (i = 0; i < GAME_MAX_PLAYERS; ++i)
			{
				if (!players[i])
					continue;

        playerDropFlag(players[i], 1);
        players[i]->Health = players[i]->MaxHealth;
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


/*
 * NAME :		otReset
 * 
 * DESCRIPTION :
 * 			Resets the overtime state.
 * 
 * NOTES :
 * 
 * ARGS : 
 * 
 * RETURN :
 * 
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */
void otReset(void)
{
  OvertimeState = 0;
  OvertimeEnd = -1;
	CtfFlags[0] = CtfFlags[1] = CtfFlags[2] = CtfFlags[3] = 0;
  patchStateContainer.OverTimeState = 0;
  memset(CtfHtOtFreezePositions, 0, sizeof(CtfHtOtFreezePositions));
}

/*
 * NAME :		otCtfBegin
 * 
 * DESCRIPTION :
 * 			Performs all the operations needed at the start of overtime.
 * 
 * NOTES :
 * 
 * ARGS : 
 * 
 * RETURN :
 * 
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */
void otCtfBegin(void)
{
  int i;
  Player** players = playerGetAll();

	// Make sure we have the flag mobies
	getFlags();

  // destroy player objects
  htDestroyPlayerObjects();

	// Indicate when the intermission should end
	OvertimeEnd = gameGetTime() + (TIME_SECOND * 3);
	OvertimeState = OT_INTERMISSION;

	// Disable saving or pickup up flag
	gameFlagSetPickupDistance(0);

  // Freeze players
  for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
    Player* player = players[i];
    if (player) {
      vector_copy(CtfHtOtFreezePositions[i], player->PlayerPosition);
    }
  }

  // disable healthboxes
  cheatsDisableHealthboxes();

	// Show popup
	uiShowPopup(0, "Overtime");
	uiShowPopup(1, "Overtime");
}

/*
 * NAME :		otCtfRespawnPlayers
 * 
 * DESCRIPTION :
 * 			Resets players and flags back to their bases.
 * 
 * NOTES :
 * 
 * ARGS : 
 * 
 * RETURN :
 * 
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */
void otCtfRespawnPlayers(void)
{
  Player** players = playerGetAll();
  VECTOR pVector, rVector = {0,0,0,0};
  Moby* moby;
  int i;

  for (i = 0; i < GAME_MAX_PLAYERS; ++i)
  {
    Player* player = players[i];
    if (!player)
      continue;

		// Kick from vehicle
		if (player->Vehicle)
			vehicleRemovePlayer(player->Vehicle, player);

		// Respawn (first res patch)
    POKE_U32(0x205e2d48, 0x24070001);
		playerRespawn(player);
    POKE_U32(0x205e2d48, 0x0000382d);
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
}

/*
 * NAME :		otCtfEnd
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
void otCtfEnd(void)
{
	// Enable flag pickup
	gameFlagSetPickupDistance(2);

  // Enable survivor
  //GameOptions* go = gameGetOptions();
  //if (go) {
  //  go->GameFlags.MultiplayerGameFlags.Survivor = 1;
  //  go->GameFlags.MultiplayerGameFlags.RespawnTime = -1;
  //}
}

/*
 * NAME :		otGetScores
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
int otGetScores(int* bestScore, int* winningTeam)
{
  int i;
  int best = -1;
  int teamsWithBest = 0;
  int teamWithBest = -1;
  int teams[4] = {0,0,0,0};
  Player** players = playerGetAll();

  for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
    Player* player = players[i];
    if (player && player->PlayerMoby && player->pNetPlayer && player->Team >= 0 && player->Team < 4) {
      teams[player->Team] += 1;
    }
  }

  GameData * gameData = gameGetData();
  if (gameData) {
    for (i = 0; i < 4; ++i) {
      if (teams[i]) {
        if (gameData->TeamStats.FlagCaptureCounts[i] > best) {
          best = gameData->TeamStats.FlagCaptureCounts[i];
          teamsWithBest = 1;
          teamWithBest = i;
        } else if (gameData->TeamStats.FlagCaptureCounts[i] == best) {
          teamsWithBest += 1;
          teamWithBest = -1;
        }
      }
    }
  }

  if (bestScore) {
    *bestScore = best;
  }

  if (winningTeam) {
    *winningTeam = teamWithBest;
  }

  return teamsWithBest;
}

/*
 * NAME :		otGetWinningTeam
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
int otGetWinningTeam(void)
{
  int i;
  int playersAlive = 0;
  int winningTeam = -1;
  Player** players = playerGetAll();
  GameOptions* go = gameGetOptions();

  // if 1 team has most caps then return that team
  if (otGetScores(NULL, &winningTeam) <= 1) {
    return winningTeam;
  }

  // otherwise no winner yet
  return -1;

  // otherwise if survivor is on, return last team alive
  winningTeam = -1;
  if (go && go->GameFlags.MultiplayerGameFlags.Survivor) {
    for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
      Player* player = players[i];

      // skip players that left/don't exist
      if (!player || !player->PlayerMoby || !player->pNetPlayer)
        continue;

      if (playerIsDead(player))
        continue;

      ++playersAlive;

      // either set winning team to player (if not yet set)
      // or if the player is not the current winning team
      // then that means we have more than one team alive
      if (winningTeam < 0) {
        winningTeam = player->Team;
      }
      else if (winningTeam != player->Team) {
        winningTeam = -1;
        break;
      }
    }
  }

  // no one is alive == draw
  if (playersAlive == 0) {
    return -2;
  }

  return winningTeam;
}

/*
 * NAME :		otOnTimelimitReached
 * 
 * DESCRIPTION :
 * 			Triggers when the game ends.
 * 
 * NOTES :
 * 
 * ARGS : 
 * 
 * RETURN :
 * 
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */
void otOnTimelimitReached(int reason)
{
  // if we're already in OT then ignore
  if (OvertimeState != OT_WAITING) {
    return;
  }

  DPRINTF("ot game end called reason=%d\n", reason);

  if (reason == 1) {

    // determine if score is tied
    int bestScore = 0;
    int teamsWithBestScore = otGetScores(&bestScore, NULL);

    DPRINTF("ot best score %d, teams with best score %d\n", bestScore, teamsWithBestScore);

    if (teamsWithBestScore > 1) {
      otCtfBegin();
      return;
    }
  }

  // call base
  internal_gameEnd(reason);
}

/*
 * NAME :		otCtfTick
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
void otCtfTick(void)
{
	int i;
	Player ** players = playerGetAll();
	int gameTime = gameGetTime();

  // prevent pad input
  padResetInput(0);
  padResetInput(1);

  patchStateContainer.OverTimeState = OvertimeState;
	switch (OvertimeState)
	{
		case OT_INTERMISSION:
		{
      // lock players to positions
      for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
        Player* player = players[i];
        if (player && player->PlayerMoby && player->IsLocal) {
          vector_copy(player->PlayerPosition, CtfHtOtFreezePositions[i]);
          vector_copy(player->PlayerMoby->Position, CtfHtOtFreezePositions[i]);
        }
      }

			if (gameTime > (OvertimeEnd - (TIME_SECOND * 2)))
				OvertimeState = OT_SWITCH;
			break;
		}
		case OT_SWITCH:
		{
			// Show popup
			if (gameTime > (OvertimeEnd - (TIME_SECOND * 2)))
			{
				//uiShowPopup(0, "Survivor is on");
				//uiShowPopup(1, "Survivor is on");

        otCtfRespawnPlayers();
				OvertimeState = OT_INTERMISSION2;
			}
			break;
		}
		case OT_INTERMISSION2:
		{
			// Drop held items
			for (i = 0; i < GAME_MAX_PLAYERS; ++i)
			{
				if (!players[i])
					continue;

        playerDropFlag(players[i], 1);
        players[i]->Health = players[i]->MaxHealth;
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

			if (gameTime > OvertimeEnd)
			{
				otCtfEnd();
				OvertimeState = OT_COMPLETED;
			}
			break;
		}
	}
	
}

/*
 * NAME :		overtimeLogic
 * 
 * DESCRIPTION :
 * 			Checks if the CTF game has reached its end and the score is tied.
 * 
 * NOTES :
 * 
 * ARGS : 
 * 
 * RETURN :
 * 
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */
void overtimeLogic(void)
{
	int timeLimit = gameGetRawTimeLimit();
	int gameTime = gameGetTime();
	GameSettings * gameSettings = gameGetSettings();

	// Check we're in game and that it is compatible
	if (!gameSettings || gameSettings->GameRules != GAMERULE_CTF)
		return;

	// 
	switch (OvertimeState)
	{
		case OT_WAITING:
		{
      // no timelimit
			if (timeLimit <= 0)
				break;

      // hook when game ends due to timelimit
      HOOK_JAL(0x00620F54, &otOnTimelimitReached);
			break;
		}
		case OT_COMPLETED:
		{
      // end game when we have a clear winner
      int winningTeam = otGetWinningTeam();
      if (winningTeam >= 0) {
        gameSetWinner(winningTeam, 1);
        internal_gameEnd(2);
        gameSetWinner(winningTeam, 1);
        DPRINTF("ot game over %d\n", winningTeam);
        OvertimeState = OT_GAMEOVER;
      } else if (winningTeam == -2) {
        // draw
        internal_gameEnd(1);
        gameSetWinner(-1, 1);
        DPRINTF("ot game over draw\n");
        OvertimeState = OT_GAMEOVER;
      }

			break;
		}
    case OT_GAMEOVER:
    {
      break;
    }
		default:
		{
			otCtfTick();
			break;
		}
	}
}
