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

extern struct State State;

//--------------------------------------------------------------------------
int onSpawnRemote(void * connection, void * data)
{
	SpawnMessage_t* message = (SpawnMessage_t*)data;
	onReceiveSpawn(message->Seed, message->GameTime);

	return sizeof(SpawnMessage_t);
}

//--------------------------------------------------------------------------
void sendSpawn(u16 seed, int gameTime)
{
	int i;
	SpawnMessage_t message;

	if (!State.IsHost)
		return;

	// send out
	message.GameTime = gameTime;
	message.Seed = seed;
	netBroadcastCustomAppMessage(netGetDmeServerConnection(), CUSTOM_MSG_SPAWN, sizeof(SpawnMessage_t), &message);
}

void netHookMessages(void)
{
	// Hook custom net events
	netInstallCustomMsgHandler(CUSTOM_MSG_SPAWN, &onSpawnRemote);
}
