#ifndef _SURVIVAL_VELDIN_H_
#define _SURVIVAL_VELDIN_H_

#define MAP_BASE_COMPLEXITY         (4000)

// ordered from least to most probable
// never more than MAX_MOB_SPAWN_PARAMS total spawn params
enum MobSpawnParamIds {
	MOB_SPAWN_PARAM_REAPER,
	MOB_SPAWN_PARAM_TREMOR,
	MOB_SPAWN_PARAM_NORMAL,
	MOB_SPAWN_PARAM_COUNT
};

enum GambitIds {
  GAMBIT_ID_NONE = 0,
  GAMBIT_ID_EASY_MODE,
  GAMBIT_ID_IMPOSSIBLE_MODE,
  GAMBIT_ID_HIGH_ROLLER,
  GAMBIT_ID_VIPERS_ONLY,
  GAMBIT_ID_FUSION_ONLY,
  GAMBIT_ID_HOLOS_ONLY,
};

void gambitsOnRoundComplete(int roundNo);
void gambitsTick(void);
void gambitsInit(void);

#endif // _SURVIVAL_VELDIN_H_
