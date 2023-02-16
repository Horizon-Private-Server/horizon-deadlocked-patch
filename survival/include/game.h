#ifndef SURVIVAL_GAME_H
#define SURVIVAL_GAME_H

#include <tamtypes.h>
#include "messageid.h"
#include <libdl/player.h>
#include <libdl/math3d.h>
#include "upgrade.h"
#include "gate.h"

#define TPS																		(60)

#define ZOMBIE_MOBY_OCLASS										(0x20F6)

#define GRAVITY_MAGNITUDE                     (15 * MATH_DT)

#define BUDGET_START													(200)
#define BUDGET_START_ACCUM										(100)
#define BUDGET_PLAYER_WEIGHT									(100)

#define MAX_MOBS_BASE													(30)
#define MAX_MOBS_ROUND_WEIGHT									(10)
#define MAX_MOBS_SPAWNED											(50)

#define ROUND_MESSAGE_DURATION_MS							(TIME_SECOND * 2)
#define ROUND_START_DELAY_MS									(TIME_SECOND * 1)

#if QUICK_SPAWN
#define ROUND_TRANSITION_DELAY_MS							(TIME_SECOND * 0)
#else
#define ROUND_TRANSITION_DELAY_MS							(TIME_SECOND * 30)
#endif

#define ROUND_BASE_BOLT_BONUS									(100)
#define ROUND_MAX_BOLT_BONUS									(10000)

#define ROUND_SPECIAL_EVERY										(5)
#define ROUND_SPECIAL_BONUS_MULTIPLIER				(5)

#define MOB_SPAWN_SEMI_NEAR_PLAYER_PROBABILITY 		(0.5)
#define MOB_SPAWN_NEAR_PLAYER_PROBABILITY 				(0.25)
#define MOB_SPAWN_AT_PLAYER_PROBABILITY 					(0.01)
#define MOB_SPAWN_NEAR_HEALTHBOX_PROBABILITY 			(0.1)

#define MOB_SPAWN_BURST_MIN_DELAY							(1 * 60)
#define MOB_SPAWN_BURST_MAX_DELAY							(2 * 60)
#define MOB_SPAWN_BURST_MIN										(5)
#define MOB_SPAWN_BURST_MAX										(30)
#define MOB_SPAWN_BURST_MAX_INC_PER_ROUND			(2)
#define MOB_SPAWN_BURST_MIN_INC_PER_ROUND			(1)

#define ZOMBIE_BASE_DAMAGE										(15)
#define ZOMBIE_DAMAGE_MUTATE									(0.02)

#define ZOMBIE_BASE_SPEED											(2)
#define ZOMBIE_SPEED_MUTATE										(0.001)

#define ZOMBIE_BASE_HEALTH										(40)
#define ZOMBIE_HEALTH_MUTATE									(0.02)

#define ZOMBIE_SPECIAL_MUTATION_PROBABILITY		(0.005)
#define ZOMBIE_SPECIAL_MUTATION_BASE_COST			(200)
#define ZOMBIE_SPECIAL_MUTATION_REL_COST			(1.0)

#define ZOMBIE_BASE_REACTION_TICKS						(0.25 * TPS)
#define ZOMBIE_BASE_ATTACK_COOLDOWN_TICKS			(2 * TPS)
#define ZOMBIE_BASE_EXPLODE_RADIUS						(5)
#define ZOMBIE_MELEE_HIT_RADIUS								(1.75)
#define ZOMBIE_EXPLODE_HIT_RADIUS							(5)
#define ZOMBIE_MELEE_ATTACK_RADIUS						(5)

#define ZOMBIE_BASE_STEP_HEIGHT								(2)
#define ZOMBIE_MAX_STEP_UP										(100)
#define ZOMBIE_MAX_STEP_DOWN									(300)

#define ZOMBIE_ANIM_ATTACK_TICKS							(30)
#define ZOMBIE_TIMEBOMB_TICKS									(60 * 2)
#define ZOMBIE_FLINCH_COOLDOWN_TICKS					(60 * 7)
#define ZOMBIE_ACTION_COOLDOWN_TICKS					(30)
#define ZOMBIE_RESPAWN_AFTER_TICKS						(60 * 30)
#define ZOMBIE_BASE_COLL_RADIUS								(0.5)
#define ZOMBIE_MAX_COLL_RADIUS								(4)
#define ZOMBIE_AUTO_DIRTY_COOLDOWN_TICKS			(60 * 5)
#define ZOMBIE_AMBSND_MIN_COOLDOWN_TICKS    	(60 * 2)
#define ZOMBIE_AMBSND_MAX_COOLDOWN_TICKS    	(60 * 3)

#define ZOMBIE_MAX_WALKABLE_SLOPE             (40 * MATH_DEG2RAD)

