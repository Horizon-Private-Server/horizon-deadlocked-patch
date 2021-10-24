#include <libdl/pad.h>
#include <libdl/ui.h>
#include <libdl/graphics.h>
#include <libdl/stdio.h>
#include <libdl/net.h>
#include <libdl/color.h>
#include <libdl/gamesettings.h>
#include <libdl/string.h>
#include <libdl/game.h>
#include <libdl/map.h>
#include "messageid.h"
#include "config.h"
#include "include/config.h"

#define LINE_HEIGHT         (0.05)
#define LINE_HEIGHT_3_2     (0.075)


// config
extern PatchConfig_t config;

// game config
extern PatchGameConfig_t gameConfig;
extern PatchGameConfig_t gameConfigHostBackup;

// 
int isConfigMenuActive = 0;
int selectedTabItem = 0;
u32 padPointer = 0;

// constants
const char footerText[] = "\x14 \x15 TAB     \x10 SELECT     \x12 BACK";

// menu display properties
const u32 colorBlack = 0x80000000;
const u32 colorBg = 0x80404040;
const u32 colorContentBg = 0x80202020;
const u32 colorTabBg = 0x80404040;
const u32 colorTabBarBg = 0x80101010;
const u32 colorRed = 0x80000040;
const u32 colorSelected = 0x80606060;
const u32 colorButtonBg = 0x80303030;
const u32 colorButtonFg = 0x80505050;
const u32 colorText = 0x80FFFFFF;
const u32 colorOpenBg = 0x20000000;

const float frameX = 0.1;
const float frameY = 0.15;
const float frameW = 0.8;
const float frameH = 0.7;
const float frameTitleH = 0.075;
const float frameFooterH = 0.05;
const float contentPaddingX = 0.01;
const float contentPaddingY = 0;
const float tabBarH = 0.075;
const float tabBarPaddingX = 0.005;

//
void configMenuDisable(void);
void configMenuEnable(void);

// action handlers
void buttonActionHandler(TabElem_t* tab, MenuElem_t* element, int actionType, void * actionArg);
void toggleActionHandler(TabElem_t* tab, MenuElem_t* element, int actionType, void * actionArg);
void listActionHandler(TabElem_t* tab, MenuElem_t* element, int actionType, void * actionArg);

// state handlers
void menuStateAlwaysHiddenHandler(TabElem_t* tab, MenuElem_t* element, int* state);
void menuStateAlwaysDisabledHandler(TabElem_t* tab, MenuElem_t* element, int* state);
void menuStateAlwaysEnabledHandler(TabElem_t* tab, MenuElem_t* element, int* state);
void menuStateHandler_InstallCustomMaps(TabElem_t* tab, MenuElem_t* element, int* state);
void menuStateHandler_GameModeOverride(TabElem_t* tab, MenuElem_t* element, int* state);

void tabDefaultStateHandler(TabElem_t* tab, int * state);
void tabGameSettingsStateHandler(TabElem_t* tab, int * state);

// list select handlers
void mapsSelectHandler(TabElem_t* tab, MenuElem_t* element);
void gmResetSelectHandler(TabElem_t* tab, MenuElem_t* element);
#ifdef DEBUG
void downloadPatchSelectHandler(TabElem_t* tab, MenuElem_t* element);
#endif

void tabDefaultStateHandler(TabElem_t* tab, int * state);
void tabGameSettingsStateHandler(TabElem_t* tab, int * state);

void navMenu(TabElem_t* tab, int direction, int loop);
void navTab(int direction);

int mapsHasTriedLoading(void);
int mapsPromptEnableCustomMaps(void);
int mapsDownloadingModules(void);

// level of detail list item
MenuElem_ListData_t dataLevelOfDetail = {
    &config.levelOfDetail,
    2,
    { "Low", "Normal", "High" }
};

