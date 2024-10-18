#ifndef RAIDS_SPAWNER_H
#define RAIDS_SPAWNER_H

#include <tamtypes.h>
#include <libdl/moby.h>
#include <libdl/math.h>
#include <libdl/time.h>
#include <libdl/player.h>
#include <libdl/math3d.h>

#define SPAWNER_OCLASS                        (0x4001)
#define SPAWNER_MAX_SPAWN_CUBOIDS             (4)
#define SPAWNER_MAX_TRIGGER_CUBOIDS           (4)
#define SPAWNER_MAX_MOB_TYPES                 (8)

enum SpawnerEventType {
	SPAWNER_EVENT_SPAWN,
  SPAWNER_EVENT_ACTIVATE,
  SPAWNER_EVENT_DEACTIVATE,
};

enum SpawnerState {
	SPAWNER_STATE_DEACTIVATED,
	SPAWNER_STATE_ACTIVATED,
};

struct SpawnerMobParams
{
  int MobParamIdx;
  float Probability;
  float DifficultyMultiplier;
  int MaxCanSpawnOrUnlimited;
};

struct SpawnerPVar
{
  int Init;
  enum SpawnerState DefaultState;
  int TriggerCuboidIds[SPAWNER_MAX_TRIGGER_CUBOIDS];
  int SpawnCuboidIds[SPAWNER_MAX_SPAWN_CUBOIDS];
  int PathGraphIdx;
  int NumMobsToSpawn;
  struct SpawnerMobParams SpawnableMobParam[SPAWNER_MAX_MOB_TYPES];
};

struct GuberMoby* spawnerGetGuber(Moby* moby);
int spawnerHandleEvent(Moby* moby, GuberEvent* event);
void spawnerInit(void);

#endif // RAIDS_SPAWNER_H
