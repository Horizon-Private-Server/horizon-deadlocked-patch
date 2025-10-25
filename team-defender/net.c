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
int onSetPlayerStatsRemote(void * connection, void * data)
{
	SetPlayerStatsMessage_t * message = (SetPlayerStatsMessage_t*)data;
	DPRINTF("recv playerstats\n");
	onSetPlayerStats(message->PlayerId, &message->Stats);

	return sizeof(SetPlayerStatsMessage_t);
}

//--------------------------------------------------------------------------
void sendPlayerStats(int playerId)
{
	int i;
	SetPlayerStatsMessage_t message;

	// send out
	message.PlayerId = playerId;
	memcpy(&message.Stats, &State.PlayerStates[playerId].Stats, sizeof(struct CGMPlayerStats));
	netBroadcastCustomAppMessage(NET_DELIVERY_CRITICAL, netGetDmeServerConnection(), CUSTOM_MSG_PLAYER_SET_STATS, sizeof(SetPlayerStatsMessage_t), &message);
}

//--------------------------------------------------------------------------
int onReceiveTeamScoreRemote(void * connection, void * data)
{
	SetTeamScoreMessage_t * message = (SetTeamScoreMessage_t*)data;
	DPRINTF("recv teamscore\n");
	onReceiveTeamScore(message->TeamScores);

	return sizeof(SetTeamScoreMessage_t);
}

//--------------------------------------------------------------------------
void sendTeamScore(void)
{
	int i;
	SetTeamScoreMessage_t message;

	// send out
	for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
    message.TeamScores[i] = State.Teams[i].Score;
	}
	
	netBroadcastCustomAppMessage(NET_DELIVERY_CRITICAL, netGetDmeServerConnection(), CUSTOM_MSG_SEND_TEAM_SCORE, sizeof(SetTeamScoreMessage_t), &message);
}

//--------------------------------------------------------------------------
int onReceivePlayerAssistRemote(void * connection, void * data)
{
  PlayerAssistMessage_t message;
  memcpy(&message, data, sizeof(PlayerAssistMessage_t));
	DPRINTF("recv player assist %d\n", message.PlayerIdx);
	onReceivePlayerAssist(message.PlayerIdx);

	return sizeof(PlayerAssistMessage_t);
}

//--------------------------------------------------------------------------
void sendPlayerAssist(int playerIdx)
{
	int i;
	PlayerAssistMessage_t message;

	// send out
  message.PlayerIdx = playerIdx;
	netBroadcastCustomAppMessage(NET_DELIVERY_CRITICAL, netGetDmeServerConnection(), CUSTOM_MSG_SEND_PLAYER_ASSIST, sizeof(PlayerAssistMessage_t), &message);
}

//--------------------------------------------------------------------------
int onReceiveFlagBasePositionRemote(void * connection, void * data)
{
  FlagBasePositionMessage_t message;
  memcpy(&message, data, sizeof(FlagBasePositionMessage_t));
	DPRINTF("recv base spawn\n");
	onReceiveFlagBasePosition(message.Position);

	return sizeof(FlagBasePositionMessage_t);
}

//--------------------------------------------------------------------------
void sendFlagBasePosition(VECTOR basePos)
{
	int i;
	FlagBasePositionMessage_t message;

	// send out
  vector_copy(message.Position, basePos);
	netBroadcastCustomAppMessage(NET_DELIVERY_CRITICAL, netGetDmeServerConnection(), CUSTOM_MSG_SEND_FLAG_BASE_POSITION, sizeof(FlagBasePositionMessage_t), &message);
}

void netHookMessages(void)
{
	// Hook custom net events
	netInstallCustomMsgHandler(CUSTOM_MSG_PLAYER_SET_STATS, &onSetPlayerStatsRemote);
	netInstallCustomMsgHandler(CUSTOM_MSG_SEND_TEAM_SCORE, &onReceiveTeamScoreRemote);
	netInstallCustomMsgHandler(CUSTOM_MSG_SEND_PLAYER_ASSIST, &onReceivePlayerAssistRemote);
	netInstallCustomMsgHandler(CUSTOM_MSG_SEND_FLAG_BASE_POSITION, &onReceiveFlagBasePositionRemote);
}
