/***************************************************
 * FILENAME :		main.c
 * 
 * DESCRIPTION :
 * 		Handles all gun game logic.
 * 
 * NOTES :
 * 		Each offset is determined per app id.
 * 		This is to ensure compatibility between versions of Deadlocked/Gladiator.
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
#include <libdl/utils.h>
#include <libdl/ui.h>
#include <libdl/stdio.h>
#include "module.h"

// TODO
// Create scoreboard abstraction in libdl

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


/*
 * 
 */
char GunGameWeaponIds[] =
{
	WEAPON_SLOT_VIPERS,
	WEAPON_SLOT_B6,
	WEAPON_SLOT_MAGMA_CANNON,
	WEAPON_SLOT_ARBITER,
	WEAPON_SLOT_FLAIL,
	WEAPON_SLOT_FUSION_RIFLE,
	WEAPON_SLOT_MINE_LAUNCHER,
	WEAPON_SLOT_OMNI_SHIELD,
	WEAPON_SLOT_WRENCH,
	-1
};

/*
 * 
 */
const int GUN_INDEX_END = 9;

/*
 * 
 */
struct GunGameState
{
	int GunIndex;
	int Promotions;
	int Demotions;
	int TimesDemotionedAnother;
	int LastGunKills;
	int LastWrenchDeaths;
	int LastSuicides;
#if DEBUG
	int PadReset;
#endif
} PlayerGunGameStates[GAME_MAX_PLAYERS];

/*
 * 
 */
struct WeaponModState
{
	int Omega;
	int Alpha[10];
} WeaponModStates[16];

/*
 * 
 */
struct GunGameGameData
{
	u32 Version;
	int Gun[GAME_MAX_PLAYERS];
	int Demotions[GAME_MAX_PLAYERS];
	int Promotions[GAME_MAX_PLAYERS];
	int TimesDemotionedAnother[GAME_MAX_PLAYERS];
};

/*
 *
 */
ScoreboardItem PlayerScores[GAME_MAX_PLAYERS];

/*
 *
 */
ScoreboardItem * SortedPlayerScores[GAME_MAX_PLAYERS];

/*
 *
 */
int WinningTeam = -1;

/*
 * 
 */
int GameOver = 0;

/*
 *
 */
int ScoreboardChanged = 0;

/*
 *
 */
int Initialized = 0;

/*
 *
 */
char shaBuffer;


/*
 * NAME :		sortScoreboard
 * 
 * DESCRIPTION :
 * 			Sorts the scoreboard by value.
 * 
 * NOTES :
 * 
 * ARGS : 
 * 		player			:		Target player's player object.
 * 		playerState 	:		Target player's gun game state.
 * 		playerWepStats 	:		Target player's weapon stats.
 * 
 * RETURN :
 * 
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */
void sortScoreboard(int dontLockLocal)
{
	int i = 0;
	int j = 0;

	// bubble sort
	for (j = GAME_MAX_PLAYERS - 1; j > 0; --j)
	{
		for (i = 0; i < j; ++i)
		{
			// Swap
			if (SortedPlayerScores[i]->TeamId < 0 ||
				((dontLockLocal || SortedPlayerScores[i]->UNK != 1) &&
				 (SortedPlayerScores[i]->Value < SortedPlayerScores[i+1]->Value || 
				 (SortedPlayerScores[i+1]->UNK == 1 && !dontLockLocal))))
			//	(SortedPlayerScores[i]->Value == SortedPlayerScores[i+1]->Value &&
			//	 SortedPlayerScores[i]->TeamId > SortedPlayerScores[i+1]->TeamId))
			{
				ScoreboardItem * temp = SortedPlayerScores[i];
				SortedPlayerScores[i] = SortedPlayerScores[i+1];
				SortedPlayerScores[i+1] = temp;
			}
		}
	}
}

