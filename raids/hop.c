#include <string.h>
#include <libdl/stdio.h>
#include <libdl/game.h>
#include <libdl/collision.h>
#include <libdl/stdlib.h>
#include <libdl/color.h>
#include <libdl/moby.h>
#include <libdl/sound.h>
#include <libdl/random.h>
#include <libdl/utils.h>
#include <libdl/net.h>
#include <libdl/ui.h>
#include <libdl/graphics.h>
#include "include/utils.h"
#include "include/hop.h"
#include "include/game.h"
#include "config.h"
#include "common.h"

extern struct RaidsState State;
extern int Initialized;

void pushSnack(char * str, int ticksAlive, int localPlayerIdx);

int hopCost = 0;

//--------------------------------------------------------------------------
void hopCancel(void)
{
  // refund host
  if (State.PendingWorldHopMapDef && State.PendingWorldHopAtTime && gameAmIHost()) {
    bankAddBolts(hopCost);
  }

  State.PendingWorldHopAtTime = 0;
  State.PendingWorldHopMapDef = NULL;
  State.PendingWorldHopDifficultyStars = 0;
  hopCost = 0;
}

//--------------------------------------------------------------------------
int hopPrepare(char* mapFilename, int difficulty, int cost, int loadAtTime)
{
  // check if msg is canceling hop
  if (!mapFilename || !mapFilename[0]) {
    hopCancel();
    return 0;
  }

  // find map
  if (PATCH_INTEROP && PATCH_INTEROP->GetCustomMapDefCount) {
    int customMapCount = PATCH_INTEROP->GetCustomMapDefCount();
    int i;
    for (i = 0; i < customMapCount; ++i) {
      CustomMapDef_t* def = PATCH_INTEROP->GetCustomMapDef(i);
      if (strncmp(def->Filename, mapFilename, sizeof(def->Filename)) == 0) {
        State.PendingWorldHopMapDef = def;
        State.PendingWorldHopAtTime = loadAtTime;
        State.PendingWorldHopDifficultyStars = difficulty;
            
        // charge host
        if (gameAmIHost()) {
          hopCost = cost;
          bankSubtractBolts(cost);
        }
        return 1;
      }
    }
  }

  hopCancel();
  return 0;
}

//--------------------------------------------------------------------------
int hopOnBeginRemote(void * connection, void * data)
{
  struct HopOnBeginMsg msg;
  memcpy(&msg, data, sizeof(msg));

  // check if msg is canceling hop
  hopCost = 0;
  if (!msg.MapFilename[0]) {
    hopCancel();
    return sizeof(msg);
  }

  // try and prepare world hop
  // if unable to, then broadcast that to everyone
  if (!hopPrepare(msg.MapFilename, msg.Difficulty, 0, msg.LoadAtTime)) {
    int clientId = gameGetMyClientId();
    netBroadcastCustomAppMessage(NET_DELIVERY_CRITICAL, connection, CUSTOM_MSG_WORLD_HOP_MISSING_MAP, sizeof(clientId), &clientId);
  }

  return sizeof(msg);
}

//--------------------------------------------------------------------------
int hopOnClientMissingMapRemote(void * connection, void * data)
{
  char strBuf[64];
  int clientId;
  memcpy(&clientId, data, sizeof(clientId));

  // find player id of source
  GameSettings* gs = gameGetSettings();
  int i;
  int playerId = -1;
  for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
    if (gs->PlayerClients[i] == clientId) {
      playerId = i;
      break;
    }
  }

  // show snack that client didn't have the map
  if (playerId >= 0) {
    snprintf(strBuf, sizeof(strBuf), "%s missing map", gs->PlayerNames[playerId]);
    pushSnack(strBuf, 30, 0);
  }

  // cancel
  hopCancel();
  return sizeof(clientId);
}

