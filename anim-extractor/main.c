/***************************************************
 * FILENAME :		main.c
 * 
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */

#include <tamtypes.h>
#include <string.h>

#include <libdl/stdio.h>
#include <libdl/math.h>
#include <libdl/math3d.h>
#include <libdl/time.h>
#include <libdl/game.h>
#include <libdl/gamesettings.h>
#include <libdl/player.h>
#include <libdl/cheats.h>
#include <libdl/sha1.h>
#include <libdl/map.h>
#include <libdl/dialog.h>
#include <libdl/graphics.h>
#include <libdl/pad.h>
#include <libdl/dl.h>
#include <libdl/ui.h>
#include <libdl/net.h>
#include <libdl/utils.h>
#include "module.h"
#include "messageid.h"

/*
 *
 */
int Initialized = 0;
int Waiting = 0;

short RippedOClasses[512];

typedef struct AnimExtractorJointCacheBlock
{
	int Offset;
	short OClass;
	short SeqId;
	float Time;
	float Scale;
	char JointCount;
	char IsEnd;
	char Buffer[0x40 * 7];
} AnimExtractorJointCacheBlock_t;

typedef struct AnimExtractorJointCacheComplete
{
	short OClass;
	short SeqId;
} AnimExtractorJointCacheComplete_t;

void animEvaluateResolve(Moby* moby)
{
	((void (*)(Moby*))0x004FAE10)(moby);
}

void sendAnimation(Moby* moby, int seqId)
{
	int j;
	void* pClass = moby->PClass;
	u32 animSeqPtr = *(u32*)((u32)pClass + 0x48 + (seqId * 4));
	const float step = 0.5;
	AnimExtractorJointCacheBlock_t messageBlock;
	AnimExtractorJointCacheComplete_t messageComplete;

	if (!moby->JointCache || !moby->JointCnt || !animSeqPtr)
		return;


	// initialize
	float t = 0;
	float maxT = (float) *(char*)((u32)animSeqPtr + 0x10);
	float scale = *(float*)((u32)pClass + 0x24);

	DPRINTF("moby:%08X class:%X seqId=%d maxT=%d sent=", (u32)moby, moby->OClass, seqId, (int)maxT);

	// iterate
	do
	{
		moby->AnimSeqId = seqId;
		moby->AnimSeqT = t;
		//moby->AnimFlags = 0x20;
		moby->AnimSeq = (void*)animSeqPtr;
		
		// evaluate
		POKE_U32(0x004FADB8, 0); // disable manipulators
		animEvaluateResolve(moby);
		POKE_U32(0x004FADB8, 0x0C13EB9A); // enable manipulators

		// send to server
		Waiting = 1;
		for (j = 0; j < moby->JointCnt; j += 7)
		{
			messageBlock.Offset = (j * 0x40);
			messageBlock.JointCount = moby->JointCnt;
			messageBlock.OClass = moby->OClass;
			messageBlock.SeqId = seqId;
			messageBlock.Time = t;
			messageBlock.Scale = scale;
			messageBlock.IsEnd = (j+7) >= moby->JointCnt;
			memcpy(messageBlock.Buffer, (void*)((u32)moby->JointCache + messageBlock.Offset), sizeof(messageBlock.Buffer));
			netSendCustomAppMessage(NET_DELIVERY_CRITICAL, netGetLobbyServerConnection(), NET_LOBBY_CLIENT_INDEX, CUSTOM_MSG_ID_GAME_MODE_START, sizeof(messageBlock), &messageBlock);
		}
		DPRINTF("%.1f,", t);

		// wait for server response
		while (Waiting) {
			((void (*)(void))0x015ada8)();
		}

		// step
		t += step;
	}
	while (t < maxT);

	// send complete
	messageComplete.OClass = moby->OClass;
	messageComplete.SeqId = moby->AnimSeqId;
	netSendCustomAppMessage(NET_DELIVERY_CRITICAL, netGetLobbyServerConnection(), NET_LOBBY_CLIENT_INDEX, CUSTOM_MSG_ID_GAME_MODE_START + 1, sizeof(AnimExtractorJointCacheComplete_t), &messageComplete);
	DPRINTF(" complete \n");
}

int onRecvConfirmation(void * connection, void * data)
{
	Waiting = 0;
	return 0;
}

