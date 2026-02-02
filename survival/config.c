#include "include/game.h"
#include <libdl/math3d.h>
#include <libdl/utils.h>

const float BOLT_TAX[] = {
  1.00,     // 0 players
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

const char ENABLED_ALPHA_MODS[] = {
  ALPHA_MOD_SPEED,
  ALPHA_MOD_AMMO,
  ALPHA_MOD_IMPACT,
  ALPHA_MOD_AREA,
  ALPHA_MOD_JACKPOT,
  ALPHA_MOD_XP
};

const int ENABLED_ALPHA_MODS_COUNT = COUNT_OF(ENABLED_ALPHA_MODS);
