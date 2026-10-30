

#ifndef RAIDS_BUBBLE_H
#define RAIDS_BUBBLE_H

#include <tamtypes.h>
#include <libdl/moby.h>
#include <libdl/math.h>
#include <libdl/math3d.h>
#include <libdl/time.h>
#include <libdl/player.h>
#include <libdl/sound.h>

#define BUBBLE_LIFE_TICKS         (TPS * 2)
#define BUBBLE_MAX_NONLOCAL_DIST  (50.0)
#define BUBBLE_POP_TICKS          (TPS / 4)

struct RaidsDamageBubble
{
  VECTOR Position;
  u32 Damage;
  u8 Life;
  char IsLocal;
  char Team;
  float PopScale;
  char Suffix;
};

void bubblePush(VECTOR position, float randomRadius, float damage, int isLocal, int team, float popScale, char suffix);
void bubbleTick(void);
void bubbleDraw(void);
void bubbleInit(void);
void bubbleDeinit(void);

#endif // RAIDS_BUBBLE_H
