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

// Config for survivor
// This is configured and sent by the server
// to a fixed meemory location (start of module)
// see linkfile and include/game.h for config attribute
SurvivalConfig_t Config = {
  .Difficulty = 1.0,
};
