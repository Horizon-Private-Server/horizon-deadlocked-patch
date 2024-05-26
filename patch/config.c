#include <libdl/pad.h>
#include <libdl/ui.h>
#include <libdl/graphics.h>
#include <libdl/stdio.h>
#include <libdl/net.h>
#include <libdl/color.h>
#include <libdl/gamesettings.h>
#include <libdl/string.h>
#include <libdl/game.h>
#include <libdl/player.h>
#include <libdl/map.h>
#include <libdl/utils.h>
#include "messageid.h"
#include "module.h"
#include "config.h"
#include "include/config.h"

#define LINE_HEIGHT         (0.05)
#define LINE_HEIGHT_3_2     (0.075)
#define DEFAULT_GAMEMODE    (0)
#define CHARACTER_TWEAKER_RANGE (10)
#define DZO_MAX_CMAPS       (10)

// config
extern PatchConfig_t config;

// game config
extern PatchGameConfig_t gameConfig;
extern PatchGameConfig_t gameConfigHostBackup;
extern int selectedMapIdHostBackup;
extern PatchStateContainer_t patchStateContainer;

extern FreecamSettings_t freecamSettings;

extern char aa_value;
extern int redownloadCustomModeBinaries;
extern int scavHuntEnabled;

extern VoteToEndState_t voteToEndState;

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
void orderedListActionHandler(TabElem_t* tab, MenuElem_t* element, int actionType, void * actionArg);
void rangeActionHandler(TabElem_t* tab, MenuElem_t* element, int actionType, void * actionArg);
void gmOverrideListActionHandler(TabElem_t* tab, MenuElem_t* element, int actionType, void * actionArg);
void labelActionHandler(TabElem_t* tab, MenuElem_t* element, int actionType, void * actionArg);

// state handlers
void menuStateAlwaysHiddenHandler(TabElem_t* tab, MenuElem_t* element, int* state);
void menuStateAlwaysDisabledHandler(TabElem_t* tab, MenuElem_t* element, int* state);
void menuStateAlwaysEnabledHandler(TabElem_t* tab, MenuElem_t* element, int* state);
void menuStateEnabledInMenusHandler(TabElem_t* tab, MenuElem_t* element, int* state);
void menuStateDzoEnabledHandler(TabElem_t* tab, MenuElem_t* element, int* state);
void menuStateScavengerHuntEnabledHandler(TabElem_t* tab, MenuElem_t* element, int* state);
void menuLabelStateHandler(TabElem_t* tab, MenuElem_t* element, int* state);
void menuStateHandler_InstallCustomMaps(TabElem_t* tab, MenuElem_t* element, int* state);
void menuStateHandler_CheckForUpdatesCustomMaps(TabElem_t* tab, MenuElem_t* element, int* state);
void menuStateHandler_InstalledCustomMaps(TabElem_t* tab, MenuElem_t* element, int* state);
void menuStateHandler_GameModeOverride(TabElem_t* tab, MenuElem_t* element, int* state);
void menuStateHandler_VoteToEndStateHandler(TabElem_t* tab, MenuElem_t* element, int* state);

int menuStateHandler_SelectedMapOverride(MenuElem_OrderedListData_t* listData, char* value);
int menuStateHandler_SelectedGameModeOverride(MenuElem_OrderedListData_t* listData, char* value);
int menuStateHandler_SelectedTrainingTypeOverride(MenuElem_ListData_t* listData, char* value);
int menuStateHandler_SelectedTrainingAggressionOverride(MenuElem_ListData_t* listData, char* value);

void menuStateHandler_SurvivalSettingStateHandler(TabElem_t* tab, MenuElem_t* element, int* state);
void menuStateHandler_PayloadSettingStateHandler(TabElem_t* tab, MenuElem_t* element, int* state);
void menuStateHandler_TrainingSettingStateHandler(TabElem_t* tab, MenuElem_t* element, int* state);
void menuStateHandler_HnsSettingStateHandler(TabElem_t* tab, MenuElem_t* element, int* state);
void menuStateHandler_CycleTrainingSettingStateHandler(TabElem_t* tab, MenuElem_t* element, int* state);
void menuStateHandler_CTFSettingStateHandler(TabElem_t* tab, MenuElem_t* element, int* state);
void menuStateHandler_KOTHSettingStateHandler(TabElem_t* tab, MenuElem_t* element, int* state);
void menuStateHandler_CQSettingStateHandler(TabElem_t* tab, MenuElem_t* element, int* state);
void menuStateHandler_SettingStateHandler(TabElem_t* tab, MenuElem_t* element, int* state);

void downloadMapUpdatesSelectHandler(TabElem_t* tab, MenuElem_t* element);
void voteToEndSelectHandler(TabElem_t* tab, MenuElem_t* element);

#if MAPEDITOR
void menuStateHandler_MapEditorSpawnPoints(TabElem_t* tab, MenuElem_t* element, int* state);
#endif

void tabDefaultStateHandler(TabElem_t* tab, int * state);
void tabFreecamStateHandler(TabElem_t* tab, int * state);
void tabGameSettingsStateHandler(TabElem_t* tab, int * state);
void tabGameSettingsHelpStateHandler(TabElem_t* tab, int * state);
void tabCustomMapStateHandler(TabElem_t* tab, int * state);

// list select handlers
void mapsSelectHandler(TabElem_t* tab, MenuElem_t* element);
void gmResetSelectHandler(TabElem_t* tab, MenuElem_t* element);
void gmRefreshMapsSelectHandler(TabElem_t* tab, MenuElem_t* element);

#ifdef DEBUG
void downloadPatchSelectHandler(TabElem_t* tab, MenuElem_t* element);
void downloadBootElfSelectHandler(TabElem_t* tab, MenuElem_t* element);
#endif

