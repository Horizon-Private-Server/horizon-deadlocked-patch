#include <libdl/game.h>
#include <libdl/gamesettings.h>
#include <libdl/stdio.h>
#include <libdl/string.h>
#include <libdl/team.h>
#include <libdl/ui.h>
#include <libdl/player.h>
#include <libdl/pad.h>
#include <libdl/time.h>
#include <libdl/graphics.h>
#include <libdl/net.h>
#include <libdl/utils.h>
#include "messageid.h"
#include "config.h"
#include "common.h"
#include "include/config.h"

#define LATENCY_PING_COOLDOWN_TICKS         (60 * 1)
#define USE_CLIENT_PING                     1

extern PatchConfig_t config;
extern PatchGameConfig_t gameConfig;

struct LatencyPing
{
  int FromClientIdx;
  int ToClientIdx;
  long Ticks;
};

struct ClientPing
{
  int FromClientIdx;
  int Ping;
};

int ClientLatency[GAME_MAX_PLAYERS];

//--------------------------------------------------------------------------
int igScoreboardOnRemoteLatencyPing(void* connection, void* data)
{
  struct LatencyPing msg;
  memcpy(&msg, data, sizeof(msg));

  if (msg.FromClientIdx == gameGetMyClientId()) {
    long dticks = timerGetSystemTime() - msg.Ticks;
    int dms = dticks / SYSTEM_TIME_TICKS_PER_MS;
    ClientLatency[msg.ToClientIdx] = dms;
    //DPRINTF("RTT to CLIENT %d : %dms\n", msg.ToClientIdx, dms);
  } else {
    // send back
    netSendCustomAppMessage(NET_DELIVERY_CRITICAL, connection, msg.FromClientIdx, CUSTOM_MSG_PLAYER_LATENCY_TEST_PING, sizeof(msg), &msg);
  }

  return sizeof(struct LatencyPing);
}

//--------------------------------------------------------------------------
void igScoreboardSendLatencyPings(void)
{
  struct LatencyPing msg;
  msg.FromClientIdx = gameGetMyClientId();
  msg.Ticks = timerGetSystemTime();

  // ensure we have a connection to the dme server
  void * connection = netGetDmeServerConnection();
  if (!connection) return;

  // send to each client
  int i;
  Player** players = playerGetAll();
  for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
    Player* player = players[i];
    if (!player || !player->pNetPlayer || player->IsLocal) continue;

    msg.ToClientIdx = player->pNetPlayer->netClientIndex;
    netSendCustomAppMessage(NET_DELIVERY_CRITICAL, connection, msg.ToClientIdx, CUSTOM_MSG_PLAYER_LATENCY_TEST_PING, sizeof(msg), &msg);
  }
}

//--------------------------------------------------------------------------
int igScoreboardOnRemoteClientPing(void* connection, void* data)
{
  struct ClientPing msg;
  memcpy(&msg, data, sizeof(msg));

  ClientLatency[msg.FromClientIdx] = msg.Ping;
  return sizeof(struct ClientPing);
}

//--------------------------------------------------------------------------
void igScoreboardBroadcastClientPing(void)
{
  struct ClientPing msg;
  msg.FromClientIdx = gameGetMyClientId();
  msg.Ping = gameGetPing();

  // save local ping
  ClientLatency[msg.FromClientIdx] = msg.Ping;

  // ensure we have a connection to the dme server
  void * connection = netGetDmeServerConnection();
  if (!connection) return;

  netBroadcastCustomAppMessage(0, connection, CUSTOM_MSG_PLAYER_LATENCY_TEST_PING, sizeof(struct ClientPing), &msg);
}

