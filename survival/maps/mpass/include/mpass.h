#ifndef _SURVIVAL_MPASS_H_
#define _SURVIVAL_MPASS_H_

#include "bigalpath.h"
#include "bigal.h"
#include "statue.h"

#define MAP_BASE_COMPLEXITY         (4000)

// ordered from least to most probable
// never more than MAX_MOB_SPAWN_PARAMS total spawn params
enum MobSpawnParamIds {
  MOB_SPAWN_PARAM_REACTOR,
  MOB_SPAWN_PARAM_ACID,
  MOB_SPAWN_PARAM_REAPER,
  MOB_SPAWN_PARAM_TREMOR,
	MOB_SPAWN_PARAM_NORMAL,
  MOB_SPAWN_PARAM_SWARMER,
	MOB_SPAWN_PARAM_COUNT
};

#endif // _SURVIVAL_MPASS_H_