// general tab menu items
MenuElem_t menuElementsGeneral[] = {
  { "Enable custom maps", buttonActionHandler, menuStateHandler_InstallCustomMaps, mapsSelectHandler },
#ifdef DEBUG
  { "Redownload patch", buttonActionHandler, menuStateAlwaysEnabledHandler, downloadPatchSelectHandler },
#endif
  { "Disable framelimiter", toggleActionHandler, menuStateAlwaysEnabledHandler, &config.disableFramelimiter },
  { "Announcers on all gamemodes", toggleActionHandler, menuStateAlwaysEnabledHandler, &config.enableGamemodeAnnouncements },
  { "Spectate mode", toggleActionHandler, menuStateAlwaysEnabledHandler, &config.enableSpectate },
  { "Singleplayer music", toggleActionHandler, menuStateAlwaysHiddenHandler, &config.enableSingleplayerMusic },
  { "Level of Detail", listActionHandler, menuStateAlwaysEnabledHandler, &dataLevelOfDetail },
  { "Sync player state", toggleActionHandler, menuStateAlwaysEnabledHandler, &config.enablePlayerStateSync },
  { "Progressive Scan", toggleActionHandler, menuStateAlwaysEnabledHandler, (char*)0x0021DE6C },
};

// map override list item
MenuElem_ListData_t dataCustomMaps = {
    &gameConfig.customMapId,
    18,
    {
      "None",
      "Sarathos SP",
      "Desert Prison",
      "Torval SP",
      "Mining Facility SP",
      "Dark Cathedral Interior",
      "Shaar SP",
      "Hoven Gorge",
      "Launch Site",
      "Metropolis MP",
      "Annihilation Nation",
      "Blackwater Docks",
      "Bakisi Isles",
      "Tyhrranosis",
      "Ghost Ship",
      "Duck Hunt",
      "Spleef",
      "Battledome SP"
    }
};

// maps with their own exclusive gamemode
char dataCustomMapsWithExclusiveGameMode[] = {
  15,
  16
};
const int dataCustomMapsWithExclusiveGameModeCount = sizeof(dataCustomMapsWithExclusiveGameMode)/sizeof(char);

// gamemode override list item
MenuElem_ListData_t dataCustomModes = {
    &gameConfig.customModeId,
    6,
    {
      "None",
      "Infected",
      "Gun Game",
      "Infinite Climber",
      "Hoverbike Race",
      "Search and Destroy"
    }
};

// 
const char* CustomModeShortNames[] = {
  NULL,
  NULL,
  NULL,
  "Climber",
  "Race",
  "SND"
};

// weather override list item
MenuElem_ListData_t dataWeather = {
    &gameConfig.grWeatherId,
    17,
    {
      "None",
      "Random",
      "Dust Storm",
      "Heavy Sand Storm",
      "Light Snow",
      "Blizzard",
      "Heavy Rain",
      "All Off",
      "Green Mist",
      "Meteor Lightning",
      "Black Hole",
      "Light Rain Lightning",
      "Settling Smoke",
      "Upper Atmosphere",
      "Ghost Station",
      "Embossed",
      "Lightning Storm",
    }
};

// vampire list item
MenuElem_ListData_t dataVampire = {
    &gameConfig.grVampire,
    4,
    {
      "Off",
      "Quarter Heal",
      "Half Heal",
      "Full Heal",
    }
};

// game settings tab menu items
MenuElem_t menuElementsGameSettings[] = {
  { "Reset", buttonActionHandler, menuStateAlwaysEnabledHandler, gmResetSelectHandler },
  { "Map override", listActionHandler, menuStateAlwaysEnabledHandler, &dataCustomMaps },
  { "Gamemode override", listActionHandler, menuStateHandler_GameModeOverride, &dataCustomModes },
  { "Weather override", listActionHandler, menuStateAlwaysEnabledHandler, &dataWeather },
  { "Disable weapon packs", toggleActionHandler, menuStateAlwaysEnabledHandler, &gameConfig.grNoPacks },
  { "Disable v2s", toggleActionHandler, menuStateAlwaysEnabledHandler, &gameConfig.grNoV2s },
  { "Disable health boxes", toggleActionHandler, menuStateAlwaysEnabledHandler, &gameConfig.grNoHealthBoxes },
  { "Mirror World", toggleActionHandler, menuStateAlwaysEnabledHandler, &gameConfig.grMirrorWorld },
  { "Vampire", listActionHandler, menuStateAlwaysEnabledHandler, &dataVampire },
  { "Half time", toggleActionHandler, menuStateAlwaysEnabledHandler, &gameConfig.grHalfTime },
  { "Better hills", toggleActionHandler, menuStateAlwaysEnabledHandler, &gameConfig.grBetterHills },
  { "Healthbars", toggleActionHandler, menuStateAlwaysEnabledHandler, &gameConfig.grHealthBars }
};

