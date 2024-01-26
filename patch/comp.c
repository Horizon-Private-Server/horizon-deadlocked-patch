#include <libdl/graphics.h>
#include <libdl/game.h>
#include <libdl/time.h>
#include <libdl/net.h>
#include <libdl/ui.h>
#include <libdl/common.h>
#include <libdl/stdio.h>
#include <libdl/string.h>
#include <libdl/pad.h>
#include <libdl/utils.h>
#include "messageid.h"

#define SNACK_MAX_COUNT           (8)

#define COMP_TRACE_QUEUE_ID       (*(char*)0x000CFEF0)

const char * QUEUE_LEAVE_REASONS[] = {
  NULL,
  "Game has been cancelled because not all players joined."
};

const char * SELECT_QUEUE_TITLE = "SELECT QUEUE";
const char * QUEUE_NAMES[] = {
  "King of the Hill",
  "Capture the Flag",
  "FFA Deathmatch",
  "Cancel"
};
const int QUEUE_NAMES_SIZE = sizeof(QUEUE_NAMES)/sizeof(char*);

const char * SELECT_VOTE_TITLE = "VOTE";
const char * SELECT_VOTE_NAMES[] = {
  "Skip Map",
  "New Teams"
};
const int SELECT_VOTE_NAMES_SIZE = sizeof(SELECT_VOTE_NAMES)/sizeof(char*);

const char * QUEUE_SHORT_NAMES[] = {
  "KOTH",
  "CTF",
  "FFA DM"
};

enum COMP_ERROR
{
  COMP_ERROR_NONE = 0,
  COMP_ERROR_UNABLE_TO_JOIN_QUEUE = 1
};

struct CompState {
  int Initialized;
  int HasShownMapUpdatesRequiredPopup;
  int InQueue;
  int LastQueue;
  int LastSelectedQueue;
  int PlayersInQueue;
  int QueueStatus;
  int TimeLastGetMyQueue;
  int TimeStartedQueue;
  int HasJoinRequest;
  int HasJoinRequestTicks;
  int HasLeaveRequest;
  int HasLeaveRequestTicks;
  int TimeUntilGameStart;
  int TimeAllReady;
  ForceJoinGameRequest_t JoinRequest;
  ForceTeamsRequest_t TeamsRequest;
  enum COMP_ERROR ErrorId;
} CompState = {0};

typedef int (*uiVTable18_func)(void * ui, int pad);

uiVTable18_func chatRoom18Func = (uiVTable18_func)0x007280F0;
uiVTable18_func endGameScoreboard18Func = (uiVTable18_func)0x0073BA08;
uiVTable18_func staging18Func = (uiVTable18_func)0x00759220;

extern int initialized;
extern int mapsRemoteGlobalVersion;
extern int mapsLocalGlobalVersion;
extern int actionState;

int mapsDownloadingModules(void);
void forceStartGame(void);

typedef struct Snack
{
  int TicksLeft;
  char Message[64];
} Snack_t;

int snackCount = 0;
Snack_t snackStack[SNACK_MAX_COUNT];

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
#if DEBUG
  COMP_TRACE_QUEUE_ID = response.CurrentQueueId;
#endif
  CompState.InQueue = response.CurrentQueueId;
  CompState.TimeStartedQueue = gameGetTime() - (TIME_SECOND * response.SecondsInQueue);
  CompState.PlayersInQueue = response.PlayersInQueue;
  
	return sizeof(GetMyQueueResponse_t);
}

//------------------------------------------------------------------------------
int onForceTeamsRequest(void * connection, void * data)
{
	// move message payload into local
	memcpy(&CompState.TeamsRequest, data, sizeof(ForceTeamsRequest_t));

	return sizeof(ForceTeamsRequest_t);
}

//------------------------------------------------------------------------------
int onShowSnackMessageRequest(void * connection, void * data)
{
  ServerShowSnackMessage_t request;

	// move message payload into local
	memcpy(&request, data, sizeof(ServerShowSnackMessage_t));

  addSnack(request.Message);

	return sizeof(ServerShowSnackMessage_t);
}

