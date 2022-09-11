#ifndef PAYLOAD_GAME_H
#define PAYLOAD_GAME_H

#include <tamtypes.h>
#include "messageid.h"
#include "bezier.h"
#include "config.h"
#include <libdl/player.h>
#include <libdl/math3d.h>

#define TPS																		(60)

#define ROUND_MESSAGE_DURATION_MS							(TIME_SECOND * 2)
#define ROUND_TRANSITION_DELAY_MS							(TIME_SECOND * 3)

#define PAYLOAD_PATH_MAX_POINTS								(32)
#define PAYLOAD_MAX_SPAWNPOINTS								(150)
#define PAYLOAD_SIZE													(2.5)

#define PAYLOAD_MAX_FORWARD_SPEED							(4)
#define PAYLOAD_MAX_BACKWARD_SPEED						(-2)
#define PAYLOAD_COLLISION_SNAP_SHARPNESS			(3)
#define PAYLOAD_SPEED_SHARPNESS								(3)
#define PAYLOAD_LEVITATE_SPEED								(3)
#define PAYLOAD_LEVITATE_HEIGHT								(0.1)
#define PAYLOAD_PLAYER_RADIUS									(15)
#define PAYLOAD_REVERSE_DELAY_TICKS						(TPS * 4)
#define PAYLOAD_EXPLOSION_COUNTDOWN_TICKS			(60 * 3)
#define PAYLOAD_HEIGHT_OFFSET									(6.0)

#define PAYLOAD_MOBY_ID												(0x2479)

#define PAYLOAD_SPAWN_DISTANCE_DEADZONE				(40)
#define PAYLOAD_SPAWN_DISTANCE_IDEAL					(80)
#define PAYLOAD_SPAWN_DISTANCE_FUDGE					(40)
#define PAYLOAD_SPAWN_NEAR_TEAM_FUDGE					(15)

#define PAYLOAD_SOUND_VOLUME									(2000)
#define PAYLOAD_ELECTRICITY_SOUND_VOLUME			(1250)

#define PLAYER_NEAR_PAYLOAD_SEND_EVERY_TICKS	(15)

#define MIN_FLOAT_MAGNITUDE										(0.0001)

#define TIMER_BASE_COLOR1			(0xFF00B7B7)
#define TIMER_BASE_COLOR2			(0xFF006767)
#define TIMER_HIGH_COLOR			(0xFFFFFFFF)

enum GameNetMessage
{
	CUSTOM_MSG_ROUND_COMPLETE = CUSTOM_MSG_ID_GAME_MODE_START,
	CUSTOM_MSG_PLAYER_SET_STATS,
	CUSTOM_MSG_SEND_PAYLOAD_STATE,
	CUSTOM_MSG_SEND_TEAM_SCORE,
	CUSTOM_MSG_PLAYER_NEAR_PAYLOAD
};

enum RoundOutcome
{
	CUSTOM_MSG_ROUND_PAYLOAD_DELIVERED,
	CUSTOM_MSG_ROUND_OVER
};

enum PayloadMobyState
{
	PAYLOAD_STATE_IDLE,
	PAYLOAD_STATE_MOVING_FORWARD,
	PAYLOAD_STATE_MOVING_BACKWARD,
	PAYLOAD_STATE_DELIVERED,
	PAYLOAD_STATE_EXPLODED
};

struct PayloadPlayerStats
{
	float PointsT;
	int Points;
	// number of kills on players near vicinity of payload while defending
	int KillsOnHotPlayers;
	// number of kills while in the vicinity of the payload as an attacker
	int KillsWhileHot;
};

struct PayloadPlayer
{
	int PlayerIndex;
	int IsAttacking;
	int SoundHandle;
	int SoundId;
	int IsNearPayload;
	struct PayloadPlayerStats Stats;
};

struct PayloadTeam
{
	int TeamId;
	int Score;
	int Time;
};

struct PayloadMobyPVar
{
	VECTOR PathPosition;
	float Distance;
	float Time;
	float Speed;
	float Levitate;
	int PathIndex;
	int SoundHandle;
	int SoundId;
	int AttackerNearCount;
	int DefenderNearCount;
	enum PayloadMobyState State;
	char IsMaxSpeed;
	u16 ExplodeTicks;
	u8 WithoutPlayerTicks;
	u8 RecomputeDistanceTicks;
};

struct PayloadMapConfig
{
	int PathVertexCount;
	int SpawnPointCount;
	short PayloadMoveSoundId;
	short PayloadElectricityEnterSoundId;
	short PayloadElectricityExitSoundId;
	VECTOR PayloadDeliveredCameraPos;
	VECTOR PayloadDeliveredCameraRot;
	BezierPoint_t Path[PAYLOAD_PATH_MAX_POINTS];
	VECTOR SpawnPoints[PAYLOAD_MAX_SPAWNPOINTS];
	u8 PayloadDeliveredCameraOcc[0x80];
};

struct PayloadState
{
	int RoundNumber;
	int RoundLimit;
	int RoundStartTime;
	int RoundDuration;
	int RoundFirstDuration;
	int RoundEndTime;
	int PayloadDeliveredTime;
	int PayloadDeliveredEndRoundTicks;
	int TimerLastPlaySoundSecond;
	int ShownOvertimeHelpMessage;
	enum RoundOutcome RoundResult;
	int InitializedTime;
	float PathSegmentLengths[PAYLOAD_PATH_MAX_POINTS];
	float PathLength;
	struct PayloadPlayer PlayerStates[GAME_MAX_PLAYERS];
	int RoundInitialized;
	struct PayloadPlayer* LocalPlayerState;
	struct PayloadTeam Teams[2];
	int GameOver;
	int WinningTeam;
	int IsHost;
	enum PayloadContestMode ContestMode;
	Moby* PayloadMoby;
};

struct PayloadGameData
{
	u32 Version;
	u32 Rounds;
	int Points[GAME_MAX_PLAYERS];
	int KillsOnHotPlayers[GAME_MAX_PLAYERS];
	int KillsWhileHot[GAME_MAX_PLAYERS];
	int TeamScore[GAME_MAX_PLAYERS];
	int TeamRoundTime[GAME_MAX_PLAYERS];
};

typedef struct RoundCompleteMessage
{
	int GameTime;
	enum RoundOutcome Outcome;
	int RoundDuration;
	struct PayloadTeam Teams[2];
} RoundCompleteMessage_t;

typedef struct SetPlayerStatsMessage
{
	int PlayerId;
	struct PayloadPlayerStats Stats;
} SetPlayerStatsMessage_t;

typedef struct SetPayloadStateMessage
{
	enum PayloadMobyState State;
	int PathIndex;
	float Time;
} SetPayloadStateMessage_t;

typedef struct SetTeamScoreMessage
{
	int TeamScores[GAME_MAX_PLAYERS];
	int TeamRoundTimes[GAME_MAX_PLAYERS];
} SetTeamScoreMessage_t;

typedef struct SetPlayerNearPayloadMessage
{
	int PlayerId;
	int IsNearPayload;
	int Time;
} SetPlayerNearPayloadMessage_t;

extern const char * PAYLOAD_ROUND_COMPLETE;

extern struct PayloadState State;

void setRoundComplete(enum RoundOutcome outcome, int roundDuration);
void sendPlayerStats(int playerId);
void sendPayloadState(enum PayloadMobyState state, int pathIndex, float time);
void sendTeamScore(void);
void sendPlayerNearPayload(int playerId, int isNearPayload);
void netHookMessages(void);
void updateTeamScore(int team);


#endif // PAYLOAD_GAME_H
