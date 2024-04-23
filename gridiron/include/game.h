#ifndef GRIDIRON_GAME_H
#define GRIDIRON_GAME_H

#include <tamtypes.h>
#include "messageid.h"
#include "config.h"
#include <libdl/player.h>
#include <libdl/math3d.h>

#define TPS																		(60)
#define THROW_BASE_POWER                      (7.0)
#define PICKUP_COOLDOWN                       (TIME_SECOND * 1.5)
#define EQUIP_PING_COOLDOWN                   (TIME_SECOND * 1.5)
#define MIN_FLOAT_MAGNITUDE										(0.0001)

#define POINTS_PER_TOUCHDOWN                  (1)

enum GameNetMessage
{
	CUSTOM_MSG_PLAYER_SET_STATS = CUSTOM_MSG_ID_GAME_MODE_START,
	CUSTOM_MSG_SEND_TEAM_SCORE,
  CUSTOM_MSG_SEND_BALL_SCORED,
  CUSTOM_MSG_BALL_PICK_UP_REQUEST,
};

enum BallResetType
{
  BALL_RESET_CENTER,
  BALL_RESET_TEAM0,
  BALL_RESET_TEAM1,
};

struct CGMPlayerStats
{
	int Points;
  float TimeWithBall;
};

struct CGMPlayer
{
	int PlayerIndex;
  int TimeLastCarrier;
  int TimeLastEquipPing;
	struct CGMPlayerStats Stats;
};

struct CGMTeam
{
  VECTOR BasePosition;
  VECTOR BallSpawnPosition;
  int TeamId;
	int Score;
};

struct CGMState
{
	int InitializedTime;
	struct CGMPlayer PlayerStates[GAME_MAX_PLAYERS];
	struct CGMPlayer* LocalPlayerState;
	struct CGMTeam Teams[2];
  Moby* BallMoby;
	int IsHost;
  VECTOR BallSpawnPosition;
};

struct CGMGameData
{
	u32 Version;
	int TeamScore[GAME_MAX_PLAYERS];
	int Points[GAME_MAX_PLAYERS];
	float TimeWithBall[GAME_MAX_PLAYERS];
	char TeamIds[GAME_MAX_PLAYERS];
};

typedef struct SetPlayerStatsMessage
{
	int PlayerId;
	struct CGMPlayerStats Stats;
} SetPlayerStatsMessage_t;

typedef struct SetTeamScoreMessage
{
	int TeamScores[2];
} SetTeamScoreMessage_t;

typedef struct BallPickupRequestMessage
{
	int PickupByPlayerId;
} BallPickupRequestMessage_t;

typedef struct BallScoredMessage
{
	int ScoredByPlayerId;
} BallScoredMessage_t;

void sendPlayerStats(int playerId);
void sendTeamScore(void);
void sendBallScored(int playerIdx);
void sendBallPickupRequest(int playerIdx);
void netHookMessages(void);
void updateTeamScore(int team);

extern struct CGMState State;

#endif // GRIDIRON_GAME_H