//--------------------------------------------------------------------------
void igScoreboardDraw(void)
{
  char buf[32];
  GameSettings* gs = gameGetSettings();
  if (!isInGame() || !gs) return;

  float anchorX = 0.5 * SCREEN_WIDTH;
  float anchorY = 0.3 * SCREEN_HEIGHT;
  float width = 0.65 * SCREEN_WIDTH;
  float height = 0.08 * SCREEN_HEIGHT;
  float rowHeight = 0.04 * SCREEN_HEIGHT;
  float aY = anchorY + height/2;
  const float offsets[] = { -160, -50, 45, 140 };

  // bg
  //gfxScreenSpaceBox(0.15, 0.3, 0.65, 0.08, 0x20000000);
  gfxHelperDrawBox(anchorX, anchorY, 0, 0, width, height, 0x20000000, TEXT_ALIGN_TOPCENTER, COMMON_DZO_DRAW_NORMAL);

  // columns
  gfxHelperDrawText(anchorX, aY, offsets[0], 0, 1, 0x80FFFFFF, "Player", -1, TEXT_ALIGN_MIDDLELEFT, COMMON_DZO_DRAW_NORMAL);
  gfxHelperDrawText(anchorX, aY, offsets[1], 0, 1, 0x80FFFFFF, "Kills", -1, TEXT_ALIGN_MIDDLECENTER, COMMON_DZO_DRAW_NORMAL);
  gfxHelperDrawText(anchorX, aY, offsets[2], 0, 1, 0x80FFFFFF, "Deaths", -1, TEXT_ALIGN_MIDDLECENTER, COMMON_DZO_DRAW_NORMAL);
#if USE_CLIENT_PING
  gfxHelperDrawText(anchorX, aY, offsets[3], 0, 1, 0x80FFFFFF, "Ping", -1, TEXT_ALIGN_MIDDLECENTER, COMMON_DZO_DRAW_NORMAL);
#else
  gfxHelperDrawText(anchorX, aY, offsets[3], 0, 1, 0x80FFFFFF, "RTT", -1, TEXT_ALIGN_MIDDLECENTER, COMMON_DZO_DRAW_NORMAL);
#endif

  // rows
  int i;
  float y = 0.38 * SCREEN_HEIGHT;
  Player** players = playerGetAll();
  GameData* gdata = gameGetData();
  for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
    if (gs->PlayerNames[i][0] == 0) continue;

    float yOff = (SCREEN_HEIGHT/2) - y - rowHeight - 1;
    aY = anchorY + rowHeight/2;

    int team = gs->PlayerTeams[i];
    if (players[i]) team = players[i]->Team;
    gfxHelperDrawBox(anchorX, aY, 0, yOff, width, rowHeight, TEAM_COLORS[team], TEXT_ALIGN_MIDDLECENTER, COMMON_DZO_DRAW_NORMAL);
    gfxHelperDrawText(anchorX, aY, offsets[0], yOff, 1, 0x80FFFFFF, gs->PlayerNames[i], -1, TEXT_ALIGN_MIDDLELEFT, COMMON_DZO_DRAW_NORMAL);

    snprintf(buf, sizeof(buf), "%d", gdata->PlayerStats.Kills[i]);
    gfxHelperDrawText(anchorX, aY, offsets[1], yOff, 1, 0x80FFFFFF, buf, -1, TEXT_ALIGN_MIDDLECENTER, COMMON_DZO_DRAW_NORMAL);
    snprintf(buf, sizeof(buf), "%d", gdata->PlayerStats.Deaths[i]);
    gfxHelperDrawText(anchorX, aY, offsets[2], yOff, 1, 0x80FFFFFF, buf, -1, TEXT_ALIGN_MIDDLECENTER, COMMON_DZO_DRAW_NORMAL);
    snprintf(buf, sizeof(buf), "%dms", ClientLatency[gs->PlayerClients[i]]);
    gfxHelperDrawText(anchorX, aY, offsets[3], yOff, 1, 0x80FFFFFF, buf, -1, TEXT_ALIGN_MIDDLECENTER, COMMON_DZO_DRAW_NORMAL);

    y -= rowHeight;
  }
}

//--------------------------------------------------------------------------
void igScoreboardRun(void)
{
  static int reset = 0;
  static int pingCooldown = 0;

#if USE_CLIENT_PING
  netInstallCustomMsgHandler(CUSTOM_MSG_PLAYER_LATENCY_TEST_PING, &igScoreboardOnRemoteClientPing);
#else
  netInstallCustomMsgHandler(CUSTOM_MSG_PLAYER_LATENCY_TEST_PING, &igScoreboardOnRemoteLatencyPing);
#endif

  if (!isInGame()) {
    PATCH_POINTERS_SCOREBOARD = 0;
    if (!reset) {
      memset(ClientLatency, 0, sizeof(ClientLatency));
      pingCooldown = 0;
      reset = 1;
    }
    return;
  }

  // draw
  PATCH_POINTERS_SCOREBOARD = padGetButton(0, PAD_L3) > 0;
  if (config.enableInGameScoreboard && PATCH_POINTERS_SCOREBOARD && !isConfigMenuActive && !gameIsStartMenuOpen(0)) {
    igScoreboardDraw();
  }

  // send pings
  if (pingCooldown) {
    --pingCooldown;
  } else {
#if USE_CLIENT_PING
    igScoreboardBroadcastClientPing();
#else
    igScoreboardSendLatencyPings();
#endif
    pingCooldown = LATENCY_PING_COOLDOWN_TICKS;
  }

  reset = 0;
}
