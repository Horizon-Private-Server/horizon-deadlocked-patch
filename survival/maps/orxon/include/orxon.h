#ifndef _SURVIVAL_ORXON_H_
#define _SURVIVAL_ORXON_H_

#include "gaspath.h"
#include "wraith.h"
#include "surge.h"

// ordered from least to most probable
// never more than MAX_MOB_SPAWN_PARAMS total spawn params
enum MobSpawnParamIds {
	MOB_SPAWN_PARAM_TITAN,
	MOB_SPAWN_PARAM_GHOST,
	MOB_SPAWN_PARAM_EXPLOSION,
	MOB_SPAWN_PARAM_ACID,
	MOB_SPAWN_PARAM_FREEZE,
	MOB_SPAWN_PARAM_RUNNER,
	MOB_SPAWN_PARAM_NORMAL,
	MOB_SPAWN_PARAM_COUNT
};

#endif // _SURVIVAL_ORXON_H_
