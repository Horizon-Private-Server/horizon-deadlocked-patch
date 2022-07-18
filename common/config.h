#ifndef _PATCH_CONFIG_
#define _PATCH_CONFIG_

typedef struct PatchConfig
{
  char disableFramelimiter;
  char enableGamemodeAnnouncements;
  char enableSpectate;
  char enableSingleplayerMusic;
  char levelOfDetail;
  char enablePlayerStateSync;
  char enableAutoMaps;
  char enableFpsCounter;
  char disableCircleToHackerRay;
  char playerAggTime;
  
#if TWEAKERS
  char characterTweakers[1 + 7*2];
#endif
} PatchConfig_t;

typedef struct SurvivalConfig
{
	u8 difficulty;
} SurvivalConfig_t;

typedef struct PatchGameConfig
{
  char customMapId;
  char customModeId;
  char prWeatherId;
  char grNoPacks;
  char grNoV2s;
  char prMirrorWorld;
  char grNoHealthBoxes;
  char grVampire;
  char grHalfTime;
  char grBetterHills;
  char grHealthBars;
  char grNoNames;
  char grNoInvTimer;
  char prPlayerSize;
  char prRotatingWeapons;
  char prHeadbutt;
  char prHeadbuttFriendlyFire;
  SurvivalConfig_t survivalConfig;
} PatchGameConfig_t;

enum CHARACTER_TWEAKER_ID
{
  CHARACTER_TWEAKER_HEAD_SCALE,
  CHARACTER_TWEAKER_UPPER_TORSO_SCALE,
  CHARACTER_TWEAKER_LEFT_ARM_SCALE,
  CHARACTER_TWEAKER_RIGHT_ARM_SCALE,
  CHARACTER_TWEAKER_LEFT_LEG_SCALE,
  CHARACTER_TWEAKER_RIGHT_LEG_SCALE,
  CHARACTER_TWEAKER_TOGGLE,
  CHARACTER_TWEAKER_LOWER_TORSO_SCALE,

  CHARACTER_TWEAKER_HEAD_POS,
  CHARACTER_TWEAKER_UPPER_TORSO_POS,
  CHARACTER_TWEAKER_LOWER_TORSO_POS,
  CHARACTER_TWEAKER_LEFT_ARM_POS,
  CHARACTER_TWEAKER_RIGHT_ARM_POS,
  CHARACTER_TWEAKER_LEFT_LEG_POS,
  CHARACTER_TWEAKER_RIGHT_POS,

  CHARACTER_TWEAKER_COUNT
};

enum CUSTOM_MAP_ID
{
  CUSTOM_MAP_NONE = 0,
  CUSTOM_MAP_ANNIHILATION_NATION,
  CUSTOM_MAP_BAKISI_ISLES,
  CUSTOM_MAP_BATTLEDOME_SP,
  CUSTOM_MAP_BLACKWATER_CITY,
  CUSTOM_MAP_BLACKWATER_DOCKS,
  CUSTOM_MAP_CONTAINMENT_SUITE,
  CUSTOM_MAP_DARK_CATHEDRAL_INTERIOR,
  CUSTOM_MAP_DESERT_PRISON,
  CUSTOM_MAP_DUCK_HUNT,
  CUSTOM_MAP_GHOST_SHIP,
  CUSTOM_MAP_HOVEN_GORGE,
  CUSTOM_MAP_HOVERBIKE_RACE,
  CUSTOM_MAP_KORGON_OUTPOST,
  CUSTOM_MAP_LAUNCH_SITE,
  CUSTOM_MAP_MARCADIA_PALACE,
  CUSTOM_MAP_METROPOLIS_MP,
  CUSTOM_MAP_MINING_FACILITY_SP,
  CUSTOM_MAP_SARATHOS_SP,
  CUSTOM_MAP_SHAAR_SP,
  CUSTOM_MAP_SHIPMENT,
  CUSTOM_MAP_SPLEEF,
  CUSTOM_MAP_TORVAL_SP,
  CUSTOM_MAP_TYHRRANOSIS,

  // Survival maps
  CUSTOM_MAP_SURVIVAL_START,
  CUSTOM_MAP_SURVIVAL_MARCADIA = CUSTOM_MAP_SURVIVAL_START,
  CUSTOM_MAP_SURVIVAL_MINING_FACILITY,
  CUSTOM_MAP_SURVIVAL_VELDIN,
  CUSTOM_MAP_SURVIVAL_END = CUSTOM_MAP_SURVIVAL_VELDIN,

  // always at the end to indicate how many items there are
  CUSTOM_MAP_COUNT
};

enum CUSTOM_MODE_ID
{
  CUSTOM_MODE_NONE = 0,
  CUSTOM_MODE_GUN_GAME,
  CUSTOM_MODE_INFECTED,
  CUSTOM_MODE_INFINITE_CLIMBER,
  CUSTOM_MODE_PAYLOAD,
  CUSTOM_MODE_SEARCH_AND_DESTROY,
  CUSTOM_MODE_SURVIVAL,
  CUSTOM_MODE_1000_KILLS,
  
#if DEV
  CUSTOM_MODE_GRIDIRON,
  CUSTOM_MODE_TEAM_DEFENDER,
#endif

  // always at the end to indicate how many items there are
  CUSTOM_MODE_COUNT
};

#endif // _PATCH_CONFIG_