// tab items
TabElem_t tabElements[] = {
  { "General", tabDefaultStateHandler, menuElementsGeneral, sizeof(menuElementsGeneral)/sizeof(MenuElem_t) },
  { "Game Settings", tabGameSettingsStateHandler, menuElementsGameSettings, sizeof(menuElementsGameSettings)/sizeof(MenuElem_t) }
};

const int tabsCount = sizeof(tabElements)/sizeof(TabElem_t);


// 
void tabDefaultStateHandler(TabElem_t* tab, int * state)
{
  *state = ELEMENT_ENABLED;
}

// 
void tabGameSettingsStateHandler(TabElem_t* tab, int * state)
{
  GameSettings * gameSettings = gameGetSettings();
  if (!gameSettings)
  {
    *state = ELEMENT_HIDDEN;
  }
  // if game has started or not the host, disable
  else if (gameSettings->GameLoadStartTime > 0 || *(u8*)0x00172170 != 0)
  {
    *state = ELEMENT_DISABLED;
  }
  else
  {
    *state = ELEMENT_ENABLED;
  }
}

#ifdef DEBUG

// 
void downloadPatchSelectHandler(TabElem_t* tab, MenuElem_t* element)
{
  // close menu
  configMenuDisable();

  // send request
  netSendCustomAppMessage(netGetLobbyServerConnection(), CUSTOM_MSG_ID_CLIENT_REQUEST_PATCH, 0, (void*)element);
}

#endif

// 
void mapsSelectHandler(TabElem_t* tab, MenuElem_t* element)
{
  // 
  if (gameIsIn())
    return;

  // close menu
  configMenuDisable();

  // 
  mapsPromptEnableCustomMaps();
}

// 
void gmResetSelectHandler(TabElem_t* tab, MenuElem_t* element)
{
  memset(&gameConfig, 0, sizeof(gameConfig));
}

//------------------------------------------------------------------------------
void menuStateAlwaysHiddenHandler(TabElem_t* tab, MenuElem_t* element, int* state)
{
  *state = ELEMENT_HIDDEN;
}

// 
void menuStateAlwaysDisabledHandler(TabElem_t* tab, MenuElem_t* element, int* state)
{
  *state = ELEMENT_DISABLED;
}

// 
void menuStateAlwaysEnabledHandler(TabElem_t* tab, MenuElem_t* element, int* state)
{
  *state = ELEMENT_ENABLED;
}

// 
void menuStateHandler_InstallCustomMaps(TabElem_t* tab, MenuElem_t* element, int* state)
{
  *state = mapsHasTriedLoading() ? ELEMENT_DISABLED : ELEMENT_ENABLED;
}

// 
void menuStateHandler_GameModeOverride(TabElem_t* tab, MenuElem_t* element, int* state)
{
  int i = 0;

  // hide gamemode for maps with exclusive gamemode
  for (i = 0; i < dataCustomMapsWithExclusiveGameModeCount; ++i)
  {
    if (gameConfig.customMapId == dataCustomMapsWithExclusiveGameMode[i])
    {
      *state = ELEMENT_HIDDEN;
      return;
    }
  }

  *state = ELEMENT_ENABLED;
}

int getMenuElementState(TabElem_t* tab, MenuElem_t* element)
{
  // get tab and element state
  int tabState = 0, state = 0;
  tab->stateHandler(tab, &tabState);
  element->stateHandler(tab, element, &state);

  // return the smaller of the two states
  if (state < tabState)
    return state;
  
  return tabState;
}

