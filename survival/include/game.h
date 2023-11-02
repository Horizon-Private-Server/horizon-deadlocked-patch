#ifndef SURVIVAL_GAME_H
#define SURVIVAL_GAME_H

#include <tamtypes.h>
#include "messageid.h"
#include <libdl/player.h>
#include <libdl/math3d.h>
#include "upgrade.h"
#include "demonbell.h"
#include "gate.h"
#include "mysterybox.h"

#define MAP_CONFIG_MAGIC                      (0xDEADBEEF)

#define TPS																		(60)

#define ZOMBIE_MOBY_OCLASS										(0x20F6)
#define EXECUTIONER_MOBY_OCLASS							  (0x2468)
#define TREMOR_MOBY_OCLASS							      (0x24D3)
#define SWARMER_MOBY_OCLASS							      (0x2695)
#define REACTOR_MOBY_OCLASS							      (0x20BE)

#define BIGAL_MOBY_OCLASS                     (0x2124)

#define GRAVITY_MAGNITUDE                     (15 * MATH_DT)

#define MAX_MOBS_BASE													(10)
#define MAX_MOBS_ROUND_WEIGHT									(20)
#define MAX_MOBS_SPAWNED											(50)

#define ROUND_MESSAGE_DURATION_MS							(TIME_SECOND * 2)
#define ROUND_START_DELAY_MS									(TIME_SECOND * 1)

#if QUICK_SPAWN
#define ROUND_TRANSITION_DELAY_MS							(TIME_SECOND * 0)
#else
#define ROUND_TRANSITION_DELAY_MS							(TIME_SECOND * 45)
#endif

#define ROUND_BASE_BOLT_BONUS									(100)
#define ROUND_MAX_BOLT_BONUS									(10000)

#define ROUND_SPECIAL_EVERY										(5)
#define ROUND_SPECIAL_BONUS_MULTIPLIER				(5)

#define MOB_TARGET_DIST_IN_SIGHT_IGNORE_PATH 		(25)
#define MOB_MOVE_SKIP_TICKS                   (2)

#define MOB_SPAWN_SEMI_NEAR_PLAYER_PROBABILITY 		(1)
#define MOB_SPAWN_NEAR_PLAYER_PROBABILITY 				(0.25)
#define MOB_SPAWN_AT_PLAYER_PROBABILITY 					(0.01)
#define MOB_SPAWN_NEAR_HEALTHBOX_PROBABILITY 			(0.1)

#define MOB_SPAWN_BURST_MIN_DELAY							(1 * 60)
#define MOB_SPAWN_BURST_MAX_DELAY							(10 * 60)
#define MOB_SPAWN_BURST_MIN										(3)
#define MOB_SPAWN_BURST_MAX										(10)
#define MOB_SPAWN_BURST_MAX_INC_PER_ROUND			(1)
#define MOB_SPAWN_BURST_MIN_INC_PER_ROUND			(0)

#define MOB_AUTO_DIRTY_COOLDOWN_TICKS			    (60 * 1)

#define MOB_BASE_DAMAGE										    (10)
#define MOB_BASE_DAMAGE_SCALE                 (0.02*1)
#define MOB_BASE_SPEED											  (3)
#define MOB_BASE_SPEED_SCALE                  (0.03*1)
#define MOB_BASE_HEALTH										    (30)
#define MOB_BASE_HEALTH_SCALE                 (0.05*1)

#define MOB_SPECIAL_MUTATION_PROBABILITY		  (0.005)
#define MOB_SPECIAL_MUTATION_BASE_COST			  (200)
#define MOB_SPECIAL_MUTATION_REL_COST			    (1.0)

#if PAYDAY
#define MOB_BASE_BOLTS											(1000000)
#else
#define MOB_BASE_BOLTS											(120)
#endif

#define JACKPOT_BOLTS													(50)
#define XP_ALPHAMOD_XP												(10)

#define DROP_COOLDOWN_TICKS_MIN								(TPS * 10)
#define DROP_COOLDOWN_TICKS_MAX								(TPS * 60)
#define DROP_DURATION													(30 * TIME_SECOND)
#define DOUBLE_POINTS_DURATION								(20 * TIME_SECOND)
#define DOUBLE_XP_DURATION								    (20 * TIME_SECOND)
#define FREEZE_DROP_DURATION									(10 * TIME_SECOND)
#define MOB_HAS_DROP_PROBABILITY						(0.01)
#define DROP_MAX_SPAWNED											(4)

