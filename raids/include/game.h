#ifndef RAIDS_GAME_H
#define RAIDS_GAME_H

#include <tamtypes.h>
#include <libdl/player.h>
#include <libdl/math3d.h>
#include "messageid.h"
#include "bank.h"

#define MAP_CONFIG_MAGIC                      (0xDEADBEEF)

#define TPS																		(60)

#define ZOMBIE_MOBY_OCLASS										(0x20F6)
#define EXECUTIONER_MOBY_OCLASS							  (0x2468)
#define TREMOR_MOBY_OCLASS							      (0x24D3)
#define SWARMER_MOBY_OCLASS							      (0x2695)
#define REAPER_MOBY_OCLASS							      (0x2570)
#define REACTOR_MOBY_OCLASS							      (0x20BE)
#define NPC_MOBY_OCLASS                       (0x4006)


#define STATUE_MOBY_OCLASS                    (0x2402)
#define BIGAL_MOBY_OCLASS                     (0x2124)

#define GRAVITY_MAGNITUDE                     (15 * MATH_DT)

#define MOBS_PLAY_SOUND_COOLDOWN              (10)
#define MOBS_PLAY_SOUND_COOLDOWN_MAX_SOUNDIDS (20)

#define MAX_MOBS_BASE													(10)
#define MAX_MOBS_ROUND_WEIGHT									(10)
#define MAX_MOBS_ALIVE											  (60)
#define MAX_MOBS_ALIVE_BUFFER									(10)
#define MAX_MOBS_ALIVE_REAL									  (MAX_MOBS_ALIVE - MAX_MOBS_ALIVE_BUFFER)

#define MOB_TARGET_DIST_IN_SIGHT_IGNORE_PATH 	(100)
#define MOB_MOVE_SKIP_TICKS                   (4)
#define MOB_MAX_STUCK_COUNTER_FOR_NEW_PATH    (5)

#define MOB_SHORT_FREEZE_DURATION_TICKS       (60)
#define MOB_SHORT_FREEZE_SPEED_FACTOR         (0.15)

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

#define MOB_AUTO_DIRTY_COOLDOWN_TICKS			    (60 * 5)

#define MOB_BASE_DAMAGE										    (10)
#define MOB_BASE_DAMAGE_SCALE                 (0.03*1)
#define MOB_BASE_SPEED											  (3)
#define MOB_BASE_SPEED_SCALE                  (0.05*1)
#define MOB_BASE_HEALTH										    (30)
#define MOB_BASE_HEALTH_SCALE                 (0.05*1)

#define MOB_SPECIAL_MUTATION_PROBABILITY		  (0.005)
#define MOB_SPECIAL_MUTATION_BASE_COST			  (200)
#define MOB_SPECIAL_MUTATION_REL_COST			    (1.0)

#if PAYDAY
#define MOB_BASE_BOLTS											  (1000000)
#else
#define MOB_BASE_BOLTS											  (220)
#endif

#define JACKPOT_BOLTS													(50)
#define XP_ALPHAMOD_XP												(10)

#define LEVELUP_XP_LINEAR_RATE                (250)

#define PLAYER_BASE_REVIVE_TICKS					    (60 * TPS)
#define PLAYER_MIN_REVIVE_TICKS					      (10 * TPS)
#define PLAYER_REVIVE_COST_PER_REVIVE_TICKS		(10 * TPS)
#define PLAYER_TIME_TO_REVIVE_TICKS		        (5 * TPS)
#define PLAYER_REVIVE_MAX_DIST								(2.5)
#define PLAYER_REVIVE_COOLDOWN_TICKS					(120)
#define PLAYER_KNOCKBACK_BASE_POWER						(3.0)
#define PLAYER_KNOCKBACK_BASE_TICKS						(10)
#define PLAYER_COLL_RADIUS          					(0.5)

#define BIG_AL_MAX_DIST												(5)
#define WEAPON_VENDOR_MAX_DIST								(3)
#define WEAPON_UPGRADE_COOLDOWN_TICKS					(15)
#define WEAPON_MENU_COOLDOWN_TICKS						(60)
#define VENDOR_MAX_WEAPON_LEVEL								(9)

#define PLAYER_SKILLPOINT_DAMAGE_FACTOR       (0.08)
#define PLAYER_SKILLPOINT_SPEED_FACTOR        (0.03)
#define PLAYER_SKILLPOINT_HEALTH_FACTOR       (5)

#define SNACK_ITEM_MAX_COUNT                  (16)
#define DAMAGE_BUBBLE_MAX_COUNT               (16)

#define MAX_MOB_SPAWN_PARAMS                  (10)
#define MAX_MOB_COMPLEXITY_DRAWN              (7500)
#define MAX_MOB_COMPLEXITY_DRAWN_DZO          (MAX_MOB_COMPLEXITY_DRAWN * 1)
#define MOB_COMPLEXITY_SKIN_FACTOR            (500)
#define MAX_MOB_COMPLEXITY_MIN                (1000)
#define MOB_COMPLEXITY_LOD_FACTOR             (500)
#define MOB_MAX_FLINCH_PROBABILITY            (0.25)

