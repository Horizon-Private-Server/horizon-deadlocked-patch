#ifndef _CLIMBER_GAME_H_
#define _CLIMBER_GAME_H_

#include <tamtypes.h>
#include <libdl/gamesettings.h>
#include "messageid.h"

enum GameNetMessage
{
	CUSTOM_MSG_SPAWN = CUSTOM_MSG_ID_GAME_MODE_START,
};

typedef struct MobyDef
{
	float ScaleHorizontal;
	float ScaleVertical;
	float ObjectScale;

	u16 OClass;
} MobyDef;

struct State
{
	int TimePlayerDied[GAME_MAX_PLAYERS];
	float PlayerFinalHeight[GAME_MAX_PLAYERS];
	float PlayerBestHeight[GAME_MAX_PLAYERS];
	int StartTime;
	int EndTime;
	float StartHeight;
  int IsHost;
	int GameOver;
} State;

struct ClimberGameData
{
	u32 Version;
	float FinalScore[GAME_MAX_PLAYERS];
	float BestScore[GAME_MAX_PLAYERS];
};

struct ClimberConfig
{
	
};

typedef struct SpawnMessage
{
	int GameTime;
	u16 Seed;
} SpawnMessage_t;

void netHookMessages(void);
void sendSpawn(u16 seed, int gameTime);

#endif // _CLIMBER_GAME_H_
