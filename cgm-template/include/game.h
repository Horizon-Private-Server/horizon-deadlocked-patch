#ifndef TEAM_DEFENDER_GAME_H
#define TEAM_DEFENDER_GAME_H

#include <tamtypes.h>
#include "messageid.h"
#include "config.h"
#include <libdl/player.h>
#include <libdl/math3d.h>

#define TPS																		(60)

#define MIN_FLOAT_MAGNITUDE										(0.0001)

enum GameNetMessage
{
	CUSTOM_MSG_PLAYER_SET_STATS = CUSTOM_MSG_ID_GAME_MODE_START,
	CUSTOM_MSG_SEND_TEAM_SCORE,
};

struct CGMPlayerStats
{
	int Points;
  float TimeWithFlag;
};

struct CGMPlayer
{
	int PlayerIndex;
	struct CGMPlayerStats Stats;
};

struct CGMTeam
{
	int TeamId;
	int Score;
};

struct CGMState
{
	int InitializedTime;
	struct CGMPlayer PlayerStates[GAME_MAX_PLAYERS];
	struct CGMPlayer* LocalPlayerState;
	struct CGMTeam Teams[2];
	int GameOver;
	int WinningTeam;
	int IsHost;
};

struct CGMGameData
{
	u32 Version;
	u32 Rounds;
	int TeamScore[GAME_MAX_PLAYERS];
	int Points[GAME_MAX_PLAYERS];
	float TimeWithFlag[GAME_MAX_PLAYERS];
};

typedef struct SetPlayerStatsMessage
{
	int PlayerId;
	struct CGMPlayerStats Stats;
} SetPlayerStatsMessage_t;

typedef struct SetTeamScoreMessage
{
	int TeamScores[GAME_MAX_PLAYERS];
} SetTeamScoreMessage_t;

void sendPlayerStats(int playerId);
void sendTeamScore(void);
void netHookMessages(void);
void updateTeamScore(int team);

extern struct CGMState State;

#endif // TEAM_DEFENDER_GAME_H
