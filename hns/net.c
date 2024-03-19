#include <tamtypes.h>
#include <string.h>

#include <libdl/time.h>
#include <libdl/game.h>
#include <libdl/gamesettings.h>
#include <libdl/player.h>
#include <libdl/weapon.h>
#include <libdl/hud.h>
#include <libdl/cheats.h>
#include <libdl/sha1.h>
#include <libdl/dialog.h>
#include <libdl/ui.h>
#include <libdl/stdio.h>
#include <libdl/graphics.h>
#include <libdl/spawnpoint.h>
#include <libdl/random.h>
#include <libdl/net.h>
#include <libdl/sound.h>
#include <libdl/dl.h>
#include "module.h"
#include "messageid.h"
#include "include/game.h"
#include "include/utils.h"

//--------------------------------------------------------------------------
int onSetPlayerTeamRemote(void * connection, void * data)
{
  SetPlayerTeamMessage_t message;
  memcpy(&message, data, sizeof(message));
	onSetPlayerTeam(message.PlayerId, message.Team);

	return sizeof(message);
}

//--------------------------------------------------------------------------
void sendPlayerTeam(int playerId, int team)
{
	int i;
	SetPlayerTeamMessage_t message;

  DPRINTF("send %d team to %d\n", playerId, team);

	// send out
	message.PlayerId = playerId;
  message.Team = team;
	netBroadcastCustomAppMessage(NET_DELIVERY_CRITICAL, netGetDmeServerConnection(), CUSTOM_MSG_PLAYER_SET_TEAM, sizeof(message), &message);
}

//--------------------------------------------------------------------------
int onReceiveRoundStageRemote(void * connection, void * data)
{
  SetRoundStageMessage_t message;
  memcpy(&message, data, sizeof(message));
	onReceiveRoundStage(message.Stage);

	return sizeof(message);
}

//--------------------------------------------------------------------------
void sendRoundStage(enum GameStage stage)
{
	int i;
	SetRoundStageMessage_t message;

	// send out
	message.Stage = stage;
	netBroadcastCustomAppMessage(NET_DELIVERY_CRITICAL, netGetDmeServerConnection(), CUSTOM_MSG_SET_ROUND_STAGE, sizeof(message), &message);
}

void netHookMessages(void)
{
	// Hook custom net events
	netInstallCustomMsgHandler(CUSTOM_MSG_PLAYER_SET_TEAM, &onSetPlayerTeamRemote);
	netInstallCustomMsgHandler(CUSTOM_MSG_SET_ROUND_STAGE, &onReceiveRoundStageRemote);
}
