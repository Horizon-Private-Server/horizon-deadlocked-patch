#ifndef RAIDS_MENU_H
#define RAIDS_MENU_H

#include "window.h"

#define RAIDS_MENU_BACKGROUND_COLOR     (0x70000000)
#define RAIDS_MENU_PANEL_COLOR          (0x78101010)
#define RAIDS_MENU_BORDER_COLOR         (0x80000020)
#define RAIDS_MENU_FOOTER_COLOR         (0x80000000)
#define RAIDS_MENU_TEXT_COLOR           (0x80FFFFFF)
#define RAIDS_MENU_SECONDARY_COLOR      (0x80B0B0B0)
#define RAIDS_MENU_TRACK_COLOR          (0x80303030)

#define RAIDS_MENU_PADDING              (8)
#define RAIDS_MENU_TITLE_HEIGHT         (24)
#define RAIDS_MENU_FOOTER_HEIGHT        (16)
#define RAIDS_MENU_FOOTER_TEXT_SCALE    (0.7)

void menuDrawFrame(Window_t* window, char* title, float titleHeight);
void menuDrawFooter(Window_t* window, char* buttons);

#endif // RAIDS_MENU_H
