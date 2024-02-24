#include <libdl/game.h>
#include <libdl/gamesettings.h>
#include <libdl/stdio.h>
#include <libdl/string.h>
#include <libdl/team.h>
#include <libdl/ui.h>
#include <libdl/hud.h>
#include <libdl/player.h>
#include <libdl/pad.h>
#include <libdl/time.h>
#include <libdl/graphics.h>
#include <libdl/net.h>
#include <libdl/utils.h>
#include <libdl/random.h>
#include "messageid.h"
#include "config.h"
#include "include/config.h"

#define QUICK_CHAT_MSG_ID_START     (50)
#define QUICK_CHAT_MAX_MSGS         (5)
#define QUICK_CHAT_SHOW_TICKS       (60 * 5)
#define MENU_OPEN_TICKS             (60 * 3)
#define NAME_MAX_WIDTH              (60)

struct QuickChatMsg
{
  char FromPlayerId;
  char ChatId;
};

enum QuickChatIds
{
  // POST GAME
  QUICK_CHAT_GG,
  QUICK_CHAT_FF,
  QUICK_CHAT_REMATCH,
  QUICK_CHAT_ONE_MORE_GAME,
  // REACTIONS
  QUICK_CHAT_WHAT_A_SAVE,
  QUICK_CHAT_WOW,
  QUICK_CHAT_CLOSE_ONE,
  QUICK_CHAT_NICE_SHOT,
  // STATUS
  QUICK_CHAT_PUSHING,
  QUICK_CHAT_DEFENDING,
  QUICK_CHAT_IN_POSITION,
  QUICK_CHAT_HAVE_V2,
  // COMMANDS
  QUICK_CHAT_GET_THE_FLAG,
  QUICK_CHAT_PUSH_UP,
  QUICK_CHAT_STAY_BACK,
  QUICK_CHAT_HELP_ME,

  // COUNT
  QUICK_CHAT_COUNT
};

extern PatchGameConfig_t gameConfig;

char* CHAT_GROUPS[4] = {
  "Post Game",
  "Reactions",
  "Status",
  "Commands"
};

char* CHAT_SHORTNAMES[QUICK_CHAT_COUNT] = {
  "gg",
  "ff",
  "rm",
  "1+",
  "save",
  "wow",
  "close",
  "shot",
  "push",
  "def",
  "pos",
  "v2",
  "flag",
  "push",
  "back",
  "help"
};

char* CHAT_MESSAGES[QUICK_CHAT_COUNT] = {
  "gg",
  "ff",
  "rematch!",
  "one. more. game.",
  "what a save!",
  "wow!",
  "close one!",
  "nice shot!",
  "pushing!",
  "defending!",
  "in position!",
  "v2 acquired!",
  "get the flag!",
  "push up!",
  "stay back!",
  "help me!"
};

int CHAT_GRID_X_OFF[4] = {
  0,
  26,
  0,
  -26
};

int CHAT_GRID_Y_OFF[4] = {
  -26,
  0,
  26,
  0
};

int PAD_INDICES[4] = {
  PAD_UP,
  PAD_RIGHT,
  PAD_DOWN,
  PAD_LEFT
};

int QuickChatCount = 0;
int QuickChatShowTicks = 0;
struct QuickChatMsg QuickChats[QUICK_CHAT_MAX_MSGS];

//--------------------------------------------------------------------------
char * quickChatGetMsgString(int fragMsgId)
{
  u32* msgIds = (u32*)0x00398218;
  if (fragMsgId < QUICK_CHAT_MSG_ID_START) return uiMsgString(msgIds[fragMsgId]);

  switch (fragMsgId - QUICK_CHAT_MSG_ID_START)
  {
    case QUICK_CHAT_GG: return "gg";
    case QUICK_CHAT_FF: return "ff";
    case QUICK_CHAT_REMATCH: return "rematch!";
    case QUICK_CHAT_ONE_MORE_GAME: return "one. more. game.";
    case QUICK_CHAT_WHAT_A_SAVE: return "what a save!";
    case QUICK_CHAT_WOW: return "wow!";
    case QUICK_CHAT_CLOSE_ONE: return "close one!";
    case QUICK_CHAT_NICE_SHOT: return "nice shot!";
    case QUICK_CHAT_PUSHING: return "pushing!";
    case QUICK_CHAT_DEFENDING: return "defending!";
    case QUICK_CHAT_IN_POSITION: return "in position!";
    case QUICK_CHAT_HAVE_V2: return "v2 acquired!";
    case QUICK_CHAT_GET_THE_FLAG: return "get the flag!";
    case QUICK_CHAT_PUSH_UP: return "push up!";
    case QUICK_CHAT_STAY_BACK: return "stay back!";
    case QUICK_CHAT_HELP_ME: return "help me!";
  }
}

