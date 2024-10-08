#ifndef SURVIVAL_GAME_H
#define SURVIVAL_GAME_H

#include <tamtypes.h>
#include "messageid.h"
#include <libdl/player.h>
#include <libdl/math3d.h>

#define MAP_CONFIG_MAGIC                      (0xDEADBEEF)

#define TPS																		(60)

#define ZOMBIE_MOBY_OCLASS										(0x20F6)
#define EXECUTIONER_MOBY_OCLASS							  (0x2468)
#define TREMOR_MOBY_OCLASS							      (0x24D3)
#define SWARMER_MOBY_OCLASS							      (0x2695)
#define REAPER_MOBY_OCLASS							      (0x2570)
#define REACTOR_MOBY_OCLASS							      (0x20BE)

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

#define ROUND_MESSAGE_DURATION_MS							(TIME_SECOND * 2)
#define ROUND_START_DELAY_MS									(TIME_SECOND * 1)

#if QUICK_SPAWN
#define ROUND_TRANSITION_DELAY_MS							(TIME_SECOND * 0)
#else
#define ROUND_TRANSITION_DELAY_MS							(TIME_SECOND * 45)
#endif

#define ROUND_BASE_BOLT_BONUS									(100)
#define ROUND_MAX_BOLT_BONUS									(10000)

#define ROUND_SPECIAL_BONUS_MULTIPLIER				(5)

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

#define DROP_COOLDOWN_TICKS_MIN								(TPS * 10)
#define DROP_COOLDOWN_TICKS_MAX								(TPS * 60)
#define DROP_DURATION													(30 * TIME_SECOND)
#define DOUBLE_POINTS_DURATION								(20 * TIME_SECOND)
#define DOUBLE_XP_DURATION								    (20 * TIME_SECOND)
#define FREEZE_DROP_DURATION									(10 * TIME_SECOND)
#define MOB_HAS_DROP_PROBABILITY						  (0.01)
#define MOB_HAS_DROP_PROBABILITY_LUCKY			  (0.05)
#define DROP_MAX_SPAWNED											(4)

#define PLAYER_BASE_REVIVE_TICKS					    (60 * TPS)
#define PLAYER_MIN_REVIVE_TICKS					      (10 * TPS)
#define PLAYER_REVIVE_COST_PER_REVIVE_TICKS		(10 * TPS)
#define PLAYER_TIME_TO_REVIVE_TICKS		        (5 * TPS)
#define PLAYER_REVIVE_MAX_DIST								(2.5)
#define PLAYER_REVIVE_COOLDOWN_TICKS					(120)
#define PLAYER_KNOCKBACK_BASE_POWER						(3.0)
#define PLAYER_KNOCKBACK_BASE_TICKS						(10)
#define PLAYER_COLL_RADIUS          					(0.5)
#define PLAYER_MAX_BLESSINGS                  (4)

#define BIG_AL_MAX_DIST												(5)
#define WEAPON_VENDOR_MAX_DIST								(3)
#define WEAPON_UPGRADE_COOLDOWN_TICKS					(15)
#define WEAPON_MENU_COOLDOWN_TICKS						(60)
#define VENDOR_MAX_WEAPON_LEVEL								(9)

#define PRESTIGE_MACHINE_MAX_DIST							(5)
#define PRESTIGE_MACHINE_BASE_COST            (100000)
#define PRESTIGE_MACHINE_COST_PER_LEVEL       (100000)
#define WEAPON_PRESTIGE_MAX                   (5)

#define PLAYER_UPGRADE_DAMAGE_FACTOR          (0.08)
#define PLAYER_UPGRADE_SPEED_FACTOR           (0.03)
#define PLAYER_UPGRADE_HEALTH_FACTOR          (5)
#define PLAYER_UPGRADE_MEDIC_FACTOR           (0.05)
#define PLAYER_UPGRADE_VENDOR_FACTOR          (0.02)
#define PLAYER_UPGRADE_CRIT_FACTOR            (0.01)

#define BAKED_SPAWNPOINT_COUNT							  (32)

#define ITEM_INVISCLOAK_DURATION              (30*TIME_SECOND)
#define ITEM_INFAMMO_DURATION                 (30*TIME_SECOND)
#define ITEM_QUAD_DURATION_TPS                (1*60*TPS)
#define ITEM_SHIELD_DURATION_TPS              (1*60*TPS)
#define ITEM_EMP_HEALTH_EFFECT_RADIUS         (15)
#define ITEM_HEALTHTORNADO_DURATION           (10*TIME_SECOND)
#define ITEM_HEALTHTORNADO_PERIOD_TICKS       (TPS * 0.25)
#define ITEM_HEALTHTORNADO_HEAL_PERCENT       (0.05)

#define ITEM_BLESSING_HEALTH_REGEN_RATE_TPS   (TPS * 0.2)
#define ITEM_BLESSING_AMMO_REGEN_RATE_TPS     (TPS * 5)
#define ITEM_BLESSING_THORN_DAMAGE_FACTOR     (0.2)
#define ITEM_BLESSING_MULTI_JUMP_COUNT        (5)

