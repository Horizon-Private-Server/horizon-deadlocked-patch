#include "include/game.h"
#include <libdl/math3d.h>

const int UPGRADE_COST[] = {
	8000,			// v2
	12000,		// v3
	20000,		// v4
	40000,		// v5
	60000,		// v6
	90000,		// v7
	150000,		// v8
	220000,		// v9
	350000,		// v10
};

const float BOLT_TAX[] = {
	1.00,			// 1 player
	0.90,			// 2 players
	0.80,			// 3 players
	0.70,			// 4 players
	0.60,			// 5 players
	0.55,			// 6 players
	0.50,			// 7 players
	0.45,			// 8 players
	0.40,			// 9 players
	0.40,			// 10 players
};

const float DIFFICULTY_MAP[] = {
	0.25,			// Couch Potato
	0.50,			// Contestant
	0.75,			// Gladiator
	1.00,			// Hero
	1.50,			// Exterminator
};

extern const short WEAPON_RESPAWN_TIMES[] = {
	30, // 1 player (vanilla is 30)
	30, // 2 players
	25, // 3 players
	20, // 4 players
	18, // 5 players
	16, // 6 players
	14, // 7 players
	12, // 8 players
	9, // 9 players
	6, // 10 players
};

SurvivalBakedConfig_t BakedConfig __attribute__((section(".config"))) = {
	1.0
};
