#include <libdl/graphics.h>
#include <libdl/game.h>
#include <libdl/time.h>
#include <libdl/net.h>
#include <libdl/ui.h>
#include <libdl/common.h>
#include <libdl/stdio.h>
#include <libdl/string.h>
#include <libdl/pad.h>
#include "messageid.h"

const char * SELECT_QUEUE_TITLE = "SELECT QUEUE";
const char * QUEUE_NAMES[] = {
  "King of the Hill",
  "Capture the Flag",
  "Cancel"
};
const char * QUEUE_SHORT_NAMES[] = {
  "KOTH",
  "CTF"
};
const int QUEUE_NAMES_SIZE = sizeof(QUEUE_NAMES)/sizeof(char*);

enum COMP_ERROR
{
  COMP_ERROR_NONE = 0,
  COMP_ERROR_UNABLE_TO_JOIN_QUEUE = 1
};

struct CompState {
  int Initialized;
  int InQueue;
  int LastQueue;
  int PlayersInQueue;
  int TimeLastGetMyQueue;
  int TimeStartedQueue;
  int HasJoinRequest;
  ForceJoinGameRequest_t JoinRequest;
  enum COMP_ERROR ErrorId;
} CompState = {0};

typedef int (*uiVTable18_func)(void * ui, void * a1);

uiVTable18_func chatRoom18Func = (uiVTable18_func)0x007280F0;
uiVTable18_func endGameScoreboard18Func = (uiVTable18_func)0x0073BA08;

//------------------------------------------------------------------------------
int onQueueBeginResponse(void * connection, void * data)
{
  QueueBeginResponse_t response;
  
	// move message payload into local
	memcpy(&response, data, sizeof(QueueBeginResponse_t));

  // save last queue
  if (response.CurrentQueueId != CompState.InQueue && CompState.InQueue > 0)
    CompState.LastQueue = CompState.InQueue;

  // 
  CompState.InQueue = response.CurrentQueueId;
  CompState.PlayersInQueue = response.PlayersInQueue;
  
  if (response.Status >= 0 && CompState.InQueue) {
    CompState.TimeStartedQueue = gameGetTime() - (TIME_SECOND * response.SecondsInQueue);
  } else if (response.Status < 0) {
    CompState.ErrorId = COMP_ERROR_UNABLE_TO_JOIN_QUEUE;
  }

	return sizeof(QueueBeginResponse_t);
}

//------------------------------------------------------------------------------
int onGetMyQueueResponse(void * connection, void * data)
{
  GetMyQueueResponse_t response;
  
	// move message payload into local
	memcpy(&response, data, sizeof(GetMyQueueResponse_t));

  // save last queue
  if (response.CurrentQueueId != CompState.InQueue && CompState.InQueue > 0)
    CompState.LastQueue = CompState.InQueue;

  // 
  CompState.InQueue = response.CurrentQueueId;
  CompState.TimeStartedQueue = gameGetTime() - (TIME_SECOND * response.SecondsInQueue);
  CompState.PlayersInQueue = response.PlayersInQueue;
  
  
	return sizeof(GetMyQueueResponse_t);
}

//------------------------------------------------------------------------------
int onForceJoinGameRequest(void * connection, void * data)
{
	// move message payload into local
	memcpy(&CompState.JoinRequest, data, sizeof(ForceJoinGameRequest_t));
  CompState.HasJoinRequest = 1;

  printf("host:%d level:%d rule:%d size:%d\n", CompState.JoinRequest.AmIHost, CompState.JoinRequest.Level, CompState.JoinRequest.Ruleset, sizeof(ForceJoinGameRequest_t));

	return sizeof(ForceJoinGameRequest_t);
}