//------------------------------------------------------------------------------
void drawToggleMenuElement(TabElem_t* tab, MenuElem_t* element, RECT* rect)
{
  // get element state
  int state = getMenuElementState(tab, element);

  float x,y;
  float lerp = (state == ELEMENT_ENABLED) ? 0.0 : 0.5;
  u32 color = colorLerp(colorText, 0, lerp);

  // draw name
  x = (rect->TopLeft[0] * SCREEN_WIDTH) + 5;
  y = (rect->TopLeft[1] * SCREEN_HEIGHT) + 5;
  gfxScreenSpaceText(x, y, 1, 1, color, element->name, -1, 0);

  // draw value
  x = (rect->TopRight[0] * SCREEN_WIDTH) - 5;
  gfxScreenSpaceText(x, y, 1, 1, color, *(char*)element->userdata ? "On" : "Off", -1, 2);
}

//------------------------------------------------------------------------------
void drawListMenuElement(TabElem_t* tab, MenuElem_t* element, MenuElem_ListData_t * listData, RECT* rect)
{
  // get element state
  int state = getMenuElementState(tab, element);

  float x,y;
  float lerp = (state == ELEMENT_ENABLED) ? 0.0 : 0.5;
  u32 color = colorLerp(colorText, 0, lerp);

  // draw name
  x = (rect->TopLeft[0] * SCREEN_WIDTH) + 5;
  y = (rect->TopLeft[1] * SCREEN_HEIGHT) + 5;
  gfxScreenSpaceText(x, y, 1, 1, color, element->name, -1, 0);

  // draw value
  x = (rect->TopRight[0] * SCREEN_WIDTH) - 5;
  gfxScreenSpaceText(x, y, 1, 1, color, listData->items[(int)*listData->value], -1, 2);
}

//------------------------------------------------------------------------------
void drawButtonMenuElement(TabElem_t* tab, MenuElem_t* element, RECT* rect)
{
  // get element state
  int state = getMenuElementState(tab, element);

  float x,y,b = 0.005;
  float lerp = (state == ELEMENT_ENABLED) ? 0.0 : 0.5;
  u32 color;
  RECT rBg = {
    { rect->TopLeft[0] + 0.05, rect->TopLeft[1] },
    { rect->TopRight[0] - 0.05, rect->TopRight[1] },
    { rect->BottomLeft[0] + 0.05, rect->BottomLeft[1] },
    { rect->BottomRight[0] - 0.05, rect->BottomRight[1] },
  };
  RECT rFg = {
    { rBg.TopLeft[0] + b, rBg.TopLeft[1] + b },
    { rBg.TopRight[0] - b, rBg.TopRight[1] + b },
    { rBg.BottomLeft[0] + b, rBg.BottomLeft[1] - b },
    { rBg.BottomRight[0] - b, rBg.BottomRight[1] - b },
  };

  // bg
  color = colorLerp(colorButtonBg, 0, lerp);
	gfxScreenSpaceQuad(&rBg, color, color, color, color);

  // fg
  color = colorLerp(colorButtonFg, 0, lerp);
	gfxScreenSpaceQuad(&rFg, color, color, color, color);

  // draw name
  x = 0.5 * SCREEN_WIDTH;
  y = ((rFg.TopLeft[1] + rFg.BottomLeft[1]) * SCREEN_HEIGHT * 0.5);
  gfxScreenSpaceText(x, y, 1, 1, colorLerp(colorText, 0, lerp), element->name, -1, 4);

  // add some padding
  rect->TopLeft[1] += 0.01;
  rect->TopRight[1] += 0.01;
}

