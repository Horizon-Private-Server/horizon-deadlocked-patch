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
#include <libdl/utils.h>
#include "messageid.h"
#include "config.h"
#include "include/config.h"

#define LINE_HEIGHT         (0.05)
#define LINE_HEIGHT_3_2     (0.075)
#define DEFAULT_GAMEMODE    (0)
#define CHARACTER_TWEAKER_RANGE (10)

// config
extern PatchConfig_t config;

// game config
extern PatchGameConfig_t gameConfig;
extern PatchGameConfig_t gameConfigHostBackup;

extern char playerFov;
extern char aa_value;
extern int redownloadCustomModeBinaries;

// 
int isConfigMenuActive = 0;
int selectedTabItem = 0;
int hasDevGameConfig = 0;
u32 padPointer = 0;
int preset = 0;

char fixWeaponLagToggle = 1;

//
int dlBytesReceived = 0;
int dlTotalBytes = 0;
int dlIsActive = 0;


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
const u32 colorRangeBar = 0x80000040;

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
void toggleInvertedActionHandler(TabElem_t* tab, MenuElem_t* element, int actionType, void * actionArg);
void listActionHandler(TabElem_t* tab, MenuElem_t* element, int actionType, void * actionArg);
void rangeActionHandler(TabElem_t* tab, MenuElem_t* element, int actionType, void * actionArg);
void gmOverrideListActionHandler(TabElem_t* tab, MenuElem_t* element, int actionType, void * actionArg);
void labelActionHandler(TabElem_t* tab, MenuElem_t* element, int actionType, void * actionArg);

// state handlers
void menuStateAlwaysHiddenHandler(TabElem_t* tab, MenuElem_t* element, int* state);
void menuStateAlwaysDisabledHandler(TabElem_t* tab, MenuElem_t* element, int* state);
void menuStateAlwaysEnabledHandler(TabElem_t* tab, MenuElem_t* element, int* state);
void menuLabelStateHandler(TabElem_t* tab, MenuElem_t* element, int* state);
void menuStateHandler_InstallCustomMaps(TabElem_t* tab, MenuElem_t* element, int* state);
void menuStateHandler_CheckForUpdatesCustomMaps(TabElem_t* tab, MenuElem_t* element, int* state);
void menuStateHandler_InstalledCustomMaps(TabElem_t* tab, MenuElem_t* element, int* state);
void menuStateHandler_GameModeOverride(TabElem_t* tab, MenuElem_t* element, int* state);

int menuStateHandler_SelectedMapOverride(MenuElem_ListData_t* listData, char* value);
int menuStateHandler_SelectedGameModeOverride(MenuElem_ListData_t* listData, char* value);
int menuStateHandler_SelectedTrainingTypeOverride(MenuElem_ListData_t* listData, char* value);

void menuStateHandler_SurvivalSettingStateHandler(TabElem_t* tab, MenuElem_t* element, int* state);
void menuStateHandler_PayloadSettingStateHandler(TabElem_t* tab, MenuElem_t* element, int* state);
void menuStateHandler_TrainingSettingStateHandler(TabElem_t* tab, MenuElem_t* element, int* state);
void menuStateHandler_CTFSettingStateHandler(TabElem_t* tab, MenuElem_t* element, int* state);
void menuStateHandler_KOTHSettingStateHandler(TabElem_t* tab, MenuElem_t* element, int* state);
void menuStateHandler_CQSettingStateHandler(TabElem_t* tab, MenuElem_t* element, int* state);
void menuStateHandler_SettingStateHandler(TabElem_t* tab, MenuElem_t* element, int* state);

void downloadMapUpdatesSelectHandler(TabElem_t* tab, MenuElem_t* element);

#if MAPEDITOR
void menuStateHandler_MapEditorSpawnPoints(TabElem_t* tab, MenuElem_t* element, int* state);
#endif

void tabDefaultStateHandler(TabElem_t* tab, int * state);
void tabFreecamStateHandler(TabElem_t* tab, int * state);
void tabGameSettingsStateHandler(TabElem_t* tab, int * state);
void tabCustomMapStateHandler(TabElem_t* tab, int * state);

// list select handlers
void mapsSelectHandler(TabElem_t* tab, MenuElem_t* element);
void gmResetSelectHandler(TabElem_t* tab, MenuElem_t* element);

#ifdef DEBUG
void downloadPatchSelectHandler(TabElem_t* tab, MenuElem_t* element);
void downloadBootElfSelectHandler(TabElem_t* tab, MenuElem_t* element);
#endif

void navMenu(TabElem_t* tab, int direction, int loop);
void navTab(int direction);

int mapsGetInstallationResult(void);
int mapsPromptEnableCustomMaps(void);
int mapsDownloadingModules(void);

// level of detail list item
MenuElem_ListData_t dataLevelOfDetail = {
    &config.levelOfDetail,
    NULL,
#if DEBUG
    4,
#else
    3,
#endif
    { "Potato", "Low", "Normal", "High" }
};

// framelimiter list item
MenuElem_ListData_t dataFramelimiter = {
    &config.framelimiter,
    NULL,
    3,
    { "On", "Auto", "Off" }
};

// minimap scale list item
MenuElem_ListData_t dataMinimapScale = {
    &config.minimapScale,
    NULL,
    2,
    { "Normal", "Half" }
};

// minimap expanded zoom
MenuElem_RangeData_t dataMinimapBigZoom = {
    .value = &config.minimapBigZoom,
    .stateHandler = NULL,
    .minValue = 0,
    .maxValue = 10,
};

// minimap shrunk zoom
MenuElem_RangeData_t dataMinimapSmallZoom = {
    .value = &config.minimapSmallZoom,
    .stateHandler = NULL,
    .minValue = 0,
    .maxValue = 10,
};

// player aggregation time offset range item
MenuElem_RangeData_t dataPlayerAggTime = {
    .value = &config.playerAggTime,
    .stateHandler = NULL,
    .minValue = -5,
    .maxValue = 5,
};

// general tab menu items
MenuElem_t menuElementsGeneral[] = {
#ifdef DEBUG
  { "Redownload patch", buttonActionHandler, menuStateAlwaysEnabledHandler, downloadPatchSelectHandler },
  { "Download boot elf", buttonActionHandler, menuStateAlwaysEnabledHandler, downloadBootElfSelectHandler },
#endif
  { "Install custom maps on login", toggleActionHandler, menuStateAlwaysEnabledHandler, &config.enableAutoMaps },
  { "16:9 Widescreen", toggleActionHandler, menuStateAlwaysEnabledHandler, (char*)0x00171DEB },
  { "Agg Time", rangeActionHandler, menuStateAlwaysEnabledHandler, &dataPlayerAggTime },
  { "Announcers on all gamemodes", toggleActionHandler, menuStateAlwaysEnabledHandler, &config.enableGamemodeAnnouncements },
  { "Camera Shake", toggleInvertedActionHandler, menuStateAlwaysEnabledHandler, &config.disableCameraShake },
  { "Disable \x11 to equip hacker ray", toggleActionHandler, menuStateAlwaysEnabledHandler, &config.disableCircleToHackerRay },
  { "Fps Counter", toggleActionHandler, menuStateAlwaysEnabledHandler, &config.enableFpsCounter },
  { "Framelimiter", listActionHandler, menuStateAlwaysEnabledHandler, &dataFramelimiter },
  { "Fusion Reticule", toggleActionHandler, menuStateAlwaysEnabledHandler, &config.enableFusionReticule },
  { "Level of Detail", listActionHandler, menuStateAlwaysEnabledHandler, &dataLevelOfDetail },
  { "Minimap Big Scale", listActionHandler, menuStateAlwaysEnabledHandler, &dataMinimapScale },
  { "Minimap Big Zoom", rangeActionHandler, menuStateAlwaysEnabledHandler, &dataMinimapBigZoom },
  { "Minimap Small Zoom", rangeActionHandler, menuStateAlwaysEnabledHandler, &dataMinimapSmallZoom },
  { "Progressive Scan", toggleActionHandler, menuStateAlwaysEnabledHandler, (char*)0x0021DE6C },
  { "Singleplayer music", toggleActionHandler, menuStateAlwaysEnabledHandler, &config.enableSingleplayerMusic },
  { "Spectate mode", toggleActionHandler, menuStateAlwaysEnabledHandler, &config.enableSpectate },
  { "Sync player state", toggleActionHandler, menuStateAlwaysEnabledHandler, &config.enablePlayerStateSync },
};