#define PLAYER_BASE_REVIVE_COST								(5000)
#define PLAYER_REVIVE_COST_PER_PLAYER					(1000)
#define PLAYER_REVIVE_COST_PER_ROUND					(0)
#define PLAYER_REVIVE_MAX_DIST								(2.5)
#define PLAYER_REVIVE_COOLDOWN_TICKS					(120)
#define PLAYER_KNOCKBACK_BASE_POWER						(0.4)
#define PLAYER_KNOCKBACK_BASE_TICKS						(5)
#define PLAYER_COLL_RADIUS          					(0.5)

#define BIG_AL_MAX_DIST												(5)
#define WEAPON_VENDOR_MAX_DIST								(3)
#define WEAPON_UPGRADE_COOLDOWN_TICKS					(60)
#define WEAPON_MENU_COOLDOWN_TICKS						(60)
#define VENDOR_MAX_WEAPON_LEVEL								(9)

#define PLAYER_UPGRADE_DAMAGE_FACTOR          (0.08)
#define PLAYER_UPGRADE_SPEED_FACTOR           (0.03)
#define PLAYER_UPGRADE_HEALTH_FACTOR          (5)
#define PLAYER_UPGRADE_MEDIC_FACTOR           (0.05)
#define PLAYER_UPGRADE_VENDOR_FACTOR          (0.02)

#define BAKED_SPAWNPOINT_COUNT							  (24)

#define ITEM_INVISCLOAK_DURATION              (30*TIME_SECOND)
#define ITEM_INFAMMO_DURATION                 (30*TIME_SECOND)

#define SNACK_ITEM_MAX_COUNT                  (16)

#define MAX_MOB_SPAWN_PARAMS                  (10)
#define MAX_MOB_COMPLEXITY_DRAWN              (20000)
#define MAX_MOB_COMPLEXITY_DRAWN_DZO          (30000)
#define MOB_COMPLEXITY_SKIN_FACTOR            (1000)
#define MOB_COMPLEXITY_LOD_FACTOR             (2500)
#define MAX_MOB_COMPLEXITY_MIN                (10000)

enum GameNetMessage
{
	CUSTOM_MSG_ROUND_COMPLETE = CUSTOM_MSG_ID_GAME_MODE_START,
	CUSTOM_MSG_ROUND_START,
	CUSTOM_MSG_UPDATE_SPAWN_VARS,
	CUSTOM_MSG_WEAPON_UPGRADE,
	CUSTOM_MSG_REVIVE_PLAYER,
	CUSTOM_MSG_PLAYER_DIED,
	CUSTOM_MSG_PLAYER_SET_WEAPON_MODS,
	CUSTOM_MSG_PLAYER_SET_STATS,
	CUSTOM_MSG_PLAYER_SET_DOUBLE_POINTS,
	CUSTOM_MSG_PLAYER_SET_DOUBLE_XP,
	CUSTOM_MSG_PLAYER_SET_FREEZE,
  CUSTOM_MSG_PLAYER_USE_ITEM,
  CUSTOM_MSG_MOB_UNRELIABLE_MSG
};

enum BakedSpawnpointType
{
	BAKED_SPAWNPOINT_NONE = 0,
	BAKED_SPAWNPOINT_UPGRADE = 1,
	BAKED_SPAWNPOINT_PLAYER_START = 2,
	BAKED_SPAWNPOINT_MYSTERY_BOX = 3,
	BAKED_SPAWNPOINT_DEMON_BELL = 4,
};

enum MobStatId
{
  MOB_STAT_NONE               = 0,
  MOB_STAT_ZOMBIE             = 1,
  MOB_STAT_ZOMBIE_FREEZE      = 2,
  MOB_STAT_ZOMBIE_ACID        = 3,
  MOB_STAT_ZOMBIE_GHOST       = 4,
  MOB_STAT_ZOMBIE_EXPLODE     = 5,
  MOB_STAT_TREMOR             = 6,
  MOB_STAT_EXECUTIONER        = 7,
  MOB_STAT_SWARMER            = 8,
  MOB_STAT_REACTOR            = 9,
  MOB_STAT_COUNT
};

struct MobConfig;
struct MobSpawnEventArgs;