#define ZOMBIE_TURN_RADIANS_PER_SEC           (90 * MATH_DEG2RAD)
#define ZOMBIE_MOVE_ACCELERATION              (25)

#if PAYDAY
#define ZOMBIE_BASE_BOLTS											(1000000)
#else
#define ZOMBIE_BASE_BOLTS											(120)
#endif

#define JACKPOT_BOLTS													(50)

#define DROP_COOLDOWN_TICKS_MIN								(TPS * 10)
#define DROP_COOLDOWN_TICKS_MAX								(TPS * 60)
#define DROP_DURATION													(30 * TIME_SECOND)
#define DOUBLE_POINTS_DURATION								(20 * TIME_SECOND)
#define FREEZE_DROP_DURATION									(10 * TIME_SECOND)
#define ZOMBIE_HAS_DROP_PROBABILITY						(0.01)
#define DROP_MAX_SPAWNED											(4)

#define PLAYER_BASE_REVIVE_COST								(5000)
#define PLAYER_REVIVE_COST_PER_PLAYER					(1000)
#define PLAYER_REVIVE_COST_PER_ROUND					(0)
#define PLAYER_REVIVE_MAX_DIST								(2.5)
#define PLAYER_REVIVE_COOLDOWN_TICKS					(120)
#define PLAYER_KNOCKBACK_BASE_POWER						(0.05)
#define PLAYER_KNOCKBACK_BASE_TICKS						(3)
#define PLAYER_COLL_RADIUS          					(0.5)

#define BIG_AL_MAX_DIST												(5)
#define WEAPON_VENDOR_MAX_DIST								(3)
#define WEAPON_UPGRADE_COOLDOWN_TICKS					(60)
#define WEAPON_MENU_COOLDOWN_TICKS						(60)
#define VENDOR_MAX_WEAPON_LEVEL								(9)

#define BAKED_SPAWNPOINT_COUNT							  (16)


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
	CUSTOM_MSG_PLAYER_SET_FREEZE,
};

enum BakedSpawnpointType
{
	BAKED_SPAWNPOINT_NONE = 0,
	BAKED_SPAWNPOINT_UPGRADE = 1,
	BAKED_SPAWNPOINT_PLAYER_START = 2,
};

typedef struct SurvivalBakedSpawnpoint
{
	enum BakedSpawnpointType Type;
	int Params;
	float Position[3];
	float Rotation[3];
} SurvivalBakedSpawnpoint_t;

typedef struct SurvivalBakedConfig
{
	float MapSize;
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
	int Upgrades[UPGRADE_COUNT];
	short AlphaMods[8];
	char BestWeaponLevel[9];
};

struct SurvivalPlayer
{
	float MinSqrDistFromMob;
	float MaxSqrDistFromMob;
	struct SurvivalPlayerState State;
	int TimeOfDoublePoints;
	u16 ReviveCooldownTicks;
	u8 ActionCooldownTicks;
	u8 MessageCooldownTicks;
	char IsLocal;
	char IsDead;
	char IsInWeaponsMenu;
	char IsDoublePoints;
	char HealthBarStrBuf[8];
};

struct SurvivalState
{
	int RoundNumber;
	long RoundBudget;
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
	int RoundIsSpecial;
	int RoundSpecialIdx;
	int InitializedTime;
	int MinMobCost;
	int MobsDrawnCurrent;
	int MobsDrawnLast;
	int MobsDrawGameTime;
	struct SurvivalPlayer PlayerStates[GAME_MAX_PLAYERS];
	int RoundInitialized;
	Moby* Vendor;
	Moby* BigAl;
	Moby* UpgradeMobies[UPGRADE_COUNT];
  Moby* GateMobies[GATE_MAX_COUNT];
	struct SurvivalPlayer* LocalPlayerState;
	int GameOver;
	int WinningTeam;
	int ActivePlayerCount;
	int IsHost;
	float Difficulty;
	int TimeOfFreeze;
	short DropCooldownTicks;
	char Freeze;
	char NumTeams;
	char RoundSpecialSpawnableZombies[2];
};

struct SurvivalSpecialRoundParam
{
	int SpawnParamCount;
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

typedef struct SurvivalSetFreezeMessage
{
	char IsActive;
} SurvivalSetFreezeMessage_t;

extern const int UPGRADE_COST[];
extern const float BOLT_TAX[];
extern const float DIFFICULTY_MAP[];
extern const short WEAPON_PICKUP_BASE_RESPAWN_TIMES[];
extern const short WEAPON_PICKUP_PLAYER_RESPAWN_TIME_OFFSETS[];
extern SurvivalBakedConfig_t BakedConfig;

struct GuberMoby* getGuber(Moby* moby);
int handleEvent(Moby* moby, GuberEvent* event);

#endif // SURVIVAL_GAME_H