//--------------------------------------------------------------------------
void quickChatDraw(void)
{
  GameSettings* gs = gameGetSettings();
  Player** players = playerGetAll();
  if (!gs) return;

  int yOff = 100;
  if (playerGetNumLocals() > 1) yOff = 10;

  char buf[64];
  int i;
  for (i = 0; i < QuickChatCount; ++i) {

    int pid = QuickChats[i].FromPlayerId;
    int chatid = QuickChats[i].ChatId;

    // draw full or truncated name
    int nameWidth = gfxGetFontWidth(gs->PlayerNames[pid], -1, 0.8);
    if (nameWidth > NAME_MAX_WIDTH)
      snprintf(buf, sizeof(buf), "%.6s. %s", gs->PlayerNames[pid], CHAT_MESSAGES[chatid]);
    else
      snprintf(buf, sizeof(buf), "%s %s", gs->PlayerNames[pid], CHAT_MESSAGES[chatid]);

    // draw team *
    int width = gfxScreenSpaceText(490, yOff + (i * 14), 0.8, 0.8, 0x80FFFFFF, buf, -1, 2);
    if (chatid > 3) {
      int team = gs->PlayerTeams[pid];
      Player* player = players[pid];
      if (player) team = player->Team;
      gfxScreenSpaceText(490 + (490 - width) - 4, yOff + (i * 14), 1, 1, hudGetTeamColor(team, 1), "*", -1, 2);
    }

    // pass to dzo
    if (CLIENT_TYPE_DZO == PATCH_INTEROP->Client) {
      CustomDzoCommandDrawQuickChat_t cmd;
      cmd.FromPlayerId = pid;
      cmd.ChatId = chatid;
      PATCH_DZO_INTEROP_FUNCS->SendCustomCommandToClient(CUSTOM_DZO_CMD_ID_DRAW_QUICK_CHAT, sizeof(cmd), &cmd);
    }
  }
}

//--------------------------------------------------------------------------
void quickChatPush(int sourcePlayerId, int chatId)
{
  //((void (*)(int, int, int))0x00621AE0)(sourcePlayerId, -1, chatId + QUICK_CHAT_MSG_ID_START);
  QuickChatShowTicks = QUICK_CHAT_SHOW_TICKS;

  if (QuickChatCount == QUICK_CHAT_MAX_MSGS) {

    // move down
    memmove(QuickChats, &QuickChats[1], sizeof(struct QuickChatMsg) * (QUICK_CHAT_MAX_MSGS - 1));

    QuickChats[QuickChatCount-1].FromPlayerId = sourcePlayerId;
    QuickChats[QuickChatCount-1].ChatId = chatId;
  } else {
    QuickChats[QuickChatCount].FromPlayerId = sourcePlayerId;
    QuickChats[QuickChatCount].ChatId = chatId;
    QuickChatCount++;
  }

}

//--------------------------------------------------------------------------
int quickChatOnReceiveRemoteQuickChat(void* connection, void* data)
{
  if (!isInGame()) return sizeof(struct QuickChatMsg);

  struct QuickChatMsg msg;
  memcpy(&msg, data, sizeof(msg));

  // if chat msg is post game, then it is global so always show
  // otherwise it is team, make sure local player(s) are on same team as remote
  if (msg.ChatId >= 4) {
    Player* fromPlayer = playerGetAll()[msg.FromPlayerId];
    if (!fromPlayer) return;

    int i;
    for (i = 0; i < GAME_MAX_LOCALS; ++i) {
      Player* localPlayer = playerGetFromSlot(i);
      if (!localPlayer || !localPlayer->PlayerMoby) continue;
      if (localPlayer->Team == fromPlayer->Team) goto push;
    }

    return;
  }

  push:;
  quickChatPush(msg.FromPlayerId, msg.ChatId);
  return sizeof(struct QuickChatMsg);
}

