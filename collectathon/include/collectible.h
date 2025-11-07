#ifndef COLLECTATHON_COLLECTIBLE_H
#define COLLECTATHON_COLLECTIBLE_H

#include <tamtypes.h>
#include <libdl/moby.h>
#include <libdl/math.h>
#include <libdl/time.h>
#include <libdl/player.h>
#include <libdl/math3d.h>

#define COLLECTIBLE_OCLASS                        (0x400E)

struct CollectiblePVar
{
  char Collected;
  char Difficulty;
  char Blip;
};

void collectibleInit(void);

#endif // COLLECTATHON_COLLECTIBLE_H
