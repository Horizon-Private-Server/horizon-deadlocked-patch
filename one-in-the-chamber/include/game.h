#ifndef OITC_GAME_H
#define OITC_GAME_H

#include <tamtypes.h>
#include "messageid.h"
#include "config.h"
#include <libdl/player.h>
#include <libdl/math3d.h>

#define TPS																		(60)
#define MIN_FLOAT_MAGNITUDE										(0.0001)

struct CGMPlayer
{
	int PlayerIndex;
};

struct CGMState
{
	int InitializedTime;
	struct CGMPlayer PlayerStates[GAME_MAX_PLAYERS];
	struct CGMPlayer* LocalPlayerState;
	int GameOver;
	int WinningTeam;
	int IsHost;
  char PopupMessageBuf[64];
};

struct CGMGameData
{
	u32 Version;
	u32 Rounds;
	int Kills[GAME_MAX_PLAYERS];
};

void netHookMessages(void);

extern struct CGMState State;

#endif // OITC_GAME_H