void navMenu(TabElem_t* tab, int direction, int loop);
void navTab(int direction);

int mapsGetInstallationResult(void);
int mapsPromptEnableCustomMaps(void);
int mapsDownloadingModules(void);
void refreshCustomMapList(void);
void sendClientVoteForEnd(void);

// level of detail list item
MenuElem_ListData_t dataLevelOfDetail = {
  .value = &config.levelOfDetail,
  .stateHandler = NULL,
#if DEBUG
  .count = 4,
#else
  .count = 3,
#endif
  .items = { "Potato", "Low", "Normal", "High" }
};

// framelimiter list item
MenuElem_ListData_t dataFramelimiter = {
  .value = &config.framelimiter,
  .stateHandler = NULL,
  .count = 3,
  .items = { "On", "Auto", "Off" }
};

// minimap scale list item
MenuElem_ListData_t dataMinimapScale = {
  .value = &config.minimapScale,
  .stateHandler = NULL,
  .count = 2,
  .items = { "Normal", "Half" }
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

// game servers
MenuElem_ListData_t dataGameServers = {
  .value = &config.preferredGameServer,
  .stateHandler = NULL,
  .count = 2,
  .items = {
    "US Central",
    "Europe"
  }
};

// player fov range item
MenuElem_RangeData_t dataFieldOfView = {
  .value = &config.playerFov,
  .stateHandler = NULL,
  .minValue = -5,
  .maxValue = 5,
};

// fixed cycle order
MenuElem_ListData_t dataFixedCycleOrder = {
  .value = &config.fixedCycleOrder,
  .stateHandler = NULL,
  .count = FIXED_CYCLE_ORDER_COUNT,
  .items = {
    "Off",
    "Mag -> Fusion -> B6",
    "Mag -> B6 -> Fusion"
  }
};

// general tab menu items
MenuElem_t menuElementsGeneral[] = {
#ifdef DEBUG
  { "Redownload patch", buttonActionHandler, menuStateAlwaysEnabledHandler, downloadPatchSelectHandler },
  { "Download boot elf", buttonActionHandler, menuStateAlwaysEnabledHandler, downloadBootElfSelectHandler },
#endif
  { "Vote to End", buttonActionHandler, menuStateHandler_VoteToEndStateHandler, voteToEndSelectHandler, "Vote to end the game. If a team/player is in the lead they will win." },
  { "Refresh Maps", buttonActionHandler, menuStateEnabledInMenusHandler, gmRefreshMapsSelectHandler },
#if SCAVENGER_HUNT
  { "Participate in Scavenger Hunt", toggleInvertedActionHandler, menuStateScavengerHuntEnabledHandler, &config.disableScavengerHunt, "If you see this option, there is a Horizon scavenger hunt active. Enabling this will spawn random Horizon bolts in game. Collect the most to win the hunt!" },
#endif
  { "Game Server (Host)", listActionHandler, menuStateAlwaysEnabledHandler, &dataGameServers, "Which game server you'd like to use when creating a game." },
  { "16:9 Widescreen", toggleActionHandler, menuStateAlwaysEnabledHandler, (char*)0x00171DEB },
  { "Announcers on all gamemodes", toggleActionHandler, menuStateAlwaysEnabledHandler, &config.enableGamemodeAnnouncements, "Enables Dallas commentary in all games." },
  { "Camera Pulling", toggleInvertedActionHandler, menuStateAlwaysEnabledHandler, &config.disableAimAssist, "Toggles code that pulls the camera towards nearby targets when aiming." },
  { "Camera Shake", toggleInvertedActionHandler, menuStateAlwaysEnabledHandler, &config.disableCameraShake, "Toggles the camera shake caused by nearby explosions." },
  { "Disable \x11 to equip hacker ray", toggleActionHandler, menuStateAlwaysEnabledHandler, &config.disableCircleToHackerRay, "Moves hacker ray into the quickselect menu (secondary select)." },
  { "Field of View", rangeActionHandler, menuStateAlwaysEnabledHandler, &dataFieldOfView },
  { "Fixed Cycle Order", listActionHandler, menuStateAlwaysEnabledHandler, &dataFixedCycleOrder, "If you have equipped the B6, Fusion, Magma the configured cycle order will be forced." },
  { "Fps Counter", toggleActionHandler, menuStateAlwaysEnabledHandler, &config.enableFpsCounter, "Toggles the in game FPS counter." },
  { "Framelimiter", listActionHandler, menuStateAlwaysEnabledHandler, &dataFramelimiter, "If Off (recommended), forces 60 FPS in all games. Otherwise games with 7+ people will run at 30 FPS." },
  { "Fusion Reticle", toggleActionHandler, menuStateAlwaysEnabledHandler, &config.enableFusionReticule, "Toggles the in game fusion reticle. Normally disabled in multiplayer this setting adds it back." },
  { "In Game Scoreboard (L3)", toggleActionHandler, menuStateAlwaysEnabledHandler, &config.enableInGameScoreboard, "Toggles the in game scoreboard. Hold L3 to display." },
  { "Level of Detail", listActionHandler, menuStateAlwaysEnabledHandler, &dataLevelOfDetail, "Configures the level of detail of the scene. Lower this to reduce the graphics requirements on laggy maps/survival." },
  { "Minimap Big Scale", listActionHandler, menuStateAlwaysEnabledHandler, &dataMinimapScale, "Toggles between half and full screen expanded radar." },
  { "Minimap Big Zoom", rangeActionHandler, menuStateAlwaysEnabledHandler, &dataMinimapBigZoom, "Tweaks the expanded radar zoom." },
  { "Minimap Small Zoom", rangeActionHandler, menuStateAlwaysEnabledHandler, &dataMinimapSmallZoom, "Tweaks the minimized radar zoom." },
  { "Progressive Scan", toggleActionHandler, menuStateAlwaysEnabledHandler, (char*)0x0021DE6C },
  { "Singleplayer music", toggleActionHandler, menuStateAlwaysEnabledHandler, &config.enableSingleplayerMusic, "When On, enables all music tracks in game. Currently not supported in Survival." },
  { "Singletap chargeboot", toggleActionHandler, menuStateAlwaysEnabledHandler, &config.enableSingleTapChargeboot, "Toggles tapping L2 once to chargeboot." },
  { "Spectate mode", toggleActionHandler, menuStateAlwaysEnabledHandler, &config.enableSpectate, "Toggles the custom spectate feature. Use \x13 when dead to spectate." },
  // { "Sync player state", toggleActionHandler, menuStateAlwaysEnabledHandler, &config.enablePlayerStateSync },
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

// character tab menu items
MenuElem_t menuElementsFreecam[] = {
  { "Airwalk", toggleActionHandler, menuStateAlwaysEnabledHandler, &freecamSettings.airwalk },
  { "Lock Position", toggleActionHandler, menuStateAlwaysEnabledHandler, &freecamSettings.lockPosition },
  { "Lock Animation", toggleActionHandler, menuStateAlwaysEnabledHandler, &freecamSettings.lockStateToggle },
};

// map override list item
MenuElem_ListData_t dataCustomMaps = {
  .value = &patchStateContainer.SelectedCustomMapId,
  .stateHandler = menuStateHandler_SelectedMapOverride,
  .count = 1,
  .items = {
    "None",
    [MAX_CUSTOM_MAP_DEFINITIONS+1] NULL
  }
};

// gamemode override list item
MenuElem_OrderedListData_t dataCustomModes = {
  .value = &gameConfig.customModeId,
  .stateHandler = menuStateHandler_SelectedGameModeOverride,
  .count = CUSTOM_MODE_COUNT,
  .items = {
    { CUSTOM_MODE_NONE, "None" },
    //{ CUSTOM_MODE_BENCHMARK, "Benchmark" },
    { CUSTOM_MODE_GRIDIRON, "DreadBall" },
    { CUSTOM_MODE_GUN_GAME, "Gun Game" },
    { CUSTOM_MODE_HNS, "Hide and Seek" },
    { CUSTOM_MODE_INFECTED, "Infected" },
    { CUSTOM_MODE_PAYLOAD, "Payload" },
    { CUSTOM_MODE_SEARCH_AND_DESTROY, "Search and Destroy" },
    { CUSTOM_MODE_SURVIVAL, "Survival" },
    { CUSTOM_MODE_1000_KILLS, "1000 Kills" },
    { CUSTOM_MODE_TRAINING, "Training" },
    { CUSTOM_MODE_TEAM_DEFENDER, "Team Defender" },
#if DEV
    { CUSTOM_MODE_ANIM_EXTRACTOR, "Anim Extractor" },
#endif
  }
};

// 
const char* CustomModeShortNames[] = {
  [CUSTOM_MODE_NONE] NULL,
  [CUSTOM_MODE_GUN_GAME] NULL,
  [CUSTOM_MODE_HNS] NULL,
  [CUSTOM_MODE_INFECTED] NULL,
  [CUSTOM_MODE_PAYLOAD] NULL,
  [CUSTOM_MODE_SEARCH_AND_DESTROY] "SND",
  [CUSTOM_MODE_SURVIVAL] NULL,
  [CUSTOM_MODE_1000_KILLS] NULL,
  [CUSTOM_MODE_TRAINING] NULL,
  [CUSTOM_MODE_TEAM_DEFENDER] NULL,
  //[CUSTOM_MODE_BENCHMARK] NULL,
  [CUSTOM_MODE_GRIDIRON] NULL,
#if DEV
  [CUSTOM_MODE_ANIM_EXTRACTOR] NULL,
#endif
};

// payload contest mode
MenuElem_ListData_t dataPayloadContestMode = {
  .value = &gameConfig.payloadConfig.contestMode,
  .stateHandler = NULL,
  .count = 3,
  .items = {
    [PAYLOAD_CONTEST_OFF] "Off",
    [PAYLOAD_CONTEST_SLOW] "Slow",
    [PAYLOAD_CONTEST_STOP] "Stop"
  }
};

// training type
MenuElem_ListData_t dataTrainingType = {
  .value = &gameConfig.trainingConfig.type,
  .stateHandler = NULL, //menuStateHandler_SelectedTrainingTypeOverride,
  .count = TRAINING_TYPE_MAX,
  .items = {
    [TRAINING_TYPE_FUSION] "Fusion",
    [TRAINING_TYPE_CYCLE] "Cycle",
    [TRAINING_TYPE_RUSH] "Rushing",
  }
};

// training variant
MenuElem_ListData_t dataTrainingVariant = {
  .value = &gameConfig.trainingConfig.variant,
  .stateHandler = NULL,
  .count = 2,
  .items = {
    "Ranked",
    "Endless"
  }
};

// training aggression
MenuElem_ListData_t dataTrainingAggression = {
  .value = &gameConfig.trainingConfig.aggression,
  .stateHandler = menuStateHandler_SelectedTrainingAggressionOverride,
  .count = 4,
  .items = {
    "Aggressive",
    "Aggressive No Damage",
    "Passive",
    "Idle"
  }
};

// payload contest mode
MenuElem_ListData_t dataHnsHideDuration = {
  .value = &gameConfig.hnsConfig.hideStageTime,
  .stateHandler = NULL,
  .count = 4,
  .items = {
    "30",
    "60",
    "90",
    "120",
  }
};

// player size list item
MenuElem_ListData_t dataPlayerSize = {
  .value = &gameConfig.prPlayerSize,
  .stateHandler = NULL,
  .count = 5,
  .items = {
    "Normal",
    "Large",
    "Giant",
    "Tiny",
    "Small"
  }
};

// headbutt damage list item
MenuElem_ListData_t dataHeadbutt = {
  .value = &gameConfig.prHeadbutt,
  .stateHandler = NULL,
  .count = 4,
  .items = {
    "Off",
    "Low Damage",
    "Medium Damage",
    "High Damage"
  }
};

// weather override list item
MenuElem_ListData_t dataWeather = {
  .value = &gameConfig.prWeatherId,
  .stateHandler = NULL,
  .count = 17,
  .items = {
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
  .value = &gameConfig.grNoSniperHelpers,
  .stateHandler = NULL,
  .count = 2,
  .items = {
    "Permitted",
    "Disabled"
  }
};

// healthbox list item
MenuElem_ListData_t dataHealthBoxes = {
  .value = &gameConfig.grNoHealthBoxes,
  .stateHandler = NULL,
  .count = 3,
  .items = {
    "On",
    "No Box",
    "Off"
  }
};

// vampire list item
MenuElem_ListData_t dataVampire = {
  .value = &gameConfig.grVampire,
  .stateHandler = NULL,
  .count = 4,
  .items = {
    "Off",
    "Quarter Heal",
    "Half Heal",
    "Full Heal",
  }
};

// v2s list item
MenuElem_ListData_t dataV2s = {
  .value = &gameConfig.grV2s,
  .stateHandler = NULL,
  .count = 3,
  .items = {
    "On",
    "Always",
    "Off",
  }
};

// presets list item
MenuElem_ListData_t dataGameConfigPreset = {
  .value = &preset,
  .stateHandler = NULL,
  .count = 3,
  .items = {
    "None",
    "Competitive",
    "1v1",
  }
};

// game settings tab menu items
MenuElem_t menuElementsGameSettings[] = {
  { "Reset", buttonActionHandler, menuStateAlwaysEnabledHandler, gmResetSelectHandler },

  // { "Game Settings", labelActionHandler, menuLabelStateHandler, (void*)LABELTYPE_HEADER },
  { "Map override", listActionHandler, menuStateAlwaysEnabledHandler, &dataCustomMaps, "Play on any of the custom maps from the Horizon Map Pack. Visit https://rac-horizon.com to download the map pack." },
  { "Gamemode override", gmOverrideListActionHandler, menuStateHandler_GameModeOverride, &dataCustomModes, "Change to one of the Horizon Custom Gamemodes." },
  { "Preset", listActionHandler, menuStateAlwaysEnabledHandler, &dataGameConfigPreset, "Select one of the preconfigured game rule presets or manually set the custom game rules below." },

  // SURVIVAL SETTINGS
  // { "Difficulty", listActionHandler, menuStateHandler_SurvivalSettingStateHandler, &dataSurvivalDifficulty },

  // PAYLOAD SETTINGS
  { "Payload Contesting", listActionHandler, menuStateHandler_PayloadSettingStateHandler, &dataPayloadContestMode, "Whether the payload will stop, slow, or move as normal when the defending team is near it." },

  // TRAINING SETTINGS
  { "Training Type", listActionHandler, menuStateHandler_TrainingSettingStateHandler, &dataTrainingType },
  { "Training Variant", listActionHandler, menuStateHandler_TrainingSettingStateHandler, &dataTrainingVariant, "Switch between endless mode and a ranked 5 minute session. Leaderboards available in the Horizon discord." },
  { "Bot Aggression", listActionHandler, menuStateHandler_TrainingSettingStateHandler, &dataTrainingAggression, "Configure bot behavior. Setting not configurable in ranked mode." },

  // HNS SETTINGS
  { "Hide Time", listActionHandler, menuStateHandler_HnsSettingStateHandler, &dataHnsHideDuration, "Time in seconds the hiders have to hide before the seekers can hunt for them." },

  // GAME RULES
  { "Game Rules", labelActionHandler, menuLabelStateHandler, (void*)LABELTYPE_HEADER },
  { "Better Flags", toggleActionHandler, menuStateHandler_CTFSettingStateHandler, &gameConfig.grBetterFlags, "Moves flag and spawn locations on some vanilla maps to more enjoyable locations." },
  { "Better Hills", toggleActionHandler, menuStateHandler_KOTHSettingStateHandler, &gameConfig.grBetterHills, "Moves hill spawns on some vanilla maps to more enjoyable locations." },
  { "CTF Halftime", toggleActionHandler, menuStateHandler_CTFSettingStateHandler, &gameConfig.grHalfTime, "If a timelimit is set, each team will swap flag bases at half time." },
  { "CTF Overtime", toggleActionHandler, menuStateHandler_CTFSettingStateHandler, &gameConfig.grOvertime, "If a timelimit is set, prevents the game from ending in a draw." },
  { "CQ Save Capture Progress", toggleActionHandler, menuStateHandler_CQSettingStateHandler, &gameConfig.grCqPersistentCapture, "Stops nodes from unhacking themselves over time." },
  { "CQ Turrets", toggleInvertedActionHandler, menuStateHandler_CQSettingStateHandler, &gameConfig.grCqDisableTurrets, "Disables turrets around nodes." },
  { "CQ Upgrades", toggleInvertedActionHandler, menuStateHandler_CQSettingStateHandler, &gameConfig.grCqDisableUpgrades, "Disables conquest node upgrades." },
  { "Damage Cooldown", toggleInvertedActionHandler, menuStateHandler_SettingStateHandler, &gameConfig.grNoInvTimer, "Disables the brief hit invincibility after taking damage." },
  { "Fix Wallsniping", toggleActionHandler, menuStateHandler_SettingStateHandler, &gameConfig.grFusionShotsAlwaysHit, "Forces sniper shots that hit to register on every client. Can result in shots that appear to phase through walls." },
  // { "Fusion Reticle", listActionHandler, menuStateAlwaysEnabledHandler, &dataFusionReticule },
  { "Healthbars", toggleActionHandler, menuStateAlwaysEnabledHandler, &gameConfig.grHealthBars, "Draws a healthbar above each player's nametag." },
  { "Healthboxes", listActionHandler, menuStateHandler_SettingStateHandler, &dataHealthBoxes, "Whether health pickups are enabled, or if there is a box enclosure that must be broken first before picking up." },
  { "Nametags", toggleInvertedActionHandler, menuStateHandler_SettingStateHandler, &gameConfig.grNoNames, "Disables in game nametags." },
  { "New Player Sync", toggleActionHandler, menuStateHandler_SettingStateHandler, &gameConfig.grNewPlayerSync, "Replaces the Insomniac player sync netcode with a better custom Horizon implementation. Reduces player teleporting, rubberbanding, and jittery movement. Known on rare occasions to freeze PS2s." },
  { "Quick Chat", toggleActionHandler, menuStateHandler_SettingStateHandler, &gameConfig.grQuickChat, "Enables in game quick chat with the D-Pad." },
  { "V2s", listActionHandler, menuStateHandler_SettingStateHandler, &dataV2s, "Configures V2 weapon upgrades to be disabled, on (default), or always on (spawn with v2 weapons)." },
  { "Vampire", listActionHandler, menuStateHandler_SettingStateHandler, &dataVampire, "Earn health for each kill." },
  { "Weapon Packs", toggleInvertedActionHandler, menuStateHandler_SettingStateHandler, &gameConfig.grNoPacks, "Toggle in game weapon packs." },
  { "Weapon Pickups", toggleInvertedActionHandler, menuStateHandler_SettingStateHandler, &gameConfig.grNoPickups, "Toggle in game weapon pickups." },

  // PARTY RULES
  { "Party Rules", labelActionHandler, menuLabelStateHandler, (void*)LABELTYPE_HEADER },
  { "Chargeboot Forever", toggleActionHandler, menuStateAlwaysEnabledHandler, &gameConfig.prChargebootForever, "Double tap and hold L2 to chargeboot forever." },
  { "Headbutt", listActionHandler, menuStateAlwaysEnabledHandler, &dataHeadbutt, "Deal damage by chargebooting into other players." },
  { "Headbutt Friendly Fire", toggleActionHandler, menuStateAlwaysEnabledHandler, &gameConfig.prHeadbuttFriendlyFire, "Toggle dealing headbutt damage to teammates." },
  { "Mirror World", toggleActionHandler, menuStateAlwaysEnabledHandler, &gameConfig.prMirrorWorld, "Enables the mirror world cheat. Currently broken in DZO." },
  { "Player Size", listActionHandler, menuStateAlwaysEnabledHandler, &dataPlayerSize, "Changes the size of the player model." },
  { "Rotate Weapons", toggleActionHandler, menuStateAlwaysEnabledHandler, &gameConfig.prRotatingWeapons, "Periodically equips the same random, enabled weapon for all players." },
  { "Weather override", listActionHandler, menuStateAlwaysEnabledHandler, &dataWeather, "Enables the weather cheat code." },

  // DEV RULES
  { "Dev Rules", labelActionHandler, menuLabelStateHandler, (void*)LABELTYPE_HEADER },
  { "Freecam", toggleActionHandler, menuStateAlwaysEnabledHandler, &gameConfig.drFreecam, "Enables freecam mod. Use D-Pad Up and L1 to activate." },
};

// game settings tab menu items
MenuElem_t menuElementsGameSettingsHelp[] = {
  { "", labelActionHandler, menuLabelStateHandler, (void*)LABELTYPE_LABEL },
  { "Please create a game to configure", labelActionHandler, menuLabelStateHandler, (void*)LABELTYPE_LABEL },
  { "the custom game settings.", labelActionHandler, menuLabelStateHandler, (void*)LABELTYPE_LABEL },
};

#if MAPEDITOR

extern int mapEditorState;
extern int mapEditorRespawnState;

// map editor enabled list item
MenuElem_ListData_t dataMapEditor = {
  .value = &mapEditorState,
  .stateHandler = NULL,
  .count = 2,
  .items = {
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
  { "Game Settings", tabGameSettingsHelpStateHandler, menuElementsGameSettingsHelp, sizeof(menuElementsGameSettingsHelp)/sizeof(MenuElem_t) },
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
void gmRefreshMapsSelectHandler(TabElem_t* tab, MenuElem_t* element)
{
  refreshCustomMapList();
  
  // popup
  if (isInMenus())
  {
    char buf[32];
    snprintf(buf, sizeof(buf), "Found %d maps", customMapDefCount);
    uiShowOkDialog("Custom Maps", buf);
  }
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
    *state = ELEMENT_HIDDEN;
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
void tabGameSettingsHelpStateHandler(TabElem_t* tab, int * state)
{

#if COMP

  *state = ELEMENT_HIDDEN;

#else

  GameSettings * gameSettings = gameGetSettings();
  if (!gameSettings)
  {
    *state = ELEMENT_SELECTABLE | ELEMENT_VISIBLE | ELEMENT_EDITABLE;
  }
  else
  {
    *state = ELEMENT_HIDDEN;
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
  patchStateContainer.SelectedCustomMapId = 0;
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

// 
void voteToEndSelectHandler(TabElem_t* tab, MenuElem_t* element)
{
  sendClientVoteForEnd();
  configMenuDisable();
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
void menuStateEnabledInMenusHandler(TabElem_t* tab, MenuElem_t* element, int* state)
{
  if (isInMenus()) *state = ELEMENT_VISIBLE | ELEMENT_EDITABLE | ELEMENT_SELECTABLE;
  else *state = ELEMENT_HIDDEN;
}

// 
void menuStateDzoEnabledHandler(TabElem_t* tab, MenuElem_t* element, int* state)
{
  if (CLIENT_TYPE_DZO == PATCH_INTEROP->Client)
    *state = ELEMENT_VISIBLE | ELEMENT_EDITABLE | ELEMENT_SELECTABLE;
  else
    *state = ELEMENT_HIDDEN;
}

// 
void menuLabelStateHandler(TabElem_t* tab, MenuElem_t* element, int* state)
{
  *state = ELEMENT_VISIBLE | ELEMENT_EDITABLE;
}

// 
void menuStateScavengerHuntEnabledHandler(TabElem_t* tab, MenuElem_t* element, int* state)
{
  if (!scavHuntEnabled)
    *state = ELEMENT_HIDDEN;
  else
    *state = ELEMENT_SELECTABLE | ELEMENT_VISIBLE | ELEMENT_EDITABLE;
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
int menuStateHandler_SelectedMapOverride(MenuElem_OrderedListData_t* listData, char* value)
{
  int i;
  if (!value)
    return 0;

  char gm = gameConfig.customModeId;
  char v = *value;

  switch (gm)
  {
    // case CUSTOM_MODE_BENCHMARK:
    // {
    //   for (i = 0; i < customMapDefCount; ++i) {
    //     if (strcmp("benchmark", customMapDefs[i].Filename) == 0) {
    //       if (v == (i+1)) return 1;
          
    //       *value = i+1;
    //       return 0;
    //     }
    //   }
      
    //   *value = 0;
    //   return 0;
    // }
    case CUSTOM_MODE_SURVIVAL:
    {
#if DEBUG
      return 1;
#endif

      // accept if selected map is survival
      if (v && customMapDefs[v-1].ForcedCustomModeId == CUSTOM_MODE_SURVIVAL)
        return 1;

      // force first survival map
      for (i = 0; i < customMapDefCount; ++i) {
        if (customMapDefs[i].ForcedCustomModeId == CUSTOM_MODE_SURVIVAL) {
          *value = i+1;
          return 0;
        }
      }

      // reset
      *value = 0;
      return 0;
    }
    case CUSTOM_MODE_SEARCH_AND_DESTROY:
    {
      // supported custom maps
      if (v && (customMapDefs[v-1].CustomModeExtraDataMask & (1 << CUSTOM_MODE_SEARCH_AND_DESTROY)) != 0)
        return 1;

      if (v == 0) return 1;

      *value = 0;
      return 0;
    }
    case CUSTOM_MODE_PAYLOAD:
    {
      // supported custom maps
      if (v && (customMapDefs[v-1].CustomModeExtraDataMask & (1 << CUSTOM_MODE_PAYLOAD)) != 0)
        return 1;

      if (v == 0) return 1;

      *value = 0;
      return 0;
    }
    case CUSTOM_MODE_TRAINING:
    {
      // endless cycle supports custom maps
      if ((gameConfig.trainingConfig.type == TRAINING_TYPE_CYCLE || gameConfig.trainingConfig.type == TRAINING_TYPE_RUSH) && gameConfig.trainingConfig.variant != 0) {
        if (v && customMapDefs[v-1].ForcedCustomModeId) {
          *value = 0;
          return 0;
        }

        return 1;
      }

      *value = 0;
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
        if (v && customMapDefs[v-1].ForcedCustomModeId && customMapDefs[v-1].ForcedCustomModeId != gm)
        {
          *value = 0;
          return 0;
        }
      }

      if (v && customMapDefs[v-1].ForcedCustomModeId > 0) {
        *value = 0;
        return 0;
      }

      return 1;
    }
  }
}

// 
void menuStateHandler_GameModeOverride(TabElem_t* tab, MenuElem_t* element, int* state)
{
  int i = 0;

  // hide gamemode for maps with exclusive gamemode
#if !DEBUG
  if (patchStateContainer.SelectedCustomMapId && customMapDefs[patchStateContainer.SelectedCustomMapId-1].ForcedCustomModeId < 0) {
    *state = ELEMENT_HIDDEN;
    return;
  }
#endif

  *state = ELEMENT_SELECTABLE | ELEMENT_VISIBLE | ELEMENT_EDITABLE;
}

// 
int menuStateHandler_SelectedGameModeOverride(MenuElem_OrderedListData_t* listData, char* value)
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
      case CUSTOM_MODE_HNS:
      case CUSTOM_MODE_TEAM_DEFENDER:
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
        if (gs->GameRules == GAMERULE_DM || gs->GameRules == GAMERULE_KOTH || gs->GameRules == GAMERULE_CTF)
          return 1;

        *value = CUSTOM_MODE_NONE;
        return 0;
      }
      case CUSTOM_MODE_GRIDIRON:
      {
        if (gs->GameRules == GAMERULE_CTF)
          return 1;
          
        *value = CUSTOM_MODE_NONE;
        return 0;
      }
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

  return 1;
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
      case GAMERULE_CTF:
      {
        if (v == TRAINING_TYPE_RUSH)
          return 1;

        *value = TRAINING_TYPE_RUSH;
        return 0;
      }
    }
  }

  return 0;
}

// 
int menuStateHandler_SelectedTrainingAggressionOverride(MenuElem_ListData_t* listData, char* value)
{
  if (!value)
    return 0;

  GameSettings* gs = gameGetSettings();
  char v = *value;

  switch (gameConfig.trainingConfig.type)
  {
    case TRAINING_TYPE_FUSION:
    {
      // if ranked variant, force passive
      if (gameConfig.trainingConfig.variant == 0)
      {
        *value = TRAINING_AGGRESSION_PASSIVE;
        return 0;
      }

      // force PASSIVE or IDLE for endless
      if (v != TRAINING_AGGRESSION_PASSIVE && v != TRAINING_AGGRESSION_IDLE)
      {
        *value = TRAINING_AGGRESSION_PASSIVE;
        return 0;
      }

      return 1;
    }
    case TRAINING_TYPE_CYCLE:
    {
      // if ranked variant, force aggro
      if (gameConfig.trainingConfig.variant == 0)
      {
        *value = TRAINING_AGGRESSION_AGGRO;
        return 0;
      }

      return 1;
    }
    case TRAINING_TYPE_RUSH:
    {
      // if ranked variant, force aggro
      if (gameConfig.trainingConfig.variant == 0)
      {
        *value = TRAINING_AGGRESSION_AGGRO;
        return 0;
      }

      return 1;
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
void menuStateHandler_HnsSettingStateHandler(TabElem_t* tab, MenuElem_t* element, int* state)
{
  if (gameConfig.customModeId != CUSTOM_MODE_HNS)
    *state = ELEMENT_HIDDEN;
  else
    *state = ELEMENT_SELECTABLE | ELEMENT_VISIBLE | ELEMENT_EDITABLE;
}

// 
void menuStateHandler_CycleTrainingSettingStateHandler(TabElem_t* tab, MenuElem_t* element, int* state)
{
  GameSettings* gs = gameGetSettings();

  if (gameConfig.customModeId != CUSTOM_MODE_TRAINING || gameConfig.trainingConfig.type != TRAINING_TYPE_CYCLE)
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

// 
void menuStateHandler_VoteToEndStateHandler(TabElem_t* tab, MenuElem_t* element, int* state)
{
  GameSettings* gs = gameGetSettings();
  int i = 0;
  
  if (isInGame()) {
    Player* p = playerGetFromSlot(0);
    if (p) {
      *state = ELEMENT_SELECTABLE | ELEMENT_VISIBLE;
      if (!voteToEndState.Votes[p->PlayerId]) *state |= ELEMENT_EDITABLE;
      return;
    }
  }
  
  *state = ELEMENT_HIDDEN;
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
void drawOrderedListMenuElement(TabElem_t* tab, MenuElem_t* element, MenuElem_OrderedListData_t * listData, RECT* rect)
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

  // find name
  int i;
  for (i = 0; i < listData->count; ++i) {
    if (listData->items[i].value == selectedIdx) break;
  }

  // invalid
  if (i >= listData->count) return;

  // draw value
  x = (rect->TopRight[0] * SCREEN_WIDTH) - 5;
  gfxScreenSpaceText(x, y, 1, 1, color, listData->items[i].name, -1, 2);
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

//------------------------------------------------------------------------------
void orderedListActionHandler(TabElem_t* tab, MenuElem_t* element, int actionType, void * actionArg)
{
  MenuElem_OrderedListData_t* listData = (MenuElem_OrderedListData_t*)element->userdata;
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
      char value = *listData->value;

      int index = 0;
      for (index = 0; index < listData->count; ++index) {
        if (listData->items[index].value == value) {
          break;
        }
      }

      int startIndex = index;
      do
      {
        index += 1;
        if (index >= listData->count)
          index = 0;
        char tValue = listData->items[index].value;
        if (listData->items[index].name && (listData->stateHandler == NULL || listData->stateHandler(listData, &tValue)))
          break;
      } while (index != startIndex);

      *listData->value = listData->items[index].value;
      break;
    }
    case ACTIONTYPE_DECREMENT:
    {
      if ((state & ELEMENT_EDITABLE) == 0)
        break;
      char value = *listData->value;

      int index = 0;
      for (index = 0; index < listData->count; ++index) {
        if (listData->items[index].value == value) {
          break;
        }
      }

      int startIndex = index;
      do
      {
        index -= 1;
        if (index < 0)
          index = listData->count - 1;
        char tValue = listData->items[index].value;
        if (listData->items[index].name && (listData->stateHandler == NULL || listData->stateHandler(listData, &tValue)))
          break;
      } while (index != startIndex);

      *listData->value = listData->items[index].value;
      break;
    }
    case ACTIONTYPE_GETHEIGHT:
    {
      *(float*)actionArg = LINE_HEIGHT;
      break;
    }
    case ACTIONTYPE_DRAW:
    {
      drawOrderedListMenuElement(tab, element, listData, (RECT*)actionArg);
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

//------------------------------------------------------------------------------
void gmOverrideListActionHandler(TabElem_t* tab, MenuElem_t* element, int actionType, void * actionArg)
{
  // update name to be based on current gamemode
  GameSettings* gs = gameGetSettings();
  if (gs && actionType == ACTIONTYPE_DRAW)
    snprintf(element->name, 40, "%s override", gameGetGameModeName(gs->GameRules));

  // pass to default list action handler
  orderedListActionHandler(tab, element, actionType, actionArg);
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

  static int helpLastItemIdx = -1;
  static int helpItemCooldown1 = 0;
  static int helpItemCooldown2 = 0;
  static float helpLastXOffset = 0;

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
        if (currentElement->help && strlen(currentElement->help) > 0) {

          if (i != helpLastItemIdx) {
            helpLastItemIdx = i;
            helpLastXOffset = 0;
            helpItemCooldown1 = 60 * 3;
            helpItemCooldown2 = 60 * 6;
          }

          // draw background
          gfxScreenSpaceBox(frameX, frameY + frameH - 1.0/SCREEN_HEIGHT, frameW, LINE_HEIGHT, 0x80000000);

          // set scissor
          gfxSetScissor(
            frameX * SCREEN_WIDTH,
            (frameX + frameW) * SCREEN_WIDTH,
            (frameY + frameH) * SCREEN_HEIGHT,
            (frameY + frameH + LINE_HEIGHT) * SCREEN_HEIGHT);
          
          // get width
          float w = gfxGetFontWidth(currentElement->help, -1, 1) / (float)SCREEN_WIDTH;
          if (helpItemCooldown1) --helpItemCooldown1;
          else if ((helpLastXOffset + w + contentPaddingX*2) >= frameW) helpLastXOffset -= 0.002;
          else if (helpItemCooldown2) --helpItemCooldown2;
          else { helpItemCooldown1 = 60 * 3; helpItemCooldown2 = 60 * 6; helpLastXOffset = 0; }
          gfxScreenSpaceText((frameX + contentPaddingX + helpLastXOffset) * SCREEN_WIDTH, (frameY + frameH) * SCREEN_HEIGHT, 1, 1, 0x80FFFFFF, currentElement->help, -1, 0);

          // reset scissor
          gfxSetScissor(0, SCREEN_WIDTH, 0, SCREEN_HEIGHT);
        }
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
        sprintf(buf, "ping %d", ping);
        gfxScreenSpaceText(0.88 * SCREEN_WIDTH, 0.15 * SCREEN_HEIGHT, 1, 1, 0x80FFFFFF, buf, -1, 2);
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
    if (patchStateContainer.SelectedCustomMapId > 0 && dataCustomMaps.items[patchStateContainer.SelectedCustomMapId])
      mapName = dataCustomMaps.items[patchStateContainer.SelectedCustomMapId];
    else if (MapLoaderState.MapName[0])
      mapName = MapLoaderState.MapName;

    // get mode override name
    if (gameConfig.customModeId > 0)
    {
      modeName = (char*)CustomModeShortNames[(int)gameConfig.customModeId];
      if (!modeName) {
        for (i = 0; i < dataCustomModes.count; ++i) {
          if (dataCustomModes.items[i].value == (int)gameConfig.customModeId) {
            modeName = dataCustomModes.items[i].name;
            break;
          }
        }
      }

      // set map name to training type
      if (gameConfig.customModeId == CUSTOM_MODE_TRAINING) {
        mapName = dataTrainingType.items[gameConfig.trainingConfig.type];
      }
    }

    // override gamemode name with map if map has exclusive gamemode
    if (patchStateContainer.SelectedCustomMapId)
    {
      if (customMapDefs[patchStateContainer.SelectedCustomMapId-1].ForcedCustomModeId < 0)
      {
        modeName = mapName;
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
  selectedMapIdHostBackup = 0;

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

    // detect when new map selected
    patchStateContainer.SelectedCustomMapChanged = selectedMapIdHostBackup != patchStateContainer.SelectedCustomMapId;

    // backup
    memcpy(&gameConfigHostBackup, &gameConfig, sizeof(PatchGameConfig_t));
    selectedMapIdHostBackup = patchStateContainer.SelectedCustomMapId;

    // send
    void * lobbyConnection = netGetLobbyServerConnection();
    if (lobbyConnection) {
      ClientSetGameConfig_t msg;

      memset(&msg, 0, sizeof(msg));
      if (patchStateContainer.SelectedCustomMapId > 0)
        memcpy(&msg.CustomMap, &customMapDefs[patchStateContainer.SelectedCustomMapId-1], sizeof(msg.CustomMap));
      memcpy(&msg.GameConfig, &gameConfig, sizeof(msg.GameConfig));
      netSendCustomAppMessage(NET_DELIVERY_CRITICAL, lobbyConnection, NET_LOBBY_CLIENT_INDEX, CUSTOM_MSG_ID_CLIENT_USER_GAME_CONFIG, sizeof(ClientSetGameConfig_t), &msg);
    }
  }
#endif

}

//------------------------------------------------------------------------------
void configMenuDisable(void)
{
  if (!isConfigMenuActive)
    return;
  
  isConfigMenuActive = PATCH_POINTERS_PATCHMENU = 0;

  // send config to server for saving
  void * lobbyConnection = netGetLobbyServerConnection();
  if (lobbyConnection)
    netSendCustomAppMessage(NET_DELIVERY_CRITICAL, lobbyConnection, NET_LOBBY_CLIENT_INDEX, CUSTOM_MSG_ID_CLIENT_USER_CONFIG, sizeof(PatchConfig_t), &config);

  // update and send gameconfig
  if (gameAmIHost())
  {
    // force game config to preset
    switch (preset)
    {
      case 1: // competitive
      {
        gameConfig.grNoHealthBoxes = 1;
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
        gameConfig.grQuickChat = 1;
        //gameConfig.grNoSniperHelpers = 1;
        gameConfig.grNewPlayerSync = 1;
        gameConfig.grCqPersistentCapture = 1;
        gameConfig.grCqDisableTurrets = 1;
        gameConfig.grCqDisableUpgrades = 1;
        break;
      }
      case 2: // 1v1
      {
        gameConfig.grNoHealthBoxes = 2;
        gameConfig.grNoNames = 0;
        gameConfig.grV2s = 2;
        gameConfig.grVampire = 3;

        gameConfig.grNewPlayerSync = 1;
        gameConfig.grFusionShotsAlwaysHit = 1;
        gameConfig.grNoInvTimer = 1;
        gameConfig.grNoPacks = 1;
        gameConfig.grNoPickups = 1;
        break;
      }
    }

    // send
    configTrySendGameConfig();
  }

  // re-enable pad
  padEnableInput();
}

//------------------------------------------------------------------------------
void configMenuEnable(void)
{
  // enable
  isConfigMenuActive = PATCH_POINTERS_PATCHMENU = 1;

  // return to first tab if current is hidden
  int state = 0;
  tabElements[selectedTabItem].stateHandler(&tabElements[selectedTabItem], &state);
  if ((state & ELEMENT_SELECTABLE) == 0 || (state & ELEMENT_VISIBLE) == 0)
    selectedTabItem = 0;
}
