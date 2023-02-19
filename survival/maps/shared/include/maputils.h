

#ifndef SURVIVAL_MAP_UTILS_H
#define SURVIVAL_MAP_UTILS_H

#include <tamtypes.h>
#include <libdl/moby.h>
#include <libdl/math.h>
#include <libdl/time.h>
#include <libdl/player.h>
#include <libdl/sound.h>
#include "game.h"


void playPaidSound(Player* player);
int tryPlayerInteract(Moby* moby, Player* player, char* message, int boltCost, int tokenCost, int actionCooldown, float sqrDistance);
GuberEvent* guberCreateEvent(Moby* moby, u32 eventType);

#endif // SURVIVAL_MAP_UTILS_H