/*
 * NAME :		setWeapon
 * 
 * DESCRIPTION :
 * 			Gives a player the given weapon and sets the appropriate alpha mods.
 * 
 * NOTES :
 * 
 * ARGS : 
 * 		player			:		Target player's player object.
 * 		weaponId 		:		Target weapon id to set.
 * 
 * RETURN :
 * 
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */
void setWeapon(Player * player, int weaponId)
{
	GadgetBox* gBox = player->GadgetBox;
	// Give
	if (gBox->Gadgets[weaponId].Level < 0)
	{
		playerGiveWeapon(player, weaponId, 0);
		gBox->Gadgets[weaponId].Level = 0;
	}

	// Set alpha mods
	memcpy(&gBox->Gadgets[weaponId].AlphaMods, &WeaponModStates[weaponId].Alpha, 10 * sizeof(int));
}

/*
 * NAME :		demotePlayer
 * 
 * DESCRIPTION :
 * 			Demotes a given player.
 * 
 * NOTES :
 * 
 * ARGS : 
 * 		player			:		Target player's player object.
 * 		playerState 	:		Target player's gun game state.
 * 		playerWepStats 	:		Target player's weapon stats.
 * 
 * RETURN :
 * 
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */
void demotePlayer(Player * player, struct GunGameState * playerState, GameData * gameData)
{
	if (!player || !playerState || !gameData)
		return;

	int playerId = player->PlayerId;
	playerState->LastWrenchDeaths = gameData->PlayerStats.WeaponDeaths[playerId][WEAPON_SLOT_WRENCH];
	playerState->LastSuicides = gameData->PlayerStats.Suicides[playerId];

	if (playerState->GunIndex <= 0)
	{
		playerState->GunIndex = 0;
		return;
	}

	// Decrement gun index
	playerState->Demotions++;
	playerState->GunIndex -= 1;
	playerState->LastGunKills = gameData->PlayerStats.WeaponKills[playerId][(int)GunGameWeaponIds[playerState->GunIndex]];

	// Show popup
	if (playerIsLocal(player))
		uiShowPopup(player->LocalPlayerIndex, "You've been demoted!");
}

