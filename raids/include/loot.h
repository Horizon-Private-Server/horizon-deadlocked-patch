#ifndef RAIDS_LOOT_H
#define RAIDS_LOOT_H

#include <tamtypes.h>
#include <libdl/moby.h>
#include <libdl/math.h>
#include <libdl/time.h>
#include <libdl/player.h>
#include <libdl/math3d.h>
#include "bank.h"

enum RaidsGenerateLootType
{
  LOOT_DROP_TYPE_MOB_DEATH = 0,
  LOOT_DROP_TYPE_MISSION_COMPLETE = 1,
  LOOT_DROP_TYPE_PRESTIGE = 2,
};

struct RaidsGenerateLootDropRequest
{
  VECTOR Position;
  int Type;
  int DifficultyStars;
  float MobHealth;
  float MobDamage;
  float MobSpeed;
  int KilledWithGadgetId;
  u16 MobMobyOClass;
};

struct RaidsGenerateLootDropResponse
{
  VECTOR Position;
  int Type;
  RaidsInventoryItem_t Drop;
};

void lootRequestFromMob(Moby* mob, int gadgetId);
void lootRequestFromPrestige(int gadgetId);
void lootRequestFromMissionComplete(VECTOR position);
void lootTick(void);
void lootInit(void);

#endif // RAIDS_LOOT_H