#if TWEAKERS

// character head size range item
MenuElem_RangeData_t dataCharacterHead = {
    .value = &config.characterTweakers[0],
    .stateHandler = NULL,
    .minValue = -CHARACTER_TWEAKER_RANGE,
    .maxValue = CHARACTER_TWEAKER_RANGE,
};

// character torso size range item
MenuElem_RangeData_t dataCharacterTorso = {
    .value = &config.characterTweakers[1],
    .stateHandler = NULL,
    .minValue = -CHARACTER_TWEAKER_RANGE,
    .maxValue = CHARACTER_TWEAKER_RANGE,
};

// character left arm size range item
MenuElem_RangeData_t dataCharacterLeftArm = {
    .value = &config.characterTweakers[2],
    .stateHandler = NULL,
    .minValue = -CHARACTER_TWEAKER_RANGE,
    .maxValue = CHARACTER_TWEAKER_RANGE,
};

// character right arm size range item
MenuElem_RangeData_t dataCharacterRightArm = {
    .value = &config.characterTweakers[3],
    .stateHandler = NULL,
    .minValue = -CHARACTER_TWEAKER_RANGE,
    .maxValue = CHARACTER_TWEAKER_RANGE,
};

// character left leg size range item
MenuElem_RangeData_t dataCharacterLeftLeg = {
    .value = &config.characterTweakers[4],
    .stateHandler = NULL,
    .minValue = -CHARACTER_TWEAKER_RANGE,
    .maxValue = CHARACTER_TWEAKER_RANGE,
};

// character right leg size range item
MenuElem_RangeData_t dataCharacterRightLeg = {
    .value = &config.characterTweakers[5],
    .stateHandler = NULL,
    .minValue = -CHARACTER_TWEAKER_RANGE,
    .maxValue = CHARACTER_TWEAKER_RANGE,
};

// character hips size range item
MenuElem_RangeData_t dataCharacterHips = {
    .value = &config.characterTweakers[7],
    .stateHandler = NULL,
    .minValue = -CHARACTER_TWEAKER_RANGE,
    .maxValue = CHARACTER_TWEAKER_RANGE,
};

// character head pos range item
MenuElem_RangeData_t dataCharacterHeadPos = {
    .value = &config.characterTweakers[CHARACTER_TWEAKER_HEAD_POS],
    .stateHandler = NULL,
    .minValue = -CHARACTER_TWEAKER_RANGE,
    .maxValue = CHARACTER_TWEAKER_RANGE,
};

// character upper torso pos range item
MenuElem_RangeData_t dataCharacterTorsoPos = {
    .value = &config.characterTweakers[CHARACTER_TWEAKER_UPPER_TORSO_POS],
    .stateHandler = NULL,
    .minValue = -CHARACTER_TWEAKER_RANGE,
    .maxValue = CHARACTER_TWEAKER_RANGE,
};

// character lower torso pos range item
MenuElem_RangeData_t dataCharacterHipsPos = {
    .value = &config.characterTweakers[CHARACTER_TWEAKER_LOWER_TORSO_POS],
    .stateHandler = NULL,
    .minValue = -CHARACTER_TWEAKER_RANGE,
    .maxValue = CHARACTER_TWEAKER_RANGE,
};

// character left arm pos range item
MenuElem_RangeData_t dataCharacterLeftArmPos = {
    .value = &config.characterTweakers[CHARACTER_TWEAKER_LEFT_ARM_POS],
    .stateHandler = NULL,
    .minValue = -CHARACTER_TWEAKER_RANGE,
    .maxValue = CHARACTER_TWEAKER_RANGE,
};

// character right arm pos range item
MenuElem_RangeData_t dataCharacterRightArmPos = {
    .value = &config.characterTweakers[CHARACTER_TWEAKER_RIGHT_ARM_POS],
    .stateHandler = NULL,
    .minValue = -CHARACTER_TWEAKER_RANGE,
    .maxValue = CHARACTER_TWEAKER_RANGE,
};

// character left leg pos range item
MenuElem_RangeData_t dataCharacterLeftLegPos = {
    .value = &config.characterTweakers[CHARACTER_TWEAKER_LEFT_LEG_POS],
    .stateHandler = NULL,
    .minValue = -CHARACTER_TWEAKER_RANGE,
    .maxValue = CHARACTER_TWEAKER_RANGE,
};

// character right leg pos range item
MenuElem_RangeData_t dataCharacterRightLegPos = {
    .value = &config.characterTweakers[CHARACTER_TWEAKER_RIGHT_POS],
    .stateHandler = NULL,
    .minValue = -CHARACTER_TWEAKER_RANGE,
    .maxValue = CHARACTER_TWEAKER_RANGE,
};

// character tab menu items
MenuElem_t menuElementsCharacter[] = {
  { "Hide All", toggleActionHandler, menuStateAlwaysEnabledHandler, &config.characterTweakers[6] },
  { "Head Scale", rangeActionHandler, menuStateAlwaysEnabledHandler, &dataCharacterHead },
  { "Upper Torso Scale", rangeActionHandler, menuStateAlwaysEnabledHandler, &dataCharacterTorso },
  { "Lower Torso Scale", rangeActionHandler, menuStateAlwaysEnabledHandler, &dataCharacterHips },
  { "Left Arm Scale", rangeActionHandler, menuStateAlwaysEnabledHandler, &dataCharacterLeftArm },
  { "Right Arm Scale", rangeActionHandler, menuStateAlwaysEnabledHandler, &dataCharacterRightArm },
  { "Left Leg Scale", rangeActionHandler, menuStateAlwaysEnabledHandler, &dataCharacterLeftLeg },
  { "Right Leg Scale", rangeActionHandler, menuStateAlwaysEnabledHandler, &dataCharacterRightLeg },
  { "Head Pos", rangeActionHandler, menuStateAlwaysEnabledHandler, &dataCharacterHeadPos },
  { "Upper Torso Pos", rangeActionHandler, menuStateAlwaysEnabledHandler, &dataCharacterTorsoPos },
  { "Lower Torso Pos", rangeActionHandler, menuStateAlwaysEnabledHandler, &dataCharacterHipsPos },
  { "Left Arm Pos", rangeActionHandler, menuStateAlwaysEnabledHandler, &dataCharacterLeftArmPos },
  { "Right Arm Pos", rangeActionHandler, menuStateAlwaysEnabledHandler, &dataCharacterRightArmPos },
  { "Left Leg Pos", rangeActionHandler, menuStateAlwaysEnabledHandler, &dataCharacterLeftLegPos },
  { "Right Leg Pos", rangeActionHandler, menuStateAlwaysEnabledHandler, &dataCharacterRightLegPos },
};

#endif

extern FreecamSettings_t freecamSettings;

// player fov range item
MenuElem_RangeData_t dataFieldOfView = {
    .value = &playerFov,
    .stateHandler = NULL,
    .minValue = -10,
    .maxValue = 10,
};

// character tab menu items
MenuElem_t menuElementsFreecam[] = {
  { "Field of View", rangeActionHandler, menuStateAlwaysEnabledHandler, &dataFieldOfView },
  { "Airwalk", toggleActionHandler, menuStateAlwaysEnabledHandler, &freecamSettings.airwalk },
  { "Lock Position", toggleActionHandler, menuStateAlwaysEnabledHandler, &freecamSettings.lockPosition },
  { "Lock Animation", toggleActionHandler, menuStateAlwaysEnabledHandler, &freecamSettings.lockStateToggle },
};

// map override list item
MenuElem_ListData_t dataCustomMaps = {
    &gameConfig.customMapId,
    menuStateHandler_SelectedMapOverride,
    CUSTOM_MAP_COUNT,
    {
      "None",
      "Ace Hardlight's Suite",
      "Annihilation Nation",
      "Bakisi Isles",
      "Battledome SP",
      "Blackwater City",
      "Blackwater Docks",
      "Canal City",
      "Containment Suite",
      "Dark Cathedral Interior",
      "Ghost Hangar",
      "Ghost Ship",
      "Hoven Gorge",
      "Infinite Climber",
      "Korgon Outpost",
      "Launch Site",
      "Marcadia Palace",
      "Metropolis MP",
      "Mining Facility SP",
      "Shaar SP",
      "Snivelak",
      "Spleef",
      "Torval Lost Factory",
      "Torval SP",
      "Tyhrranosis",
      // -- SURVIVAL MAPS --
      [CUSTOM_MAP_SURVIVAL_MINING_FACILITY] "Orxon",
    }
};

