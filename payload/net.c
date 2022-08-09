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
int onSetRoundCompleteRemote(void * connection, void * data)
{
	RoundCompleteMessage_t * message = (RoundCompleteMessage_t*)data;
	DPRINTF("recv roundcomplete\n");
	if (isInGame())
		onSetRoundComplete(message->GameTime, message->Outcome, message->RoundDuration, message->Teams);

	return sizeof(RoundCompleteMessage_t);
}

//--------------------------------------------------------------------------
void setRoundComplete(enum RoundOutcome outcome, int roundDuration)
{
	RoundCompleteMessage_t message;

	// don't allow overwriting existing outcome
	if (State.RoundEndTime)
		return;

	// don't allow changing outcome when not host
	if (!State.IsHost)
		return;

	// send out
	message.GameTime = gameGetTime();
  message.Outcome = outcome;
	message.RoundDuration = roundDuration;
	memcpy(message.Teams, State.Teams, sizeof(State.Teams));
	netBroadcastCustomAppMessage(NET_DELIVERY_CRITICAL, netGetDmeServerConnection(), CUSTOM_MSG_ROUND_COMPLETE, sizeof(RoundCompleteMessage_t), &message);

	// set locally
	onSetRoundComplete(message.GameTime, message.Outcome, message.RoundDuration, message.Teams);
}

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
	memcpy(&message.Stats, &State.PlayerStates[playerId].Stats, sizeof(struct PayloadPlayerStats));
	netBroadcastCustomAppMessage(NET_DELIVERY_CRITICAL, netGetDmeServerConnection(), CUSTOM_MSG_PLAYER_SET_STATS, sizeof(SetPlayerStatsMessage_t), &message);
}

//--------------------------------------------------------------------------
int onReceiveTeamScoreRemote(void * connection, void * data)
{
	SetTeamScoreMessage_t * message = (SetTeamScoreMessage_t*)data;
	DPRINTF("recv teamscore\n");
	onReceiveTeamScore(message->TeamScores, message->TeamRoundTimes);

	return sizeof(SetTeamScoreMessage_t);
}

//--------------------------------------------------------------------------
void sendTeamScore(void)
{
	int i;
	SetTeamScoreMessage_t message;

	// send out
	for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
		if (State.Teams[0].TeamId == i) {
			message.TeamScores[i] = State.Teams[0].Score;
			message.TeamRoundTimes[i] = State.Teams[0].Time;
		} else if (State.Teams[1].TeamId == i) {
			message.TeamScores[i] = State.Teams[1].Score;
			message.TeamRoundTimes[i] = State.Teams[1].Time;
		} else {
			message.TeamScores[i] = 0;
			message.TeamRoundTimes[i] = 0;
		}
	}
	
	netBroadcastCustomAppMessage(NET_DELIVERY_CRITICAL, netGetDmeServerConnection(), CUSTOM_MSG_SEND_TEAM_SCORE, sizeof(SetTeamScoreMessage_t), &message);
}

//--------------------------------------------------------------------------
int onReceivePayloadStateRemote(void * connection, void * data)
{
	SetPayloadStateMessage_t * message = (SetPayloadStateMessage_t*)data;
	DPRINTF("recv payloadstate %d %d %d\n", message->Time, message->State, message->PathIndex);
	if (isInGame())
		onReceivePayloadState(message->State, message->PathIndex, message->Time);

	return sizeof(SetPayloadStateMessage_t);
}

//--------------------------------------------------------------------------
void sendPayloadState(enum PayloadMobyState state, int pathIndex, float time)
{
	int i;
	SetPayloadStateMessage_t message;

	if (!State.IsHost)
		return;

	// send out
	message.State = state;
	message.PathIndex = pathIndex;
	message.Time = time;
	netBroadcastCustomAppMessage(NET_DELIVERY_CRITICAL, netGetDmeServerConnection(), CUSTOM_MSG_SEND_PAYLOAD_STATE, sizeof(SetPayloadStateMessage_t), &message);
}

//--------------------------------------------------------------------------
int onReceivePlayerNearPayloadRemote(void * connection, void * data)
{
	static int playerLastRecvTime[GAME_MAX_PLAYERS] = {0,0,0,0,0,0,0,0,0,0};
	SetPlayerNearPayloadMessage_t message;
	
	memcpy(&message, data, sizeof(SetPlayerNearPayloadMessage_t));

	DPRINTF("recv nearpayload %d %d %d\n", message.Time, message.PlayerId, message.IsNearPayload);

	// only accept if in game
	// and the gametime is greater than the last accepted message
	// since this is sent unreliable and packets can come in out of order
	if (isInGame() && message.Time > playerLastRecvTime[message.PlayerId]) {
		playerLastRecvTime[message.PlayerId] = message.Time;
		onReceivePlayerNearPayload(message.PlayerId, message.IsNearPayload);
	}

	return sizeof(SetPlayerNearPayloadMessage_t);
}

//--------------------------------------------------------------------------
void sendPlayerNearPayload(int playerId, int isNearPayload)
{
	static char playerSendCooldown[GAME_MAX_PLAYERS] = {0,0,0,0,0,0,0,0,0,0};
	int i;
	SetPlayerNearPayloadMessage_t message;

	if (playerId < 0 || playerId >= GAME_MAX_PLAYERS)
		return;

	// check cooldown
	int cooldown = playerSendCooldown[playerId];
	if (cooldown > 0) {
		playerSendCooldown[playerId] = (char)(cooldown - 1);
		return;
	}

	// send out
	message.PlayerId = playerId;
	message.IsNearPayload = isNearPayload;
	message.Time = gameGetTime();
	netBroadcastCustomAppMessage(0, netGetDmeServerConnection(), CUSTOM_MSG_PLAYER_NEAR_PAYLOAD, sizeof(SetPlayerNearPayloadMessage_t), &message);

	// set cooldown
	playerSendCooldown[playerId] = PLAYER_NEAR_PAYLOAD_SEND_EVERY_TICKS;
}

void netHookMessages(void)
{
	// Hook custom net events
	netInstallCustomMsgHandler(CUSTOM_MSG_ROUND_COMPLETE, &onSetRoundCompleteRemote);
	netInstallCustomMsgHandler(CUSTOM_MSG_PLAYER_SET_STATS, &onSetPlayerStatsRemote);
	netInstallCustomMsgHandler(CUSTOM_MSG_SEND_PAYLOAD_STATE, &onReceivePayloadStateRemote);
	netInstallCustomMsgHandler(CUSTOM_MSG_SEND_TEAM_SCORE, &onReceiveTeamScoreRemote);
	netInstallCustomMsgHandler(CUSTOM_MSG_PLAYER_NEAR_PAYLOAD, &onReceivePlayerNearPayloadRemote);
}
