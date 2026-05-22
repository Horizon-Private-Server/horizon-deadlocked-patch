#include <libdl/net.h>
#include <libdl/string.h>
#include "include/game.h"

struct CgmStats Stats = {};

//--------------------------------------------------------------------------
void statsFormatValue(char* dest, int size, int value, int format)
{
  if (!dest) return;
  if (size <= 0) return;

  switch (format)
  {
    case CGM_SCORE_STAT_TYPE_INT:
      {
        uiPrintCommaNumber(dest, size, value, 0);
        return;
      }
    case CGM_SCORE_STAT_TYPE_TIME_SECONDS:
      {
        snprintf(dest, size, "%02d:%02d", value / 60, value % 60);
        return;
      }
    case CGM_SCORE_STAT_TYPE_TIME_MILLISECONDS:
      {
        int sec = value / 1000;
        snprintf(dest, size, "%02d:%02d", sec / 60, sec % 60);
        return;
      }
    case CGM_SCORE_STAT_TYPE_FLOAT:
      {
        snprintf(dest, size, "%.2f", value / 1024.0);
        return;
      }
  }
}

//--------------------------------------------------------------------------
int onRecvGameStats(void *connection, void *data)
{
  memcpy(&Stats, data, sizeof(Stats));
  return sizeof(struct CgmStats);
}

//--------------------------------------------------------------------------
void statsBroadcastSync(void)
{
  void *connection = netGetDmeServerConnection();
  if (!connection) return;

  netBroadcastCustomAppMessage(NET_DELIVERY_CRITICAL, connection, CGM_MSG_ID_SEND_GAME_STATS, sizeof(Stats), &Stats);
}

//--------------------------------------------------------------------------
void statsInit(void)
{
  netInstallCustomMsgHandler(CGM_MSG_ID_SEND_GAME_STATS, &onRecvGameStats);
}
