#include <libdl/net.h>
#include <libdl/string.h>
#include <libdl/stdio.h>
#include <libdl/pad.h>
#include <libdl/graphics.h>
#include <libdl/ui.h>
#include <libdl/stdlib.h>
#include <libdl/game.h>
#include "messageid.h"

#define BANNER_IMAGE_PTR                      (*(void**)0x000CFFA0)

int mapsGetInstallationResult(void);
extern int dlIsActive;

void bannerDraw(void)
{
  void* ptr = BANNER_IMAGE_PTR;
  if (!ptr) return;
  if (!isInMenus()) return;
  if (uiGetActive() != UI_ID_ONLINE_MAIN_MENU) return;

  u16 ulog = *(u16*)(ptr + 0);
  u16 vlog = *(u16*)(ptr + 2);
  if (isInMenus() && ulog && vlog) {
    int r1 = gfxLoadPalToGs(ptr + 0x010, 0x13);
    int r2 = gfxLoadTexToGs(ptr + 0x410, ulog, vlog, 0x13);
    u64 t = gfxConstructEffectTex(r2, r1, ulog, vlog, 0x13);
    
    gfxSetupGifPaging(0);
    gfxDrawSprite(207, 86, 279, 118, 0, 0, 1 << ulog, 1 << vlog, 0x40808080, t);
    gfxDoGifPaging();
  }
}

void bannerTick(void)
{
  static int initialized = 0;

  if (!isInMenus()) {
    if (BANNER_IMAGE_PTR) {
      free(BANNER_IMAGE_PTR);
      BANNER_IMAGE_PTR = NULL;
    }

    initialized = 0;
    return;
  } 
  
  void* connection = netGetLobbyServerConnection();
  int ui = uiGetActive();
  if (!initialized && ui != UI_ID_ONLINE_SELECT_PROFILE) {
    if (connection) {

      if (!BANNER_IMAGE_PTR) {
        BANNER_IMAGE_PTR = malloc(0x10 + 0x400 + 128 * 64);
        DPRINTF("banner: %08X\n", BANNER_IMAGE_PTR);
      }

      u32 payload = (u32)BANNER_IMAGE_PTR;
      if (payload && !dlIsActive && mapsGetInstallationResult()) {
        netSendCustomAppMessage(NET_DELIVERY_CRITICAL, connection, NET_LOBBY_CLIENT_INDEX, CUSTOM_MSG_ID_REQUEST_ANNOUNCEMENT_IMAGE, sizeof(payload), &payload);
        initialized = 1;
      }
    }
  } else if (!connection) {
    initialized = 0;
  }
}
