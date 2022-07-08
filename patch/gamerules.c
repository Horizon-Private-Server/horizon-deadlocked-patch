/***************************************************
 * FILENAME :		gamerules.c
 * 
 * DESCRIPTION :
 * 		Gamerules entrypoint and logic.
 * 
 * NOTES :
 * 		
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */

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
#include <libdl/sha1.h>
#include <libdl/spawnpoint.h>
#include <libdl/graphics.h>
#include "module.h"
#include "halftime.h"
#include "include/config.h"

// config
extern PatchConfig_t config;

// game config
extern PatchGameConfig_t gameConfig;

/*
 *
 */
int GameRulesInitialized = 0;

/*
 *
 */
int BetterHillsInitialized = 0;

/*
 *
 */
int HasDisabledHealthboxes = 0;

/*
 *
 */
u32 WeatherOverrideId = 0;

/*
 * 
 */
short PlayerKills[GAME_MAX_PLAYERS];

/*
 *
 */
float VampireHealRate[] = {
	PLAYER_MAX_HEALTH * 0.25,
	PLAYER_MAX_HEALTH * 0.50,
	PLAYER_MAX_HEALTH * 1.00
};

/*
 * Custom hill spawn points
 */
SpawnPoint BetterHillPoints[] = {
	// TORVAL ID 0x13
	{ { 6, 0, 0, 0, 0, 10, 0, 0, 0, 0, 10, 0, 233.28482, 278.00937, 106.855156, 0 }, { 0.16666667, 0, 0, 0, 0, 0.1, 0, 0, 0, 0, 0.1, 0, 233.28482, 278.00937, 106.855156, 0 } },

	// MARAXUS ID 0x13
	{ { 10, 0, 0, 0, 0, 10, 0, 0, 0, 0, 10, 0, 525.83344, 742.3953, 103.92746, 1 }, { 0.1, 0, 0, 0, 0, 0.1, 0, 0, 0, 0, 0.1, 0, 525.83344, 742.3953, 103.92746, 1 } },

	// SARATHOS ID 0x11
	{ { 12, 0, 0, 0, 0, 12, 0, 0, 0, 0, 12, 0, 493.81552, 218.55286, 105.77197, 1 }, { 0.083333336, 0, 0, 0, 0, 0.083333336, 0, 0, 0, 0, 0.083333336, 0, 493.81552, 218.55286, 105.77197, 1 } },

	// SARATHOS ID 14 (0x0E)
	{ { 10, 0, 0, 0, 0, 10, 0, 0, 0, 0, 10, 0, 337.56897, 193.39743, 106.15551, 1 }, { 0.1, 0, 0, 0, 0, 0.1, 0, 0, 0, 0, 0.1, 0, 337.56897, 193.39743, 106.15551, 1 } },

	// CATACROM ID 0x0D
	{ { 7, 0, 0, 0, 0, 7, 0, 0, 0, 0, 7, 0, 333.3385, 255.27766, 63.045467, 0 }, { 0.14285715, 0, 0, 0, 0, 0.14285715, 0, 0, 0, 0, 0.14285715, 0, 333.3385, 255.27766, 63.045467, 0 } },
	
	// CATACROM ID 0x11
	{ { 7, 0, 0, 0, 0, 7, 0, 0, 0, 0, 7, 0, 324.36548, 366.6925, 65.79945, 0 }, { 0.14285715, 0, 0, 0, 0, 0.14285715, 0, 0, 0, 0, 0.14285715, 0, 324.36548, 366.6925, 65.79945, 0 } },

	// DC ID 12 (0x0C)
	{ { 12, 0, 0, 0, 0, 12, 0, 0, 0, 0, 10, 0, 503.79434, 415.31488, 641.28125, 1 }, { 0.083333336, 0, 0, 0, 0, 0.083333336, 0, 0, 0, 0, 0.1, 0, 503.79434, 415.31488, 641.28125, 1 } },

	// DC ID 14
	{ { 12, 0, 0, 0, 0, 12, 0, 0, 0, 0, 12, 0, 503.789, 494.31857, 641.2969, 1 }, { 0.083333336, 0, 0, 0, 0, 0.083333336, 0, 0, 0, 0, 0.083333336, 0, 503.789, 494.31857, 641.2969, 1 } },

	// DC ID 16
	{ { 16, 0, 0, 0, 0, 16, 0, 0, 0, 0, 16, 0, 349.3588, 529.6109, 641.25, 1 }, { 0.0625, 0, 0, 0, 0, 0.0625, 0, 0, 0, 0, 0.0625, 0, 349.3588, 529.6109, 641.25, 1 } },

	// DC ID 17 (0x11)
	{ { 12, 0, 0, 0, 0, 12, 0, 0, 0, 0, 10, 0, 455.52982, 493.50287, 641.03125, 1 }, { 0.083333336, 0, 0, 0, 0, 0.083333336, 0, 0, 0, 0, 0.1, 0, 455.52982, 493.50287, 641.03125, 1 } },

	// SHAAR ID 0x20
	{ { 12, 0, 0, 0, 0, 12, 0, 0, 0, 0, 12, 0, 621.2297, 562.5671, 509.29688, 0 }, { 0.083333336, 0, 0, 0, 0, 0.083333336, 0, 0, 0, 0, 0.083333336, 0, 621.2297, 562.5671, 509.29688, 0 } },

	// SHAAR ID 0x23
	{ { 10, 0, 0, 0, 0, 10, 0, 0, 0, 0, 10, 0, 487.00687, 682.48615, 509.3125, 0 }, { 0.1, 0, 0, 0, 0, 0.1, 0, 0, 0, 0, 0.1, 0, 487.00687, 682.48615, 509.3125, 0 } },

	// VALIX ID 13 (0x0D)
	{ { 8.660254, 5, 0, 0, -5, 8.660254, 0, 0, 0, 0, 10, 0, 292.09567, 472.517, 331.46976, 0 }, { 0.08660254, 0.05, 0, 0, -0.05, 0.08660254, 0, 0, 0, 0, 0.1, 0, 292.09567, 472.517, 331.46976, 0 } },

	// VALIX ID 16 (0x10)
	{ { 8.660254, 5, 0, 0, -5, 8.660254, 0, 0, 0, 0, 10, 0, 351.16696, 400.823, 324.90625, 0 }, { 0.08660254, 0.05, 0, 0, -0.05, 0.08660254, 0, 0, 0, 0, 0.1, 0, 351.16696, 400.823, 324.90625, 0 } },

	// MF ID 20 (0x14)
	{ { 8, 0, 0, 0, 0, 14, 0, 0, 0, 0, 5, 0, 444.0099, 600.22675, 428.04242, 0 }, { 0.125, 0, 0, 0, 0, 0.071428575, 0, 0, 0, 0, 0.2, 0, 444.0099, 600.22675, 428.04242, 0 } },
	
	// MF ID 22 (0x16)
	{ { 10, 0, 0, 0, 0, 10, 0, 0, 0, 0, 10, 0, 410.27823, 598.204, 434.209, 0 }, { 0.1, 0, 0, 0, 0, 0.1, 0, 0, 0, 0, 0.1, 0, 410.27823, 598.204, 434.209, 0 } },

	// MF ID 22 (0x18)
	{ { 10, 0, 0, 0, 0, 10, 0, 0, 0, 0, 10, 0, 356.82983, 653.3447, 428.36734, 0 }, { 0.1, 0, 0, 0, 0, 0.1, 0, 0, 0, 0, 0.1, 0, 356.82983, 653.3447, 428.36734, 0 } },

	// GC ID 0x20
	{ { 7, 0, 0, 0, 0, 7, 0, 0, 0, 0, 7, 0, 636.95593, 401.78497, 102.765625, 0 }, { 0.14285715, 0, 0, 0, 0, 0.14285715, 0, 0, 0, 0, 0.14285715, 0, 636.95593, 401.78497, 102.765625, 0 } },

	// GC ID 0x21
	{ { 12, 0, 0, 0, 0, 12, 0, 0, 0, 0, 12, 0, 728.6185, 563.659, 101.578125, 1 }, { 0.083333336, 0, 0, 0, 0, 0.083333336, 0, 0, 0, 0, 0.083333336, 0, 728.6185, 563.659, 101.578125, 1 } },

	// GC ID 22
	{ { 6.7093644, 4.3571124, 0, 0, -4.9017515, 7.548035, 0, 0, 0, 0, 8, 0, 586.91223, 541.42474, 102.828125, 0 }, { 0.10483382, 0.06807988, 0, 0, -0.060515452, 0.09318562, 0, 0, 0, 0, 0.125, 0, 586.91223, 541.42474, 102.828125, 0 } },

	// GC ID 23 (0x17)
	{ { 10, 0, 0, 0, 0, 10, 0, 0, 0, 0, 10, 0, 637.3847, 522.3367, 103, 1 }, { 0.1, 0, 0, 0, 0, 0.1, 0, 0, 0, 0, 0.1, 0, 637.3847, 522.3367, 103, 1 } },

	// TEMPUS ID 16
	{ { 10, 0, 0, 0, 0, 10, 0, 0, 0, 0, 10, 0, 530.847, 538.41187, 100.375, 0 }, { 0.1, 0, 0, 0, 0, 0.1, 0, 0, 0, 0, 0.1, 0, 530.847, 538.41187, 100.375, 0 } },

	// TEMPUS ID 17
	{ { 4.9999995, 8.6602545, 0, 0, -6.9282036, 3.9999998, 0, 0, 0, 0, 10, 0, 491.03555, 425.8757, 100.828125, 0 }, { 0.049999997, 0.086602546, 0, 0, -0.10825318, 0.062499996, 0, 0, 0, 0, 0.1, 0, 491.03555, 425.8757, 100.828125, 0 } },

	// TEMPUS ID 18
	{ { 27.499998, 47.6314, 0, 0, -47.6314, 27.499998, 0, 0, 0, 0, 10, 0, 452, 443, 100.828125, 1 }, { 0.009090908, 0.015745917, 0, 0, -0.015745917, 0.009090908, 0, 0, 0, 0, 0.1, 0, 452, 443, 100.828125, 1 } },

	// VALIX CTF SPAWN POINT 01
	{ { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 333.81165, 413.57132, 330, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 333.81165, 413.57132, 330, 0 } },
};

