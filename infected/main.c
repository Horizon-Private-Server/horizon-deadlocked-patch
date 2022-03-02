/***************************************************
 * FILENAME :		main.c
 * 
 * DESCRIPTION :
 * 		Infected entrypoint and logic.
 * 
 * NOTES :
 * 		Each offset is determined per app id.
 * 		This is to ensure compatibility between versions of Deadlocked/Gladiator.
 * 		
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */

#include <tamtypes.h>

#include <libdl/stdio.h>
#include <libdl/time.h>
#include "module.h"
#include <libdl/game.h>
#include <libdl/gamesettings.h>
#include <libdl/player.h>
#include <libdl/cheats.h>
#include <libdl/ui.h>
#include <libdl/string.h>

/*
 * Infected team.
 */
#define INFECTED_TEAM			(TEAM_GREEN)

/*
 * 
 */
struct InfectedGameData
{
	u32 Version;
	int Infections[GAME_MAX_PLAYERS];
	char IsInfected[GAME_MAX_PLAYERS];
	char IsFirstInfected[GAME_MAX_PLAYERS];
};

/*
 *
 */
int InfectedMask = 0;

/*
 *
 */
int WinningTeam = 0;

/*
 *
 */
int Initialized = 0;

/*
 *
 */
char FirstInfected[GAME_MAX_PLAYERS];

/*
 *
 */
int Infections[GAME_MAX_PLAYERS];

/*
 * 
 */
char InfectedPopupBuffer[64];

/*
 *
 */
const char * InfectedPopupFormat = "%s has been infected!";

/*
 * NAME :		isInfected
 * 
 * DESCRIPTION :
 * 			Returns true if the given player is infected.
 * 
 * NOTES :
 * 
 * ARGS : 
 * 		playerId:	Player index
 * 
 * RETURN :
 * 
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */
inline int isInfected(int playerId)
{
	return InfectedMask & (1 << playerId);
}

/*
 * NAME :		infect
 * 
 * DESCRIPTION :
 * 			Infects the given player.
 * 
 * NOTES :
 * 
 * ARGS : 
 * 		playerId:	Player index
 * 
 * RETURN :
 * 
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */
void infect(int playerId)
{
	InfectedMask |= (1 << playerId);

	GameSettings * gameSettings = gameGetSettings();
	if (!gameSettings)
		return;

	InfectedPopupBuffer[0] = 0;
	sprintf(InfectedPopupBuffer, InfectedPopupFormat, gameSettings->PlayerNames[playerId]);
	InfectedPopupBuffer[63] = 0;

	uiShowPopup(0, InfectedPopupBuffer);
	uiShowPopup(1, InfectedPopupBuffer);
}

/*
 * NAME :		processPlayer
 * 
 * DESCRIPTION :
 * 			Process player.
 * 
 * NOTES :
 * 
 * ARGS : 
 * 		player:		Player to process.
 * 
 * RETURN :
 * 
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */
void processPlayer(Player * player)
{
	if (!player)
		return;

	int teamId = player->Team;
	GameData * gameData = gameGetData();

	// 
	if (isInfected(player->PlayerId))
	{
		// If not on the right team then set it
		if (teamId != INFECTED_TEAM)
			playerSetTeam(player, INFECTED_TEAM);

		player->Speed = 4.0;
		player->DamageMultiplier = 1.001;
		player->timers.damageMuliplierTimer = 0x1000;
		
		// Force wrench
		if (player->WeaponHeldId != WEAPON_ID_WRENCH &&
			player->WeaponHeldId != WEAPON_ID_SWINGSHOT)
			playerSetWeapon(player, WEAPON_ID_WRENCH);
	}
	// If the player is already on the infected team
	// or if they've died
	// then infect them
	else if (teamId == INFECTED_TEAM || gameData->PlayerStats.Deaths[player->PlayerId] > 0)
	{
		infect(player->PlayerId);
	}
	// Process survivor logic
	else
	{

	}
}

/*
 * NAME :		getRandomSurvivor
 * 
 * DESCRIPTION :
 * 			Returns a random survivor.
 * 
 * NOTES :
 * 
 * ARGS : 
 * 		seed :		Used to determine the random survivor.
 * 
 * RETURN :
 * 
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */
Player * getRandomSurvivor(u32 seed)
{
	Player ** playerObjects = playerGetAll();

	int value = (seed % GAME_MAX_PLAYERS) + 1;
	int i = 0;
	int counter = 0;

	while (counter < value)
	{
		for (i = 0; i < GAME_MAX_PLAYERS; ++i)
		{
			if (playerObjects[i] && !isInfected(playerObjects[i]->PlayerId))
			{
				++counter;

				if (value == counter)
					return playerObjects[i];
			}
		}

		// This means that there are no survivors left
		if (counter == 0)
			return NULL;
	}

	return NULL;
}

/*
 * NAME :		onPlayerKill
 * 
 * DESCRIPTION :
 * 			Triggers whenever a player is killed.
 * 			Handles detection of when a player infects another.
 * 
 * ARGS : 
 * 
 * RETURN :
 * 
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */
void onPlayerKill(char * fragMsg)
{
	VECTOR delta;

	// call base function
	((void (*)(char*))0x00621CF8)(fragMsg);

	char weaponId = fragMsg[3];
	char killedPlayerId = fragMsg[2];
	char sourcePlayerId = fragMsg[0];

	if (sourcePlayerId >= 0 && isInfected(sourcePlayerId) && killedPlayerId >= 0 && !isInfected(killedPlayerId)) {
		Infections[sourcePlayerId]++;
	}
}

