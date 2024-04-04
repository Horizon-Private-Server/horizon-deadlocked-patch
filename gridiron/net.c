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
  static int sent = 0;
  if (sent) return;

	int i;
	SetTeamScoreMessage_t message;
  sent = 1;

	// send out
  message.TeamScores[0] = State.Teams[0].Score;
  message.TeamScores[1] = State.Teams[1].Score;
	netBroadcastCustomAppMessage(NET_DELIVERY_CRITICAL, netGetDmeServerConnection(), CUSTOM_MSG_SEND_TEAM_SCORE, sizeof(SetTeamScoreMessage_t), &message);
}

//--------------------------------------------------------------------------
int onReceiveBallPickupRequestRemote(void * connection, void * data)
{
  BallPickupRequestMessage_t msg;
  memcpy(&msg, data, sizeof(msg));
	DPRINTF("recv BallPickupRequestMessage\n");
	onReceiveBallPickupRequest(msg.PickupByPlayerId);

	return sizeof(BallPickupRequestMessage_t);
}

//--------------------------------------------------------------------------
void sendBallPickupRequest(int playerIdx)
{
  static int timeLastSent = 0;
	BallPickupRequestMessage_t msg;

  // prevent spamming request
  if ((gameGetTime() - timeLastSent) < TIME_SECOND*0.25) return;

  // send to host
  msg.PickupByPlayerId = playerIdx;
	netSendCustomAppMessage(NET_DELIVERY_CRITICAL, netGetDmeServerConnection(), gameGetHostId(), CUSTOM_MSG_BALL_PICK_UP_REQUEST, sizeof(msg), &msg);
  timeLastSent = gameGetTime();
}

//--------------------------------------------------------------------------
int onReceiveBallScoredRemote(void * connection, void * data)
{
  BallScoredMessage_t msg;
  memcpy(&msg, data, sizeof(msg));
	DPRINTF("recv BallScoredMessage\n");
	onReceiveBallScored(msg.ScoredByPlayerId);

	return sizeof(BallScoredMessage_t);
}

//--------------------------------------------------------------------------
void sendBallScored(int playerIdx)
{
	BallScoredMessage_t msg;

  // send to all
  msg.ScoredByPlayerId = playerIdx;
	netBroadcastCustomAppMessage(NET_DELIVERY_CRITICAL, netGetDmeServerConnection(), CUSTOM_MSG_SEND_BALL_SCORED, sizeof(msg), &msg);

  // receive this one locally
  onReceiveBallScored(playerIdx);
}

//--------------------------------------------------------------------------
void netHookMessages(void)
{
	// Hook custom net events
	netInstallCustomMsgHandler(CUSTOM_MSG_PLAYER_SET_STATS, &onSetPlayerStatsRemote);
	netInstallCustomMsgHandler(CUSTOM_MSG_SEND_TEAM_SCORE, &onReceiveTeamScoreRemote);
	netInstallCustomMsgHandler(CUSTOM_MSG_SEND_BALL_SCORED, &onReceiveBallScoredRemote);
	netInstallCustomMsgHandler(CUSTOM_MSG_BALL_PICK_UP_REQUEST, &onReceiveBallPickupRequestRemote);
}
