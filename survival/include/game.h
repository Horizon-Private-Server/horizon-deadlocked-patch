#ifndef SURVIVAL_GAME_H
#define SURVIVAL_GAME_H

#include <tamtypes.h>
#include "messageid.h"
#include <libdl/player.h>
#include <libdl/math3d.h>

#define TPS																		(60)

#define ZOMBIE_MOBY_OCLASS										(0x20F6)

#define BUDGET_START													(200)
#define BUDGET_START_ACCUM										(100)
#define BUDGET_PLAYER_WEIGHT									(100)

#define MAX_MOBS_BASE													(30)
#define MAX_MOBS_ROUND_WEIGHT									(10)
#define MAX_MOBS_SPAWNED											(70)

#define ROUND_MESSAGE_DURATION_MS							(TIME_SECOND * 2)

#if QUICK_SPAWN
#define ROUND_START_DELAY_MS									(TIME_SECOND * 1)
#else
#define ROUND_START_DELAY_MS									(TIME_SECOND * 10)
#endif

#define ROUND_BASE_BOLT_BONUS									(100)
#define ROUND_MAX_BOLT_BONUS									(10000)

#define MOB_SPAWN_NEAR_PLAYER_PROBABILITY 		(0.75)
#define MOB_SPAWN_AT_PLAYER_PROBABILITY 			(0.01)

#define ZOMBIE_BASE_DAMAGE										(15)
#define ZOMBIE_DAMAGE_MUTATE									(0.02)

#define ZOMBIE_BASE_SPEED											(0.08)
#define ZOMBIE_SPEED_MUTATE										(0.001)

#define ZOMBIE_BASE_HEALTH										(40)
#define ZOMBIE_HEALTH_MUTATE									(0.02)

#define ZOMBIE_BASE_REACTION_TICKS						(0.25 * TPS)
#define ZOMBIE_BASE_ATTACK_COOLDOWN_TICKS			(2 * TPS)
#define ZOMBIE_BASE_EXPLODE_RADIUS						(5)
#define ZOMBIE_MELEE_HIT_RADIUS								(1)
#define ZOMBIE_EXPLODE_HIT_RADIUS							(5)
#define ZOMBIE_MELEE_ATTACK_RADIUS						(2.5)

#define ZOMBIE_BASE_STEP_HEIGHT								(2)
#define ZOMBIE_MAX_STEP_HEIGHT								(40)

#define MOB_CORN_LIFETIME_TICKS								(60 * 3)
#define MOB_CORN_MAX_ON_SCREEN								(15)

#if PAYDAY
#define ZOMBIE_BASE_BOLTS											(1000000)
#else
#define ZOMBIE_BASE_BOLTS											(100)
#endif

#define JACKPOT_BOLTS													(50)

#define PLAYER_BASE_REVIVE_COST								(5000)
#define PLAYER_REVIVE_COST_PER_ROUND					(0)
#define PLAYER_REVIVE_MAX_DIST								(2.5)
#define PLAYER_REVIVE_COOLDOWN_TICKS					(120)
#define PLAYER_KNOCKBACK_BASE_POWER						(0.05)
#define PLAYER_KNOCKBACK_BASE_TICKS						(3)

#define BIG_AL_MAX_DIST												(5)
#define WEAPON_VENDOR_MAX_DIST								(3)
#define WEAPON_UPGRADE_COOLDOWN_TICKS					(60)
#define WEAPON_MENU_COOLDOWN_TICKS						(60)
#define VENDOR_MAX_WEAPON_LEVEL								(9)

enum GameNetMessage
{
	CUSTOM_MSG_ROUND_COMPLETE = CUSTOM_MSG_ID_GAME_MODE_START,
	CUSTOM_MSG_ROUND_START,
	CUSTOM_MSG_UPDATE_SPAWN_VARS,
	CUSTOM_MSG_WEAPON_UPGRADE,
	CUSTOM_MSG_REVIVE_PLAYER,
	CUSTOM_MSG_PLAYER_DIED,
	CUSTOM_MSG_PLAYER_SET_WEAPON_MODS,
	CUSTOM_MSG_PLAYER_SET_STATS
};

struct SurvivalPlayerState
{
	int Bolts;
	int TotalBolts;
	int Kills;
	int Revives;
	int TimesRevived;
};

struct SurvivalPlayer
{
	Player * Player;
	float MinSqrDistFromMob;
	float MaxSqrDistFromMob;
	struct SurvivalPlayerState State;
	u16 ReviveCooldownTicks;
	u8 ActionCooldownTicks;
	u8 MessageCooldownTicks;
	char IsLocal;
	char IsDead;
	char IsInWeaponsMenu;
};

struct SurvivalState
{
	int RoundNumber;
	int RoundBudget;
	int RoundStartTime;
	int RoundCompleteTime;
	int RoundEndTime;
	int RoundMobCount;
	int RoundMobSpawnedCount;
	int RoundMaxMobCount;
	int RoundSpawnTicker;
	int MinMobCost;
	int MobsDrawnCurrent;
	int MobsDrawnLast;
	int MobsDrawGameTime;
	u32 CornTicker;
	struct SurvivalPlayer PlayerStates[GAME_MAX_PLAYERS];
	int RoundInitialized;
	Moby* Vendor;
	Moby* BigAl;
	int GameOver;
	int WinningTeam;
	int IsHost;
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
} SurvivalWeaponUpgradeMessage_t;

typedef struct SurvivalRoundStartMessage
{
	int GameTime;
	int RoundNumber;
} SurvivalRoundStartMessage_t;

typedef struct SurvivalReviveMessage
{
	int PlayerId;
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

typedef struct SurvivalConfig
{
	float Difficulty;
} SurvivalConfig_t;

// Where the defending team spawns
extern SurvivalConfig_t Config __attribute__((section(".config")));
extern const int UPGRADE_COST[];

#endif // SURVIVAL_GAME_H