//------------------------------------------------------------------------------
void buttonActionHandler(TabElem_t* tab, MenuElem_t* element, int actionType, void * actionArg)
{
  // get element state
  int state = getMenuElementState(tab, element);

  // do nothing if hidden
  if (state == ELEMENT_HIDDEN)
    return;

  switch (actionType)
  {
    case ACTIONTYPE_SELECT:
    {
      if (state != ELEMENT_ENABLED)
        break;

      if (element->userdata)
        ((ButtonSelectHandler)element->userdata)(tab, element);
      break;
    }
    case ACTIONTYPE_GETHEIGHT:
    {
      *(float*)actionArg = LINE_HEIGHT * 2;
      break;
    }
    case ACTIONTYPE_DRAW:
    {
      drawButtonMenuElement(tab, element, (RECT*)actionArg);
      break;
    }
  }
}

//------------------------------------------------------------------------------
void listActionHandler(TabElem_t* tab, MenuElem_t* element, int actionType, void * actionArg)
{
  MenuElem_ListData_t* listData = (MenuElem_ListData_t*)element->userdata;
  int itemCount = listData->count;

  // get element state
  int state = getMenuElementState(tab, element);

  // do nothing if hidden
  if (state == ELEMENT_HIDDEN)
    return;

  switch (actionType)
  {
    case ACTIONTYPE_INCREMENT:
    case ACTIONTYPE_SELECT:
    {
      if (state != ELEMENT_ENABLED)
        break;
      char newValue = *listData->value + 1;
      if (newValue >= itemCount)
        newValue = 0;
      *listData->value = newValue;
      break;
    }
    case ACTIONTYPE_DECREMENT:
    {
      if (state != ELEMENT_ENABLED)
        break;
      char newValue = *listData->value - 1;
      if (newValue < 0)
        newValue = itemCount - 1;
      *listData->value = newValue;
      break;
    }
    case ACTIONTYPE_GETHEIGHT:
    {
      *(float*)actionArg = LINE_HEIGHT;
      break;
    }
    case ACTIONTYPE_DRAW:
    {
      drawListMenuElement(tab, element, listData, (RECT*)actionArg);
      break;
    }
  }
}

//------------------------------------------------------------------------------
void toggleActionHandler(TabElem_t* tab, MenuElem_t* element, int actionType, void * actionArg)
{
  // get element state
  int state = getMenuElementState(tab, element);

  // do nothing if hidden
  if (state == ELEMENT_HIDDEN)
    return;

  switch (actionType)
  {
    case ACTIONTYPE_INCREMENT:
    case ACTIONTYPE_SELECT:
    case ACTIONTYPE_DECREMENT:
    {
      if (state != ELEMENT_ENABLED)
        break;
      // toggle
      *(char*)element->userdata = !(*(char*)element->userdata);;
      break;
    }
    case ACTIONTYPE_GETHEIGHT:
    {
      *(float*)actionArg = LINE_HEIGHT;
      break;
    }
    case ACTIONTYPE_DRAW:
    {
      drawToggleMenuElement(tab, element, (RECT*)actionArg);
      break;
    }
  }
}

//------------------------------------------------------------------------------
void drawFrame(void)
{
  int i;
  TabElem_t * tab = NULL;
  int state = 0;
  float tabX = frameX;
  float tabY = frameY + frameTitleH;

  // bg
  gfxScreenSpaceBox(frameX, frameY, frameW, frameH, colorBg);

  // title bg
  gfxScreenSpaceBox(frameX, frameY, frameW, frameTitleH, colorRed);

  // title
  gfxScreenSpaceText(0.5 * SCREEN_WIDTH, (frameY + frameTitleH * 0.5) * SCREEN_HEIGHT, 1, 1, colorText, "Patch Config", -1, 4);

  // footer bg
  gfxScreenSpaceBox(frameX, frameY + frameH - frameFooterH, frameW, frameFooterH, colorRed);

  // footer
  gfxScreenSpaceText(((frameX + frameW) * SCREEN_WIDTH) - 5, (frameY + frameH) * SCREEN_HEIGHT - 5, 1, 1, colorText, footerText, -1, 8);

  // content bg
  gfxScreenSpaceBox(frameX + contentPaddingX, frameY + frameTitleH + tabBarH + contentPaddingY, frameW - (contentPaddingX*2), frameH - frameTitleH - tabBarH - frameFooterH - (contentPaddingY * 2), colorContentBg);

  // tab bar
  gfxScreenSpaceBox(tabX, tabY, frameW, tabBarH, colorTabBarBg);

  // tabs
  for (i = 0; i < tabsCount; ++i)
  {
    // get tab state
    tab = &tabElements[i];
    tab->stateHandler(tab, &state);

    // skip hidden elements
    if (state != ELEMENT_HIDDEN)
    {
      // get tab title width
      float pWidth = (4 * tabBarPaddingX) + gfxGetFontWidth(tab->name, -1, 1) / (float)SCREEN_WIDTH;

      // get color
      float lerp = state == ELEMENT_ENABLED ? 0.0 : 0.5;
      u32 color = colorLerp(colorText, 0, lerp);

      // draw bar
      u32 barColor = selectedTabItem == i ? colorSelected : colorTabBg;
      gfxScreenSpaceBox(tabX + tabBarPaddingX, tabY, pWidth - (2 * tabBarPaddingX), tabBarH, barColor);

      // draw text
      gfxScreenSpaceText((tabX + 2*tabBarPaddingX) * SCREEN_WIDTH, (tabY + (0.5 * tabBarH)) * SCREEN_HEIGHT, 1, 1, color, tab->name, -1, 3);

      // increment X
      tabX += pWidth - tabBarPaddingX;
    }
  }
}


