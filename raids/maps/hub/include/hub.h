#ifndef _RAIDS_HUB_H_
#define _RAIDS_HUB_H_

#define MAP_BASE_COMPLEXITY         (4000)

// ordered from least to most probable
// never more than MAX_MOB_SPAWN_PARAMS total spawn params
enum MobSpawnParamIds {
	MOB_SPAWN_PARAM_NORMAL,
	MOB_SPAWN_PARAM_SWARMER,
	MOB_SPAWN_PARAM_COUNT
};

#endif // _RAIDS_HUB_H_
