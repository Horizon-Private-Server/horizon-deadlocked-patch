#ifndef RAIDS_INVENTORYMENU_H
#define RAIDS_INVENTORYMENU_H

#include <tamtypes.h>
#include <libdl/moby.h>
#include <libdl/math.h>
#include <libdl/time.h>
#include <libdl/player.h>
#include <libdl/math3d.h>
#include "inventory.h"

#define INVENTORYMENU_TAB_COUNT                         (WEAPON_SLOT_COUNT)
#define INVENTORYMENU_TAB_SPRITE_PADDING                (8)

#define INVENTORYMENU_NOTIFY_SPRITE_ID                  (88) // star
#define INVENTORYMENU_NOTIFY_COLOR                      (0x80FF4000)

#define INVENTORYMENU_DRAW_CENTER_X                     (SCREEN_WIDTH / 2.0)
#define INVENTORYMENU_DRAW_CENTER_Y                     (SCREEN_HEIGHT / 2.0)
#define INVENTORYMENU_DRAW_FULL_W                       (SCREEN_WIDTH - 100)
#define INVENTORYMENU_DRAW_FULL_H                       (SCREEN_HEIGHT - 170)
#define INVENTORYMENU_DRAW_FRAME_BORDER_W               (2)

#define INVENTORYMENU_DRAW_WEAPONS_DIM                  (8)
#define INVENTORYMENU_DRAW_WEAPONS_M                    (4)
#define INVENTORYMENU_DRAW_WEAPONS_W                    (INVENTORYMENU_DRAW_FULL_W / 2.0)
#define INVENTORYMENU_DRAW_WEAPONS_H                    (INVENTORYMENU_DRAW_WEAPONS_W)

#define INVENTORYMENU_DRAW_INFO_W                       (INVENTORYMENU_DRAW_FULL_W - (INVENTORYMENU_DRAW_WEAPONS_W))
#define INVENTORYMENU_DRAW_INFO_H                       (INVENTORYMENU_DRAW_INFO_W)

typedef struct InventoryMenuDrawState
{
  int SelectedIdx;
  int FilterIdx;
} InventoryMenuDrawState_t;

void inventoryMenuOpen(void);
void inventoryMenuClose(void);
void inventoryMenuSetFilter(int filter);
void inventoryMenuTick(void);
void inventoryMenuInit(void);

#endif // RAIDS_INVENTORYMENU_H
