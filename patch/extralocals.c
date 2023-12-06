#include <libdl/game.h>
#include <libdl/gamesettings.h>
#include <libdl/stdio.h>
#include <libdl/string.h>
#include <libdl/team.h>
#include <libdl/ui.h>
#include <libdl/pad.h>
#include <libdl/utils.h>
#include "messageid.h"
#include "config.h"

extern PatchGameConfig_t gameConfig;

//--------------------------------------------------------------------------
void extraLocalsEnableNewPlayerSyncIfClientHasExtraLocals(void)
{
  static int hasCheckedThisGame = 0;
  GameSettings* gs = gameGetSettings();
  if (!gs || !isInGame()) { hasCheckedThisGame = 0; return; }
  if (gameConfig.grNewPlayerSync) return;
  if (hasCheckedThisGame) return;

  int i;
  int clientCount[GAME_MAX_PLAYERS];
  memset(clientCount, 0, sizeof(clientCount));

  hasCheckedThisGame = 1;
  for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
    int clientId = gs->PlayerClients[i];
    if (clientId < 0) continue;

    int clientLocalIdx = clientCount[clientId]++;
    if (clientLocalIdx > 1) {
      gameConfig.grNewPlayerSync = 1;
      DPRINTF("Extra locals detected. Forcing new player sync on\n");
      return;
    }
  }
}

//--------------------------------------------------------------------------
void* extraLocalsGetPlayerPad(void * ui, int a1, int a2)
{
  //void* ptr = ((void* (*)(void*, int, int))0x0075B3A8)(ui, a1, a2);
  //return ptr;

  int i;
  struct PAD** pads = (struct PAD**)0x0021ddd8;
  struct PAD* localPads = 0x001EE600;

  if (a1 == 0) {
    if (pads[0] == 0) pads[0] = &localPads[0];
    return pads[0];
  } else {

    // only support up to 4 locals
    if (a1 > 3) return 0;
  
    // get previously assigned
    if (pads[a1]) {

      // check still initialized
      if (!pads[a1]->initialized) {
        // remove
        ((void (*)(void*, int))0x0075c8f8)(ui, a2);
        pads[a1] = 0;
      }

      if (pads[a1]) return pads[a1];
    }

    // find next available
    for (i = 1; i < 8; ++i) {
      if (localPads[i].initialized) {
        struct PAD* localPad = &localPads[i];
        if (pads[1] == localPad || pads[2] == localPad || pads[3] == localPad) continue;

        if (localPad->hudBitsOn & 0x6F) {
          return pads[a1] = localPad;
        }
      }
    }
  }

  return NULL;
}

//--------------------------------------------------------------------------
void extraLocalsUpdateGameSettings(void)
{
  GameSettings* gs = gameGetSettings();
  if (!gs) return;

  int i;
  int clientCount[GAME_MAX_PLAYERS];
  int clientPrimaryIdx[GAME_MAX_PLAYERS];
  memset(clientCount, 0, sizeof(clientCount));
  memset(clientPrimaryIdx, 0, sizeof(clientPrimaryIdx));
  
  for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
    int clientIdx = gs->PlayerClients[i];
    if (clientIdx < 0) continue;

    int clientLocalIdx = clientCount[clientIdx]++;
    if (clientLocalIdx > 1) {
      if (gs->PlayerSkins[i] < 0 || gs->PlayerSkins[i] > 21) gs->PlayerSkins[i] = 0;
      gs->PlayerAccountIds[i] = gs->PlayerAccountIds[clientPrimaryIdx[clientIdx]];
      gs->PlayerRanks[i] = gs->PlayerRanks[clientPrimaryIdx[clientIdx]];
      gs->PlayerRankDeviations[i] = gs->PlayerRankDeviations[clientPrimaryIdx[clientIdx]];

      // name is either "parent ~ #" or "parent~#" if the parent has a long name
      int parentNameLen = strlen(gs->PlayerNames[clientPrimaryIdx[clientIdx]]);
      if (parentNameLen > 13) {
        snprintf(gs->PlayerNames[i], 16, "%.12s~%d", gs->PlayerNames[clientPrimaryIdx[clientIdx]], clientLocalIdx + 1);
      } else {
        snprintf(gs->PlayerNames[i], 16, "%s ~ %d", gs->PlayerNames[clientPrimaryIdx[clientIdx]], clientLocalIdx + 1);
      }

    } else if (clientLocalIdx == 0) {
      clientPrimaryIdx[clientIdx] = i;
    }
  }
  
}

//--------------------------------------------------------------------------
int extraLocalsOnNWClientJoinMessageCallbackExit(void)
{
  extraLocalsUpdateGameSettings();
  return 0x58;
}

//--------------------------------------------------------------------------
int extraLocalsOnNwClientJoinResponseCallbackExit(void)
{
  extraLocalsUpdateGameSettings();
  return 0xF8;
}

//--------------------------------------------------------------------------
void extraLocalsRun(void)
{
  extraLocalsEnableNewPlayerSyncIfClientHasExtraLocals();
  if (!isInMenus()) return;

  // handles which pad is assigned to which player
  // vanilla ignores P3 and P4
  HOOK_JAL(0x0075B56C, &extraLocalsGetPlayerPad);

  // when joining make sure player settings are correct
  HOOK_J(0x0016077c, &extraLocalsOnNWClientJoinMessageCallbackExit);
  HOOK_J(0x00160868, &extraLocalsOnNwClientJoinResponseCallbackExit);

  POKE_U16(0x0015ab50, 3 + 2); // increase netobject count by 2
  POKE_U32(0x0075B598, 0); // enable staging pad input for player 3,4
  POKE_U16(0x0075C3CA, 0x7FA0); // initialize controller settings buffer to 0 for 1-4

  // set max number of locals to 4
  UiMenu_t * ptr = uiGetPointer(UI_MENU_ID_ONLINE_LOBBY);
  if (ptr) {
    void * localCountUiElem = ptr->Children[9];
    if (localCountUiElem) {
      POKE_U32((u32)localCountUiElem + 0x6C, 4);
    }
  }
}