// maps with their own exclusive gamemode
char dataCustomMapsWithExclusiveGameMode[] = {
  CUSTOM_MAP_SPLEEF,
  CUSTOM_MAP_INFINITE_CLIMBER
};
const int dataCustomMapsWithExclusiveGameModeCount = sizeof(dataCustomMapsWithExclusiveGameMode)/sizeof(char);

// gamemode override list item
MenuElem_ListData_t dataCustomModes = {
    &gameConfig.customModeId,
    menuStateHandler_SelectedGameModeOverride,
    CUSTOM_MODE_COUNT,
    {
      "None",
      "Gun Game",
      "Infected",
      // "Infinite Climber",
      "Payload",
      "Search and Destroy",
      "Survival",
      "1000 Kills",
      "Training",
#if DEV
      "Gridiron",
      "Team Defenders",
      "Anim Extractor",
#endif
    }
};

// 
const char* CustomModeShortNames[] = {
  NULL,
  NULL,
  NULL,
  //"Climber",
  NULL,
  "SND",
  NULL,
  NULL,
  NULL,
#if DEV
  NULL,
  NULL,
  NULL,
#endif
};

// survival difficulty
/*
MenuElem_ListData_t dataSurvivalDifficulty = {
    &gameConfig.survivalConfig.difficulty,
    NULL,
    5,
    {
      "Couch Potato",
      "Contestant",
      "Gladiator",
      "Hero",
      "Exterminator"
    }
};
*/

// payload contest mode
MenuElem_ListData_t dataPayloadContestMode = {
    &gameConfig.payloadConfig.contestMode,
    NULL,
    3,
    {
      [PAYLOAD_CONTEST_OFF] "Off",
      [PAYLOAD_CONTEST_SLOW] "Slow",
      [PAYLOAD_CONTEST_STOP] "Stop"
    }
};

// training type
MenuElem_ListData_t dataTrainingType = {
    &gameConfig.trainingConfig.type,
    menuStateHandler_SelectedTrainingTypeOverride,
    2,
    {
      "Fusion",
      "Cycle",
      "B6",
    }
};

// player size list item
MenuElem_ListData_t dataPlayerSize = {
    &gameConfig.prPlayerSize,
    NULL,
    5,
    {
      "Normal",
      "Large",
      "Giant",
      "Tiny",
      "Small"
    }
};

// headbutt damage list item
MenuElem_ListData_t dataHeadbutt = {
    &gameConfig.prHeadbutt,
    NULL,
    4,
    {
      "Off",
      "Low Damage",
      "Medium Damage",
      "High Damage"
    }
};

