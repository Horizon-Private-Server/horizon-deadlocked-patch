#ifndef HNS_GAME_H
#define HNS_GAME_H

#include <tamtypes.h>
#include "messageid.h"
#include "config.h"
#include <libdl/player.h>
#include <libdl/math3d.h>

#define TPS																		(60)
#define HIDE_STAGE_INCREMENT_SEC              (30)
#define TEAM_HIDERS                           (TEAM_BLUE)
#define TEAM_SEEKERS                          (TEAM_RED)

enum GameNetMessage
{
	CUSTOM_MSG_PLAYER_SET_TEAM = CUSTOM_MSG_ID_GAME_MODE_START,
	CUSTOM_MSG_SET_ROUND_STAGE
};

enum GameStage
{
  GAME_STAGE_HIDE,
  GAME_STAGE_SEEK
};

struct CGMState
{
	int InitializedTime;
  enum GameStage Stage;
  int StageTicks;
  int NumHiders;
  char AlphaSeeker[GAME_MAX_PLAYERS];
};

typedef struct SetPlayerTeamMessage
{
	int PlayerId;
  int Team;
} SetPlayerTeamMessage_t;

typedef struct SetRoundStageMessage
{
  enum GameStage Stage;
} SetRoundStageMessage_t;

void onSetPlayerTeam(int playerId, int team);
void onReceiveRoundStage(enum GameStage stage);
void sendPlayerTeam(int playerId, int team);
void sendRoundStage(enum GameStage stage);
void netHookMessages(void);
void updateTeamScore(int team);

extern struct CGMState State;

#endif // HNS_GAME_H