//------------------------------------------------------------------------------
void drawTab(TabElem_t* tab)
{
  if (!tab)
    return;

  int i = 0;
  int menuElementRenderEnd = tab->menuOffset;
  MenuElem_t * menuElements = tab->elements;
	int menuElementsCount = tab->elementsCount;
  MenuElem_t* currentElement;

  float contentX = frameX + contentPaddingX;
  float contentY = frameY + frameTitleH + tabBarH + contentPaddingY;
  float contentW = frameW - (contentPaddingX * 2);
  float contentH = frameH - frameTitleH - tabBarH - frameFooterH - (contentPaddingY * 2);
  RECT drawRect = {
    { contentX, contentY },
    { contentX + contentW, contentY },
    { contentX, contentY },
    { contentX + contentW, contentY }
  };

  // draw items
  for (i = tab->menuOffset; i < menuElementsCount; ++i)
  {
    currentElement = &menuElements[i];
    float itemHeight = 0;
    currentElement->handler(tab, currentElement, ACTIONTYPE_GETHEIGHT, &itemHeight);

    // ensure item is within content bounds
    if ((drawRect.BottomLeft[1] + itemHeight) > (contentY + contentH))
      break;

    // set rect to height
    drawRect.BottomLeft[1] = drawRect.TopLeft[1] + itemHeight;
    drawRect.BottomRight[1] = drawRect.TopRight[1] + itemHeight;

    // draw selection
    if (i == tab->selectedMenuItem) {
      gfxScreenSpaceQuad(&drawRect, colorSelected, colorSelected, colorSelected, colorSelected);
    }

    // draw
    currentElement->handler(tab, currentElement, ACTIONTYPE_DRAW, &drawRect);

    // increment rect
    drawRect.TopLeft[1] += itemHeight;
    drawRect.TopRight[1] += itemHeight;

    menuElementRenderEnd = i + 1;
  }
  
  // draw scroll bar
  if (tab->menuOffset > 0 || menuElementRenderEnd < menuElementsCount)
  {
    float scrollValue = tab->menuOffset / (float)(menuElementsCount - (menuElementRenderEnd-tab->menuOffset));
    float scrollBarHeight = 0.05;
    float contentRectHeight = contentH - scrollBarHeight;

    gfxScreenSpaceBox(contentX + contentW, contentY + (scrollValue * contentRectHeight), 0.01, scrollBarHeight, colorRed);
  }

  // 
  if (tab->selectedMenuItem >= menuElementRenderEnd)
    ++tab->menuOffset;
  if (tab->selectedMenuItem < tab->menuOffset)
    tab->menuOffset = tab->selectedMenuItem;

  // get selected element
  if (tab->selectedMenuItem >= menuElementsCount)
    return;

  currentElement = &menuElements[tab->selectedMenuItem];
  int state = getMenuElementState(tab, currentElement);

  // find next selectable item if hidden
  if (state == ELEMENT_HIDDEN)
    navMenu(tab, 1, 1);

  // nav down
  if (padGetButtonDown(0, PAD_DOWN) > 0)
  {
    navMenu(tab, 1, 0);
  }
  // nav up
  else if (padGetButtonDown(0, PAD_UP) > 0)
  {
    navMenu(tab, -1, 0);
  }
  // nav select
  else if (padGetButtonDown(0, PAD_CROSS) > 0)
  {
    if (state == ELEMENT_ENABLED)
      currentElement->handler(tab, currentElement, ACTIONTYPE_SELECT, NULL);
  }
  // nav inc
  else if (padGetButtonDown(0, PAD_RIGHT) > 0)
  {
    if (state == ELEMENT_ENABLED)
      currentElement->handler(tab, currentElement, ACTIONTYPE_INCREMENT, NULL);
  }
  // nav dec
  else if (padGetButtonDown(0, PAD_LEFT) > 0)
  {
    if (state == ELEMENT_ENABLED)
      currentElement->handler(tab, currentElement, ACTIONTYPE_DECREMENT, NULL);
  }
}

