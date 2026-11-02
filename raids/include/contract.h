#ifndef RAIDS_CONTRACT_H
#define RAIDS_CONTRACT_H

#include <tamtypes.h>
#include <libdl/moby.h>
#include <libdl/math.h>
#include <libdl/time.h>
#include <libdl/player.h>
#include <libdl/math3d.h>

int contractCheckForCompletion(int contractIdx);
void contractHandleKill(int mobDefIdx);
void contractMissionComplete(int timeMs);
void contractTick(void);

#endif // RAIDS_CONTRACT_H