//------------------------------------------------------------------------------
int onForceMapRequest(void * connection, void * data)
{
  ForceMapRequest_t request;

	// move message payload into local
	memcpy(&request, data, sizeof(ForceMapRequest_t));

  // set level
  CompState.JoinRequest.Level = (char)request.Level;

	return sizeof(ForceMapRequest_t);
}

//------------------------------------------------------------------------------
int onSetGameStartTimeRequest(void * connection, void * data)
{
  SetGameStartTimeRequest_t request;

	// move message payload into local
	memcpy(&request, data, sizeof(SetGameStartTimeRequest_t));

  if (request.SecondsUntilStart < 0)
    CompState.TimeUntilGameStart = -1;
  else
    CompState.TimeUntilGameStart = gameGetTime() + (TIME_SECOND * request.SecondsUntilStart);

	return sizeof(SetGameStartTimeRequest_t);
}

//------------------------------------------------------------------------------
int onForceJoinGameRequest(void * connection, void * data)
{
  ForceJoinGameRequest_t request;

	// move message payload into local
	memcpy(&request, data, sizeof(ForceJoinGameRequest_t));

  // only trigger join on changed world id
  // or if we've not already joined a game
  if (!CompState.HasJoinRequest || request.DmeWorldId != gameGetWorldId())
  {
    CompState.HasJoinRequest = 1;
    CompState.TimeUntilGameStart = -1;
  }

  memcpy(&CompState.JoinRequest, &request, sizeof(ForceJoinGameRequest_t));

	return sizeof(ForceJoinGameRequest_t);
}

//------------------------------------------------------------------------------
int onForceLeaveGameRequest(void * connection, void * data)
{
  ForceLeaveGameRequest_t request;

	// move message payload into local
	memcpy(&request, data, sizeof(ForceLeaveGameRequest_t));

  CompState.HasLeaveRequest = request.Reason + 1;

	return sizeof(ForceLeaveGameRequest_t);
}

//------------------------------------------------------------------------------
int onForceStartGameRequest(void * connection, void * data)
{
  forceStartGame();

	return 0;
}

//------------------------------------------------------------------------------
void removeSnackAt(int index)
{
  if (index >= SNACK_MAX_COUNT)
    return;

  memmove(&snackStack[index], &snackStack[index+1], sizeof(Snack_t) * (SNACK_MAX_COUNT - index - 1));
  memset(&snackStack[SNACK_MAX_COUNT - 1], 0, sizeof(Snack_t));
}

//------------------------------------------------------------------------------
void addSnack(char * message)
{
  int index = snackCount;
  if (index >= SNACK_MAX_COUNT)
  {
    removeSnackAt(0);
    index = SNACK_MAX_COUNT - 1;
  }
  else
  {
    snackCount++;
  }

  snackStack[index].TicksLeft = 60 * 5;
  strncpy(snackStack[index].Message, message, sizeof(snackStack[index].Message));
}

//------------------------------------------------------------------------------
void forceStartGame(void)
{
  GameSettings* gs = gameGetSettings();
  int clientId = gameGetMyClientId();
  int i;
  if (!gs || !gameAmIHost())
    return;

  // start
  gs->GameLoadStartTime = gameGetTime();
  for (i = 0; i < 10; ++i) {
    if (gs->PlayerClients[i] == clientId) {
      gs->PlayerStates[i] = 7;
      break;
    }
  }
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
    if (CompState.QueueStatus == 1) {
      sprintf(dst, "Found %s match... waiting for lobby...", QUEUE_SHORT_NAMES[CompState.InQueue-1]);
    }
    else if (minutesInQueue > 0) {
      sprintf(dst, "In %s Queue%s (%d players) (%dm %ds)", QUEUE_SHORT_NAMES[CompState.InQueue-1], ellipsesBuf, CompState.PlayersInQueue, minutesInQueue, secondsInQueue);
    } else {
      sprintf(dst, "In %s Queue%s (%d players) (%ds)", QUEUE_SHORT_NAMES[CompState.InQueue-1], ellipsesBuf, CompState.PlayersInQueue, secondsInQueue);
    }
  } else {
    dst[0] = 0;
  }
}

