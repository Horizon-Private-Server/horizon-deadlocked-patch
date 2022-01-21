#include "include/game.h"
#include <libdl/math3d.h>

// Config for survivor
// This is configured and sent by the server
// to a fixed meemory location (start of module)
// see linkfile and include/game.h for config attribute
SurvivalConfig_t Config = {
  .Difficulty = 1.0,
};