enum BETTER_HILL_PTS
{
	TORVAL_13 = 0,
	MARAXUS_13 = 1,
	SARATHOS_11 = 2,
	SARATHOS_14 = 3,
	CATACROM_0D = 4,
	CATACROM_11 = 5,
	DC_11 = 6,
	DC_14 = 7,
	DC_16 = 8,
	DC_17 = 9,
	SHAAR_14 = 10,
	SHAAR_17 = 11,
	VALIX_01 = 24,
	VALIX_13 = 12,
	VALIX_16 = 13,
	MF_20 = 14,
	MF_22 = 15,
	MF_24 = 16,
	GS_20 = 17,
	GS_21 = 18,
	GS_22 = 19,
	GS_23 = 20,
	TEMPUS_16 = 21,
	TEMPUS_17 = 22,
	TEMPUS_18 = 23
};

/*
 * NAME :		playerSizeLogic
 * 
 * DESCRIPTION :
 * 			Sets player size depending on setting.
 * 
 * NOTES :
 * 
 * ARGS : 
 * 
 * RETURN :
 * 
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */
void playerSizeLogic(void)
{
	int i, j;
	Player** players = playerGetAll();
	float size = gameConfig.grPlayerSize == 1 ? 1.5 : 0.6666;
	
	// disable fixed player scale
	*(u32*)0x005D1580 = 0;

	// change velocity dampening function's radius
	// to original so new radius doesn't affect movement physics
	*(u16*)0x005E72C4 = 0x238;
	*(u16*)0x005E72C8 = 0x23C;

	for (i = 0; i < GAME_MAX_PLAYERS; ++i)
	{
		Player * player = players[i];
		if (player) {

			// write original radius and radius squared
			*(float*)((u32)player + 0x238) = 0.45;
			*(float*)((u32)player + 0x23C) = 0.45 * 0.45;

			// update collision size
			player->PlayerConstants->colRadius = 0.45 * size;
			player->PlayerConstants->colTop = 1.05 * size;
			player->PlayerConstants->colBot = 0.70 * size;
			player->PlayerConstants->colBotFall = 0.50 * size;

			// update camera
			player->CameraOffset[0] = -6 * size;
			player->CameraOffset[2] = gameConfig.grPlayerSize == 1 ? 1 : -1;

			// update player size
			for (j = 0; j < 1; ++j) {
				player->Tweakers[j].scale = size;
			}
		}
	}
}