//------------------------------------------------------------------------------
void generateQueueText(char * dst)
{
  static int ellipseCount = 1;
  char ellipsesBuf[] = { 0, 0, 0, 0 };
  int i;

  // display in queue text when queueing
  if (CompState.InQueue > 0) {
    float totalSecondsInQueue = (gameGetTime() - CompState.TimeStartedQueue) / (float)TIME_SECOND;
    int minutesInQueue = (int)(totalSecondsInQueue / 60);
    int secondsInQueue = (int)totalSecondsInQueue % 60;

    // loop ellipse count
    ellipseCount = (secondsInQueue % 3) + 1;

    // create ellipses
    for (i = 0; i < 3; ++i)
      ellipsesBuf[i] = (i < ellipseCount) ? '.' : ' ';

    // create display text
    if (minutesInQueue > 0) {
      sprintf(dst, "In %s Queue%s (%d players) (%dm %ds)", QUEUE_SHORT_NAMES[CompState.InQueue-1], ellipsesBuf, CompState.PlayersInQueue, minutesInQueue, secondsInQueue);
    } else {
      sprintf(dst, "In %s Queue%s (%d players) (%ds)", QUEUE_SHORT_NAMES[CompState.InQueue-1], ellipsesBuf, CompState.PlayersInQueue, secondsInQueue);
    }
  } else {
    dst[0] = 0;
  }
}

//------------------------------------------------------------------------------
int onLogoutChatRoom(void * ui, int a1)
{
  char * title = uiMsgString(0xf64);
  char * body = uiMsgString(0x1031);
  int response = uiShowYesNoDialog(title, body);
  if (response == 1) {
    // logout
    ((void (*)(int))0x007647B0)(0x23);

    uiChangeMenu(UI_MENU_ID_ONLINE_PROFILE_SELECT);
  }
   
  return -1;
}

//------------------------------------------------------------------------------
void onJoinGame()
{
  if (!CompState.JoinRequest.AmIHost || !gameAmIHost())
    return;
  

}

//------------------------------------------------------------------------------
int openQueueSelect(void) {
  int selected = 0, count = QUEUE_NAMES_SIZE - 1;
  if (CompState.LastQueue > 0)
    selected = CompState.LastQueue - 1;
  if (CompState.InQueue > 0) {
    selected = count;
    count++;
  }

  selected = uiShowSelectDialog(SELECT_QUEUE_TITLE, QUEUE_NAMES, count, selected);
  void * connection = netGetLobbyServerConnection();

  // send queue request
  if (connection && selected >= 0) {
  
    // if cancel is selected then change to -1
    if (selected == (QUEUE_NAMES_SIZE - 1)) {
      selected = -1;
    }

    // send
    QueueBeginRequest_t request;
    request.QueueId = selected + 1;
    netSendCustomAppMessage(connection, NET_LOBBY_CLIENT_INDEX, CUSTOM_MSG_ID_QUEUE_BEGIN_REQUEST, sizeof(QueueBeginRequest_t), &request);
  }
  
  return -1;
}

//------------------------------------------------------------------------------
int openLogoutDialog(void) {
  char * title = uiMsgString(0xf64);
  char * body = uiMsgString(0x1031);
  int response = uiShowYesNoDialog(title, body);
  if (response == 1) {
    // logout
    ((void (*)(int))0x007647B0)(0x23);

    uiChangeMenu(UI_MENU_ID_ONLINE_PROFILE_SELECT);
    return 1;
  }

  return 0;
}

//------------------------------------------------------------------------------
int onCompEndGameScoreboard(void * ui, int pad) {
  u32 * uiElements = (u32*)((u32)ui + 0xB0);
  
  // call game function we're replacing
	int result = endGameScoreboard18Func(ui, pad);

  // hide play again/join
  *(u32*)(uiElements[6] + 4) = 1;

  return result;
}

//------------------------------------------------------------------------------
int onCompChatRoom(void * ui, int pad) {
  u32 * uiElements = (u32*)((u32)ui + 0xB0);

  // handle queue and stats pad input
  // if this is 10, then we're in the keyboard
  // and ignore pad
  int context = *(int*)((u32)ui + 0x230);
  if (context != 10) {
    if (pad == 8) {
      // open stats
      uiChangeMenu(UI_MENU_ID_STATS);

    } else if (pad == 6) {
      // open queue
      openQueueSelect();

    } else if (context == 8 && pad == 7) {
      // prevent selecting user
      pad = 0;
    }
  }

  // call game function we're replacing
	int result = chatRoom18Func(ui, pad);

  // rename clan room to CIRCLE QUEUE
  sprintf((char*)(uiElements[11] + 0x60), "\x11 QUEUE");

  // rename select to stats
  sprintf((char*)(uiElements[12] + 0x60), "\x13 STATS");

  // rename back to logout
  sprintf((char*)(uiElements[13] + 0x60), "\x12 LOGOUT");

  // rename clan name to queue info
  generateQueueText((char*)(uiElements[2] + 0x60));

  // show select/stats at all times
  *(u32*)(uiElements[12] + 4) = 2;

  return result;
}

