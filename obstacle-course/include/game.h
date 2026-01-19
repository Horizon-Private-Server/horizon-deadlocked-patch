#ifndef OITC_GAME_H
#define OITC_GAME_H

#include <tamtypes.h>
#include "messageid.h"
#include "config.h"
#include <libdl/player.h>
#include <libdl/math3d.h>

#define TPS																		(60)
#define MIN_FLOAT_MAGNITUDE										(0.0001)
#define CHECKPOINT_MAX_CHECKPOINTS            (32)

enum GameNetMessage
{
	CUSTOM_MSG_PLAYER_REACHED_END = CUSTOM_MSG_ID_GAME_MODE_START,
  CUSTOM_MSG_CLIENT_SET_LAST_CHECKPOINT,
  CUSTOM_MSG_CLIENT_SET_COMPLETE_TIME,
};

typedef void (*SetLocalPlayerReachedEnd_t)(void);
typedef void (*SetLocalPlayerReachedCheckpoint_t)(Moby* checkpoint);

typedef struct PlayerReachedEndMessage
{
	u64 Ticks;
	int PlayerId;
} PlayerReachedEndMessage_t;

struct ObstacleMapConfig
{
  u32 Magic;
  SetLocalPlayerReachedEnd_t SetLocalPlayerReachedEnd;
  SetLocalPlayerReachedCheckpoint_t SetLocalPlayerReachedCheckpoint;
};

struct CGMCustomMapExData
{
	u32 Version;
	u32 GadgetsMask;
  char GameRule;
  char Vehicles;
  char Timelimit;
  char RespawnTime;
  char Survivor;
  char SpawnWithChargeboots;
  char UnlimitedAmmo;
  char AutospawnWeapons;
  char CQLockdown;
  char CQHomenodes;
  char CQBoltCranks;
};

struct CGMPlayer
{
	int PlayerIndex;
  int TimeCompleted;
  u64 TotalTicks;
};

struct CGMState
{
  int StartDelay;
	int WaitingForClientsReady;
	int InitializedTime;
	struct CGMPlayer PlayerStates[GAME_MAX_PLAYERS];
	struct CGMPlayer* LocalPlayerState;
  struct CGMCustomMapExData MapData;
	int GameOver;
	int WinningTeam;
	int IsHost;
  int ShowRestartComboTicks;
  char PopupMessageBuf[64];
  char HasMapData;
  char HasFirstFrame;
  char HasLoadedLast;
};

struct CGMGameData
{
	u32 Version;
	u32 Rounds;
	int Kills[GAME_MAX_PLAYERS];
};

struct CheckpointManagerPVar
{
  char Log;
  char LastCheckpoint;
  Moby* DefaultCheckpointMoby;
  Moby* CheckpointMobys[CHECKPOINT_MAX_CHECKPOINTS];
};

typedef struct SetPlayerSavedCheckpointRequest
{
  u64 CheckpointTicks;
  int CheckpointUid;
} SetPlayerSavedCheckpointRequest_t;

typedef struct SetPlayerCompleteTimeRequest
{
  u64 TotalTicks;
} SetPlayerCompleteTimeRequest_t;

struct ServerConfig {
  u64 CheckpointTicks;
  int LastCheckpointUid;
};

void netHookMessages(void);
void sendPlayerReachedEnd(int playerId, u64 ticks);

extern struct CGMState State;

#endif // OITC_GAME_H
