#ifndef RAIDS_TRACKER_H
#define RAIDS_TRACKER_H

#include <tamtypes.h>
#include <libdl/moby.h>
#include <libdl/math.h>
#include <libdl/time.h>
#include <libdl/player.h>
#include <libdl/math3d.h>

void trackerLogKill(int killedByPlayerId, u64 bolts, u64 playerXp, u64 weaponXp, int weaponId);
void trackerLogPlayerLevelUp(void);
void trackerLogWeaponLevelUp(int weaponId);
void trackerLogLootDrop(void);
void trackerTick(void);
void trackerInit(void);

#endif // RAIDS_TRACKER_H
