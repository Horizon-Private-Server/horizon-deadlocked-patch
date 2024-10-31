#ifndef RAIDS_LOOT_H
#define RAIDS_LOOT_H

#include <tamtypes.h>
#include <libdl/moby.h>
#include <libdl/math.h>
#include <libdl/time.h>
#include <libdl/player.h>
#include <libdl/math3d.h>
#include "bank.h"

struct RaidsGenerateLootDropRequest
{
  VECTOR Position;
  int Type;
  float MobHealth;
  float MobDamage;
  float MobSpeed;
  float MobDifficulty;
  int KilledWithGadgetId;
  u16 MobMobyOClass;
};

struct RaidsGenerateLootDropResponse
{
  VECTOR Position;
  RaidsInventoryWeapon_t Drop;
};

void lootRequestFromMob(Moby* mob, int gadgetId);
void lootRequestFromMissionComplete(VECTOR position);
void lootTick(void);
void lootInit(void);

#endif // RAIDS_LOOT_H
