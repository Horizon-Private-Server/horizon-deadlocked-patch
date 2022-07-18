#ifndef RACE_GAME_H
#define RACE_GAME_H

#include <tamtypes.h>
#include "messageid.h"
#include "bezier.h"
#include <libdl/player.h>
#include <libdl/math3d.h>

#define TPS																		(60)

#define ROUND_MESSAGE_DURATION_MS							(TIME_SECOND * 2)
#define ROUND_TRANSITION_DELAY_MS							(TIME_SECOND * 3)

#define RACE_PATH_MAX_POINTS									(32)
#define RACE_MAX_CHECKPOINTS									(100)

#define CHECKPOINT_REACHED_FADE_TICKS					(20)
#define CHECKPOINT_REACHED_FADE_MAX_SCALE			(5)
#define CHECKPOINT_RADIUS											(5)
#define CHECKPOINT_HIT_DEPTH									(2.5)

#define MIN_FLOAT_MAGNITUDE										(0.0001)

enum GameNetMessage
{
	CUSTOM_MSG_PICKUP_ITEM_SPAWNED = CUSTOM_MSG_ID_GAME_MODE_START,
};

struct RacePlayerStats
{
	int LapNumber;
	int LastCheckpoint;
};

struct RacePlayer
{
	Player * Player;
	Vehicle * Hoverbike;
	int TicksSinceReachedLastCheckpoint;
	struct RacePlayerStats Stats;
};

struct RaceConfig
{
	int PathVertexCount;
	int CheckPointCount;
	int TargetLaps;
	BezierPoint_t Path[RACE_PATH_MAX_POINTS];
	VECTOR CheckPoints[RACE_MAX_CHECKPOINTS];
};

struct RaceState
{
	int InitializedTime;
	int RaceStartTime;
	float PathSegmentLengths[RACE_PATH_MAX_POINTS];
	float PathLength;
	float CheckPointLengths[RACE_MAX_CHECKPOINTS];
	struct RacePlayer PlayerStates[GAME_MAX_PLAYERS];
	struct RacePlayer* LocalPlayerState;
	int GameOver;
	int WinningTeam;
	int IsHost;
};

struct RaceGameData
{
	u32 Version;
	int Points[GAME_MAX_PLAYERS];
};

typedef struct PickupItemSpawnedMessage
{
	VECTOR Position;
	int Type;
} PickupItemSpawnedMessage_t;

extern struct RaceState State;

void spawnPickupItem(VECTOR position, int type);
void netHookMessages(void);
void updateTeamScore(int team);


#endif // RACE_GAME_H