//------------------------------------------------------------------------------
void onMenuUpdate(int inGame)
{
  TabElem_t* tab = &tabElements[selectedTabItem];

  if (isConfigMenuActive)
  {
    // prevent pad from affecting menus
    padDisableInput();

    // draw
    if (padGetButton(0, PAD_L3) <= 0)
    {
      // draw frame
      drawFrame();

      // draw tab
      drawTab(tab);
    }

    // nav tab right
    if (padGetButtonDown(0, PAD_R1) > 0)
    {
      navTab(1);
    }
    // nav tab left
    else if (padGetButtonDown(0, PAD_L1) > 0)
    {
      navTab(-1);
    }
    // close
    else if (padGetButtonUp(0, PAD_TRIANGLE) > 0 || padGetButtonDown(0, PAD_START) > 0)
    {
      configMenuDisable();
    }
  }
  else if (!inGame && !mapsDownloadingModules())
  {
    if (uiGetActive() == UI_ID_ONLINE_MAIN_MENU)
    {
      // render message
      gfxScreenSpaceBox(0.1, 0.75, 0.4, 0.05, colorOpenBg);
      gfxScreenSpaceText(SCREEN_WIDTH * 0.3, SCREEN_HEIGHT * 0.775, 1, 1, 0x80FFFFFF, "\x1f Open Config Menu", -1, 4);
    }

		// check for pad input
		if (padGetButtonDown(0, PAD_START) > 0)
		{
      configMenuEnable();
		}
  }
}

//------------------------------------------------------------------------------
void onConfigUpdate(void)
{
  int i;

  // in staging, update game info
  GameSettings * gameSettings = gameGetSettings();
  if (gameSettings && gameSettings->GameLoadStartTime < 0)
  {
    // 
    char * mapName = mapGetName(gameSettings->GameLevel);
    char * modeName = gameGetGameModeName(gameSettings->GameRules);

    // get map override name
    if (gameConfig.customMapId > 0)
      mapName = dataCustomMaps.items[(int)gameConfig.customMapId];

    // get mode override name
    if (gameConfig.customModeId > 0)
    {
      modeName = (char*)CustomModeShortNames[(int)gameConfig.customModeId];
      if (!modeName)
        modeName = dataCustomModes.items[(int)gameConfig.customModeId];
    }
    
    // override gamemode name with map if map has exclusive gamemode
    for (i = 0; i < dataCustomMapsWithExclusiveGameModeCount; ++i)
    {
      if (gameConfig.customMapId == dataCustomMapsWithExclusiveGameMode[i])
      {
        modeName = mapName;
        break;
      }
    }

    // update ui strings
    strncpy((char*)0x013C8C30, mapName, 32);
    strncpy((char*)0x013EC974, mapName, 32);
    strncpy((char*)0x013C8D68, modeName, 32);
    strncpy((char*)0x013ECAAC, modeName, 32);
  }
}

