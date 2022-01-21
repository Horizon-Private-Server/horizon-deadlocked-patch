#ifndef SURVIVAL_GAME_H
#define SURVIVAL_GAME_H

#include <tamtypes.h>
#include "messageid.h"
#include <libdl/player.h>
#include <libdl/math3d.h>

#define TPS																		(60)

#define BUDGET_START													(100)
#define BUDGET_START_ACCUM										(50)
#define BUDGET_PLAYER_WEIGHT									(50)

#define ROUND_MESSAGE_DURATION_MS							(TIME_SECOND * 2)

#if QUICK_SPAWN
#define ROUND_START_DELAY_MS									(TIME_SECOND * 1)
#else
#define ROUND_START_DELAY_MS									(TIME_SECOND * 10)
#endif

#define ROUND_BASE_BOLT_BONUS									(100)
#define ROUND_MAX_BOLT_BONUS									(10000)

#define MAX_MOBS_SPAWNED											(50)
#define MAX_MOBS_PER_ROUND										(200)

#define MOB_SPAWN_NEAR_PLAYER_PROBABILITY 		(0.2)
#define MOB_SPAWN_AT_PLAYER_PROBABILITY 			(0.01)

#define ZOMBIE_BASE_DAMAGE										(15)
#define ZOMBIE_MAX_DAMAGE											(100)
#define ZOMBIE_DAMAGE_MUTATE									(0.02)

#define ZOMBIE_BASE_SPEED											(0.08)
#define ZOMBIE_MAX_SPEED											(2)
#define ZOMBIE_SPEED_MUTATE										(0.02)

#define ZOMBIE_BASE_HEALTH										(40)
#define ZOMBIE_MAX_HEALTH											(1000)
#define ZOMBIE_HEALTH_MUTATE									(0.01)

#define ZOMBIE_COST_MUTATE										(5)

#define ZOMBIE_BASE_REACTION_TICKS						(0.25 * TPS)
#define ZOMBIE_BASE_ATTACK_COOLDOWN_TICKS			(2 * TPS)
#define ZOMBIE_BASE_EXPLODE_RADIUS						(5)
#define ZOMBIE_MELEE_HIT_RADIUS								(1)
#define ZOMBIE_EXPLODE_HIT_RADIUS							(5)
#define ZOMBIE_MELEE_ATTACK_RADIUS						(2.5)

#if PAYDAY
#define ZOMBIE_BASE_BOLTS											(10000)
#else
#define ZOMBIE_BASE_BOLTS											(150)
#endif

#define PLAYER_BASE_REVIVE_COST								(5000)
#define PLAYER_REVIVE_COST_PER_ROUND					(0)
#define PLAYER_REVIVE_MAX_DIST								(2.5)

#define WEAPON_UPGRADE_BASE_COST							(5000)
#define WEAPON_UPGRADE_COST_PER_UPGRADE				(10000)
#define WEAPON_VENDOR_MAX_DIST								(3)
#define WEAPON_UPGRADE_COOLDOWN_TICKS					(60)
#define VENDOR_MAX_WEAPON_LEVEL								(9)


enum GameNetMessage
{
	CUSTOM_MSG_ROUND_COMPLETE = CUSTOM_MSG_ID_GAME_MODE_START,
	CUSTOM_MSG_ROUND_START,
	CUSTOM_MSG_UPDATE_SPAWN_VARS,
	CUSTOM_MSG_WEAPON_UPGRADE
};

struct SurvivalPlayerState
{
	int Bolts;
};

struct SurvivalPlayer
{
	Player * Player;
	struct SurvivalPlayerState State;
	u8 UpgradeCooldownTicks;
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
	int RoundSpawnTicker;
	int MinMobCost;
	struct SurvivalPlayer PlayerStates[GAME_MAX_PLAYERS];
	int RoundInitialized;
	Moby* Vendor;
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

typedef struct SurvivalConfig
{
	float Difficulty;
} SurvivalConfig_t;

// Where the defending team spawns
extern SurvivalConfig_t Config __attribute__((section(".config")));

#endif // SURVIVAL_GAME_H
