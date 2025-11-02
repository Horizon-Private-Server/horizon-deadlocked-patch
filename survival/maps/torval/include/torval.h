#ifndef _SURVIVAL_TORVAL_H_
#define _SURVIVAL_TORVAL_H_

#define MAP_BASE_COMPLEXITY         (5000)

// ordered from least to most probable
// never more than MAX_MOB_SPAWN_PARAMS total spawn params
enum MobSpawnParamIds {
  MOB_SPAWN_PARAM_REACTOR,
  MOB_SPAWN_PARAM_KING_LEVIATHAN,
  MOB_SPAWN_PARAM_EXECUTIONER,
  MOB_SPAWN_PARAM_LEVIATHAN,
  MOB_SPAWN_PARAM_REAPER,
  MOB_SPAWN_PARAM_NORMAL,
  MOB_SPAWN_PARAM_SWARMER,
  MOB_SPAWN_PARAM_COUNT
};

enum GambitIds {
  GAMBIT_ID_NONE = 0,
  GAMBIT_ID_EASY_MODE,
  GAMBIT_ID_IMPOSSIBLE_MODE,
  GAMBIT_ID_WEAPON_LIFE,
  GAMBIT_ID_BIG_MOBS_ONLY,
  GAMBIT_ID_VAMPIRE,
  GAMBIT_ID_FLAIL_ONLY,
  GAMBIT_ID_FUSION_ONLY,
  GAMBIT_ID_B6_ONLY,
};

void gambitsOnRoundComplete(int roundNo);
void gambitsTick(void);
void gambitsInit(void);

#endif // _SURVIVAL_TORVAL_H_