typedef void (*UpgradePlayerWeapon_func)(int playerId, int weaponId, int giveAlphaMod);
typedef void (*PushSnack_func)(char * string, int ticksAlive, int localPlayerIdx);
typedef void (*PopulateSpawnArgs_func)(struct MobSpawnEventArgs* output, struct MobConfig* config, int spawnParamsIdx, int isBaseConfig, int freeAgent);
typedef void (*MapOnMobSpawned_func)(Moby* moby);
typedef int (*MapOnMobCreate_func)(int spawnParamsIdx, VECTOR position, float yaw, int spawnFromUID, int freeAgent, struct MobConfig *config);

typedef struct SurvivalBakedSpawnpoint
{
	enum BakedSpawnpointType Type;
	int Params;
	float Position[3];
	float Rotation[3];
} SurvivalBakedSpawnpoint_t;

typedef struct SurvivalBakedConfig
{
	float Difficulty;
	SurvivalBakedSpawnpoint_t BakedSpawnPoints[BAKED_SPAWNPOINT_COUNT];
} SurvivalBakedConfig_t;

struct SurvivalPlayerState
{
	u64 TotalBolts;
	u32 XP;
	int Bolts;
	int Kills;
	int Revives;
	int TimesRevived;
	int TimesRevivedSinceLastFullDeath;
	int TotalTokens;
	int CurrentTokens;
  int Item;
  int BestRound;
  int TimesRolledMysteryBox;
  int TimesActivatedDemonBell;
  int TimesActivatedPower;
  int TokensUsedOnGates;
	int KillsPerMob[MAX_MOB_SPAWN_PARAMS];
	short DeathsByMob[MAX_MOB_SPAWN_PARAMS];
	short Upgrades[UPGRADE_COUNT];
	short AlphaMods[8];
	char BestWeaponLevel[9];
};

struct SurvivalPlayer
{
	float MinSqrDistFromMob;
	float MaxSqrDistFromMob;
	struct SurvivalPlayerState State;
	int TimeOfDoublePoints;
	int TimeOfDoubleXP;
  int InvisibilityCloakStopTime;
	u16 ReviveCooldownTicks;
	u8 ActionCooldownTicks;
	u8 MessageCooldownTicks;
	char IsLocal;
	char IsDead;
	char IsInWeaponsMenu;
	char IsDoublePoints;
	char IsDoubleXP;
	char HealthBarStrBuf[8];
};

struct SurvivalState
{
	int RoundNumber;
	int RoundStartTime;
	int RoundCompleteTime;
	int RoundEndTime;
	int RoundMobCount;
	int RoundMobSpawnedCount;
	int RoundMaxMobCount;
	int RoundMaxSpawnedAtOnce;
	int RoundSpawnTicker;
	int RoundSpawnTickerCounter;
	int RoundNextSpawnTickerCounter;
  int RoundDemonBellCount;
	int RoundIsSpecial;
	int RoundSpecialIdx;
	int InitializedTime;
  int DemonBellCount;
  int TotalMobsSpawned;
	int MinMobCost;
	int MobsDrawnCurrent;
	int MobsDrawnLast;
	int MobsDrawGameTime;
	struct SurvivalPlayer PlayerStates[GAME_MAX_PLAYERS];
  char ClientReady[GAME_MAX_PLAYERS];
	int RoundInitialized;
	Moby* Vendor;
	Moby* BigAl;
	Moby* UpgradeMobies[UPGRADE_COUNT];
  Moby* GateMobies[GATE_MAX_COUNT];
  Moby* MysteryBoxMoby;
	struct SurvivalPlayer* LocalPlayerState;
	int GameOver;
	int WinningTeam;
	int ActivePlayerCount;
	int IsHost;
	float Difficulty;
	int TimeOfFreeze;
  int InfiniteAmmoStopTime;
	short DropCooldownTicks;
	char Freeze;
	char NumTeams;
	char RoundSpecialSpawnableZombies[2];
};

struct SurvivalMapConfig
{
  u32 Magic;
  int ClientsReady;
  struct SurvivalState* State;
  struct SurvivalBakedConfig* BakedConfig;

  struct MobSpawnParams* DefaultSpawnParams;
  int DefaultSpawnParamsCount; 
  struct SurvivalSpecialRoundParam* SpecialRoundParams;
  int SpecialRoundParamsCount; 
  
