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
#include "include/config.h"

#define LATENCY_PING_COOLDOWN_TICKS         (60 * 5)

extern PatchGameConfig_t gameConfig;

struct LatencyPing
{
  int FromClientIdx;
  int ToClientIdx;
  long Ticks;
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
    DPRINTF("RTT to CLIENT %d : %dms\n", msg.ToClientIdx, dms);
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
void igScoreboardDraw(void)
{
  char buf[32];
  GameSettings* gs = gameGetSettings();
  if (!isInGame() || !gs) return;

  // bg
  gfxScreenSpaceBox(0.15, 0.3, 0.65, 0.08, 0x20000000);

  // columns
  gfxScreenSpaceText(0.17 * SCREEN_WIDTH, 0.32 * SCREEN_HEIGHT, 1, 1, 0x80FFFFFF, "Player", -1, 0);
  gfxScreenSpaceText(0.40 * SCREEN_WIDTH, 0.32 * SCREEN_HEIGHT, 1, 1, 0x80FFFFFF, "Kills", -1, 1);
  gfxScreenSpaceText(0.57 * SCREEN_WIDTH, 0.32 * SCREEN_HEIGHT, 1, 1, 0x80FFFFFF, "Deaths", -1, 1);
  gfxScreenSpaceText(0.74 * SCREEN_WIDTH, 0.32 * SCREEN_HEIGHT, 1, 1, 0x80FFFFFF, "RTT", -1, 1);

  // rows
  int i;
  float y = 0.38 * SCREEN_HEIGHT;
  Player** players = playerGetAll();
  GameData* gdata = gameGetData();
  for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
    if (gs->PlayerNames[i][0] == 0) continue;

    int team = gs->PlayerTeams[i];
    if (players[i]) team = players[i]->Team;
    gfxScreenSpaceBox(0.15, y / SCREEN_HEIGHT, 0.65, 0.04, TEAM_COLORS[team]);
    gfxScreenSpaceText(0.17 * SCREEN_WIDTH, y, 1, 1, 0x80FFFFFF, gs->PlayerNames[i], -1, 0);
    snprintf(buf, sizeof(buf), "%d", gdata->PlayerStats.Kills[i]); gfxScreenSpaceText(0.40 * SCREEN_WIDTH, y, 1, 1, 0x80FFFFFF, buf, -1, 1);
    snprintf(buf, sizeof(buf), "%d", gdata->PlayerStats.Deaths[i]); gfxScreenSpaceText(0.57 * SCREEN_WIDTH, y, 1, 1, 0x80FFFFFF, buf, -1, 1);
    snprintf(buf, sizeof(buf), "%dms", ClientLatency[gs->PlayerClients[i]]); gfxScreenSpaceText(0.74 * SCREEN_WIDTH, y, 1, 1, 0x80FFFFFF, buf, -1, 1);

    y += 0.04 * SCREEN_HEIGHT;
  }
}

//--------------------------------------------------------------------------
void igScoreboardRun(void)
{
  static int reset = 0;
  static int pingCooldown = 0;

  netInstallCustomMsgHandler(CUSTOM_MSG_PLAYER_LATENCY_TEST_PING, &igScoreboardOnRemoteLatencyPing);
  
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
  if (padGetButtonDown(0, PAD_L3) > 0) PATCH_POINTERS_SCOREBOARD = !PATCH_POINTERS_SCOREBOARD;
  if (PATCH_POINTERS_SCOREBOARD && !isConfigMenuActive && !gameIsStartMenuOpen(0)) {
    igScoreboardDraw();
  }

  // send pings
  if (pingCooldown) {
    --pingCooldown;
  } else {
    igScoreboardSendLatencyPings();
    pingCooldown = LATENCY_PING_COOLDOWN_TICKS;
  }

  reset = 0;
}