// weather override list item
MenuElem_ListData_t dataWeather = {
    &gameConfig.prWeatherId,
    NULL,
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

// fusion reticule allow/disable list item
MenuElem_ListData_t dataFusionReticule = {
    &gameConfig.grNoSniperHelpers,
    NULL,
    2,
    {
      "Permitted",
      "Disabled"
    }
};

// vampire list item
MenuElem_ListData_t dataVampire = {
    &gameConfig.grVampire,
    NULL,
    4,
    {
      "Off",
      "Quarter Heal",
      "Half Heal",
      "Full Heal",
    }
};

// v2s list item
MenuElem_ListData_t dataV2s = {
    &gameConfig.grV2s,
    NULL,
    3,
    {
      "On",
      "Always",
      "Off",
    }
};

// presets list item
MenuElem_ListData_t dataGameConfigPreset = {
    &preset,
    NULL,
    3,
    {
      "None",
      "Competitive",
      "1v1",
    }
};

// game settings tab menu items
MenuElem_t menuElementsGameSettings[] = {
  { "Reset", buttonActionHandler, menuStateAlwaysEnabledHandler, gmResetSelectHandler },

  // { "Game Settings", labelActionHandler, menuLabelStateHandler, (void*)LABELTYPE_HEADER },
  { "Map override", listActionHandler, menuStateAlwaysEnabledHandler, &dataCustomMaps },
  { "Gamemode override", gmOverrideListActionHandler, menuStateHandler_GameModeOverride, &dataCustomModes },
  { "Preset", listActionHandler, menuStateAlwaysEnabledHandler, &dataGameConfigPreset },

  // SURVIVAL SETTINGS
  // { "Difficulty", listActionHandler, menuStateHandler_SurvivalSettingStateHandler, &dataSurvivalDifficulty },

  // PAYLOAD SETTINGS
  { "Payload Contesting", listActionHandler, menuStateHandler_PayloadSettingStateHandler, &dataPayloadContestMode },

  // TRAINING SETTINGS
  { "Training Type", listActionHandler, menuStateHandler_TrainingSettingStateHandler, &dataTrainingType },

  // GAME RULES
  { "Game Rules", labelActionHandler, menuLabelStateHandler, (void*)LABELTYPE_HEADER },
  { "Better flags", toggleActionHandler, menuStateHandler_CTFSettingStateHandler, &gameConfig.grBetterFlags },
  { "Better hills", toggleActionHandler, menuStateHandler_KOTHSettingStateHandler, &gameConfig.grBetterHills },
  { "CTF Halftime", toggleActionHandler, menuStateHandler_CTFSettingStateHandler, &gameConfig.grHalfTime },
  { "CTF Overtime", toggleActionHandler, menuStateHandler_CTFSettingStateHandler, &gameConfig.grOvertime },
  { "CQ Save Capture Progress", toggleActionHandler, menuStateHandler_CQSettingStateHandler, &gameConfig.grCqPersistentCapture },
  { "CQ Turrets", toggleInvertedActionHandler, menuStateHandler_CQSettingStateHandler, &gameConfig.grCqDisableTurrets },
  { "CQ Upgrades", toggleInvertedActionHandler, menuStateHandler_CQSettingStateHandler, &gameConfig.grCqDisableUpgrades },
  { "Damage cooldown", toggleInvertedActionHandler, menuStateHandler_SettingStateHandler, &gameConfig.grNoInvTimer },
  { "Fix Wallsniping", toggleActionHandler, menuStateHandler_SettingStateHandler, &gameConfig.grFusionShotsAlwaysHit },
  { "Fusion Reticule", listActionHandler, menuStateHandler_SettingStateHandler, &dataFusionReticule },
  { "Healthbars", toggleActionHandler, menuStateHandler_SettingStateHandler, &gameConfig.grHealthBars },
  { "Healthboxes", toggleInvertedActionHandler, menuStateHandler_SettingStateHandler, &gameConfig.grNoHealthBoxes },
  { "Nametags", toggleInvertedActionHandler, menuStateHandler_SettingStateHandler, &gameConfig.grNoNames },
  { "V2s", listActionHandler, menuStateHandler_SettingStateHandler, &dataV2s },
  { "Vampire", listActionHandler, menuStateHandler_SettingStateHandler, &dataVampire },
  { "Weapon packs", toggleInvertedActionHandler, menuStateHandler_SettingStateHandler, &gameConfig.grNoPacks },
  { "Weapon pickups", toggleInvertedActionHandler, menuStateHandler_SettingStateHandler, &gameConfig.grNoPickups },

  // PARTY RULES
  { "Party Rules", labelActionHandler, menuLabelStateHandler, (void*)LABELTYPE_HEADER },
  { "Chargeboot Forever", toggleActionHandler, menuStateAlwaysEnabledHandler, &gameConfig.prChargebootForever },
  { "Headbutt", listActionHandler, menuStateAlwaysEnabledHandler, &dataHeadbutt },
  { "Headbutt Friendly Fire", toggleActionHandler, menuStateAlwaysEnabledHandler, &gameConfig.prHeadbuttFriendlyFire },
  { "Mirror World", toggleActionHandler, menuStateAlwaysEnabledHandler, &gameConfig.prMirrorWorld },
  { "Player Size", listActionHandler, menuStateAlwaysEnabledHandler, &dataPlayerSize },
  { "Rotate Weapons", toggleActionHandler, menuStateAlwaysEnabledHandler, &gameConfig.prRotatingWeapons },
  { "Weather override", listActionHandler, menuStateAlwaysEnabledHandler, &dataWeather },

  // DEV RULES
  { "Dev Rules", labelActionHandler, menuLabelStateHandler, (void*)LABELTYPE_HEADER },
  { "Freecam", toggleActionHandler, menuStateAlwaysEnabledHandler, &gameConfig.drFreecam },
};

// custom map tab menu items
MenuElem_t menuElementsCustomMap[] = {
  { "", labelActionHandler, menuStateHandler_InstalledCustomMaps, (void*)LABELTYPE_HEADER },
  { "To play on custom maps you must first go to", labelActionHandler, menuLabelStateHandler, (void*)LABELTYPE_LABEL },
  { "rac-horizon.com and download the maps.", labelActionHandler, menuLabelStateHandler, (void*)LABELTYPE_LABEL },
  { "Then install the map files onto a USB drive", labelActionHandler, menuLabelStateHandler, (void*)LABELTYPE_LABEL },
  { "and insert it into your PS2.", labelActionHandler, menuLabelStateHandler, (void*)LABELTYPE_LABEL },
  { "Finally install the custom maps modules here.", labelActionHandler, menuLabelStateHandler, (void*)LABELTYPE_LABEL },
  { "Install custom map modules", buttonActionHandler, menuStateHandler_InstallCustomMaps, mapsSelectHandler },
  //{ "Check for map updates", buttonActionHandler, menuStateHandler_CheckForUpdatesCustomMaps, downloadMapUpdatesSelectHandler },
};

#if MAPEDITOR

extern int mapEditorState;
extern int mapEditorRespawnState;

// map editor enabled list item
MenuElem_ListData_t dataMapEditor = {
    &mapEditorState,
    NULL,
    2,
    {
      "Off",
      "Spawn Points",
    }
};

// map editor tab menu items
MenuElem_t menuElementsMapEditor[] = {
  { "Enabled", listActionHandler, menuStateAlwaysEnabledHandler, &dataMapEditor },
  { "Always Respawn", toggleActionHandler, menuStateHandler_MapEditorSpawnPoints, &mapEditorRespawnState },
};
#endif

// tab items
TabElem_t tabElements[] = {
  { "General", tabDefaultStateHandler, menuElementsGeneral, sizeof(menuElementsGeneral)/sizeof(MenuElem_t) },
  { "Free Cam", tabFreecamStateHandler, menuElementsFreecam, sizeof(menuElementsFreecam)/sizeof(MenuElem_t) },
#if TWEAKERS
  { "Character", tabDefaultStateHandler, menuElementsCharacter, sizeof(menuElementsCharacter)/sizeof(MenuElem_t) },
#endif
  { "Game Settings", tabGameSettingsStateHandler, menuElementsGameSettings, sizeof(menuElementsGameSettings)/sizeof(MenuElem_t) },
  { "Custom Maps", tabCustomMapStateHandler, menuElementsCustomMap, sizeof(menuElementsCustomMap)/sizeof(MenuElem_t) },
#if MAPEDITOR
  { "Map Editor", tabDefaultStateHandler, menuElementsMapEditor, sizeof(menuElementsMapEditor)/sizeof(MenuElem_t) },
#endif
};

const int tabsCount = sizeof(tabElements)/sizeof(TabElem_t);


// 
void tabDefaultStateHandler(TabElem_t* tab, int * state)
{
  *state = ELEMENT_SELECTABLE | ELEMENT_VISIBLE | ELEMENT_EDITABLE;
}

// 
void tabFreecamStateHandler(TabElem_t* tab, int * state)
{
  if (gameConfig.drFreecam && isInGame())
    *state = ELEMENT_SELECTABLE | ELEMENT_VISIBLE | ELEMENT_EDITABLE;
  else
    *state = ELEMENT_HIDDEN;
}

// 
void tabGameSettingsStateHandler(TabElem_t* tab, int * state)
{

#if COMP

  GameSettings * gameSettings = gameGetSettings();
  if (!gameSettings)
  {
    *state = ELEMENT_VISIBLE;
  }
  else
  {
    // don't let users change anything in COMP mode
    *state = ELEMENT_SELECTABLE | ELEMENT_VISIBLE;
  }

#else

  GameSettings * gameSettings = gameGetSettings();
  if (!gameSettings)
  {
    *state = ELEMENT_VISIBLE;
  }
#if !DEBUG
  // if game has started or not the host, disable editing
  else if (gameSettings->GameLoadStartTime > 0 || *(u8*)0x00172170 != 0)
  {
    *state = ELEMENT_SELECTABLE | ELEMENT_VISIBLE;
  }
#endif
  else
  {
    *state = ELEMENT_SELECTABLE | ELEMENT_VISIBLE | ELEMENT_EDITABLE;
  }
#endif

}

// 
void tabCustomMapStateHandler(TabElem_t* tab, int * state)
{
  if (isInGame())
  {
    *state = ELEMENT_VISIBLE;
  }
  else
  {
    *state = ELEMENT_SELECTABLE | ELEMENT_VISIBLE | ELEMENT_EDITABLE;
  }
}

#ifdef DEBUG

// 
void downloadPatchSelectHandler(TabElem_t* tab, MenuElem_t* element)
{
  // close menu
  configMenuDisable();

  // send request
  void * lobbyConnection = netGetLobbyServerConnection();
  if (lobbyConnection)
    netSendCustomAppMessage(NET_DELIVERY_CRITICAL, lobbyConnection, NET_LOBBY_CLIENT_INDEX, CUSTOM_MSG_ID_CLIENT_REQUEST_PATCH, 0, (void*)element);
}

// 
void downloadBootElfSelectHandler(TabElem_t* tab, MenuElem_t* element)
{
  ClientRequestBootElf_t request;
  request.BootElfId = 1;
  
  // close menu
  configMenuDisable();

  // send request
  void * lobbyConnection = netGetLobbyServerConnection();
  if (lobbyConnection)
    netSendCustomAppMessage(NET_DELIVERY_CRITICAL, lobbyConnection, NET_LOBBY_CLIENT_INDEX, CUSTOM_MSG_ID_CLIENT_REQUEST_BOOT_ELF, sizeof(ClientRequestBootElf_t), &request);
}

#endif

#ifdef MAPEDITOR

// 
void menuStateHandler_MapEditorSpawnPoints(TabElem_t* tab, MenuElem_t* element, int* state)
{
  if (mapEditorState == 1)
    *state = ELEMENT_VISIBLE | ELEMENT_EDITABLE | ELEMENT_SELECTABLE;
  else
    *state = ELEMENT_HIDDEN;
}

#endif

// 
void mapsSelectHandler(TabElem_t* tab, MenuElem_t* element)
{
  // 
  if (!isInMenus())
    return;

  // close menu
  configMenuDisable();

  // 
  if (mapsPromptEnableCustomMaps() < 0)
  {
    uiShowOkDialog("Custom Maps", "Error. Not enough memory.");
  }
}

// 
void gmResetSelectHandler(TabElem_t* tab, MenuElem_t* element)
{
  preset = 0;
  memset(&gameConfig, 0, sizeof(gameConfig));
}

// 
void downloadMapUpdatesSelectHandler(TabElem_t* tab, MenuElem_t* element)
{
  ClientRequestBootElf_t request;
  request.BootElfId = 1;
  
  // close menu
  configMenuDisable();

  // send request
  void * lobbyConnection = netGetLobbyServerConnection();
  if (lobbyConnection)
    netSendCustomAppMessage(NET_DELIVERY_CRITICAL, lobbyConnection, NET_LOBBY_CLIENT_INDEX, CUSTOM_MSG_ID_CLIENT_REQUEST_BOOT_ELF, sizeof(ClientRequestBootElf_t), &request);
}

//------------------------------------------------------------------------------
void menuStateAlwaysHiddenHandler(TabElem_t* tab, MenuElem_t* element, int* state)
{
  *state = ELEMENT_HIDDEN;
}

// 
void menuStateAlwaysDisabledHandler(TabElem_t* tab, MenuElem_t* element, int* state)
{
  *state = ELEMENT_VISIBLE | ELEMENT_SELECTABLE;
}

// 
void menuStateAlwaysEnabledHandler(TabElem_t* tab, MenuElem_t* element, int* state)
{
  *state = ELEMENT_VISIBLE | ELEMENT_EDITABLE | ELEMENT_SELECTABLE;
}

// 
void menuLabelStateHandler(TabElem_t* tab, MenuElem_t* element, int* state)
{
  *state = ELEMENT_VISIBLE | ELEMENT_EDITABLE;
}

// 
void menuStateHandler_InstallCustomMaps(TabElem_t* tab, MenuElem_t* element, int* state)
{
  *state = isInMenus() && mapsGetInstallationResult() == 0 ? (ELEMENT_VISIBLE | ELEMENT_EDITABLE | ELEMENT_SELECTABLE) : ELEMENT_HIDDEN;
}

// 
void menuStateHandler_CheckForUpdatesCustomMaps(TabElem_t* tab, MenuElem_t* element, int* state)
{
  *state = isInMenus() && uiGetActive() == UI_ID_ONLINE_MAIN_MENU && mapsGetInstallationResult() == 1 ? (ELEMENT_VISIBLE | ELEMENT_EDITABLE | ELEMENT_SELECTABLE) : ELEMENT_HIDDEN;
}

// 
void menuStateHandler_InstalledCustomMaps(TabElem_t* tab, MenuElem_t* element, int* state)
{
  *state = ELEMENT_VISIBLE | ELEMENT_EDITABLE;
  
  int installResult = mapsGetInstallationResult();
  switch (installResult)
  {
    case 1:
    {
      strncpy(element->name, "Custom map modules installed", 40);
      break;
    }
    case 2:
    {
      strncpy(element->name, "There are custom map updates available", 40);
      break;
    }
    case 255:
    {
      strncpy(element->name, "Error installing custom map modules", 40);
      break;
    }
    default:
    {
      *state = ELEMENT_HIDDEN;
      break;
    }
  }
}

// 
int menuStateHandler_SelectedMapOverride(MenuElem_ListData_t* listData, char* value)
{
  int i;
  if (!value)
    return 0;

  char gm = gameConfig.customModeId;
  char v = *value;

  switch (gm)
  {
    case CUSTOM_MODE_SURVIVAL:
    {
#if DEBUG
      return 1;
#endif
      if (v >= CUSTOM_MAP_SURVIVAL_START && v <= CUSTOM_MAP_SURVIVAL_END)
        return 1;

      *value = CUSTOM_MAP_SURVIVAL_START;
      return 0;
    }
    case CUSTOM_MODE_SEARCH_AND_DESTROY:
    {
      // supported custom maps
      switch (v)
      {
        case CUSTOM_MAP_BAKISI_ISLES:
        case CUSTOM_MAP_CANAL_CITY:
        case CUSTOM_MAP_GHOST_HANGAR:
        case CUSTOM_MAP_GHOST_SHIP:
        case CUSTOM_MAP_HOVEN_GORGE:
        case CUSTOM_MAP_KORGON_OUTPOST:
        case CUSTOM_MAP_METROPOLIS_MP:
        case CUSTOM_MAP_MINING_FACILITY_SP:
        case CUSTOM_MAP_SHAAR_SP:
        case CUSTOM_MAP_SNIVELAK:
        case CUSTOM_MAP_TORVAL_LOST_FACTORY:
        case CUSTOM_MAP_TORVAL_SP:
        case CUSTOM_MAP_TYHRRANOSIS:
          return 1;
      }

      *value = CUSTOM_MAP_NONE;
      return 0;
    }
    case CUSTOM_MODE_PAYLOAD:
    {
      if (v == CUSTOM_MAP_SNIVELAK || v == CUSTOM_MAP_NONE)
        return 1;

      *value = CUSTOM_MAP_SNIVELAK;
      return 0;
    }
    case CUSTOM_MODE_TRAINING:
    {
      *value = CUSTOM_MAP_NONE;
      return 0;
    }
    default:
    {
#if DEBUG
      return 1;
#endif

      // hide maps with gamemode override
      if (gm > CUSTOM_MODE_NONE)
      {
        for (i = 0; i < dataCustomMapsWithExclusiveGameModeCount; ++i)
        {
          if (v == dataCustomMapsWithExclusiveGameMode[i])
          {
            *value = CUSTOM_MAP_NONE;
            return 0;
          }
        }
      }

      if (v < CUSTOM_MAP_SURVIVAL_START)
        return 1;
      
      *value = CUSTOM_MAP_NONE;
      return 0;
    }
  }
}

// 
void menuStateHandler_GameModeOverride(TabElem_t* tab, MenuElem_t* element, int* state)
{
  int i = 0;

  // hide gamemode for maps with exclusive gamemode
#if !DEBUG
  for (i = 0; i < dataCustomMapsWithExclusiveGameModeCount; ++i)
  {
    if (gameConfig.customMapId == dataCustomMapsWithExclusiveGameMode[i])
    {
      *state = ELEMENT_HIDDEN;
      return;
    }
  }
#endif

  *state = ELEMENT_SELECTABLE | ELEMENT_VISIBLE | ELEMENT_EDITABLE;
}

// 
int menuStateHandler_SelectedGameModeOverride(MenuElem_ListData_t* listData, char* value)
{
  if (!value)
    return 0;

  GameSettings* gs = gameGetSettings();
  char v = *value;

  if (gs)
  {
    switch (v)
    {
      case CUSTOM_MODE_INFECTED:
      case CUSTOM_MODE_GUN_GAME:
      //case CUSTOM_MODE_INFINITE_CLIMBER:
      case CUSTOM_MODE_1000_KILLS:
      case CUSTOM_MODE_SURVIVAL:
      case CUSTOM_MODE_PAYLOAD:
      {
        if (gs->GameRules == GAMERULE_DM)
          return 1;
        
        *value = CUSTOM_MODE_NONE;
        return 0;
      }
      case CUSTOM_MODE_SEARCH_AND_DESTROY:
      {
        if (gs->GameRules == GAMERULE_CQ)
          return 1;

        *value = CUSTOM_MODE_NONE;
        return 0;
      }
      case CUSTOM_MODE_TRAINING:
      {
        if (gs->GameRules == GAMERULE_DM || gs->GameRules == GAMERULE_KOTH)
          return 1;

        *value = CUSTOM_MODE_NONE;
        return 0;
      }
#if DEV
      case CUSTOM_MODE_GRIDIRON:
      case CUSTOM_MODE_TEAM_DEFENDER:
      {
        if (gs->GameRules == GAMERULE_CTF)
          return 1;
          
        *value = CUSTOM_MODE_NONE;
        return 0;
      }
#endif
    }
  }

  return 1;
}

// 
int menuStateHandler_SelectedTrainingTypeOverride(MenuElem_ListData_t* listData, char* value)
{
  if (!value)
    return 0;

  GameSettings* gs = gameGetSettings();
  char v = *value;

  if (gs)
  {
    switch (gs->GameRules)
    {
      case GAMERULE_DM:
      {
        if (v == TRAINING_TYPE_FUSION)
          return 1;

        *value = TRAINING_TYPE_FUSION;
        return 0;
      }
      case GAMERULE_KOTH:
      {
        if (v == TRAINING_TYPE_CYCLE)
          return 1;

        *value = TRAINING_TYPE_CYCLE;
        return 0;
      }
    }
  }

  return 0;
}

// 
void menuStateHandler_SurvivalSettingStateHandler(TabElem_t* tab, MenuElem_t* element, int* state)
{
  if (gameConfig.customModeId != CUSTOM_MODE_SURVIVAL)
    *state = ELEMENT_HIDDEN;
  else
    *state = ELEMENT_SELECTABLE | ELEMENT_VISIBLE | ELEMENT_EDITABLE;
}

// 
void menuStateHandler_PayloadSettingStateHandler(TabElem_t* tab, MenuElem_t* element, int* state)
{
  if (gameConfig.customModeId != CUSTOM_MODE_PAYLOAD)
    *state = ELEMENT_HIDDEN;
  else
    *state = ELEMENT_SELECTABLE | ELEMENT_VISIBLE | ELEMENT_EDITABLE;
}

// 
void menuStateHandler_TrainingSettingStateHandler(TabElem_t* tab, MenuElem_t* element, int* state)
{
  GameSettings* gs = gameGetSettings();

  if (gameConfig.customModeId != CUSTOM_MODE_TRAINING)
    *state = ELEMENT_HIDDEN;
  else
    *state = ELEMENT_SELECTABLE | ELEMENT_VISIBLE | ELEMENT_EDITABLE;
}

// 
void menuStateHandler_CTFSettingStateHandler(TabElem_t* tab, MenuElem_t* element, int* state)
{
  GameSettings* gs = gameGetSettings();

  if (!gs || gs->GameRules != GAMERULE_CTF)
    *state = ELEMENT_HIDDEN;
  else if (preset)
    *state = ELEMENT_SELECTABLE | ELEMENT_VISIBLE;
  else
    *state = ELEMENT_SELECTABLE | ELEMENT_VISIBLE | ELEMENT_EDITABLE;
}

// 
void menuStateHandler_CQSettingStateHandler(TabElem_t* tab, MenuElem_t* element, int* state)
{
  GameSettings* gs = gameGetSettings();

  if (!gs || gs->GameRules != GAMERULE_CQ)
    *state = ELEMENT_HIDDEN;
  else if (preset)
    *state = ELEMENT_SELECTABLE | ELEMENT_VISIBLE;
  else
    *state = ELEMENT_SELECTABLE | ELEMENT_VISIBLE | ELEMENT_EDITABLE;
}

// 
void menuStateHandler_KOTHSettingStateHandler(TabElem_t* tab, MenuElem_t* element, int* state)
{
  GameSettings* gs = gameGetSettings();

  if (!gs || gs->GameRules != GAMERULE_KOTH)
    *state = ELEMENT_HIDDEN;
  else if (preset)
    *state = ELEMENT_SELECTABLE | ELEMENT_VISIBLE;
  else
    *state = ELEMENT_SELECTABLE | ELEMENT_VISIBLE | ELEMENT_EDITABLE;
}

// 
void menuStateHandler_SettingStateHandler(TabElem_t* tab, MenuElem_t* element, int* state)
{
  if (preset)
    *state = ELEMENT_SELECTABLE | ELEMENT_VISIBLE;
  else
    *state = ELEMENT_SELECTABLE | ELEMENT_VISIBLE | ELEMENT_EDITABLE;
}

int getMenuElementState(TabElem_t* tab, MenuElem_t* element)
{
  // get tab and element state
  int tabState = 0, state = 0;
  tab->stateHandler(tab, &tabState);
  element->stateHandler(tab, element, &state);

  return tabState & state;
}

//------------------------------------------------------------------------------
void drawToggleMenuElement(TabElem_t* tab, MenuElem_t* element, RECT* rect)
{
  // get element state
  int state = getMenuElementState(tab, element);

  float x,y;
  float lerp = (state & ELEMENT_EDITABLE) ? 0.0 : 0.5;
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
void drawToggleInvertedMenuElement(TabElem_t* tab, MenuElem_t* element, RECT* rect)
{
  // get element state
  int state = getMenuElementState(tab, element);

  float x,y;
  float lerp = (state & ELEMENT_EDITABLE) ? 0.0 : 0.5;
  u32 color = colorLerp(colorText, 0, lerp);

  // draw name
  x = (rect->TopLeft[0] * SCREEN_WIDTH) + 5;
  y = (rect->TopLeft[1] * SCREEN_HEIGHT) + 5;
  gfxScreenSpaceText(x, y, 1, 1, color, element->name, -1, 0);

  // draw value
  x = (rect->TopRight[0] * SCREEN_WIDTH) - 5;
  gfxScreenSpaceText(x, y, 1, 1, color, *(char*)element->userdata ? "Off" : "On", -1, 2);
}

//------------------------------------------------------------------------------
void drawRangeMenuElement(TabElem_t* tab, MenuElem_t* element, MenuElem_RangeData_t * rangeData, RECT* rect)
{
  char buf[32];

  // get element state
  int state = getMenuElementState(tab, element);

  float x,y,w,v,h;
  float lerp = (state & ELEMENT_EDITABLE) ? 0.0 : 0.5;
  u32 color = colorLerp(colorText, 0, lerp);

  // draw name
  x = (rect->TopLeft[0] * SCREEN_WIDTH) + 5;
  y = (rect->TopLeft[1] * SCREEN_HEIGHT) + 5;
  gfxScreenSpaceText(x, y, 1, 1, color, element->name, -1, 0);

  // draw box
  u32 barColor = colorLerp(colorRangeBar, 0, lerp);
  v = (float)(*rangeData->value - rangeData->minValue) / (float)(rangeData->maxValue - rangeData->minValue);
  w = (rect->TopRight[0] - rect->TopLeft[0]) * 0.5 * SCREEN_WIDTH;
  x = (rect->TopRight[0] * SCREEN_WIDTH) - w - 5;
  h = (rect->BottomRight[1] - rect->TopRight[1]) * SCREEN_HEIGHT;
  gfxPixelSpaceBox(x, y - 4, w * v, h - 2, barColor);

  // draw name
  sprintf(buf, "%d", *rangeData->value);
  x = (rect->TopRight[0] * SCREEN_WIDTH) - 5;
  gfxScreenSpaceText(x, y, 1, 1, color, buf, -1, 2);
}

//------------------------------------------------------------------------------
void drawListMenuElement(TabElem_t* tab, MenuElem_t* element, MenuElem_ListData_t * listData, RECT* rect)
{
  // get element state
  int state = getMenuElementState(tab, element);

  int selectedIdx = (int)*listData->value;
  if (selectedIdx < 0)
    selectedIdx = 0;
  
  float x,y;
  float lerp = (state & ELEMENT_EDITABLE) ? 0.0 : 0.5;
  u32 color = colorLerp(colorText, 0, lerp);

  // draw name
  x = (rect->TopLeft[0] * SCREEN_WIDTH) + 5;
  y = (rect->TopLeft[1] * SCREEN_HEIGHT) + 5;
  gfxScreenSpaceText(x, y, 1, 1, color, element->name, -1, 0);

  // draw value
  x = (rect->TopRight[0] * SCREEN_WIDTH) - 5;
  gfxScreenSpaceText(x, y, 1, 1, color, listData->items[selectedIdx], -1, 2);
}

//------------------------------------------------------------------------------
void drawButtonMenuElement(TabElem_t* tab, MenuElem_t* element, RECT* rect)
{
  // get element state
  int state = getMenuElementState(tab, element);

  float x,y,b = 0.005;
  float lerp = (state & ELEMENT_EDITABLE) ? 0.0 : 0.5;
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
void drawLabelMenuElement(TabElem_t* tab, MenuElem_t* element, RECT* rect)
{
  // get element state
  int state = getMenuElementState(tab, element);

  float x,y;
  float lerp = (state & ELEMENT_EDITABLE) ? 0.0 : 0.5;

  // draw label
  x = 0.5 * SCREEN_WIDTH;
  y = ((rect->TopLeft[1] + rect->BottomLeft[1]) * SCREEN_HEIGHT * 0.5);
  gfxScreenSpaceText(x, y, 1, 1, colorLerp(colorText, 0, lerp), element->name, -1, 4);
}

//------------------------------------------------------------------------------
void buttonActionHandler(TabElem_t* tab, MenuElem_t* element, int actionType, void * actionArg)
{
  // get element state
  int state = getMenuElementState(tab, element);

  // do nothing if hidden
  if ((state & ELEMENT_VISIBLE) == 0)
    return;

  switch (actionType)
  {
    case ACTIONTYPE_SELECT:
    {
      if ((state & ELEMENT_EDITABLE) == 0)
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
void labelActionHandler(TabElem_t* tab, MenuElem_t* element, int actionType, void * actionArg)
{
  // get element state
  int state = getMenuElementState(tab, element);

  // do nothing if hidden
  if ((state & ELEMENT_VISIBLE) == 0)
    return;

  switch (actionType)
  {
    case ACTIONTYPE_GETHEIGHT:
    {
      switch ((int)element->userdata)
      {
        case LABELTYPE_LABEL:
        {
          *(float*)actionArg = LINE_HEIGHT * 0.75;
          break;
        }
        default:
        {
          *(float*)actionArg = LINE_HEIGHT * 2;
          break;
        }
      }
      break;
    }
    case ACTIONTYPE_DRAW:
    {
      drawLabelMenuElement(tab, element, (RECT*)actionArg);
      break;
    }
  }
}

//------------------------------------------------------------------------------
void rangeActionHandler(TabElem_t* tab, MenuElem_t* element, int actionType, void * actionArg)
{
  MenuElem_RangeData_t* rangeData = (MenuElem_RangeData_t*)element->userdata;

  // get element state
  int state = getMenuElementState(tab, element);

  // do nothing if hidden
  if ((state & ELEMENT_VISIBLE) == 0)
    return;

  switch (actionType)
  {
    case ACTIONTYPE_INCREMENT:
    case ACTIONTYPE_SELECT:
    {
      if ((state & ELEMENT_EDITABLE) == 0)
        break;
      char newValue = *rangeData->value + 1;
      if (newValue > rangeData->maxValue)
        newValue = rangeData->minValue;

      *rangeData->value = newValue;
      break;
    }
    case ACTIONTYPE_SELECT_SECONDARY:
    {
      *rangeData->value = (rangeData->minValue + rangeData->maxValue) / 2;
      break;
    }
    case ACTIONTYPE_DECREMENT:
    {
      if ((state & ELEMENT_EDITABLE) == 0)
        break;
      char newValue = *rangeData->value - 1;
      if (newValue < rangeData->minValue)
        newValue = rangeData->maxValue;

      *rangeData->value = newValue;
      break;
    }
    case ACTIONTYPE_GETHEIGHT:
    {
      *(float*)actionArg = LINE_HEIGHT;
      break;
    }
    case ACTIONTYPE_DRAW:
    {
      drawRangeMenuElement(tab, element, rangeData, (RECT*)actionArg);
      break;
    }
    case ACTIONTYPE_VALIDATE:
    {
      if (rangeData->stateHandler != NULL)
        rangeData->stateHandler(rangeData, rangeData->value);
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
  if ((state & ELEMENT_VISIBLE) == 0)
    return;

  switch (actionType)
  {
    case ACTIONTYPE_INCREMENT:
    case ACTIONTYPE_SELECT:
    {
      if ((state & ELEMENT_EDITABLE) == 0)
        break;
      char newValue = *listData->value;

      do
      {
        newValue += 1;
        if (newValue >= itemCount)
          newValue = 0;
        char tValue = newValue;
        if (listData->stateHandler == NULL || listData->stateHandler(listData, &tValue))
          break;
      } while (newValue != *listData->value);

      *listData->value = newValue;
      break;
    }
    case ACTIONTYPE_DECREMENT:
    {
      if ((state & ELEMENT_EDITABLE) == 0)
        break;
      char newValue = *listData->value;

      do
      {
        newValue -= 1;
        if (newValue < 0)
          newValue = itemCount - 1;
        char tValue = newValue;
        if (listData->stateHandler == NULL || listData->stateHandler(listData, &tValue))
          break;
      } while (newValue != *listData->value);

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
    case ACTIONTYPE_VALIDATE:
    {
      if (listData->stateHandler != NULL)
        listData->stateHandler(listData, listData->value);
      break;
    }
  }
}

void gmOverrideListActionHandler(TabElem_t* tab, MenuElem_t* element, int actionType, void * actionArg)
{
  // update name to be based on current gamemode
  GameSettings* gs = gameGetSettings();
  if (gs && actionType == ACTIONTYPE_DRAW)
    snprintf(element->name, 40, "%s override", gameGetGameModeName(gs->GameRules));

  // pass to default list action handler
  listActionHandler(tab, element, actionType, actionArg);
}

//------------------------------------------------------------------------------
void toggleInvertedActionHandler(TabElem_t* tab, MenuElem_t* element, int actionType, void * actionArg)
{
  // get element state
  int state = getMenuElementState(tab, element);

  // do nothing if hidden
  if ((state & ELEMENT_VISIBLE) == 0)
    return;

  switch (actionType)
  {
    case ACTIONTYPE_INCREMENT:
    case ACTIONTYPE_SELECT:
    case ACTIONTYPE_DECREMENT:
    {
      if ((state & ELEMENT_EDITABLE) == 0)
        break;

      // toggle
      *(char*)element->userdata = !(*(char*)element->userdata);
      break;
    }
    case ACTIONTYPE_GETHEIGHT:
    {
      *(float*)actionArg = LINE_HEIGHT;
      break;
    }
    case ACTIONTYPE_DRAW:
    {
      drawToggleInvertedMenuElement(tab, element, (RECT*)actionArg);
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
  if ((state & ELEMENT_VISIBLE) == 0)
    return;

  switch (actionType)
  {
    case ACTIONTYPE_INCREMENT:
    case ACTIONTYPE_SELECT:
    case ACTIONTYPE_DECREMENT:
    {
      if ((state & ELEMENT_EDITABLE) == 0)
        break;

      // toggle
      *(char*)element->userdata = !(*(char*)element->userdata);
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
    if (state & ELEMENT_VISIBLE)
    {
      // get tab title width
      float pWidth = (4 * tabBarPaddingX) + gfxGetFontWidth(tab->name, -1, 1) / (float)SCREEN_WIDTH;

      // get color
      float lerp = state & ELEMENT_EDITABLE ? 0.0 : 0.5;
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

  int i = 0, state = 0;
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
      state = getMenuElementState(tab, currentElement);
      if (state & ELEMENT_SELECTABLE) {
        gfxScreenSpaceQuad(&drawRect, colorSelected, colorSelected, colorSelected, colorSelected);
      }
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
  state = getMenuElementState(tab, currentElement);

  // find next selectable item if hidden or not selectable
  if ((state & ELEMENT_VISIBLE) == 0 || (state & ELEMENT_SELECTABLE) == 0)
    navMenu(tab, 1, 1);

  // nav down
  if (padGetButtonUp(0, PAD_DOWN) > 0)
  {
    navMenu(tab, 1, 0);
  }
  // nav page down
  if (padGetButtonUp(0, PAD_R2) > 0)
  {
    for (i = 0; i < 10; ++i)
      navMenu(tab, 1, 0);
  }
  // nav up
  else if (padGetButtonUp(0, PAD_UP) > 0)
  {
    navMenu(tab, -1, 0);
  }
  // nav up
  else if (padGetButtonUp(0, PAD_L2) > 0)
  {
    for (i = 0; i < 10; ++i)
      navMenu(tab, -1, 0);
  }
  // nav select
  else if (padGetButtonDown(0, PAD_CROSS) > 0)
  {
    if (state & ELEMENT_EDITABLE)
      currentElement->handler(tab, currentElement, ACTIONTYPE_SELECT, NULL);
  }
  // nav select secondary
  else if (padGetButtonDown(0, PAD_SQUARE) > 0)
  {
    if (state & ELEMENT_EDITABLE)
      currentElement->handler(tab, currentElement, ACTIONTYPE_SELECT_SECONDARY, NULL);
  }
  // nav inc
  else if (padGetButtonUp(0, PAD_RIGHT) > 0)
  {
    if (state & ELEMENT_EDITABLE)
      currentElement->handler(tab, currentElement, ACTIONTYPE_INCREMENT, NULL);
  }
  // nav dec
  else if (padGetButtonUp(0, PAD_LEFT) > 0)
  {
    if (state & ELEMENT_EDITABLE)
      currentElement->handler(tab, currentElement, ACTIONTYPE_DECREMENT, NULL);
  }
}

//------------------------------------------------------------------------------
void onMenuUpdate(int inGame)
{
  char buf[16];
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

      // draw ping overlay
      if (gameGetSettings())
      {
        int ping = gameGetPing();
        sprintf(buf, "%d ms", ping);
        gfxScreenSpaceText(0.9 * SCREEN_WIDTH, 0.1 * SCREEN_HEIGHT, 1, 1, 0x808080FF, buf, -1, 2);
      }
    }

    // nav tab right
    if (padGetButtonUp(0, PAD_R1) > 0)
    {
      navTab(1);
    }
    // nav tab left
    else if (padGetButtonUp(0, PAD_L1) > 0)
    {
      navTab(-1);
    }
    // close
    else if (padGetButtonUp(0, PAD_TRIANGLE) > 0 || padGetButtonUp(0, PAD_START) > 0)
    {
      configMenuDisable();
    }
  }
  else if (!inGame && !mapsDownloadingModules() && netGetLobbyServerConnection())
  {
    if (uiGetActive() == UI_ID_ONLINE_MAIN_MENU)
    {
      // render message
      gfxScreenSpaceBox(0.1, 0.75, 0.4, 0.05, colorOpenBg);
      gfxScreenSpaceText(SCREEN_WIDTH * 0.3, SCREEN_HEIGHT * 0.775, 1, 1, 0x80FFFFFF, "\x1f Open Config Menu", -1, 4);
    }

		// check for pad input
		if (padGetButtonUp(0, PAD_START) > 0)
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
  if (gameSettings && gameSettings->GameLoadStartTime < 0 && netGetLobbyServerConnection())
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

      // set map name to training type
      if (gameConfig.customModeId == CUSTOM_MODE_TRAINING) {
        mapName = dataTrainingType.items[gameConfig.trainingConfig.type];
      }
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

	  u32 * stagingUiElements = (u32*)(*(u32*)(0x011C7064 + 4*33) + 0xB0);
	  u32 * stagingDetailsUiElements = (u32*)(*(u32*)(0x011C7064 + 4*34) + 0xB0);

    // update ui strings
    if ((u32)stagingUiElements > 0x100000)
    {
      strncpy((char*)(stagingUiElements[3] + 0x60), mapName, 32);
      strncpy((char*)(stagingUiElements[4] + 0x60), modeName, 32);
    }
    if ((u32)stagingDetailsUiElements > 0x100000)
    {
      strncpy((char*)(stagingDetailsUiElements[2] + 0x60), mapName, 32);
      strncpy((char*)(stagingDetailsUiElements[3] + 0x60), modeName, 32);
    }
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
      if (loop && tab->selectedMenuItem != 0)
        newElement = 0;
      else
        break;
    }
    else if (newElement < 0)
    {
      if (loop && tab->selectedMenuItem != (tab->elementsCount - 1))
        newElement = tab->elementsCount - 1;
      else
        break;
    }

    // get newly selected element state
    elem = &tab->elements[newElement];
    elem->stateHandler(tab, elem, &state);

    // skip if hidden
    if ((state & ELEMENT_VISIBLE) == 0 || (state & ELEMENT_SELECTABLE) == 0)
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
    if ((state & ELEMENT_VISIBLE) == 0 || (state & ELEMENT_SELECTABLE) == 0)
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
int onServerDownloadDataRequest(void * connection, void * data)
{
	ServerDownloadDataRequest_t* request = (ServerDownloadDataRequest_t*)data;

	// copy bytes to target
  dlIsActive = request->Id;
	dlTotalBytes = request->TotalSize;
	dlBytesReceived += request->DataSize;
	memcpy((void*)request->TargetAddress, request->Data, request->DataSize);
	DPRINTF("DOWNLOAD: %d/%d, writing %d to %08X\n", dlBytesReceived, request->TotalSize, request->DataSize, request->TargetAddress);
  
	// respond
	if (connection)
	{
		ClientDownloadDataResponse_t response;
		response.Id = request->Id;
		response.BytesReceived = dlBytesReceived;
		netSendCustomAppMessage(NET_DELIVERY_CRITICAL, connection, NET_LOBBY_CLIENT_INDEX, CUSTOM_MSG_ID_CLIENT_DOWNLOAD_DATA_RESPONSE, sizeof(ClientDownloadDataResponse_t), &response);
	}

  // reset at end
  if (dlBytesReceived >= request->TotalSize)
  {
    dlTotalBytes = 0;
    dlBytesReceived = 0;
    dlIsActive = 0;
    //*(u32*)0x00167F54 = 1000 * 3;
  }
  else
  {
    //*(u32*)0x00167F54 = 1000 * 15;
  }

	return sizeof(ServerDownloadDataRequest_t) - sizeof(request->Data) + request->DataSize;
}

//------------------------------------------------------------------------------
int onSetGameConfig(void * connection, void * data)
{
  PatchGameConfig_t config;

  // move to temporary object
  memcpy(&config, data, sizeof(PatchGameConfig_t));

  // check for changes
  int mapChanged = config.customMapId != gameConfig.customMapId;
  redownloadCustomModeBinaries |= config.customModeId != gameConfig.customModeId;

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
  // draw download data box
	if (dlTotalBytes > 0)
	{
    // reset when we lose connection
    if (!netGetLobbyServerConnection() || !dlIsActive)
    {
      dlTotalBytes = 0;
      dlBytesReceived = 0;
      dlIsActive = 0;
      DPRINTF("lost connection\n");
    }

    gfxScreenSpaceBox(0.2, 0.35, 0.6, 0.125, colorBlack);
    gfxScreenSpaceBox(0.2, 0.45, 0.6, 0.05, colorContentBg);
    gfxScreenSpaceText(SCREEN_WIDTH * 0.4, SCREEN_HEIGHT * 0.4, 1, 1, colorText, "Downloading...", 11 + (gameGetTime()/240 % 4), 3);

		float w = (float)dlBytesReceived / (float)dlTotalBytes;
		gfxScreenSpaceBox(0.2, 0.45, 0.6 * w, 0.05, colorRed);
	}

  onMenuUpdate(0);
}

//------------------------------------------------------------------------------
void onConfigInitialize(void)
{
	// install net handlers
  *(u32*)0x00211E64 = 0; // reset net callbacks
	netInstallCustomMsgHandler(CUSTOM_MSG_ID_SERVER_SET_GAME_CONFIG, &onSetGameConfig);
  netInstallCustomMsgHandler(CUSTOM_MSG_ID_SERVER_DOWNLOAD_DATA_REQUEST, &onServerDownloadDataRequest);

  // reset game configs
  memset(&gameConfigHostBackup, 0, sizeof(gameConfigHostBackup));
  memset(&gameConfig, 0, sizeof(gameConfig));

  // set defaults
  //gameConfigHostBackup.survivalConfig.difficulty = 4;

#if DEFAULT_GAMEMODE > 0
  gameConfigHostBackup.customModeId = DEFAULT_GAMEMODE;
  gameConfig.customModeId = DEFAULT_GAMEMODE;
#endif
}

//------------------------------------------------------------------------------
void configTrySendGameConfig(void)
{
#if COMP
  // disable changing game config in COMP mode
  return;
#else
  int state = 0;
  int i = 0, j = 0;
  TabElem_t* gameSettingsTab = &tabElements[2];

  // send game config to server for saving if tab is enabled
  gameSettingsTab->stateHandler(gameSettingsTab, &state);
  if (state & ELEMENT_EDITABLE)
  {
    // validate everything
    for (i = 0; i < tabsCount; ++i)
    {
      TabElem_t* tab = &tabElements[i];
      for (j = 0; j < tab->elementsCount; ++j)
      {
        MenuElem_t* elem = &tab->elements[j];
        if (elem->handler)
          elem->handler(tab, elem, ACTIONTYPE_VALIDATE, NULL);
      }
    }

    // backup
    memcpy(&gameConfigHostBackup, &gameConfig, sizeof(PatchGameConfig_t));

    // send
    void * lobbyConnection = netGetLobbyServerConnection();
    if (lobbyConnection)
      netSendCustomAppMessage(NET_DELIVERY_CRITICAL, lobbyConnection, NET_LOBBY_CLIENT_INDEX, CUSTOM_MSG_ID_CLIENT_USER_GAME_CONFIG, sizeof(PatchGameConfig_t), &gameConfig);
  }
#endif

}

//------------------------------------------------------------------------------
void configMenuDisable(void)
{
  if (!isConfigMenuActive)
    return;
  
  isConfigMenuActive = 0;

  // force game config to preset
  switch (preset)
  {
    case 1: // competitive
    {
      gameConfig.grNoHealthBoxes = 0;
      gameConfig.grNoNames = 0;
      gameConfig.grV2s = 0;
      gameConfig.grVampire = 0;

      gameConfig.grBetterFlags = 1;
      gameConfig.grBetterHills = 1;
      gameConfig.grFusionShotsAlwaysHit = 1;
      gameConfig.grOvertime = 1;
      gameConfig.grHalfTime = 1;
      gameConfig.grNoInvTimer = 1;
      gameConfig.grNoPacks = 1;
      gameConfig.grNoPickups = 1;
      gameConfig.grNoSniperHelpers = 1;
      gameConfig.grCqPersistentCapture = 1;
      gameConfig.grCqDisableTurrets = 1;
      gameConfig.grCqDisableUpgrades = 1;
      break;
    }
    case 2: // 1v1
    {
      gameConfig.grNoHealthBoxes = 1;
      gameConfig.grNoNames = 0;
      gameConfig.grV2s = 2;
      gameConfig.grVampire = 3;

      gameConfig.grFusionShotsAlwaysHit = 1;
      gameConfig.grNoInvTimer = 1;
      gameConfig.grNoPacks = 1;
      gameConfig.grNoPickups = 1;
      break;
    }
  }

  // send config to server for saving
  void * lobbyConnection = netGetLobbyServerConnection();
  if (lobbyConnection)
    netSendCustomAppMessage(NET_DELIVERY_CRITICAL, lobbyConnection, NET_LOBBY_CLIENT_INDEX, CUSTOM_MSG_ID_CLIENT_USER_CONFIG, sizeof(PatchConfig_t), &config);

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
  if ((state & ELEMENT_SELECTABLE) == 0 || (state & ELEMENT_VISIBLE) == 0)
    selectedTabItem = 0;
}
