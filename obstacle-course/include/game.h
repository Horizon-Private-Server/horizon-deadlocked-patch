#ifndef OITC_GAME_H
#define OITC_GAME_H

#include <tamtypes.h>
#include "messageid.h"
#include "config.h"
#include <libdl/player.h>
#include <libdl/math3d.h>

#define TPS																		(60)
#define MIN_FLOAT_MAGNITUDE										(0.0001)

enum GameNetMessage
{
	CUSTOM_MSG_PLAYER_REACHED_END = CUSTOM_MSG_ID_GAME_MODE_START,
};

typedef void (*SetLocalPlayerReachedEnd_t)(void);

typedef struct PlayerReachedEndMessage
{
	int PlayerId;
	int Time;
} PlayerReachedEndMessage_t;

struct ObstacleMapConfig
{
  u32 Magic;
  SetLocalPlayerReachedEnd_t SetLocalPlayerReachedEnd;
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
};

struct CGMState
{
	int InitializedTime;
	struct CGMPlayer PlayerStates[GAME_MAX_PLAYERS];
	struct CGMPlayer* LocalPlayerState;
  struct CGMCustomMapExData MapData;
	int GameOver;
	int WinningTeam;
	int IsHost;
  char PopupMessageBuf[64];
  char HasMapData;
};

struct CGMGameData
{
	u32 Version;
	u32 Rounds;
	int Kills[GAME_MAX_PLAYERS];
};

void netHookMessages(void);
void sendPlayerReachedEnd(int playerId, int time);

extern struct CGMState State;

#endif // OITC_GAME_H
