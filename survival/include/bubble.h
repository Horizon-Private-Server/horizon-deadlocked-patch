

#ifndef SURVIVAL_BUBBLE_H
#define SURVIVAL_BUBBLE_H

#include <tamtypes.h>
#include <libdl/moby.h>
#include <libdl/math.h>
#include <libdl/math3d.h>
#include <libdl/time.h>
#include <libdl/player.h>
#include <libdl/sound.h>

#define BUBBLE_LIFE_TICKS         (TPS * 2)

struct SurvivalDamageBubble
{
  VECTOR Position;
  u16 Life;
  u16 Damage;
};

void bubblePush(VECTOR position, float randomRadius, float damage);
void bubbleTick(void);
void bubbleInit(void);
void bubbleDeinit(void);

#endif // SURVIVAL_BUBBLE_H