//--------------------------------------------------------------------------
void quickChatBroadcast(int playerId, int chatId)
{
  if (!isInGame()) return;

  struct QuickChatMsg msg;
  msg.FromPlayerId = playerId;
  msg.ChatId = chatId;

  // local
  quickChatPush(msg.FromPlayerId, msg.ChatId);

  // broadcast
  void* connection = netGetDmeServerConnection();
  if (connection) {
    netBroadcastCustomAppMessage(NET_DELIVERY_CRITICAL, connection, CUSTOM_MSG_PLAYER_QUICK_CHAT, sizeof(msg), &msg);
  }
}

//--------------------------------------------------------------------------
void quickChatRun(void)
{
  static int ticksMenuOpen[GAME_MAX_LOCALS];
  static int quickChatMenu[GAME_MAX_LOCALS];
  static int init = 0;

  // save quick chat menu for p0
  PATCH_POINTERS_QUICKCHAT = quickChatMenu[0];

  if (!isInGame()) {
    if (init) {
      memset(quickChatMenu, 0, sizeof(quickChatMenu));
      memset(ticksMenuOpen, 0, sizeof(ticksMenuOpen));
      memset(QuickChats, 0, sizeof(QuickChats));
      init = 0;
    }
    return;
  }
  if (!gameConfig.grQuickChat) return;
  if (gameConfig.customModeId == CUSTOM_MODE_SURVIVAL) return;
  if (gameConfig.customModeId == CUSTOM_MODE_TRAINING) return;
  if (gameGetSettings()->GameRules == GAMERULE_CQ && gameConfig.grCqDisableUpgrades == 0) return;
  if (isConfigMenuActive) return;
  
  int localCount = playerGetNumLocals();
  int splitScreen = localCount > 1;

  //
  netInstallCustomMsgHandler(CUSTOM_MSG_PLAYER_QUICK_CHAT, &quickChatOnReceiveRemoteQuickChat);

  // allow frag msg with -1 player
  //POKE_U32(0x0061FA44, 0);

  // hook frag msg get string from id
  //HOOK_JAL(0x0061fa74, &quickChatGetMsgString);
  //POKE_U32(0x0061fa78, 0x8E440000);

  // draw quick chats
  if (QuickChatShowTicks) {
    quickChatDraw();
  }

  int l,i;
  for (l = 0; l < GAME_MAX_LOCALS; ++l) {

    int menuId = quickChatMenu[l];
    Player* player = playerGetFromSlot(l);
    if (!player || !player->PlayerMoby) continue;
    if (gameIsStartMenuOpen(l)) continue;

    // draw
    PlayerHUDFlags* hudFlags = hudGetPlayerFlags(l);
    if (menuId) {

      // enable cq select menu
      hudFlags->Flags.ConquestUpgradeSelect = 1;

      // print group
      char * group = CHAT_GROUPS[menuId-1];
      gfxScreenSpaceText(60, 300 * (splitScreen ? 0.5 : 1.0), 1, 1, 0x80FFFFFF, group, -1, 4);

      // print options
      for (i = 0; i < 4; ++i) {
        char * name = CHAT_SHORTNAMES[(menuId-1)*4 + i];
        gfxScreenSpaceText(60 + CHAT_GRID_X_OFF[i], (352 + CHAT_GRID_Y_OFF[i]) * (splitScreen ? 0.5 : 1.0), 0.7, 0.75, 0x80202020, name, -1, 4);
      }

    } else {
      hudFlags->Flags.ConquestUpgradeSelect = 0;
    }

    // input
    int pad = -1;
    for (i = 0; i < 4; ++i) {
      if (padGetAnyButtonDown(l, PAD_INDICES[i]) > 0) {
        pad = i;
        break;
      }
    }

    if (pad >= 0) {

      // send chat or open menu
      if (menuId) {
        quickChatBroadcast(player->PlayerId, (menuId - 1)*4 + pad);
        quickChatMenu[l] = 0;
      } else {
        quickChatMenu[l] = pad + 1;
        ticksMenuOpen[l] = MENU_OPEN_TICKS;
      }
    }
    
    // ticks
    if (ticksMenuOpen[l]) {
      ticksMenuOpen[l]--;
      if (!ticksMenuOpen[l]) quickChatMenu[l] = 0;
    }
  }

  if (QuickChatShowTicks) --QuickChatShowTicks;
  init = 1;
}
