#ifndef _PATCH_CONFIG_
#define _PATCH_CONFIG_

/*
 * Fixed pointers to patch container for use by external modules.
 */
#define PATCH_POINTERS			((void*)0x000CFFC0)

typedef struct PatchConfig
{
  char framelimiter;
  char enableGamemodeAnnouncements;
  char enableSpectate;
  char enableSingleplayerMusic;
  char levelOfDetail;
  char enablePlayerStateSync;
  char enableAutoMaps;
  char enableFpsCounter;
  char disableCircleToHackerRay;
  char playerAggTime;
  char disableCameraShake;
  char minimapScale;
  char minimapBigZoom;
  char minimapSmallZoom;
#if FREECAM
  char playerFov;
#endif
  
#if TWEAKERS
  char characterTweakers[1 + 7*2];
#endif
} PatchConfig_t;

typedef struct SurvivalConfig
{
	//u8 difficulty;
} SurvivalConfig_t;

enum PayloadContestMode
{
	PAYLOAD_CONTEST_OFF,
	PAYLOAD_CONTEST_SLOW,
	PAYLOAD_CONTEST_STOP
};

typedef struct PayloadConfig
{
	u8 contestMode;
} PayloadConfig_t;

typedef struct TrainingConfig
{
	u8 type;
} TrainingConfig_t;

typedef struct PatchGameConfig
{
  char customMapId;
  char customModeId;
  char prWeatherId;
  char grNoPacks;
  char grV2s;
  char prMirrorWorld;
  char grNoHealthBoxes;
  char grVampire;
  char grHalfTime;
  char grOvertime;
  char grBetterHills;
  char grBetterFlags;
  char grHealthBars;
  char grNoNames;
  char grNoInvTimer;
  char grNoPickups;
  char grFusionShotsAlwaysHit;
  char prPlayerSize;
  char prRotatingWeapons;
  char prHeadbutt;
  char prHeadbuttFriendlyFire;
  char prChargebootForever;
  SurvivalConfig_t survivalConfig;
  PayloadConfig_t payloadConfig;
  TrainingConfig_t trainingConfig;
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
  CUSTOM_MAP_ACE_HARDLIGHT_SUITE,
  CUSTOM_MAP_ANNIHILATION_NATION,
  CUSTOM_MAP_BAKISI_ISLES,
  CUSTOM_MAP_BATTLEDOME_SP,
  CUSTOM_MAP_BLACKWATER_CITY,
  CUSTOM_MAP_BLACKWATER_DOCKS,
  CUSTOM_MAP_CANAL_CITY,
  CUSTOM_MAP_CONTAINMENT_SUITE,
  CUSTOM_MAP_DARK_CATHEDRAL_INTERIOR,
  CUSTOM_MAP_DESERT_PRISON,
  CUSTOM_MAP_DUCK_HUNT,
  CUSTOM_MAP_GHOST_SHIP,
  CUSTOM_MAP_HOVEN_GORGE,
  CUSTOM_MAP_HOVERBIKE_RACE,
  CUSTOM_MAP_INFINITE_CLIMBER,
  CUSTOM_MAP_KORGON_OUTPOST,
  CUSTOM_MAP_LAUNCH_SITE,
  CUSTOM_MAP_MARCADIA_PALACE,
  CUSTOM_MAP_METROPOLIS_MP,
  CUSTOM_MAP_MINING_FACILITY_SP,
  CUSTOM_MAP_SARATHOS_SP,
  CUSTOM_MAP_SHAAR_SP,
  CUSTOM_MAP_SHIPMENT,
  CUSTOM_MAP_SNIVELAK,
  CUSTOM_MAP_SPLEEF,
  CUSTOM_MAP_TORVAL_LOST_FACTORY,
  CUSTOM_MAP_TORVAL_SP,
  CUSTOM_MAP_TYHRRANOSIS,

  // Survival maps
  CUSTOM_MAP_SURVIVAL_START,
  CUSTOM_MAP_SURVIVAL_MINING_FACILITY = CUSTOM_MAP_SURVIVAL_START,
  CUSTOM_MAP_SURVIVAL_END = CUSTOM_MAP_SURVIVAL_MINING_FACILITY,

  // always at the end to indicate how many items there are
  CUSTOM_MAP_COUNT
};

enum CUSTOM_MODE_ID
{
  CUSTOM_MODE_NONE = 0,
  CUSTOM_MODE_GUN_GAME,
  CUSTOM_MODE_INFECTED,
  // CUSTOM_MODE_INFINITE_CLIMBER,
  CUSTOM_MODE_PAYLOAD,
  CUSTOM_MODE_SEARCH_AND_DESTROY,
  CUSTOM_MODE_SURVIVAL,
  CUSTOM_MODE_1000_KILLS,
  CUSTOM_MODE_TRAINING,
  
#if DEV
  CUSTOM_MODE_GRIDIRON,
  CUSTOM_MODE_TEAM_DEFENDER,
  CUSTOM_MODE_ANIM_EXTRACTOR,
#endif

  // always at the end to indicate how many items there are
  CUSTOM_MODE_COUNT
};

enum TRAINING_TYPE
{
	TRAINING_TYPE_FUSION,
	TRAINING_TYPE_CYCLE,
	TRAINING_TYPE_B6,
	TRAINING_TYPE_MAX
};

enum CLIENT_TYPE
{
  CLIENT_TYPE_NORMAL = 0,
  CLIENT_TYPE_DZO = 1
};

#endif // _PATCH_CONFIG_