//--------------------------------------------------------------------------
void hopBegin(char* mapFilename, int difficulty, int cost, int delayMs)
{
  void* connection = netGetDmeServerConnection();
  if (!connection) return;

#if DEBUG
  cost = 0;
#endif

  if (hopPrepare(mapFilename, difficulty, cost, gameGetTime() + delayMs)) {

    // broadcast
    struct HopOnBeginMsg msg = { .LoadAtTime = State.PendingWorldHopAtTime, .Difficulty = difficulty };
    if (State.PendingWorldHopMapDef)
      strncpy(msg.MapFilename, State.PendingWorldHopMapDef->Filename, sizeof(msg.MapFilename));
      
    netBroadcastCustomAppMessage(NET_DELIVERY_CRITICAL, connection, CUSTOM_MSG_BEGIN_WORLD_HOP, sizeof(msg), &msg);
  }
}

//--------------------------------------------------------------------------
void hopDo(void)
{
  if (!State.PendingWorldHopMapDef) return;
  if (!PATCH_INTEROP) return;
  if (!PATCH_INTEROP->HopToCustomMap) return;

  // save last equipslots
  int i;
  for (i = 0; i < GAME_MAX_LOCALS; ++i) {
    Player* player = playerGetFromSlot(i);
    if (!player) continue;

    State.PlayerStates[i].LastEquipslots[0] = playerGetLocalEquipslot(i, 0);
    State.PlayerStates[i].LastEquipslots[1] = playerGetLocalEquipslot(i, 1);
    State.PlayerStates[i].LastEquipslots[2] = playerGetLocalEquipslot(i, 2);
  }

  // reset init
  State.MissionStartTime = 0;
  State.MissionStatus = RAIDS_MISSION_ACTIVE;
  State.MissionCompleteTime = 0;
  State.ClientsReady = 0;
  State.DifficultyStars = State.PendingWorldHopDifficultyStars;
  Initialized = 0;

  // hop
  CustomMapDef_t* def = State.PendingWorldHopMapDef;
  State.CurrentMapDef = def;
  State.PendingWorldHopMapDef = NULL;
  State.PendingWorldHopAtTime = 0;
  State.PendingWorldHopDifficultyStars = 0;
  PATCH_INTEROP->HopToCustomMap(def);
}

//--------------------------------------------------------------------------
void hopTick(void)
{
  if (State.PendingWorldHopAtTime <= 0 || !State.PendingWorldHopMapDef) return;

  int delaySeconds = (State.PendingWorldHopAtTime - gameGetTime()) / TIME_SECOND;
  if (delaySeconds <= 0) {
    hopDo();
    return;
  }

  float x = SCREEN_WIDTH * 0.5;
  float y = SCREEN_HEIGHT * 0.2;
  float offY = 0;

  if (strncmp(State.PendingWorldHopMapDef->Filename, RAIDS_HUB_MAPFILENAME, sizeof(State.PendingWorldHopMapDef->Filename)) != 0) {
    drawStars(x, y, 0, offY, 16, 4, 0x80008080, TEXT_ALIGN_MIDDLECENTER, State.PendingWorldHopDifficultyStars + 1);
    offY += 20;
  }
  
  gfxHelperDrawText(x, y, 0, offY, 0.9, 0x80FFFFFF, State.PendingWorldHopMapDef->Name, -1, TEXT_ALIGN_MIDDLECENTER, COMMON_DZO_DRAW_NORMAL);
  offY += 16;

  char strBuf[64];
  snprintf(strBuf, sizeof(strBuf), "%d", delaySeconds);
  gfxHelperDrawText(x, y, 0, offY, 0.8, 0x80FFFFFF, strBuf, -1, TEXT_ALIGN_MIDDLECENTER, COMMON_DZO_DRAW_NORMAL);
}

//--------------------------------------------------------------------------
void hopInit(void)
{
  netInstallCustomMsgHandler(CUSTOM_MSG_BEGIN_WORLD_HOP, &hopOnBeginRemote);
  netInstallCustomMsgHandler(CUSTOM_MSG_WORLD_HOP_MISSING_MAP, &hopOnClientMissingMapRemote);
}
