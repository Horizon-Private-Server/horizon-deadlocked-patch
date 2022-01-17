/***************************************************
 * FILENAME :		main.c
 * 
 * DESCRIPTION :
 * 		Zombies.
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
#include "include/mob.h"

#define SPLEEF_ROUNDRESULT_WIN					(1)
#define SPLEEF_ROUNDRESULT_LOSS					(2)

const char * SPLEEF_ROUND_COMPLETE = "Round complete!";

int Initialized = 0;
struct SpleefState
{
	int RoundNumber;
	int RoundStartTicks;
	int RoundEndTicks;
	char RoundResult;
	char RoundPlayerState[GAME_MAX_PLAYERS];
	int RoundInitialized;
	int GameOver;
	int WinningTeam;
	int IsHost;
} SpleefState;

struct MobConfig defaultConfigs[] = {
	{
		.MobType = MOB_NORMAL,
		.Damage = 10,
		.Speed = 5 / 60.0,
		.MaxHealth = 15,
		.AttackRadius = 2.5,
		.HitRadius = 1,
		.ReactionTickCount = 15,
		.AttackCooldownTickCount = 120
	},
	{
		.MobType = MOB_NORMAL,
		.Damage = 2,
		.Speed = 25 / 60.0,
		.MaxHealth = 10,
		.AttackRadius = 2.5,
		.HitRadius = 1,
		.ReactionTickCount = 15,
		.AttackCooldownTickCount = 120
	},
	{
		.MobType = MOB_FREEZE,
		.Damage = 5,
		.Speed = 5 / 60.0,
		.MaxHealth = 10,
		.AttackRadius = 2.5,
		.HitRadius = 1,
		.ReactionTickCount = 15,
		.AttackCooldownTickCount = 120
	},
	{
		.MobType = MOB_ACID,
		.Damage = 10,
		.Speed = 10 / 60.0,
		.MaxHealth = 20,
		.AttackRadius = 2.5,
		.HitRadius = 1,
		.ReactionTickCount = 25,
		.AttackCooldownTickCount = 120
	},
	{
		.MobType = MOB_EXPLODE,
		.Damage = 25,
		.Speed = 10 / 60.0,
		.MaxHealth = 5,
		.AttackRadius = 2.5,
		.HitRadius = 5,
		.ReactionTickCount = 15,
		.AttackCooldownTickCount = 120
	},
	{
		.MobType = MOB_GHOST,
		.Damage = 15,
		.Speed = 5 / 60.0,
		.MaxHealth = 24,
		.AttackRadius = 2.5,
		.HitRadius = 5,
		.ReactionTickCount = 15,
		.AttackCooldownTickCount = 120
	}
};

const int defaultConfigsCount = sizeof(defaultConfigs) / sizeof(struct MobConfig);

void drawRoundMessage(const char * message, float scale)
{
	int fw = gfxGetFontWidth(message, -1, scale);
	float x = 0.5;
	float y = 0.16;
	float p = 0.02;
	float w = maxf(196.0, fw);
	float h = 40.0;

	// draw container
	gfxScreenSpaceBox(x-(w/(SCREEN_WIDTH*2.0)), y, (w / SCREEN_WIDTH) + p, (h / SCREEN_HEIGHT) + p, 0x20ffffff);

	// draw message
	y *= SCREEN_HEIGHT;
	x *= SCREEN_WIDTH;
	gfxScreenSpaceText(x, y + 5, scale, scale * 1.5, 0x80FFFFFF, message, -1, 1);
}

int playerIsDead(Player* p)
{
	return p->PlayerState == 57 // dead
								|| p->PlayerState == 106 // drown
								|| p->PlayerState == 118 // death fall
								|| p->PlayerState == 122 // death sink
								|| p->PlayerState == 123 // death lava
								|| p->PlayerState == 148 // death no fall
								;
}

Player * playerGetRandom(void)
{
	int r = rand(GAME_MAX_PLAYERS);
	int i = 0, c = 0;
	Player ** players = playerGetAll();

	do {
		if (players[i])
			++c;
		
		++i;
		if (i == GAME_MAX_PLAYERS) {
			if (c == 0)
				return NULL;
			i = 0;
		}
	} while (c < r);

	return players[i-1];
}

int spawnPointGetNearestTo(VECTOR point, VECTOR out)
{
	VECTOR t;
	int spCount = spawnPointGetCount();
	int i;
	float bestPointDist = 100000;

	for (i = 0; i < spCount; ++i) {
		SpawnPoint* sp = spawnPointGet(i);
		vector_subtract(t, (float*)&sp->M0[12], point);
		float d = vector_sqrmag(t) + randRange(0, 10);
		if (d < bestPointDist) {
			vector_copy(out, (float*)&sp->M0[12]);
			vector_fromyaw(t, randRadian());
			vector_scale(t, t, 3);
			vector_add(out, out, t);
			bestPointDist = d;
		}
	}

	return spCount > 0;
}

int spawnGetRandomPoint(VECTOR out) {
	Player * targetPlayer = playerGetRandom();
	if (targetPlayer)
		return spawnPointGetNearestTo(targetPlayer->PlayerPosition, out);

	return 0;
}

GuberMoby* spawnRandomMob(void) {
	VECTOR sp;
	if (spawnGetRandomPoint(sp))
		return mobCreate(sp, 0, &defaultConfigs[rand(defaultConfigsCount)]);
	
	return NULL;
}

void resetRoundState(void)
{
	int gameTime = gameGetTime();

	// 
	SpleefState.RoundInitialized = 0;
	SpleefState.RoundStartTicks = gameTime;
	SpleefState.RoundEndTicks = 0;
	SpleefState.RoundResult = 0;
	memset(SpleefState.RoundPlayerState, -1, GAME_MAX_PLAYERS);

	// 
	SpleefState.RoundInitialized = 1;
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
	GameOptions * gameOptions = gameGetOptions();

	// Set survivor
	gameOptions->GameFlags.MultiplayerGameFlags.Survivor = 1;
	gameOptions->GameFlags.MultiplayerGameFlags.RespawnTime = -1;
	gameOptions->GameFlags.MultiplayerGameFlags.Teamplay = 1;
	
	// 
	mobInitialize();

	// initialize state
	SpleefState.GameOver = 0;
	SpleefState.RoundNumber = 0;
	resetRoundState();

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
void gameStart(void)
{
	GameSettings * gameSettings = gameGetSettings();
	Player * localPlayer = (Player*)0x00347AA0;
	int i;

	// first
	dlPreUpdate();

	// Ensure in game
	if (!gameSettings || !gameIsIn())
		return;

	// Determine if host
	SpleefState.IsHost = gameIsHost(localPlayer->Guber.Id.GID.HostId);

	if (!Initialized)
		initialize();

#if DEBUG
	if (padGetButton(0, PAD_L3 | PAD_R3) > 0)
		SpleefState.GameOver = 1;
#endif

	// 
#if DEBUG
	if (padGetButtonDown(0, PAD_UP) > 0) {
		static int aaa = 0;
		VECTOR t;
		vector_copy(t, localPlayer->PlayerPosition);
		t[0] += 1;

		mobCreate(t, 0, &defaultConfigs[(aaa++ % defaultConfigsCount)]);
	}
#endif

	if (!gameHasEnded() && !SpleefState.GameOver)
	{
		if (SpleefState.RoundResult)
		{
			if (SpleefState.RoundEndTicks)
			{
				// draw round message
				drawRoundMessage(SPLEEF_ROUND_COMPLETE, 1.5);

				// handle when round properly ends
				if (gameGetTime() > SpleefState.RoundEndTicks)
				{
					if (SpleefState.RoundResult == SPLEEF_ROUNDRESULT_WIN)
					{
						// increment round
						++SpleefState.RoundNumber;

						// reset round state
						resetRoundState();
					}
					else
					{
						SpleefState.GameOver = 1;
					}
				}
			}
			else
			{
				// set when next round starts
				SpleefState.RoundEndTicks = gameGetTime() + (TIME_SECOND * 5);
			}
		}
		else
		{
			// iterate each player
			for (i = 0; i < GAME_MAX_PLAYERS; ++i)
			{
				
			}

			// host specific logic
			if (SpleefState.IsHost && (gameGetTime() - SpleefState.RoundStartTicks) > (5 * TIME_SECOND))
			{
				static int ticker = 60;
				static int spawnCount = 0;
				if (ticker == 0) {
					if (spawnRandomMob()) {
						++spawnCount;

						if ((spawnCount % 40) == 0) {
							for (i = 0; i < defaultConfigsCount; ++i) {
								defaultConfigs[i].Speed *= 1.2;
								defaultConfigs[i].MaxHealth *= 1.2;
								defaultConfigs[i].Damage *= 1.2;
							}
						}
					}
					ticker = 60;
				} else {
					--ticker;
				}
			}
		}
	}
	else
	{
		// set winner
		gameSetWinner(0, 0);

		// end game
		if (SpleefState.GameOver == 1)
		{
			gameEnd(4);
			SpleefState.GameOver = 2;
		}
	}

	// last
	dlPostUpdate();
	return;
}

void setLobbyGameOptions(void)
{
	return;
	int i;

	// deathmatch options
	static char options[] = { 
		0, 0, 			// 0x06 - 0x08
		0, 0, 0, 0, 	// 0x08 - 0x0C
		1, 1, 1, 0,  	// 0x0C - 0x10
		0, 1, 0, 0,		// 0x10 - 0x14
		-1, -1, 0, 1,	// 0x14 - 0x18
	};

	//
	GameSettings * gameSettings = gameGetSettings();
	for (i = 0; i < GAME_MAX_PLAYERS; ++i)
		if (gameSettings->PlayerTeams[i] > 0)
			gameSettings->PlayerTeams[i] = 0;

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
void lobbyStart(void)
{
	int activeId = uiGetActive();
	static int initializedScoreboard = 0;

	// scoreboard
	switch (activeId)
	{
		case 0x15C:
		{
			if (initializedScoreboard)
				break;

			setEndGameScoreboard();
			initializedScoreboard = 1;
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
