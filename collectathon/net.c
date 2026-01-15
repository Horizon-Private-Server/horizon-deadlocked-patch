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
int onPlayerCollectedBoltRemote(void * connection, void * data)
{
  PlayerCollectedBoltMessage_t msg;
	memcpy(&msg, data, sizeof(msg));

  GameSettings* gs = gameGetSettings();
  snprintf(State.PopupMessageBuf, sizeof(State.PopupMessageBuf), "%s found a bolt!", gs->PlayerNames[msg.PlayerId]);
  uiShowPopup(0, State.PopupMessageBuf);
  uiShowPopup(1, State.PopupMessageBuf);

	return sizeof(msg);
}

//--------------------------------------------------------------------------
void sendCollectedBolt(u32 boltUid)
{
  Player* player = playerGetFromSlot(0);
  if (!playerIsValid(player)) return;

  // send to server
	PlayerCollectedBoltMessage_t msg;
	msg.PlayerId = player->PlayerId;
	msg.BoltUid = boltUid;
	netSendCustomAppMessage(NET_DELIVERY_CRITICAL, netGetLobbyServerConnection(), NET_LOBBY_CLIENT_INDEX, CUSTOM_MSG_CLIENT_COLLECTED_BOLT, sizeof(msg), &msg);

	// send out to other players
	netBroadcastCustomAppMessage(NET_DELIVERY_CRITICAL, netGetDmeServerConnection(), CUSTOM_MSG_CLIENT_COLLECTED_BOLT, sizeof(msg), &msg);

  // add to list if not already
  int i;
  for (i = 0; i < MAX_BOLTS; ++i) {
    if (ServerConfig.CollectedBoltsUids[i] == boltUid) break;
    if (ServerConfig.CollectedBoltsUids[i] == 0) {
      ServerConfig.CollectedBoltsUids[i] = boltUid;
      break;
    }
  }
}

//--------------------------------------------------------------------------
void netHookMessages(void)
{
	// Hook custom net events
	netInstallCustomMsgHandler(CUSTOM_MSG_CLIENT_COLLECTED_BOLT, &onPlayerCollectedBoltRemote);
}