/*
 * NAME :		updateGameState
 * 
 * DESCRIPTION :
 * 			Updates the gamemode state for the server stats.
 * 
 * ARGS : 
 * 
 * RETURN :
 * 
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */
void updateGameState(PatchStateContainer_t * gameState)
{
	int i,j;

	// stats
	if (gameState->UpdateCustomGameStats)
	{
		struct InfectedGameData* sGameData = (struct InfectedGameData*)gameState->CustomGameStats.Payload;
		sGameData->Version = 0x00000001;

		for (i = 0; i < GAME_MAX_PLAYERS; ++i)
		{
			sGameData->Infections[i] = Infections[i];
			sGameData->IsInfected[i] = isInfected(i);
			sGameData->IsFirstInfected[i] = FirstInfected[i];
			DPRINTF("%d: infections:%d infected:%d firstInfected:%d\n", i, Infections[i], isInfected(i) != 0, FirstInfected[i]);
		}
	}
}

/*
 * NAME :		initialize
 * 
 * DESCRIPTION :
 * 			Initializes the gamemode.
 * 
 * NOTES :
 * 			This is called only once at the start.
 * 
 * ARGS : 
 * 
 * RETURN :
 * 
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */
void initialize(void)
{
	// No packs
	cheatsApplyNoPacks();

	// 
	memset(FirstInfected, 0, sizeof(FirstInfected));
	memset(Infections, 0, sizeof(Infections));

	// hook into player kill event
	*(u32*)0x00621c7c = 0x0C000000 | ((u32)&onPlayerKill >> 2);

	// Disable health boxes
	if (cheatsDisableHealthboxes())
		Initialized = 1;
}


/*
 * NAME :		gameStart
 * 
 * DESCRIPTION :
 * 			Infected game logic entrypoint.
 * 
 * NOTES :
 * 			This is called only when in game.
 * 
 * ARGS : 
 * 
 * RETURN :
 * 
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */
void gameStart(struct GameModule * module, PatchConfig_t * config, PatchGameConfig_t * gameConfig, PatchStateContainer_t * gameState)
{
	int i = 0;
	int infectedCount = 0;
	int playerCount = 0;
	GameSettings * gameSettings = gameGetSettings();
	GameOptions * gameOptions = gameGetOptions();
	Player ** players = playerGetAll();

	// Ensure in game
	if (!gameSettings || !gameIsIn())
		return;

	if (!Initialized)
		initialize();

	// 
	updateGameState(gameState);

	if (!gameHasEnded())
	{
		// Reset to infected team
		// If one player isn't infected then their team
		// is set to winning team
		WinningTeam = INFECTED_TEAM;

		// Iterate through players
		for (i = 0; i < GAME_MAX_PLAYERS; ++i)
		{
			if (!players[i])
				continue;
			
			// Process
			processPlayer(players[i]);

			// Count
			++playerCount;
			if (isInfected(players[i]->PlayerId))
			{
				++infectedCount;
			}
			else
			{
				WinningTeam = players[i]->Team;
			}
		}
	}

	// 
	gameSetWinner(WinningTeam, 1);

	if (!gameHasEnded())
	{
		// If no survivors then end game
		if (playerCount == infectedCount && gameOptions->GameFlags.MultiplayerGameFlags.Timelimit > 0)
		{
			// End game
			gameEnd(2);
		}
		else if (infectedCount == 0)
		{
			// Infect first player after 10 seconds
			if ((gameGetTime() - gameSettings->GameStartTime) > (10 * TIME_SECOND))
			{
				Player * survivor = getRandomSurvivor(gameSettings->GameStartTime);
				if (survivor)
				{
					infect(survivor->PlayerId);
					FirstInfected[survivor->PlayerId] = 1;
				}
			}
		}
	}

	return;
}

void setLobbyGameOptions(void)
{
	// deathmatch options
	static char options[] = { 
		0, 0, 			  // 0x06 - 0x08
		0, 0, 0, 0, 	// 0x08 - 0x0C
		1, 1, 1, 0,  	// 0x0C - 0x10
		0, 1, 0, 0,		// 0x10 - 0x14
		-1, -1, 0, 1,	// 0x14 - 0x18
	};

	// set game options
	GameOptions * gameOptions = gameGetOptions();
	if (!gameOptions)
		return;
		
	// apply options
	memcpy((void*)&gameOptions->GameFlags.Raw[6], (void*)options, sizeof(options)/sizeof(char));
	gameOptions->GameFlags.MultiplayerGameFlags.Juggernaut = 0;
	gameOptions->GameFlags.MultiplayerGameFlags.Lockdown = 0;
	gameOptions->GameFlags.MultiplayerGameFlags.NodeType = 0;
}

/*
 * NAME :		lobbyStart
 * 
 * DESCRIPTION :
 * 			Infected lobby logic entrypoint.
 * 
 * NOTES :
 * 			This is called only when in lobby.
 * 
 * ARGS : 
 * 
 * RETURN :
 * 
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */
void lobbyStart(struct GameModule * module, PatchConfig_t * config, PatchGameConfig_t * gameConfig, PatchStateContainer_t * gameState)
{
	int activeId = uiGetActive();

	// 
	updateGameState(gameState);

	// scoreboard
	switch (activeId)
	{
		case UI_ID_GAME_LOBBY:
		{
			setLobbyGameOptions();
			break;
		}
	}
}

/*
 * NAME :		loadStart
 * 
 * DESCRIPTION :
 * 			Load logic entrypoint.
 * 
 * NOTES :
 * 			This is called only when the game has finished reading the level from the disc and before it has started processing the data.
 * 
 * ARGS : 
 * 
 * RETURN :
 * 
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */
void loadStart(void)
{
	
}