/*
 * NAME :		promotePlayer
 * 
 * DESCRIPTION :
 * 			Promotes a given player.
 * 
 * NOTES :
 * 
 * ARGS : 
 * 		player			:		Target player's player object.
 * 		playerState 	:		Target player's gun game state.
 * 		playerWepStats 	:		Target player's weapon stats.
 * 
 * RETURN :
 * 
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */
void promotePlayer(Player * player, struct GunGameState * playerState, GameData * gameData)
{
	if (!player || !playerState || !gameData)
		return;

	int playerId = player->PlayerId;

	// Player has already promoted as far as possible
	if (playerState->GunIndex >= GUN_INDEX_END)
	{
		playerState->GunIndex = GUN_INDEX_END;
		return;
	}

	// Increment gun index
	playerState->GunIndex += 1;
	playerState->LastGunKills = gameData->PlayerStats.WeaponKills[playerId][(int)GunGameWeaponIds[playerState->GunIndex]];
	playerState->LastWrenchDeaths = gameData->PlayerStats.WeaponDeaths[playerId][WEAPON_SLOT_WRENCH];
	playerState->LastSuicides = gameData->PlayerStats.Suicides[playerId];
	playerState->Promotions++;

	// Show popup
	if (playerIsLocal(player))
	{
		uiShowPopup(player->LocalPlayerIndex, "You've promoted to the next weapon!");

		// Generate our rng seed from SHA1 of spawn seed
		sha1(&shaBuffer, 4, (void*)&shaBuffer, 4);

		switch (shaBuffer & 3)
		{
			case 0:
			{
				dialogPlaySound(DIALOG_ID_TRY_THIS_ONE_ON_FOR_SIZE, 0);
				break;
			}
			case 1:
			{
				dialogPlaySound(DIALOG_ID_LETS_SEE_HOW_HE_DOES_WITH_THIS, 0);
				break;
			}
		}
	}
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

	int playerId = player->PlayerId;
	struct GunGameState * playerState = &PlayerGunGameStates[player->PlayerId];
	char activeGunSlotId = GunGameWeaponIds[playerState->GunIndex];
	char activeGunId = weaponSlotToId(activeGunSlotId);
	GameData * gameData = gameGetData();

	// If player has reached end then game over
	if (playerState->GunIndex >= GUN_INDEX_END)
	{
		WinningTeam = player->PlayerId;
		GameOver = 1;
		return;
	}

	// Clamp bounds
	if (playerState->GunIndex < 0)
		playerState->GunIndex = 0;

	// Enable all weapons
	setWeapon(player, WEAPON_ID_VIPERS);
	setWeapon(player, WEAPON_ID_MAGMA_CANNON);
	setWeapon(player, WEAPON_ID_ARBITER);
	setWeapon(player, WEAPON_ID_FUSION_RIFLE);
	setWeapon(player, WEAPON_ID_MINE_LAUNCHER);
	setWeapon(player, WEAPON_ID_B6);
	setWeapon(player, WEAPON_ID_OMNI_SHIELD);
	setWeapon(player, WEAPON_ID_FLAIL);

	// Only allow swingshot, wrench and active gun
	if (player->WeaponHeldId != WEAPON_ID_WRENCH &&
		player->WeaponHeldId != WEAPON_ID_SWINGSHOT &&
		player->WeaponHeldId != activeGunId)
	{
		if (playerIsLocal(player))
		{
			playerSetLocalEquipslot(player->LocalPlayerIndex, 1, WEAPON_ID_EMPTY);
			playerSetLocalEquipslot(player->LocalPlayerIndex, 2, WEAPON_ID_EMPTY);
		}

		playerEquipWeapon(player, activeGunId);
	}
	
	// Check for demotion
	if (gameData->PlayerStats.WeaponDeaths[playerId][WEAPON_SLOT_WRENCH] > playerState->LastWrenchDeaths ||
		gameData->PlayerStats.Suicides[playerId] > playerState->LastSuicides)
		demotePlayer(player, playerState, gameData);

	// Check for promotion
	else if (gameData->PlayerStats.WeaponKills[playerId][(int)activeGunSlotId] > playerState->LastGunKills)
		promotePlayer(player, playerState, gameData);

#if DEBUG
	if (playerState->PadReset == 1 && (player->Paddata->btns & PAD_L3))
	{
		playerState->PadReset = 0;
	}
	else if (playerState->PadReset == 2 && (player->Paddata->btns & PAD_R3))
	{
		playerState->PadReset = 0;
	}
	else if (playerState->PadReset == 0 && !(player->Paddata->btns & PAD_L3) && playerState->GunIndex > 0)
	{
		playerState->PadReset = 1;
		demotePlayer(player, playerState, gameData);
	}
	else if (playerState->PadReset == 0 && !(player->Paddata->btns & PAD_R3) && playerState->GunIndex < (GUN_INDEX_END - 1))
	{
		playerState->PadReset = 2;
		promotePlayer(player, playerState, gameData);
	}
#endif

	// Update scoreboard
	if (PlayerScores[playerId].Value != playerState->GunIndex)
	{
		PlayerScores[playerId].Value = playerState->GunIndex;
		ScoreboardChanged = 1;
	}
}

/*
 * NAME :		onPlayerKill
 * 
 * DESCRIPTION :
 * 			Triggers whenever a player is killed.
 * 			Handles detection of when a player demotes another.
 * 
 * ARGS : 
 * 
 * RETURN :
 * 
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */
void onPlayerKill(char * fragMsg)
{
	// call base function
	((void (*)(char*))0x00621CF8)(fragMsg);

	char weaponId = fragMsg[3];
	char killedPlayerId = fragMsg[2];
	char sourcePlayerId = fragMsg[0];

	if (sourcePlayerId >= 0 && killedPlayerId >= 0 && weaponId == WEAPON_ID_WRENCH) {
		if (PlayerGunGameStates[killedPlayerId].GunIndex > 0) {
			PlayerGunGameStates[sourcePlayerId].TimesDemotionedAnother++;
			DPRINTF("%d demoted %d\n", sourcePlayerId, killedPlayerId);
		}
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
    gameState->CustomGameStatsSize = sizeof(struct GunGameGameData);
		struct GunGameGameData* sGameData = (struct GunGameGameData*)gameState->CustomGameStats.Payload;
		sGameData->Version = 0x00000001;

		for (i = 0; i < GAME_MAX_PLAYERS; ++i)
		{
			sGameData->Gun[i] = PlayerGunGameStates[i].GunIndex;
			sGameData->Promotions[i] = PlayerGunGameStates[i].Promotions;
			sGameData->Demotions[i] = PlayerGunGameStates[i].Demotions;
			sGameData->TimesDemotionedAnother[i] = PlayerGunGameStates[i].TimesDemotionedAnother;
			DPRINTF("%d: gun:%d promo:%d demo:%d demoAnother:%d\n", i, sGameData->Gun[i], sGameData->Promotions[i], sGameData->Demotions[i], sGameData->TimesDemotionedAnother[i]);
		}
	}
}

