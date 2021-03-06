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

SurvivalBakedConfig_t BakedConfig __attribute__((section(".config"))) = {
	1.0
};
