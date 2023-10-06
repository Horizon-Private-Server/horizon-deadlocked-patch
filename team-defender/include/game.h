#ifndef TEAM_DEFENDER_GAME_H
#define TEAM_DEFENDER_GAME_H

#include <tamtypes.h>
#include "messageid.h"
#include "config.h"
#include <libdl/player.h>
#include <libdl/math3d.h>

#define TPS																		(60)

#define MIN_FLOAT_MAGNITUDE										(0.0001)

#define POINTS_PER_KILL                       (1)
#define POINTS_PER_ASSIST                     (1)
#define POINTS_PER_FLAG_KILL                  (4)

#define ASSIST_FEED_MAX_ITEMS                 (8)
#define ASSIST_FEED_ITEM_LIFETIME_TICKS       (2 * TPS)

enum GameNetMessage
{
	CUSTOM_MSG_PLAYER_SET_STATS = CUSTOM_MSG_ID_GAME_MODE_START,
	CUSTOM_MSG_SEND_TEAM_SCORE,
  CUSTOM_MSG_SEND_PLAYER_ASSIST,
  CUSTOM_MSG_SEND_FLAG_BASE_POSITION,
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
	int Score;
};

struct CGMState
{
	int InitializedTime;
	struct CGMPlayer PlayerStates[GAME_MAX_PLAYERS];
	struct CGMPlayer* LocalPlayerState;
	struct CGMTeam Teams[GAME_MAX_PLAYERS];
  Moby* FlagMoby;
  int CurrentFlagCarrierPlayerIdx;
	int IsHost;
};

struct CGMGameData
{
	u32 Version;
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

typedef struct PlayerAssistMessage
{
  int PlayerIdx;
} PlayerAssistMessage_t;

typedef struct FlagBasePositionMessage
{
  VECTOR Position;
} FlagBasePositionMessage_t;

struct FlagPVars
{
	VECTOR BasePosition;
	short CarrierIdx;
	short LastCarrierIdx;
	short Team;
	char UNK_16[6];
	int TimeFlagDropped;
};

struct AssistFeedItem
{
  int LocalPlayerIdx;
  int TicksLeft;
};

void sendPlayerStats(int playerId);
void sendTeamScore(void);
void sendPlayerAssist(int playerIdx);
void sendFlagBasePosition(VECTOR basePos);
void netHookMessages(void);
void updateTeamScore(int team);

extern struct CGMState State;

#endif // TEAM_DEFENDER_GAME_H