#define SWARMER_RENDER_COST                   (40)
#define ZOMBIE_RENDER_COST                    (85)
#define TREMOR_RENDER_COST                    (150)
#define REAPER_RENDER_COST                    (150)
#define REACTOR_RENDER_COST                   (300)
#define EXECUTIONER_RENDER_COST               (300)

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
  CUSTOM_MSG_MOB_UNRELIABLE_MSG,
	CUSTOM_MSG_WEAPON_PRESTIGE,
	CUSTOM_MSG_INTERACT_BANK_BOX,
  CUSTOM_MSG_WITHDRAWN_BANK_BOX,
  CUSTOM_MSG_SET_ROUND_50_TIME,
};

struct MobConfig;
struct MobSpawnEventArgs;
struct MobCreateArgs;

typedef void (*PushSnack_func)(char * string, int ticksAlive, int localPlayerIdx);
typedef RaidsPlayerBank_t* (*GetBank_func)(void);
typedef void (*SendBankAccountToServer_func)(void);
typedef void (*PopulateSpawnArgs_func)(struct MobSpawnEventArgs* output, struct MobConfig* config, int spawnParamsIdx, int isBaseConfig, float difficultyMult);
typedef void (*RegisterNpc_func)(Moby* moby);
typedef int (*OnGuberEvent_func)(Moby* moby, GuberEvent* event);
typedef struct Guber* (*OnGetGuber_func)(Moby* moby);
typedef int (*TryCreateMob_func)(struct MobCreateArgs* args);

typedef void (*MapOnMobSpawned_func)(Moby* moby);
typedef int (*MapOnMobCreate_func)(struct MobCreateArgs* args);
typedef void (*MapOnMobUpdate_func)(Moby* moby);
typedef void (*MapOnMobKilled_func)(Moby* moby, int killedByPlayerId, int weaponId);
typedef void (*FrameTick_func)(void);

typedef struct RaidsBakedConfig
{
	float Difficulty;
  float SpawnDistanceFactor;
  int BoltRankMultiplier;
} RaidsBakedConfig_t;

struct RaidsPlayerState
{
  u64 Experience;
	int Kills;
	int Deaths;
  u16 Skills[RAIDS_SKILLS_COUNT];
};

struct RaidsPlayer
{
	float MinSqrDistFromMob;
	float MaxSqrDistFromMob;
  float LastHealth;
	struct RaidsPlayerState State;
  RaidsPlayerEquippedInventory_t Inventory;
	int TimeOfDoublePoints;
	int TimeOfDoubleXP;
  int InvisibilityCloakStopTime;
  int HealthTornadoStopTime;
  int HealthTornadoActivateTicks;
  int TicksSinceHealthChanged;
  int RevivingPlayerId;
	u16 ReviveCooldownTicks;
  u16 RevivingPlayerTicks;
	u8 ActionCooldownTicks;
	u8 MessageCooldownTicks;
	char IsLocal;
	char IsDead;
	char IsDoublePoints;
	char IsDoubleXP;
	char HealthBarStrBuf[8];
};

struct RaidsMobStats
{
	int MobsDrawnCurrent;
	int MobsDrawnLast;
	int MobsDrawGameTime;
  int TotalSpawning;
  int TotalAlive;
  int TotalSpawnedThisRound;
  int TotalSpawned;
  int NumSpawnedThisRound[MAX_MOB_SPAWN_PARAMS];
  u8 NumAlive[MAX_MOB_SPAWN_PARAMS];
};

struct RaidsState
{
	int InitializedTime;
  int MapBaseComplexity;
  struct RaidsMobStats MobStats;
	struct RaidsPlayer PlayerStates[GAME_MAX_PLAYERS];
  char ClientReady[GAME_MAX_PLAYERS];
	int InventoryOpen;
	Moby* Vendor;
	Moby* BigAl;
	struct RaidsPlayer* LocalPlayerState;
	int GameOver;
	int WinningTeam;
	int ActivePlayerCount;
	int IsHost;
	float Difficulty;
	char NumTeams;
};

struct RaidsMapConfig
{
  u32 Magic;
  int ClientsReady;
  struct RaidsState* State;
  struct RaidsBakedConfig* BakedConfig;
  struct MobSpawnParams* MobSpawnParams;
  int MobSpawnParamsCount;

  // mode
  PushSnack_func PushSnackFunc;
  GetBank_func GetBankFunc;
  SendBankAccountToServer_func SendBankAccountToServerFunc;
  PopulateSpawnArgs_func PopulateSpawnArgsFunc;
  RegisterNpc_func RegisterNpcFunc;
  OnGuberEvent_func OnGuberEventFunc;
  OnGetGuber_func OnGetGuberFunc;
  TryCreateMob_func TryCreateMobFunc;

  // map
  MapOnMobCreate_func OnMobCreateFunc;
  MapOnMobSpawned_func OnMobSpawnedFunc;
  MapOnMobUpdate_func OnMobUpdateFunc;
  MapOnMobKilled_func OnMobKilledFunc;
  FrameTick_func OnFrameTickFunc;
};

struct RaidsGameData
{
	u32 Version;
	u64 Points[GAME_MAX_PLAYERS];
	int Kills[GAME_MAX_PLAYERS];
	int Deaths[GAME_MAX_PLAYERS];
};

struct RaidsSnackItem
{
  int TicksAlive;
  char DisplayForLocalPlayerIdx;
  char Str[64];
};

struct Guber* getGuber(Moby* moby);
int handleEvent(Moby* moby, GuberEvent* event);

#endif // RAIDS_GAME_H
