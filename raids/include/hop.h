#ifndef RAIDS_HOP_H
#define RAIDS_HOP_H

#include <tamtypes.h>
#include <libdl/moby.h>
#include <libdl/math.h>
#include <libdl/time.h>
#include <libdl/player.h>
#include <libdl/math3d.h>

struct HopOnBeginMsg
{
  int LoadAtTime;
  int Difficulty;
  char MapFilename[64];
};

void hopLoadMapStats(char* mapFilename);
void hopBegin(char* mapFilename, int difficulty, int cost, int delayMs);

void hopTick(void);
void hopInit(void);

#endif // RAIDS_HOP_H
