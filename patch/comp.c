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
const int QUEUE_NAMES_SIZE = sizeof(QUEUE_NAMES)/sizeof(char*);

const char * SELECT_VOTE_TITLE = "VOTE";
const char * SELECT_VOTE_NAMES[] = {
  "Skip Map"
};
const int SELECT_VOTE_NAMES_SIZE = sizeof(SELECT_VOTE_NAMES)/sizeof(char*);

const char * QUEUE_SHORT_NAMES[] = {
  "KOTH",
  "CTF"
};

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

void forceStartGame(void);


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
int onForceTeamsRequest(void * connection, void * data)
{
	// move message payload into local
	memcpy(&CompState.TeamsRequest, data, sizeof(ForceTeamsRequest_t));

	return sizeof(ForceTeamsRequest_t);
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
	// move message payload into local
	memcpy(&CompState.JoinRequest, data, sizeof(ForceJoinGameRequest_t));
  CompState.HasJoinRequest = 1;
  CompState.TimeUntilGameStart = -1;

	return sizeof(ForceJoinGameRequest_t);
}

//------------------------------------------------------------------------------
int onForceStartGameRequest(void * connection, void * data)
{
  forceStartGame();

	return 0;
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
int onCompStaging(void * ui, int pad) {
  int i,j;
  GameSettings* gs = gameGetSettings();
  u32 * uiElements = (u32*)((u32)ui + 0xB0);
  int gameTime = gameGetTime();
  int canHostStart = CompState.TimeAllReady > 0 && gameTime > (CompState.TimeAllReady + (TIME_SECOND * 3));

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
      char prefix = canHostStart ? '\x11' : ' ';
      int secondsLeft = (CompState.TimeUntilGameStart - gameGetTime()) / TIME_SECOND;
      if (secondsLeft < 1)
        secondsLeft = 1;
      sprintf((char*)(uiElements[55] + 0x18), "%cStarting...%d", prefix, secondsLeft);
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
    
    int numReady = 0;
    for (i = 0; i < 10; ++i) {
      // force teams
      int accountId = CompState.TeamsRequest.AccountIds[i];
      for (j = 0; j < 10; ++j) {
        if (gs->PlayerAccountIds[j] == accountId) {
          gs->PlayerTeams[j] = CompState.TeamsRequest.Teams[i];
          break;
        }
      }

      
      if (gs->PlayerStates[i] == 6)
        numReady++;
    }

    if (numReady != CompState.JoinRequest.PlayerCount) {
      CompState.TimeAllReady = -1;
    } else if (CompState.TimeAllReady < 0) {
      CompState.TimeAllReady = gameGetTime();
    }
  }

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
  GameSettings* gs = gameGetSettings();
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
  netInstallCustomMsgHandler(CUSTOM_MSG_ID_FORCE_TEAMS_REQUEST, &onForceTeamsRequest);
  netInstallCustomMsgHandler(CUSTOM_MSG_ID_FORCE_MAP_REQUEST, &onForceMapRequest);
  netInstallCustomMsgHandler(CUSTOM_MSG_ID_FORCE_START_GAME_REQUEST, &onForceStartGameRequest);
  netInstallCustomMsgHandler(CUSTOM_MSG_ID_SET_GAME_START_TIME_REQUEST, &onSetGameStartTimeRequest);
  


  // refresh queue every 5 seconds
  int gameTime = gameGetTime();
  if ((gameTime - CompState.TimeLastGetMyQueue) > (5 * TIME_SECOND)) {
    netSendCustomAppMessage(NET_DELIVERY_CRITICAL, connection, NET_LOBBY_CLIENT_INDEX, CUSTOM_MSG_ID_GET_MY_QUEUE_REQUEST, 0, NULL);
    CompState.TimeLastGetMyQueue = gameTime;
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

    // write config to game setup
    memcpy((void*)0x001737e8, CompState.JoinRequest.GameFlags, sizeof(CompState.JoinRequest.GameFlags));
    *(u32*)0x00173824 = CompState.JoinRequest.WeaponFlags;

    // join
    *(u8*)0x00173A50 = CompState.JoinRequest.AmIHost;
    ((void (*)(int, int, char*, int))0x0070BBE0)(CompState.JoinRequest.ChannelWorldId, CompState.JoinRequest.GameWorldId, NULL, 0);
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

