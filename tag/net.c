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
  SetPlayerStatsMessage_t message;
  memcpy(&message, data, sizeof(SetPlayerStatsMessage_t));

	onSetPlayerStats(message.PlayerId, &message.Stats);
	return sizeof(SetPlayerStatsMessage_t);
}

//--------------------------------------------------------------------------
int onSetSeekerRemote(void * connection, void * data)
{
  SetSeekerMessage_t message;
  memcpy(&message, data, sizeof(SetSeekerMessage_t));
  
	onSetSeeker(message.PlayerId);
	onSetPlayerStats(message.PlayerId, &message.Stats);
	return sizeof(SetSeekerMessage_t);
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
void sendSeeker(int playerId)
{
	int i;
	SetSeekerMessage_t message;

	// send out
	message.PlayerId = playerId;
	memcpy(&message.Stats, &State.PlayerStates[playerId].Stats, sizeof(struct CGMPlayerStats));
	netBroadcastCustomAppMessage(NET_DELIVERY_CRITICAL, netGetDmeServerConnection(), CUSTOM_MSG_PLAYER_SET_SEEKER, sizeof(SetSeekerMessage_t), &message);
}

//--------------------------------------------------------------------------
void netHookMessages(void)
{
	// Hook custom net events
	netInstallCustomMsgHandler(CUSTOM_MSG_PLAYER_SET_STATS, &onSetPlayerStatsRemote);
	netInstallCustomMsgHandler(CUSTOM_MSG_PLAYER_SET_SEEKER, &onSetSeekerRemote);
}
