#ifndef _SURVIVAL_TORVAL_H_
#define _SURVIVAL_TORVAL_H_

#define MAP_BASE_COMPLEXITY         (5000)

// ordered from least to most probable
// never more than MAX_MOB_SPAWN_PARAMS total spawn params
enum MobSpawnParamIds {
  MOB_SPAWN_PARAM_REACTOR,
  MOB_SPAWN_PARAM_EXECUTIONER,
  MOB_SPAWN_PARAM_LEVIATHAN,
  MOB_SPAWN_PARAM_REAPER,
  MOB_SPAWN_PARAM_NORMAL,
  MOB_SPAWN_PARAM_SWARMER,
  MOB_SPAWN_PARAM_COUNT
};

#endif // _SURVIVAL_TORVAL_H_