//------------------------------------------------------------------------------
void runCompMenuLogic(void) {
  
  // display error message
  if (CompState.ErrorId != COMP_ERROR_NONE) {
    switch (CompState.ErrorId)
    {
      case COMP_ERROR_UNABLE_TO_JOIN_QUEUE:
      {
        uiShowOkDialog("Queue", "Unable to join queue.");
        break;
      }
    }

    CompState.ErrorId = COMP_ERROR_NONE;
  }

  // if at main menu, put player in clan room
  if (uiGetActive() == UI_ID_ONLINE_MAIN_MENU)
  {
    uiChangeMenu(UI_MENU_ID_CLAN_ROOM);
  }
}

//------------------------------------------------------------------------------
void runCompLogic(void) {
  void* connection = netGetLobbyServerConnection();
  if (!connection) {
    CompState.InQueue = 0;
    return;
  }

  if (!CompState.Initialized) {
    memset(&CompState, 0, sizeof(CompState));
    CompState.Initialized = 1;
  }

  // 
  netInstallCustomMsgHandler(CUSTOM_MSG_ID_QUEUE_BEGIN_RESPONSE, &onQueueBeginResponse);
  netInstallCustomMsgHandler(CUSTOM_MSG_ID_GET_MY_QUEUE_RESPONSE, &onGetMyQueueResponse);
  netInstallCustomMsgHandler(CUSTOM_MSG_ID_FORCE_JOIN_GAME_REQUEST, &onForceJoinGameRequest);

  // refresh queue every 5 seconds
  int gameTime = gameGetTime();
  if ((gameTime - CompState.TimeLastGetMyQueue) > (5 * TIME_SECOND)) {
    netSendCustomAppMessage(connection, NET_LOBBY_CLIENT_INDEX, CUSTOM_MSG_ID_GET_MY_QUEUE_REQUEST, 0, NULL);
    CompState.TimeLastGetMyQueue = gameTime;
  }

  // hook menu stuff
  int menu = uiGetActive();
  if (!gameIsIn() && (menu == UI_ID_ONLINE_MAIN_MENU || menu == 0x15c)) {
    *(u32*)0x004BB6C8 = &onCompChatRoom;
    *(u32*)0x004BD1B8 = &onCompEndGameScoreboard;
    *(u32*)0x00728620 = 0x0C000000 | ((u32)&onLogoutChatRoom / 4);
    *(u32*)0x0072862C = 0xAFA00148;
    
    // change clan room channel name to "Default"
    strncpy((char*)0x00220A80, "Default", 7);
  }

  // force number of locals to 1
  *(u32*)0x00172174 = 1;

  if (padGetButtonDown(0, PAD_L3 | PAD_R3) != 0)
  {
    uiChangeMenu(UI_MENU_ID_CLAN_ROOM);
  }
  else if (padGetButtonDown(0, PAD_UP | PAD_L1) != 0)
  {
    uiChangeMenu(UI_MENU_ID_STATS);
  }

  if (CompState.HasJoinRequest == 1) {
    // join game
    CompState.HasJoinRequest = 2;

    // join
    *(u8*)0x00173A50 = CompState.JoinRequest.AmIHost;
    ((void (*)(int, int, char*, int))0x0070BBE0)(CompState.JoinRequest.ChannelWorldId, CompState.JoinRequest.GameWorldId, NULL, 0);
  } else if (CompState.HasJoinRequest == 2) {

    // await join game
    GameSettings* gs = gameGetSettings();
    GameOptions* gOpts = gameGetOptions();
    if (gs && gOpts) {

      gs->GameLevel = CompState.JoinRequest.Level;
      gs->GameRules = CompState.JoinRequest.Ruleset;

      memcpy(gOpts->GameFlags.Raw, CompState.JoinRequest.GameFlags, sizeof(CompState.JoinRequest.GameFlags));
      gOpts->WeaponFlags.Raw = CompState.JoinRequest.WeaponFlags;

      // started
      if (gs->GameLoadStartTime > 0) {
        CompState.HasJoinRequest = 3;
      }
    }
  }
}

