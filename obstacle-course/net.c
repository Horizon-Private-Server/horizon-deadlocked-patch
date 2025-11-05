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
void onPlayerReachedEnd(PlayerReachedEndMessage_t* msg)
{
  // mark completed
  State.PlayerStates[msg->PlayerId].TimeCompleted = gameGetTime();
  State.PlayerStates[msg->PlayerId].TotalTicks = msg->Ticks;
}

//--------------------------------------------------------------------------
int onPlayerReachedEndRemote(void * connection, void * data)
{
  PlayerReachedEndMessage_t msg;
	memcpy(&msg, data, sizeof(msg));
  onPlayerReachedEnd(&msg);
	return sizeof(msg);
}

//--------------------------------------------------------------------------
void sendPlayerReachedEnd(int playerId, u64 ticks)
{
	// send out
	PlayerReachedEndMessage_t msg;
	msg.PlayerId = playerId;
	msg.Ticks = ticks;
	netBroadcastCustomAppMessage(0, netGetDmeServerConnection(), CUSTOM_MSG_PLAYER_REACHED_END, sizeof(msg), &msg);
  onPlayerReachedEnd(&msg);
}

//--------------------------------------------------------------------------
void netHookMessages(void)
{
	// Hook custom net events
	netInstallCustomMsgHandler(CUSTOM_MSG_PLAYER_REACHED_END, &onPlayerReachedEndRemote);
}
