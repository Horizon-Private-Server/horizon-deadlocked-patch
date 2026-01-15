#ifndef COLLECTATHON_GAME_H
#define COLLECTATHON_GAME_H

#include <tamtypes.h>
#include "messageid.h"
#include "config.h"
#include <libdl/player.h>
#include <libdl/math3d.h>

#define TPS																		(60)
#define MIN_FLOAT_MAGNITUDE										(0.0001)
#define MAX_BOLTS                             (64)

enum GameNetMessage
{
	CUSTOM_MSG_CLIENT_COLLECTED_BOLT = CUSTOM_MSG_ID_GAME_MODE_START,
};

typedef struct PlayerCollectedBoltMessage
{
	int PlayerId;
  u32 BoltUid;
} PlayerCollectedBoltMessage_t;

struct CGMCustomMapExData
{
	u32 Version;
  u8 EasyBoltsCount;
  u8 MediumBoltsCount;
  u8 HardBoltsCount;
  u8 VeryHardBoltsCount;
  u32 BoltUids[0];
};

struct CGMState
{
  int StartDelay;
	int WaitingForClientsReady;
	int InitializedTime;
  struct CGMCustomMapExData MapData;
	int IsHost;
  int BoltCount;
  char PopupMessageBuf[64];
  char HasMapData;
  char HasFirstFrame;
  char HasLoadedLast;
};

struct CGMGameData
{
	u32 Version;
};

struct ServerConfig {
  u32 CollectedBoltsUids[MAX_BOLTS];
};

void sendCollectedBolt(u32 boltUid);
void netHookMessages(void);

extern struct ServerConfig ServerConfig;
extern struct CGMState State;

#endif // COLLECTATHON_GAME_H