/*
 * NAME :		vampireLogic
 * 
 * DESCRIPTION :
 * 			Checks if a player has gotten a kill and heals them if so.
 * 
 * NOTES :
 * 
 * ARGS : 
 * 
 * RETURN :
 * 
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */
void vampireLogic(float healRate)
{
	int i;
	Player ** playerObjects = playerGetAll();
	Player * player;
	GameData * gameData = gameGetData();

	for (i = 0; i < GAME_MAX_PLAYERS; ++i)
	{
		// Check if player has killed someone
		if (gameData->PlayerStats.Kills[i] > PlayerKills[i])
		{
			// Try to heal if player exists
			player = playerObjects[i];
			if (player)
				playerSetHealth(player, clamp(player->Health + healRate, 0, PLAYER_MAX_HEALTH));
			
			// Update our cached kills count
			PlayerKills[i] = gameData->PlayerStats.Kills[i];
		}
	}
}

/*
 * NAME :		betterHillsLogic
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
void betterHillsLogic(void)
{
	if (BetterHillsInitialized)
		return;

	//
	BetterHillsInitialized = 1;

	// 
	switch (gameGetSettings()->GameLevel)
	{
		case MAP_ID_CATACROM:
		{
			spawnPointSet(&BetterHillPoints[CATACROM_0D], 0x0D);
			spawnPointSet(&BetterHillPoints[CATACROM_11], 0x11);
			break;
		}
		case MAP_ID_SARATHOS:
		{
			spawnPointSet(&BetterHillPoints[SARATHOS_11], 0x11);
			spawnPointSet(&BetterHillPoints[SARATHOS_14], 0x0E);
			break;
		}
		case MAP_ID_DC:
		{
			spawnPointSet(&BetterHillPoints[DC_11], 0x0C);
			spawnPointSet(&BetterHillPoints[DC_16], 0x10);
			spawnPointSet(&BetterHillPoints[DC_14], 0x0E);
			spawnPointSet(&BetterHillPoints[DC_17], 0x11);
			break;
		}
		case MAP_ID_SHAAR: 
		{
			spawnPointSet(&BetterHillPoints[SHAAR_14], 0x14);
			spawnPointSet(&BetterHillPoints[SHAAR_17], 0x17);
			break;
		}
		case MAP_ID_VALIX: 
		{
			spawnPointSet(&BetterHillPoints[VALIX_01], 0x01);
			spawnPointSet(&BetterHillPoints[VALIX_13], 0x0D);
			spawnPointSet(&BetterHillPoints[VALIX_16], 0x10);
			break;
		}
		case MAP_ID_MF:
		{
			spawnPointSet(&BetterHillPoints[MF_20], 0x14);
			spawnPointSet(&BetterHillPoints[MF_22], 0x16);
			spawnPointSet(&BetterHillPoints[MF_24], 0x18);
			break;
		}
		case MAP_ID_TEMPUS:
		{
			spawnPointSet(&BetterHillPoints[TEMPUS_16], 0x10);
			spawnPointSet(&BetterHillPoints[TEMPUS_17], 0x11);
			spawnPointSet(&BetterHillPoints[TEMPUS_18], 0x12);
			break;
		}
		case MAP_ID_MARAXUS: 
		{
			spawnPointSet(&BetterHillPoints[MARAXUS_13], 0x13);
			break;
		}
		case MAP_ID_GS:
		{
			spawnPointSet(&BetterHillPoints[GS_20], 0x14);
			spawnPointSet(&BetterHillPoints[GS_21], 0x15);
			spawnPointSet(&BetterHillPoints[GS_22], 0x16);
			spawnPointSet(&BetterHillPoints[GS_23], 0x17);
			break;
		}
		case MAP_ID_TORVAL:
		{
			spawnPointSet(&BetterHillPoints[TORVAL_13], 0x13);
			break;
		}
	}
}

/*
 * NAME :		healthbarsLogic
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
void healthbarsHook(int nameX, int nameY, u32 nameColor, char * nameStr, int nameStrLen)
{
	// get player index
	int i;
	asm volatile (
		"move %0, $s7"
		: : "r" (i)
	);

	// call pass through function
	gfxScreenSpaceText(nameX, nameY, 1, 1, nameColor, nameStr, nameStrLen, 1);
	
	// ensure player index is valid
	if (i < 0 || i >= GAME_MAX_PLAYERS)
		return;

	// get player and validate state
	Player ** players = playerGetAll();
	Player * player = players[i];
	if (!player || player->Health <= 0)
		return;

	// Draw boxes
	float x = (float)nameX / SCREEN_WIDTH;
	float y = (float)nameY / SCREEN_HEIGHT;
	float health = player->Health / 50;
	float w = (0.05 * 1) + 0.02, h = 0.01, p = 0.002;
	float right = w * health;
	x -= w / 2;
	gfxScreenSpaceBox(x,y,w-p,h-p, 0x80000000);
	gfxScreenSpaceBox(x,y,right,h, nameColor);
}

/*
 * NAME :		invTimerLogic
 * 
 * DESCRIPTION :
 * 			Sets each player's invicibility timer to 0.
 * 
 * NOTES :
 * 
 * ARGS : 
 * 
 * RETURN :
 * 
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */
void invTimerLogic(void)
{
	int i;
	Player ** players = playerGetAll();

	for (i = 0; i < GAME_MAX_PLAYERS; ++i)
	{
		Player * p = players[i];
		if (p)
			p->timers.postHitInvinc = 0;
	}
}