//------------------------------------------------------------------------------
int onLeaveStaging(void * ui, int a1)
{
  cancelCurrentQueue();
  return ((int (*)(void*, int))0x0075BA00)(ui, a1);
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
  if (CompState.LastSelectedQueue >= 0)
    selected = CompState.LastSelectedQueue;
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

    // save
    if (selected >= 0)
      CompState.LastSelectedQueue = selected;

    // send
    QueueBeginRequest_t request;
    request.QueueId = selected + 1;
    netSendCustomAppMessage(NET_DELIVERY_CRITICAL, connection, NET_LOBBY_CLIENT_INDEX, CUSTOM_MSG_ID_QUEUE_BEGIN_REQUEST, sizeof(QueueBeginRequest_t), &request);
  }
  
  return -1;
}

//------------------------------------------------------------------------------
int cancelCurrentQueue(void) {
  QueueBeginRequest_t request;
  void * connection = netGetLobbyServerConnection();

  // send queue request
  if (connection) {
  
    // send
    request.QueueId = 0;
    netSendCustomAppMessage(NET_DELIVERY_CRITICAL, connection, NET_LOBBY_CLIENT_INDEX, CUSTOM_MSG_ID_QUEUE_BEGIN_REQUEST, sizeof(QueueBeginRequest_t), &request);
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
int onGetReturnToMenuId(UiMenu_t * menu)
{
  int id = menu->ReturnToMenuId;
  if (id == UI_MENU_ID_ONLINE_LOBBY)
    return UI_MENU_ID_CLAN_ROOM;

  return id;
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
      if (!netDoIHaveNetError())
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

  // logout if maps aren't up to date
  if (mapsLocalGlobalVersion != mapsRemoteGlobalVersion && mapsRemoteGlobalVersion > 0) {
    ((void (*)(int))0x007647B0)(0x23);
    uiChangeMenu(UI_MENU_ID_ONLINE_PROFILE_SELECT);
    return -1;
  }

  return result;
}

//------------------------------------------------------------------------------
int onCompStaging(void * ui, int pad) {
  int i,j;
  GameSettings* gs = gameGetSettings();
  u32 * uiElements = (u32*)((u32)ui + 0xB0);
  int gameTime = gameGetTime();
  int canHostStart = gameAmIHost() && CompState.TimeAllReady > 0 && gameTime > (CompState.TimeAllReady + (TIME_SECOND * 3));

  // intercept pad
  int context = *(int*)((u32)ui + 0x230);
  if (context != 0x33) {
    // prevent host from starting game
    if (!canHostStart && pad == 6) {
      pad = 0;
    } else if (pad == 8) {
      // open vote menu
      int vote = uiShowSelectDialog(SELECT_VOTE_TITLE, SELECT_VOTE_NAMES, SELECT_VOTE_NAMES_SIZE, 0);
      switch (vote)
      {
        case 0: // skip map
        {
          // send request
          VoteRequest_t request;
          request.Context = VOTE_CONTEXT_GAME_SKIP_MAP;
          request.Vote = 1;
          netSendCustomAppMessage(NET_DELIVERY_CRITICAL, netGetLobbyServerConnection(), NET_LOBBY_CLIENT_INDEX, CUSTOM_MSG_ID_VOTE_REQUEST, sizeof(request), &request);
          break;
        }
        case 1: // new teams
        {
          // send request
          VoteRequest_t request;
          request.Context = VOTE_CONTEXT_GAME_NEW_TEAMS;
          request.Vote = 1;
          netSendCustomAppMessage(NET_DELIVERY_CRITICAL, netGetLobbyServerConnection(), NET_LOBBY_CLIENT_INDEX, CUSTOM_MSG_ID_VOTE_REQUEST, sizeof(request), &request);
          break;
        }
      }

      pad = 0;
    }
  }
  
  // call game function we're replacing
	int result = staging18Func(ui, pad);
  
  // rename INVITE to VOTE
  sprintf((char*)(uiElements[53] + 0x18), "\x13 VOTE");
  
  // rename START to STARTING
  if (CompState.TimeUntilGameStart > 0) {
    if (gs->GameLoadStartTime < 0) {

      // compute seconds left
      int secondsLeft = (CompState.TimeUntilGameStart - gameGetTime()) / TIME_SECOND;
      if (secondsLeft < 1)
        secondsLeft = 0;

      if (canHostStart) {
        sprintf((char*)(uiElements[55] + 0x18), "\x11 Start (%d)", secondsLeft);
      } else {
        sprintf((char*)(uiElements[55] + 0x18), "Starting... %d", secondsLeft);
      }
    }

    // show starting
    *(u32*)(uiElements[55] + 4) = 2;
  } else {
    // hide starting
    *(u32*)(uiElements[55] + 4) = 1;
  }
  
  // rename SELECT to empty
  *(u8*)(uiElements[54] + 0x18) = 0;

  if (gameAmIHost()) {
    
    int allReady = 1;
    for (i = 0; i < 10; ++i) {
      // force teams
      int accountId = CompState.TeamsRequest.AccountIds[i];
      for (j = 0; j < 10; ++j) {
        if (gs->PlayerAccountIds[j] == accountId) {
          gs->PlayerTeams[j] = CompState.TeamsRequest.Teams[i];
          gs->PlayerRanks[j] = CompState.TeamsRequest.BaseRanks[i];
          break;
        }
      }

      if (accountId > 0 && gs->PlayerStates[i] != 6 && gs->PlayerStates[i] != 7)
        allReady = 0;
    }

    if (!allReady || CompState.TimeUntilGameStart < 0) {
      CompState.TimeAllReady = -1;
    } else if (CompState.TimeAllReady < 0) {
      CompState.TimeAllReady = gameGetTime();
    }
  }

  return result;
}

//------------------------------------------------------------------------------
void drawSnacks(void) {


  int i;
  float y = SCREEN_HEIGHT - 10, x = SCREEN_WIDTH - 10;
  const float scale = 0.9;
  const float paddingx = 4;
  for (i = 0; i < SNACK_MAX_COUNT; ++i)
  {
    if (snackStack[i].TicksLeft == 1)
    {
      removeSnackAt(i);
      --i;
      continue;
    }
    else if (snackStack[i].TicksLeft)
    {
      --snackStack[i].TicksLeft;

      float textWidth = gfxGetFontWidth(snackStack[i].Message, -1, scale);
      float height = 20 * scale;
      gfxPixelSpaceBox(x - textWidth - (paddingx*2), y - height, textWidth + (paddingx*2), height + 2, 0x80000040);
      gfxScreenSpaceText(x - textWidth - paddingx, y - height + 1, scale, scale, 0x80FFFFFF, snackStack[i].Message, -1, 0);

      y -= height + 5;
    }
  }
}

//------------------------------------------------------------------------------
void runCompMenuLogic(void) {
  int i;
  if (initialized != 1 || mapsDownloadingModules())
    return;

  // display error message
  if (CompState.ErrorId != COMP_ERROR_NONE) {
    switch (CompState.ErrorId)
    {
      case COMP_ERROR_UNABLE_TO_JOIN_QUEUE:
      {
        addSnack("Unable to join queue.");
        break;
      }
    }

    CompState.ErrorId = COMP_ERROR_NONE;
  }
  
  if (CompState.HasLeaveRequest) {
    
    // game already started so ignore leave
    if (gameGetSettings() && gameGetSettings()->GameLoadStartTime > 0) {
      CompState.HasLeaveRequestTicks = 0;
      CompState.TimeLastGetMyQueue = 0;
      CompState.HasLeaveRequest = 0;
      CompState.HasJoinRequest = 0;
      cancelCurrentQueue();
    }

    // in staging, try and leave
    // or if already left
    else if (isInMenus() && 
      ((uiGetActive() == UI_ID_GAME_LOBBY && uiGetActivePointer() == uiGetPointer(UI_MENU_ID_STAGING)) || !gameGetSettings())) {
      
      // tell user
      char* reason = QUEUE_LEAVE_REASONS[CompState.HasLeaveRequest-1];
      if (reason)
        addSnack(reason);
      
      // try to leave
      if (gameGetSettings() && netGetDmeServerConnection()) {
        ((void (*)(void*, int))0x0075ba00)(uiGetActivePointer(), 0);
      }

      // reset state
      CompState.HasLeaveRequestTicks = 0;
      CompState.TimeLastGetMyQueue = 0;
      CompState.HasLeaveRequest = 0;
    }
    
    // can't leave.. so try and warn the user before kicking from queue
    else {
      CompState.HasLeaveRequestTicks++;

      if (CompState.HasLeaveRequestTicks == 60) {
        addSnack("please leave or you will be removed from the queue");
      }
      else if (CompState.HasLeaveRequestTicks > (60 * 3)) {
        cancelCurrentQueue();
      }
    }
  }

  if (!CompState.HasShownMapUpdatesRequiredPopup && !netDoIHaveNetError() && uiGetPointer(UI_MENU_ID_ONLINE_LOBBY) == uiGetActivePointer() && mapsLocalGlobalVersion != mapsRemoteGlobalVersion && mapsRemoteGlobalVersion > 0) {
    uiShowOkDialog("Custom Maps", "You must install the latest custom maps to play on the Comp Server.");
    CompState.HasShownMapUpdatesRequiredPopup = 1;
  }

  // if at main menu, put player in clan room
  static int ticksWantingClanRoom = 0;
  if (!netDoIHaveNetError() && uiGetPointer(UI_MENU_ID_ONLINE_LOBBY) == uiGetActivePointer() && netGetLobbyServerConnection() && uiGetActive() == UI_ID_ONLINE_MAIN_MENU)
  {
    ++ticksWantingClanRoom;
    if (ticksWantingClanRoom > 10) {
      uiChangeMenu(UI_MENU_ID_CLAN_ROOM);
      DPRINTF("%08X\n", uiGetActivePointer());
      ticksWantingClanRoom = 0;
    }
  }
  else
  {
    ticksWantingClanRoom = 0;
  }

  drawSnacks();
}

//------------------------------------------------------------------------------
void runCompLogic(void) {
  GameSettings* gs = gameGetSettings();
  void* connection = netGetLobbyServerConnection();
  if (!connection) {
    CompState.InQueue = 0;
    CompState.HasShownMapUpdatesRequiredPopup = 0;
    return;
  }

  if (initialized != 1 || mapsDownloadingModules())
    return;

  if (!CompState.Initialized) {
    memset(&CompState, 0, sizeof(CompState));
    CompState.Initialized = 1;
  }

  // reset join state
  if (!gameGetSettings() && CompState.HasJoinRequest > 2)
    CompState.HasJoinRequest = 0;
  if (!netGetLobbyServerConnection())
    CompState.HasJoinRequest = 0;

  // 
  netInstallCustomMsgHandler(CUSTOM_MSG_ID_QUEUE_BEGIN_RESPONSE, &onQueueBeginResponse);
  netInstallCustomMsgHandler(CUSTOM_MSG_ID_GET_MY_QUEUE_RESPONSE, &onGetMyQueueResponse);
  netInstallCustomMsgHandler(CUSTOM_MSG_ID_FORCE_JOIN_GAME_REQUEST, &onForceJoinGameRequest);
  netInstallCustomMsgHandler(CUSTOM_MSG_ID_FORCE_LEAVE_GAME_REQUEST, &onForceLeaveGameRequest);
  netInstallCustomMsgHandler(CUSTOM_MSG_ID_FORCE_TEAMS_REQUEST, &onForceTeamsRequest);
  netInstallCustomMsgHandler(CUSTOM_MSG_ID_FORCE_MAP_REQUEST, &onForceMapRequest);
  netInstallCustomMsgHandler(CUSTOM_MSG_ID_FORCE_START_GAME_REQUEST, &onForceStartGameRequest);
  netInstallCustomMsgHandler(CUSTOM_MSG_ID_SET_GAME_START_TIME_REQUEST, &onSetGameStartTimeRequest);
  netInstallCustomMsgHandler(CUSTOM_MSG_ID_SERVER_SHOW_SNACK_MESSAGE_REQUEST, &onShowSnackMessageRequest);
  
  // refresh queue every 5 seconds
  int gameTime = gameGetTime();
  if ((gameTime - CompState.TimeLastGetMyQueue) > (5 * TIME_SECOND)) {
    netSendCustomAppMessage(NET_DELIVERY_CRITICAL, connection, NET_LOBBY_CLIENT_INDEX, CUSTOM_MSG_ID_GET_MY_QUEUE_REQUEST, 0, NULL);
    CompState.TimeLastGetMyQueue = gameTime;
  }

  if (isInMenus()) {
    drawSnacks();
  }

  // hook menu stuff
  int menu = uiGetActive();
  if (isInMenus() && (menu == UI_ID_ONLINE_MAIN_MENU || menu == 0x15c)) {
    *(u32*)0x004BB6C8 = &onCompChatRoom;
    *(u32*)0x004BD1B8 = &onCompEndGameScoreboard;
    *(u32*)0x004BFA70 = &onCompStaging;
    *(u32*)0x00728620 = 0x0C000000 | ((u32)&onLogoutChatRoom / 4);
    *(u32*)0x0072862C = 0xAFA00148;
    *(u32*)0x0075A150 = 0x10000153; // disable add buddy/ignored/kick in staging 
    *(u32*)0x00763DC0 = 0x24020003; // disable changing team in staging
    *(u32*)0x0075a7ec = 0x0C000000 | ((u32)&onLeaveStaging / 4);
    *(u32*)0x00759448 = 0; // disable game cancelled popup when host leaves
    HOOK_J(0x0071C168, &onGetReturnToMenuId); // force return to last menu never returns to main menu
    //POKE_U16(0x0072A004, (short)UI_MENU_ID_CLAN_ROOM); // change leave game return to menu to clan room

    // change clan room channel name to "Default"
    strncpy((char*)0x00220A80, "Default", 7);
  }

  // force number of locals to 1
  *(u32*)0x00172174 = 1;

#if DEBUG
  if (padGetButtonDown(0, PAD_L3 | PAD_R3) != 0)
  {
    uiChangeMenu(UI_MENU_ID_CLAN_ROOM);
  }
  else if (padGetButtonDown(0, PAD_UP | PAD_L1) != 0)
  {
    uiChangeMenu(UI_MENU_ID_STATS);
  }
#endif

  if (CompState.HasJoinRequest == 1) {

    DPRINTF("%08X\n", uiGetActivePointer());
    if (uiGetActivePointer() == uiGetPointer(UI_MENU_ID_CLAN_ROOM) || uiGetActivePointer() == uiGetPointer(UI_MENU_ID_STATS) || uiGetActivePointer() == uiGetPointer(22))
    {
      // join game
      CompState.HasJoinRequest = 2;
      CompState.HasJoinRequestTicks = 0;

      // write config to game setup
      memcpy((void*)0x001737e8, CompState.JoinRequest.GameFlags, sizeof(CompState.JoinRequest.GameFlags));
      *(u32*)0x00173824 = CompState.JoinRequest.WeaponFlags;

      // join
      *(u8*)0x00173A50 = CompState.JoinRequest.AmIHost;
      int r = ((int (*)(int, int, char*, int))0x0070BBE0)(CompState.JoinRequest.ChannelWorldId, CompState.JoinRequest.GameWorldId, NULL, 0);
      DPRINTF("join: %d\n", r);
    }
    else
    {
      CompState.HasJoinRequestTicks++;

      if (CompState.HasJoinRequestTicks == 60) {
        addSnack("please return to the correct menu to join..");
      }
      else if (CompState.HasJoinRequestTicks > (60 * 3)) {
        addSnack("unable to join..");
        cancelCurrentQueue();
        CompState.HasJoinRequest = 0;
        CompState.HasJoinRequestTicks = 0;
      }
    }
  } else if (CompState.HasJoinRequest == 2) {

    // await join game
    *(u8*)0x00173A50 = CompState.JoinRequest.AmIHost;
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