//------------------------------------------------------------------------------
void navMenu(TabElem_t* tab, int direction, int loop)
{
  int newElement = tab->selectedMenuItem + direction;
  MenuElem_t *elem = NULL;
  int state = 0;

  // handle case where tab has no items
  if (tab->elementsCount == 0)
  {
    tab->selectedMenuItem = 0;
    tab->menuOffset = 0;
    return;
  }

  while (newElement != tab->selectedMenuItem)
  {
    if (newElement >= tab->elementsCount)
    {
      if (loop)
        newElement = 0;
      else
        break;
    }
    else if (newElement < 0)
    {
      if (loop)
        newElement = tab->elementsCount - 1;
      else
        break;
    }

    // get newly selected element state
    elem = &tab->elements[newElement];
    elem->stateHandler(tab, elem, &state);

    // skip if hidden
    if (state == ELEMENT_HIDDEN)
    {
      newElement += direction;
      continue;
    }

    // set new tab
    tab->selectedMenuItem = newElement;
    break;
  }
}

void navTab(int direction)
{
  int newTab = selectedTabItem + direction;
  TabElem_t *tab = NULL;
  int state = 0;

  while (1)
  {
    if (newTab >= tabsCount)
      break;
    if (newTab < 0)
      break;
    
    // get new tab state
    tab = &tabElements[newTab];
    tab->stateHandler(tab, &state);

    // skip if hidden
    if (state == ELEMENT_HIDDEN)
    {
      newTab += direction;
      continue;
    }

    // set new tab
    selectedTabItem = newTab;
    break;
  }
}

//------------------------------------------------------------------------------
int onSetGameConfig(void * connection, void * data)
{
  // copy it over
  memcpy(&gameConfig, data, sizeof(PatchGameConfig_t));
  return sizeof(PatchGameConfig_t);
}

//------------------------------------------------------------------------------
void onConfigGameMenu(void)
{
  onMenuUpdate(1);
}

//------------------------------------------------------------------------------
void onConfigOnlineMenu(void)
{
  onMenuUpdate(0);
}

//------------------------------------------------------------------------------
void onConfigInitialize(void)
{
	// install net handlers
	netInstallCustomMsgHandler(CUSTOM_MSG_ID_SERVER_SET_GAME_CONFIG, &onSetGameConfig);

  // reset game configs
  memset(&gameConfigHostBackup, 0, sizeof(gameConfigHostBackup));
  memset(&gameConfig, 0, sizeof(gameConfig));
}

//------------------------------------------------------------------------------
void configTrySendGameConfig(void)
{
  int state = 0;

  // send game config to server for saving if tab is enabled
  tabElements[1].stateHandler(&tabElements[1], &state);
  if (state == ELEMENT_ENABLED)
  {
    // backup
    memcpy(&gameConfigHostBackup, &gameConfig, sizeof(PatchGameConfig_t));

    // send
    netSendCustomAppMessage(netGetLobbyServerConnection(), CUSTOM_MSG_ID_CLIENT_USER_GAME_CONFIG, sizeof(PatchGameConfig_t), &gameConfig);
  }
}

//------------------------------------------------------------------------------
void configMenuDisable(void)
{
  if (!isConfigMenuActive)
    return;
  
  isConfigMenuActive = 0;

  // send config to server for saving
  netSendCustomAppMessage(netGetLobbyServerConnection(), CUSTOM_MSG_ID_CLIENT_USER_CONFIG, sizeof(PatchConfig_t), &config);

  // 
  configTrySendGameConfig();

  // re-enable pad
  padEnableInput();
}

//------------------------------------------------------------------------------
void configMenuEnable(void)
{
  // enable
  isConfigMenuActive = 1;

  // return to first tab if current is hidden
  int state = 0;
  tabElements[selectedTabItem].stateHandler(&tabElements[selectedTabItem], &state);
  if (state == ELEMENT_HIDDEN)
    selectedTabItem = 0;
}
