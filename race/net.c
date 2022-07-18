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
int onPickupItemSpawnedRemote(void * connection, void * data)
{
	PickupItemSpawnedMessage_t * message = (PickupItemSpawnedMessage_t*)data;
	if (isInGame())
		onPickupItemSpawned(message->Type, message->Position);

	return sizeof(PickupItemSpawnedMessage_t);
}

//--------------------------------------------------------------------------
void spawnPickupItem(VECTOR position, int type)
{
	PickupItemSpawnedMessage_t message;

	// don't allow spawning item when not host
	if (!State.IsHost)
		return;

	// send out
	vector_copy(message.Position, position);
	message.Type = type;
	netBroadcastCustomAppMessage(NET_DELIVERY_CRITICAL, netGetDmeServerConnection(), CUSTOM_MSG_PICKUP_ITEM_SPAWNED, sizeof(PickupItemSpawnedMessage_t), &message);

	// set locally
	onPickupItemSpawned(message.Type, message.Position);
}

//--------------------------------------------------------------------------
void netHookMessages(void)
{
	// Hook custom net events
	netInstallCustomMsgHandler(CUSTOM_MSG_PICKUP_ITEM_SPAWNED, &onPickupItemSpawnedRemote);
}
