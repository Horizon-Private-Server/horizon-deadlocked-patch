#ifndef TAG_GAME_H
#define TAG_GAME_H

#include <tamtypes.h>
#include "messageid.h"
#include "config.h"
#include <libdl/player.h>
#include <libdl/math3d.h>

#define TPS																		(60)
#define MIN_FLOAT_MAGNITUDE										(0.0001)
#define TAG_INITIAL_SEEKER_SELECTOR_DELAY     (TPS * 3)
#define TAG_PTS_DELTA_DURATION_TICKS          (TPS * 2)
#define TAG_POINTS_STEAL_PERCENT              (0.25)
#define TAG_POINTS_TAG_BONUS                  (5)
#define TAG_HIDER_MAX_HEALTH_PERCENT          (0.2)

enum GameNetMessage
{
	CUSTOM_MSG_PLAYER_SET_STATS = CUSTOM_MSG_ID_GAME_MODE_START,
	CUSTOM_MSG_PLAYER_SET_SEEKER,
};

struct CGMPlayerStats
{
	int Points;
  int PointsBonus;
  int PointsLostStolen;
  int PointsGainStolen;
};

struct CGMPlayer
{
	int PlayerIndex;
  int IsSeeker;
  int LastPtsDelta;
  int DisplayLastPtsDeltaTicks;
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
  int TicksUntilSeekerSelection;
	int GameOver;
	int WinningTeam;
	int IsHost;
  char PopupMessageBuf[64];
};

struct CGMGameData
{
	u32 Version;
	u32 Rounds;
	int Points[GAME_MAX_PLAYERS];
	int PointsLostStolen[GAME_MAX_PLAYERS];
	int PointsGainStolen[GAME_MAX_PLAYERS];
	int PointsBonus[GAME_MAX_PLAYERS];
};

typedef struct SetPlayerStatsMessage
{
	int PlayerId;
	struct CGMPlayerStats Stats;
} SetPlayerStatsMessage_t;

typedef struct SetSeekerMessage
{
	int PlayerId;
	struct CGMPlayerStats Stats;
} SetSeekerMessage_t;

void sendPlayerStats(int playerId);
void sendSeeker(int playerId);
void netHookMessages(void);
void updateTeamScore(int team);

void onSetPlayerStats(int playerId, struct CGMPlayerStats* stats);
void onSetSeeker(int player);

extern struct CGMState State;

#endif // TAG_GAME_H
