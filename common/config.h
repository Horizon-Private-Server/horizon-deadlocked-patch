#ifndef _PATCH_CONFIG_
#define _PATCH_CONFIG_

#include <libdl/gamesettings.h>

/*
 * Fixed pointers to patch container for use by external modules.
 */
#define PATCH_INTEROP             (*(PatchInterop_t**)0x000CFFC0)
#define PATCH_DZO_INTEROP_FUNCS	  (*(DzoInteropFunctions_t**)0x000CFFC4)
#define PATCH_POINTERS_SPECTATE   (*(u8*)(0x000CFFC0 + 8))
#define PATCH_POINTERS_PATCHMENU  (*(u8*)(0x000CFFC0 + 9))
#define PATCH_POINTERS_SCOREBOARD (*(u8*)(0x000CFFC0 + 10))
#define PATCH_POINTERS_QUICKCHAT  (*(u8*)(0x000CFFC0 + 11))
#define DZO_MAPLOADER_WAD_BUFFER  ((void*)0x02100000)

typedef void (*SendCustomCommandToClientFunc_t)(int id, int size, void * data);
typedef void (*SetSpectateFunc_t)(int localPlayerIndex, int spectatePlayerOrDisable);

typedef struct PatchConfig
{
  char framelimiter;
  char enableGamemodeAnnouncements;
  char enableSpectate;
  char enableSingleplayerMusic;
  char levelOfDetail;
  char enablePlayerStateSync;
  char disableAimAssist;
  char enableFpsCounter;
  char disableCircleToHackerRay;
  char disableScavengerHunt;
  char disableCameraShake;
  char minimapScale;
  char minimapBigZoom;
  char minimapSmallZoom;
  char enableFusionReticule;
  char playerFov;
  char preferredGameServer;
  char fixedCycleOrder;
  char enableSingleTapChargeboot;
  char enableInGameScoreboard;
  char enableNPSLagComp;
  char enableFastLoad;

#if TWEAKERS
  char characterTweakers[1 + 7*2];
#endif
} PatchConfig_t;

typedef struct SurvivalConfig
{
	//u8 difficulty;
} SurvivalConfig_t;

enum FixedCycleOrderMode
{
  FIXED_CYCLE_ORDER_OFF = 0,
  FIXED_CYCLE_ORDER_MAG_FUS_B6 = 1,
  FIXED_CYCLE_ORDER_MAG_B6_FUS = 2,
  FIXED_CYCLE_ORDER_COUNT = 3
};

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
  u8 variant;
  u8 aggression;
  u8 opt3;
} TrainingConfig_t;

typedef struct HNSConfig
{
  u8 hideStageTime;
} HNSConfig_t;

typedef struct PatchGameConfig
{
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
  char grNoSniperHelpers;
  char grCqPersistentCapture;
  char grCqDisableTurrets;
  char grCqDisableUpgrades;
  char grNewPlayerSync;
  char grQuickChat;
  char grNoFusionADS;
  char prPlayerSize;
  char prRotatingWeapons;
  char prHeadbutt;
  char prHeadbuttFriendlyFire;
  char prChargebootForever;
  char drFreecam;
  SurvivalConfig_t survivalConfig;
  PayloadConfig_t payloadConfig;
  TrainingConfig_t trainingConfig;
  HNSConfig_t hnsConfig;
} PatchGameConfig_t;

typedef struct PatchInterop
{
  PatchConfig_t* Config;
  PatchGameConfig_t* GameConfig;
  char Client;
  char Month;
  SetSpectateFunc_t SetSpectate;
  char* MapLoaderFilename;
} PatchInterop_t;

typedef struct DzoInteropFunctions
{
  SendCustomCommandToClientFunc_t SendCustomCommandToClient;
} DzoInteropFunctions_t;

typedef struct CustomMapDef
{
  int Version;
  int CustomModeExtraDataMask;
  short ShrubMinRenderDistance;
  char BaseMapId;
  char ForcedCustomModeId;
  char Name[32];
  char Filename[64];
} CustomMapDef_t;

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

enum CUSTOM_MODE_ID
{
  CUSTOM_MODE_NONE = 0,
  CUSTOM_MODE_1000_KILLS,
  CUSTOM_MODE_GUN_GAME,
  CUSTOM_MODE_INFECTED,
  // CUSTOM_MODE_INFINITE_CLIMBER,
  CUSTOM_MODE_PAYLOAD,
  CUSTOM_MODE_SEARCH_AND_DESTROY,
  CUSTOM_MODE_SURVIVAL,
  CUSTOM_MODE_TEAM_DEFENDER,
  CUSTOM_MODE_TRAINING,
  //CUSTOM_MODE_BENCHMARK,
  CUSTOM_MODE_HNS,
  CUSTOM_MODE_GRIDIRON,
  CUSTOM_MODE_TAG,
  CUSTOM_MODE_RAIDS,
  
#if DEV
  CUSTOM_MODE_ANIM_EXTRACTOR,
#endif

  // always at the end to indicate how many items there are
  CUSTOM_MODE_COUNT
};

enum TRAINING_TYPE
{
	TRAINING_TYPE_FUSION,
	TRAINING_TYPE_CYCLE,
  TRAINING_TYPE_RUSH,
	TRAINING_TYPE_MAX
};

enum TRAINING_AGGRESSION
{
	TRAINING_AGGRESSION_AGGRO,
	TRAINING_AGGRESSION_AGGRO_NO_DAMAGE,
	TRAINING_AGGRESSION_PASSIVE,
	TRAINING_AGGRESSION_IDLE,
  TRAINING_AGGRESSION_MAX
};

enum CLIENT_TYPE
{
  CLIENT_TYPE_NORMAL = 0,
  CLIENT_TYPE_DZO = 1,
  CLIENT_TYPE_PCSX2 = 2
};

#endif // _PATCH_CONFIG_
