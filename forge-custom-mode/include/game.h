#ifndef FORGE_CGM_GAME_H
#define FORGE_CGM_GAME_H

#include <tamtypes.h>
#include <libdl/player.h>
#include <libdl/math3d.h>
#include "messageid.h"

#define MAP_CONFIG_MAGIC                      (0xDEADBEEF)
#define MAX_SCOREBOARD_STATS                  (4)
#define MAX_TRACKED_STATS                     (8)
#define MAX_CUSTOM_STATS                      (16)

enum CgmMessageIds
{
  CGM_MSG_ID_SEND_GAME_STATS = CUSTOM_MSG_ID_GAME_MODE_START,
  CGM_MSG_ID_SEND_CUSTOM_PLAYER_STAT,
  CGM_MSG_ID_SEND_CUSTOM_TEAM_STAT,
	CGM_MSG_ID_SEND_ROUND_ENDED,
};

enum CgmTeamType
{
  TEAM_TYPE_ALLOW_ANY = 0,
  TEAM_TYPE_ALL_BLUE,
  TEAM_TYPE_ALL_RED,
  TEAM_TYPE_FFA,
  TEAM_TYPE_RED_BLUE_EVEN,
  TEAM_TYPE_2_TEAMS_EVEN,
  TEAM_TYPE_3_TEAMS_EVEN,
};

enum CgmScoreStatValueType
{
	CGM_SCORE_STAT_TYPE_INT,
	CGM_SCORE_STAT_TYPE_TIME_SECONDS,
	CGM_SCORE_STAT_TYPE_FLOAT,
	CGM_SCORE_STAT_TYPE_TIME_MILLISECONDS,
};

enum CgmScoreStatTrackerType
{
	CGM_SCORE_STAT_TRACK_ADD,
	CGM_SCORE_STAT_TRACK_MAX,
	CGM_SCORE_STAT_TRACK_MIN,
	CGM_SCORE_STAT_TRACK_SET,
};

struct CgmStats;

typedef void (*Stub_func)(void);
typedef void (*Tick_func)(PatchStateContainer_t * gameState);
typedef void (*UpdateGameState_func)(PatchStateContainer_t * gameState);
typedef void (*UpdateStats_func)(struct CgmStats* stats);

struct CgmMapFunctions
{
  Tick_func TickFrame;
  Tick_func TickGame;
  UpdateGameState_func UpdateGameState;
};

struct CgmModeFunctions
{
  UpdateStats_func UpdateStats;
};

struct CgmMapConfig
{
  u32 Magic;

  // 64 mode functions
  union {
    struct CgmModeFunctions ModeFunctions;
    Stub_func _padding[64];
  };
  
  // 64 map functions
  union {
    struct CgmMapFunctions MapFunctions;
    Stub_func _padding[64];
  };
};

struct CgmStats
{
	int WinningTeam;
	int TeamScores[GAME_MAX_PLAYERS];
	char PlayerStatNames[MAX_SCOREBOARD_STATS][16];
	int PlayerStatValues[GAME_MAX_PLAYERS][MAX_SCOREBOARD_STATS];
	char PlayerStatTypes[MAX_SCOREBOARD_STATS];
	char TeamScoreType;
	char TeamPlacements[GAME_MAX_PLAYERS];
};

struct CgmTrackedStat
{
	char Name[16];
	char ValueType;
	char TrackerType;
	char TrackerSlot;
	char TrackerSave;
};

struct CgmCustomGameStats
{
	int Version;
	int RuntimeMs;
	char Name[32];
	char SharedRankCode[32];
	char MinTeamsForRank;
	char MinTeamsForStats;
	char PADDING[2];
	int TeamScores[GAME_MAX_PLAYERS];
	char PlayerTeams[GAME_MAX_PLAYERS];
	char TeamsEnabled;
	char OrderScoreByAscending; // if non-zero, indicates a lower score is better
	struct CgmTrackedStat TrackedStats[MAX_TRACKED_STATS];
	int TrackedStatValues[MAX_TRACKED_STATS][GAME_MAX_PLAYERS];
};

struct CGMCustomMapExData
{
	u32 Version;
  char GameModeName[32];
  char SharedRankCode[32];
  int ParametersOffset;
	u32 GadgetsMask;

  // general
  char GameRule;
  char RadarBlips;
  char Vehicles;
  char SpecialPickups;
  char SpawnWithChargeboots;
  char AutospawnWeapons;
  char UnlimitedAmmo;
  char Timelimit;
  char RespawnTime;

  // dm / juggy
  char KillsToWin;
  char Survivor;
  char JuggernautVis;
  char JuggernautHealing;

  // ctf
  char CapsToWin;
  char CrazyMode;
  char FlagReturn;
  char VehicleCarry;
  char GrCtfHalftime;
  char GrCtfOvertime;
  
  // koth
  char HillTimeToWin;
  char MovingHillTime;
  char HillSharing;
  char HillArmor;

  // cq
  char BoltsToWin;
  char SpecialRules;
  char NodeType;
  char Turrets;
  char TeleporterUpgrade;
  char UpgradeTimer;
  char VoteTime;
  char GrCqPersistentCapture;
  char GrCqDisableUpgrades;

  // patch
  char GrDamageCooldown;
  char GrHealthbars;
  char GrHealthboxes;
  char GrInstantDeath;
  char GrNametags;
  char GrRadarShortDistance;
  char GrRadarShortShared;
  char GrSpawnImmunity;
  char GrV2s;
  char GrVampire;
  char GrWeaponPacks;
  char GrWeaponPickups;

  // party
  char PrChargebootForever;
  char PrHeadbutt;
  char PrHeadbuttFriendlyFire;
  char PrRotatingWeapons;

  // teams
  char TeamType;

  // tracked stats
  char TrackBaseStats;
  char TrackCustomStats;
  char MinTeamsForRank;
  char MinTeamsForStats;
};

#endif // FORGE_CGM_GAME_H