  UpgradePlayerWeapon_func UpgradePlayerWeaponFunc;
  PushSnack_func PushSnackFunc;
  PopulateSpawnArgs_func PopulateSpawnArgsFunc;
  MapOnMobCreate_func OnMobCreateFunc;
  MapOnMobSpawned_func OnMobSpawnedFunc;
};

struct SurvivalSpecialRoundParam
{
	int SpawnParamCount;
	float SpawnCountFactor;
  float SpawnRateFactor;
	int MaxSpawnedAtOnce;
	char SpawnParamIds[4];
	char Name[32];
};

struct SurvivalGameData
{
	u32 Version;
	u32 RoundNumber;
	u64 Points[GAME_MAX_PLAYERS];
	int Kills[GAME_MAX_PLAYERS];
	int Revives[GAME_MAX_PLAYERS];
	int TimesRevived[GAME_MAX_PLAYERS];
  int KillsPerMob[GAME_MAX_PLAYERS][MAX_MOB_SPAWN_PARAMS];
  short DeathsByMob[GAME_MAX_PLAYERS][MAX_MOB_SPAWN_PARAMS];
  short MobIds[MAX_MOB_SPAWN_PARAMS];
	short BestRound[GAME_MAX_PLAYERS];
	short PlayerUpgrades[GAME_MAX_PLAYERS][UPGRADE_COUNT];
	short TimesRolledMysteryBox[GAME_MAX_PLAYERS];
	short TimesActivatedDemonBell[GAME_MAX_PLAYERS];
	short TimesActivatedPower[GAME_MAX_PLAYERS];
  short TokensUsedOnGates[GAME_MAX_PLAYERS];
	char AlphaMods[GAME_MAX_PLAYERS][8];
	char BestWeaponLevel[GAME_MAX_PLAYERS][9];
};

typedef struct SurvivalRoundCompleteMessage
{
	int GameTime;
	int BoltBonus;
} SurvivalRoundCompleteMessage_t;

typedef struct SurvivalWeaponUpgradeMessage
{
	char PlayerId;
	char WeaponId;
	char Level;
	char Alphamod;
} SurvivalWeaponUpgradeMessage_t;

typedef struct SurvivalRoundStartMessage
{
	int GameTime;
	int RoundNumber;
} SurvivalRoundStartMessage_t;

typedef struct SurvivalReviveMessage
{
	int PlayerId;
	int FromPlayerId;
} SurvivalReviveMessage_t;

typedef struct SurvivalSetPlayerDeadMessage
{
	int PlayerId;
	char IsDead;
} SurvivalSetPlayerDeadMessage_t;

typedef struct SurvivalSetWeaponModsMessage
{
	int PlayerId;
	u8 WeaponId;
	u8 Mods[10];
} SurvivalSetWeaponModsMessage_t;

typedef struct SurvivalSetPlayerStatsMessage
{
	int PlayerId;
	struct SurvivalPlayerState Stats;
} SurvivalSetPlayerStatsMessage_t;

typedef struct SurvivalSetPlayerDoublePointsMessage
{
	int TimeOfDoublePoints[GAME_MAX_PLAYERS];
	char IsActive[GAME_MAX_PLAYERS];
} SurvivalSetPlayerDoublePointsMessage_t;

typedef struct SurvivalSetPlayerDoubleXPMessage
{
	int TimeOfDoubleXP[GAME_MAX_PLAYERS];
	char IsActive[GAME_MAX_PLAYERS];
} SurvivalSetPlayerDoubleXPMessage_t;

typedef struct SurvivalSetFreezeMessage
{
	char IsActive;
} SurvivalSetFreezeMessage_t;

typedef struct SurvivalPlayerUseItem
{
	int PlayerId;
  enum MysteryBoxItem Item;
} SurvivalPlayerUseItem_t;

struct SurvivalSnackItem
{
  int TicksAlive;
  char DisplayForLocalPlayerIdx;
  char Str[64];
};

extern const int UPGRADE_COST[];
extern const float BOLT_TAX[];
extern const float DIFFICULTY_MAP[];
extern const short WEAPON_PICKUP_BASE_RESPAWN_TIMES[];
extern const short WEAPON_PICKUP_PLAYER_RESPAWN_TIME_OFFSETS[];
extern SurvivalBakedConfig_t BakedConfig;

struct GuberMoby* getGuber(Moby* moby);
int handleEvent(Moby* moby, GuberEvent* event);

#endif // SURVIVAL_GAME_H
