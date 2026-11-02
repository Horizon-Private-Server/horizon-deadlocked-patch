#ifndef RAIDS_TRACKER_H
#define RAIDS_TRACKER_H

#include <tamtypes.h>
#include <libdl/moby.h>
#include <libdl/math.h>
#include <libdl/time.h>
#include <libdl/player.h>
#include <libdl/math3d.h>

void trackerLogKill(int killedByPlayerId, u64 bolts, u64 playerXp);
void trackerLogLootDrop(void);
void trackerTick(void);
void trackerDraw(void);
void trackerInit(void);

#endif // RAIDS_TRACKER_H
