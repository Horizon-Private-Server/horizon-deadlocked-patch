#include "include/menu.h"

//--------------------------------------------------------------------------
void menuDrawFrame(Window_t* window, char* title, float titleHeight)
{
  // labels fit within the fixed settings menu bounds
  windowDrawBox(window, TEXT_ALIGN_TOPLEFT, 0, 0, window->Width, window->Height, RAIDS_MENU_BACKGROUND_COLOR, TEXT_ALIGN_TOPLEFT);
  windowDrawText(window, TEXT_ALIGN_TOPLEFT, window->Width * 0.5, titleHeight * 0.5, 1.1, RAIDS_MENU_TEXT_COLOR, title, -1, TEXT_ALIGN_MIDDLECENTER);
  windowDrawBox(window, TEXT_ALIGN_TOPLEFT, RAIDS_MENU_PADDING, titleHeight - 1, window->Width - 2*RAIDS_MENU_PADDING, 1, RAIDS_MENU_TRACK_COLOR, TEXT_ALIGN_TOPLEFT);

  // draw the four fixed border strips without the general border helper
  int i;
  for (i = 0; i < 2; ++i) {
    windowDrawBox(window, TEXT_ALIGN_TOPLEFT, i * (window->Width - 1), 0, 1, window->Height, RAIDS_MENU_BORDER_COLOR, TEXT_ALIGN_TOPLEFT);
    windowDrawBox(window, TEXT_ALIGN_TOPLEFT, 0, i * (window->Height - SCREEN_RATIO_INV), window->Width, SCREEN_RATIO_INV, RAIDS_MENU_BORDER_COLOR, TEXT_ALIGN_TOPLEFT);
  }
}

//--------------------------------------------------------------------------
void menuDrawFooter(Window_t* window, char* buttons)
{
  windowDrawBox(window, TEXT_ALIGN_TOPLEFT, 0, 0, window->Width, window->Height, RAIDS_MENU_FOOTER_COLOR, TEXT_ALIGN_TOPLEFT);
  windowDrawText(window, TEXT_ALIGN_TOPLEFT, RAIDS_MENU_PADDING, window->Height * 0.5,
    RAIDS_MENU_FOOTER_TEXT_SCALE, RAIDS_MENU_TEXT_COLOR, buttons, -1, TEXT_ALIGN_MIDDLELEFT);
}