/*
 * NAME :		initialize
 * 
 * DESCRIPTION :
 * 			Initializes the game mode.
 * 			Resets states, generates random weapon ordering, generates random alpha mods.
 * 
 * NOTES :
 * 			This is called once at start.
 * 
 * ARGS : 
 * 
 * RETURN :
 * 
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */
void initialize(void)
{
	int i = 0;
	int j = 0;
	u8 rngSeed[12];
	u8 rngBuf[12];
	GameSettings * gameSettings = gameGetSettings();
	GameOptions * gameOptions = gameGetOptions();
	Player ** players = playerGetAll();

	// Reset states to 0
	for (i = 0; i < GAME_MAX_PLAYERS; ++i)
	{
		PlayerGunGameStates[i].GunIndex = 0;
		PlayerGunGameStates[i].LastGunKills = 0;
		PlayerGunGameStates[i].LastWrenchDeaths = 0;
		PlayerGunGameStates[i].LastSuicides = 0;
		PlayerGunGameStates[i].Promotions = 0;
		PlayerGunGameStates[i].Demotions = 0;
		PlayerGunGameStates[i].TimesDemotionedAnother = 0;
		
#if DEBUG
		PlayerGunGameStates[i].PadReset = 0;
#endif
	}

	// Randomize middle tier weapons
	u32 seed = gameSettings->SpawnSeed;
	j = 1;
	while (seed)
	{
		// If lowest bit is set then swap with next id
		if (seed & 1)
		{
			char temp = GunGameWeaponIds[j];
			GunGameWeaponIds[j] = GunGameWeaponIds[j+1];
			GunGameWeaponIds[j+1] = temp;
		}

		// Increment weapon id index
		++j;
		
		// Ensure id is within the randomize bounds
		if (j > 5)
			j = 1;

		// Shift seed down
		seed >>= 1;
	}

	// Generate our rng seed from SHA1 of spawn seed
	sha1(&gameSettings->SpawnSeed, 4, (void*)rngSeed, 12);

	// Initialize weapon mods
	for (i = 0; i < 16; ++i)
	{
		// SHA1 of spawn seed
		// This is our way of generating random numbers
		sha1((const void*)rngSeed, 12, (void*)rngBuf, 12);
		memcpy(rngSeed, rngBuf, 12);

		WeaponModStates[i].Omega = 0;
		for (j = 0; j < 10; ++j)
			WeaponModStates[i].Alpha[j] = rngBuf[j] % 9;
	}

	// Initialize scoreboard
	for (i = 0; i < GAME_MAX_PLAYERS; ++i)
	{
		Player * p = players[i];
		PlayerScores[i].TeamId = p ? i : -1;
		PlayerScores[i].UNK = playerIsLocal(p);
		PlayerScores[i].Value = 0;
		GAME_SCOREBOARD_ARRAY[i] = p ? &PlayerScores[i] : 0;
		
		SortedPlayerScores[i] = &PlayerScores[i];
	}

	// No packs
	cheatsApplyNoPacks();

	// hook into player kill event
	*(u32*)0x00621c7c = 0x0C000000 | ((u32)&onPlayerKill >> 2);

	// Set respawn time to 2
	gameOptions->GameFlags.MultiplayerGameFlags.RespawnTime = 2;

	// Set kill target to 0 (disable)
	gameOptions->GameFlags.MultiplayerGameFlags.KillsToWin = 0;

	// Set holoshields to instant kill
	weaponSetDamage(WEAPON_ID_OMNI_SHIELD, 0, 50);
	
	// Reset winning team to default
	WinningTeam = 0;

	// Reset gameover
	GameOver = 0;

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
	GameSettings * gameSettings = gameGetSettings();
	Player ** players = playerGetAll();

	// Ensure in game
	if (!gameSettings)
		return;

	if (!Initialized)
		initialize();

	// 
	updateGameState(gameState);

	if (!gameHasEnded() && !GameOver)
	{
		// Disable weapon hud
		hudGetPlayerFlags(0)->Flags.Weapons = 0;
		hudGetPlayerFlags(1)->Flags.Weapons = 0;

		// Iterate through players
		for (i = 0; i < GAME_MAX_PLAYERS; ++i)
		{
			if (GameOver)
				break;

			// Force scoreboard to custom scoreboard values
			if ((players[i] || GAME_SCOREBOARD_ARRAY[i]) && GAME_SCOREBOARD_ARRAY[i] != SortedPlayerScores[i])
			{
				GAME_SCOREBOARD_ARRAY[i] = SortedPlayerScores[i];
				GAME_SCOREBOARD_REFRESH_FLAG = 1;
			}

			if (!players[i])
				continue;
			
			// 
			PlayerScores[i].TeamId = players[i]->PlayerId;

			// Process
			processPlayer(players[i]);
		}
	}
	else
	{
		// Correct scoreboard
		for (i = 0; i < GAME_SCOREBOARD_ITEM_COUNT; ++i)
		{
			// Force scoreboard to custom scoreboard values
			if (GAME_SCOREBOARD_ARRAY[i] != SortedPlayerScores[i])
			{
				GAME_SCOREBOARD_ARRAY[i] = SortedPlayerScores[i];
				GAME_SCOREBOARD_REFRESH_FLAG = 1;
			}
		}
	}

	// Update scoreboard on change
	if (ScoreboardChanged)
	{
		sortScoreboard(0);
		GAME_SCOREBOARD_REFRESH_FLAG = 1;
		ScoreboardChanged = 0;
	}

	// Target to last gun index
	GAME_SCOREBOARD_TARGET = GUN_INDEX_END;

	// Set winner
	gameSetWinner(WinningTeam, 0);

	// End game
	if (GameOver && !gameHasEnded())
	{
		gameEnd(2);
		ScoreboardChanged = 1;
	}

	return;
}