/*
 * NAME :		grInitialize
 * 
 * DESCRIPTION :
 * 			Initializes this module.
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
void grInitialize(void)
{
	int i;

	// Initialize player kills to 0
	for (i = 0; i < GAME_MAX_PLAYERS; ++i)
		PlayerKills[i] = 0;

	// reset
	htReset();
	BetterHillsInitialized = 0;
	HasDisabledHealthboxes = 0;

	GameRulesInitialized = 1;
}

/*
 * NAME :		grGameStart
 * 
 * DESCRIPTION :
 * 			Gamerules game logic entrypoint.
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
void grGameStart(void)
{
	int i = 0;
	GameSettings * gameSettings = gameGetSettings();

	// Initialize
	if (GameRulesInitialized != 1)
	{
		// 
		grInitialize();

		// convert weather id to game value
		WeatherOverrideId = gameConfig.grWeatherId;

		// random weather
		if (WeatherOverrideId == 1)
		{
			sha1(&gameSettings->GameLoadStartTime, 4, &WeatherOverrideId, 4);
			WeatherOverrideId = 1 + (WeatherOverrideId % 16);
			if (WeatherOverrideId == 8)
				WeatherOverrideId = 9;
			else if (WeatherOverrideId == 15)
				WeatherOverrideId = 16;
		}
		// shift down weather (so that the list value is equivalent to game value)
		else if (WeatherOverrideId > 1 && WeatherOverrideId < 9)
			--WeatherOverrideId;
	}

	// Apply weather
	cheatsApplyWeather(WeatherOverrideId);

#if DEBUG
	dlPreUpdate();

	halftimeLogic();
	if (padGetButtonDown(0, PAD_L3 | PAD_R3) > 0)
	{
		htCtfBegin();
		HalfTimeState = 1;
	}

	dlPostUpdate();
#endif

	if (gameConfig.grNoPacks)
		cheatsApplyNoPacks();

	if (gameConfig.grNoV2s)
		cheatsApplyNoV2s();

	if (gameConfig.grMirrorWorld)
		cheatsApplyMirrorWorld(1);

	if (gameConfig.grNoHealthBoxes && !HasDisabledHealthboxes)
		HasDisabledHealthboxes = cheatsDisableHealthboxes();

	if (gameConfig.grVampire)
		vampireLogic(VampireHealRate[gameConfig.grVampire-1]);

	if (gameConfig.grHalfTime)
		halftimeLogic();

	if (gameConfig.grBetterHills && gameConfig.customMapId == 0)
		betterHillsLogic();

	if (gameConfig.grPlayerSize)
		playerSizeLogic();

	if (gameConfig.grHealthBars && isInGame())
	{
		u32 hookValue = 0x0C000000 | ((u32)&healthbarsHook >> 2);
		*(u32*)0x005d608c = hookValue;
		*(u32*)0x005d60c4 = hookValue;
	}

	if (gameConfig.grNoNames)
		gameSettings->PlayerNamesOn = 0;

	if (gameConfig.grNoInvTimer)
		invTimerLogic();
}

/*
 * NAME :		grLobbyStart
 * 
 * DESCRIPTION :
 * 			Gamerules lobby logic entrypoint.
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
void grLobbyStart(void)
{
	GameSettings * gameSettings = gameGetSettings();

	// If we're not in staging then reset
	if (gameSettings)
		return;

	// Reset
	GameRulesInitialized = 0;

	// Reset mirror world in lobby
	cheatsApplyMirrorWorld(0);
}