int hasRippedOClass(short oClass)
{
	int i = 0;
	for (i = 0; i < 512; ++i)
		if (RippedOClasses[i] == oClass)
			return 1;

	return 0;
}

void addRippedOClass(short oClass)
{
	int i = 0;
	for (i = 0; i < 512; ++i) {
		if (RippedOClasses[i] == 0) {
			RippedOClasses[i] = oClass;
			return;
		}
	}
}

/*
 * NAME :		gameStart
 * 
 * DESCRIPTION :
 * 			Game logic entrypoint.
 * 
 * NOTES :
 * 			This is called only when in game.
 * 
 * ARGS : 
 * 
 * RETURN :
 * 
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */
void gameStart(struct GameModule * module, PatchStateContainer_t * gameState)
{
	int i;
	GameSettings * gameSettings = gameGetSettings();

	dlPreUpdate();

	// Ensure in game
	if (!gameSettings || !isInGame())
		return;

	// init
	if (!Initialized)
	{
		memset(RippedOClasses, 0, sizeof(RippedOClasses));
		Initialized = 1;
	}

	// 
	netInstallCustomMsgHandler(102, &onRecvConfirmation);

	Player* player = playerGetFromSlot(0);
	Moby* moby = mobyListGetStart();
	moby = mobyFindNextByOClass(moby, 0x23FA);
	static float sMin = 1000;
	static float sMax = 0;
	static int sId = -1;
	if (moby && player) {

		if (sId != moby->AnimSeqId) {
			sId = moby->AnimSeqId;
			sMin = moby->AnimSeqT;
			sMax = sMin;
		} else if (moby->AnimSeqT > sMax) {
			sMax = moby->AnimSeqT;
		} else if (moby->AnimSeqT < sMin) {
			sMin = moby->AnimSeqT;
		}

		char buf[128];
		sprintf(buf, "seq:%d spd:%.2f min|max:(%.1f, %.1f) state:%d noin:%d h:%f", moby->AnimSeqId, moby->AnimSpeed, sMin, sMax, player->PlayerState, player->timers.noInput, player->PlayerPosition[2]);
		gfxScreenSpaceText(15, SCREEN_HEIGHT - 40, 1, 1, 0x80FFFFFF, buf, -1, 0);
	}

	int x,y;
	if (player && gfxWorldSpaceToScreenSpace(player->PlayerPosition, &x, &y)) {
		gfxScreenSpaceText(x, y, 1, 1, 0x80FFFFFF, "+", -1, 4);
	}

	// start
	if (padGetButtonDown(0, PAD_L1 | PAD_UP) > 0) {

		//Moby* moby = mobyListGetStart();
		Moby* endMoby = mobyListGetEnd();

		while (moby < endMoby)
		{
			if (moby && moby->PClass && moby->JointCache && !hasRippedOClass(moby->OClass)) {
				int seqCount = *(char*)((u32)moby->PClass + 0x0C);
				for (i = 0; i < seqCount; ++i) {
					sendAnimation(moby, i);
				}

				addRippedOClass(moby->OClass);
			}
			++moby;
			break;
		}
	}

	dlPostUpdate();
}

/*
 * NAME :		lobbyStart
 * 
 * DESCRIPTION :
 * 			Lobby logic entrypoint.
 * 
 * NOTES :
 * 			This is called only when in lobby.
 * 
 * ARGS : 
 * 
 * RETURN :
 * 
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */
void lobbyStart(struct GameModule * module, PatchStateContainer_t * gameState)
{
	
}

/*
 * NAME :		loadStart
 * 
 * DESCRIPTION :
 * 			Load logic entrypoint.
 * 
 * NOTES :
 * 			This is called only when the game has finished reading the level from the disc and before it has started processing the data.
 * 
 * ARGS : 
 * 
 * RETURN :
 * 
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */
void loadStart(struct GameModule * module, PatchStateContainer_t * gameState)
{
	
}

//--------------------------------------------------------------------------
void start(struct GameModule * module, PatchStateContainer_t * gameState, enum GameModuleContext context)
{
  switch (context)
  {
    case GAMEMODULE_LOBBY: lobbyStart(module, gameState); break;
    case GAMEMODULE_LOAD: loadStart(module, gameState); break;
    case GAMEMODULE_GAME: gameStart(module, gameState); break;
  }
}