void setLobbyGameOptions(void)
{
	// deathmatch options
	static char options[] = { 
		0, 0, 				// 0x06 - 0x08
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
}

void setEndGameScoreboard(void)
{
	u32 * uiElements = (u32*)(*(u32*)(0x011C7064 + 4*18) + 0xB0);
	int i;
	char buf[24];

	// reset buf
	memset(buf, 0, sizeof(buf));

	// sort scoreboard again
	sortScoreboard(1);

	// names start at 6
	// column headers start at 17
	strncpy((char*)(uiElements[18] + 0x60), "GUN", 4);
	strncpy((char*)(uiElements[19] + 0x60), "KILLS", 6);
	strncpy((char*)(uiElements[20] + 0x60), "DEATHS", 7);

	// rows
	for (i = 0; i < GAME_MAX_PLAYERS; ++i)
	{
		int pid = SortedPlayerScores[i]->TeamId;
		if (pid >= 0)
		{
			// set gun
			sprintf(buf, "%d", SortedPlayerScores[i]->Value);
			strncpy((char*)(uiElements[22 + (i*4) + 0] + 0x60), buf, strlen(buf) + 1);

			// moves deaths over
			strncpy((char*)(uiElements[22 + (i*4) + 2] + 0x60), (char*)(uiElements[22 + (i*4) + 1] + 0x60), 8);
			
			// moves kills over
			strncpy((char*)(uiElements[22 + (i*4) + 1] + 0x60), (char*)(uiElements[22 + (i*4) + 0] + 0x60), 8);
		}
	}
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
	static int initializedScoreboard = 0;

	// 
	updateGameState(gameState);

	// scoreboard
	switch (activeId)
	{
		case 0x15C:
		{
			if (initializedScoreboard)
				break;

			setEndGameScoreboard();
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