#define ITEM_STACKABLE_HOVERBOOTS_DUR_TPS     (2 * TPS)
#define ITEM_STACKABLE_HOVERBOOTS_SPEED_BUF   (0.1)
#define ITEM_STACKABLE_LOW_HEALTH_DMG_BUF_FAC (0.75)
#define ITEM_STACKABLE_LOW_HEALTH_DMG_BUF_RAMP (0.5)
#define ITEM_STACKABLE_ALPHA_MOD_AMT          (2)
#define ITEM_STACKABLE_VAMPIRE_HEALTH_AMT     (3)

#define SNACK_ITEM_MAX_COUNT                  (16)
#define DAMAGE_BUBBLE_MAX_COUNT               (16)

#define MAX_MOB_SPAWN_PARAMS                  (10)
#define MAX_MOB_COMPLEXITY_DRAWN              (7500)
#define MAX_MOB_COMPLEXITY_DRAWN_DZO          (MAX_MOB_COMPLEXITY_DRAWN * 1)
#define MOB_COMPLEXITY_SKIN_FACTOR            (500)
#define MAX_MOB_COMPLEXITY_MIN                (1000)
#define MOB_COMPLEXITY_LOD_FACTOR             (500)
#define MOB_MAX_FLINCH_PROBABILITY            (0.25)
#define MOB_FORCED_BLIP_COOLDOWN_TICKS        (TPS * 5)

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

enum BakedSpawnpointType
{
	BAKED_SPAWNPOINT_NONE = 0,
	BAKED_SPAWNPOINT_UPGRADE = 1,
	BAKED_SPAWNPOINT_PLAYER_START = 2,
	BAKED_SPAWNPOINT_MYSTERY_BOX = 3,
	BAKED_SPAWNPOINT_DEMON_BELL = 4,
	BAKED_SPAWNPOINT_STACK_BOX = 5,
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
  MOB_STAT_REAPER             = 10,
  MOB_STAT_COUNT
};

enum BlessingItemId
{
  BLESSING_ITEM_NONE           = 0,
  BLESSING_ITEM_MULTI_JUMP     = 1,
  BLESSING_ITEM_LUCK           = 2,
  BLESSING_ITEM_BULL           = 3,
  BLESSING_ITEM_ELEM_IMMUNITY  = 4,
  BLESSING_ITEM_HEALTH_REGEN   = 5,
  BLESSING_ITEM_AMMO_REGEN     = 6,
  BLESSING_ITEM_THORNS         = 7,
  BLESSING_ITEM_COUNT
};

enum StackableItemId
{
  STACKABLE_ITEM_LOW_HEALTH_DMG_BUF     = 0, // stack dmg buf
  STACKABLE_ITEM_EXTRA_JUMP             = 1, // stack +1 jump
  STACKABLE_ITEM_EXTRA_SHOT             = 2, // stack +1 shot (dmg mult)
  STACKABLE_ITEM_HOVERBOOTS             = 3, // stack movement speed
  STACKABLE_ITEM_ALPHA_MOD_SPEED        = 4, // stack +2 speed mod
  STACKABLE_ITEM_ALPHA_MOD_IMPACT       = 5, // stack +2 impact mod
  STACKABLE_ITEM_ALPHA_MOD_AREA         = 6, // stack +2 area mod
  STACKABLE_ITEM_ALPHA_MOD_AMMO         = 7, // stack +2 ammo mod
  STACKABLE_ITEM_VAMPIRE                = 8, // stack +X health gain
  STACKABLE_ITEM_COUNT
};

typedef void (*PushSnack_func)(char * string, int ticksAlive, int localPlayerIdx);
typedef int (*FrameTick_func)(void);

typedef struct SurvivalBakedConfig
{
	float Difficulty;
  float SpawnDistanceFactor;
  int BoltRankMultiplier;
} SurvivalBakedConfig_t;

struct SurvivalPlayerState
{
	u64 TotalBolts;
	u32 XP;
	int Bolts;
	int Kills;
	int Deaths;
  int Level;
};

struct SurvivalPlayer
{
	float MinSqrDistFromMob;
	float MaxSqrDistFromMob;
  float LastHealth;
	struct SurvivalPlayerState State;
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

struct SurvivalMobStats
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

struct SurvivalState
{
	int InitializedTime;
  int MapBaseComplexity;
  struct SurvivalMobStats MobStats;
	struct SurvivalPlayer PlayerStates[GAME_MAX_PLAYERS];
  char ClientReady[GAME_MAX_PLAYERS];
	int RoundInitialized;
	Moby* Vendor;
	Moby* BigAl;
	struct SurvivalPlayer* LocalPlayerState;
	int GameOver;
	int WinningTeam;
	int ActivePlayerCount;
	int IsHost;
	float Difficulty;
	char NumTeams;
};

struct SurvivalMapConfig
{
  u32 Magic;
  int ClientsReady;
  struct SurvivalState* State;
  struct SurvivalBakedConfig* BakedConfig;
};

struct SurvivalGameData
{
	u32 Version;
	u64 Points[GAME_MAX_PLAYERS];
	int Kills[GAME_MAX_PLAYERS];
	int Deaths[GAME_MAX_PLAYERS];
};

struct SurvivalSnackItem
{
  int TicksAlive;
  char DisplayForLocalPlayerIdx;
  char Str[64];
};

struct GuberMoby* getGuber(Moby* moby);
int handleEvent(Moby* moby, GuberEvent* event);

#endif // SURVIVAL_GAME_H
