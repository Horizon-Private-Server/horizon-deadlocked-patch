#ifndef _SURVIVAL_ORXON_H_
#define _SURVIVAL_ORXON_H_

#include "gaspath.h"
#include "wraith.h"
#include "surge.h"

#define MAP_BASE_COMPLEXITY         (4000)

// ordered from least to most probable
// never more than MAX_MOB_SPAWN_PARAMS total spawn params
enum MobSpawnParamIds {
	MOB_SPAWN_PARAM_TITAN,
	MOB_SPAWN_PARAM_GHOST,
	MOB_SPAWN_PARAM_EXPLOSION,
	MOB_SPAWN_PARAM_ACID,
	MOB_SPAWN_PARAM_FREEZE,
	MOB_SPAWN_PARAM_TREMOR,
	MOB_SPAWN_PARAM_NORMAL,
  MOB_SPAWN_PARAM_SWARMER,
	MOB_SPAWN_PARAM_COUNT
};

enum GambitIds {
  GAMBIT_ID_NONE = 0,
  GAMBIT_ID_EASY_MODE,
  GAMBIT_ID_IMPOSSIBLE_MODE,
  GAMBIT_ID_HYPER_INFLATION,
  GAMBIT_ID_FLAIL_ONLY,
  GAMBIT_ID_MAGMA_CANNON_ONLY,
  GAMBIT_ID_B6_ONLY,
};

void gambitsOnRoundComplete(int roundNo);
void gambitsTick(void);
void gambitsInit(void);

#endif // _SURVIVAL_ORXON_H_
