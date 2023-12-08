#include "include/utils.h"
#include "include/game.h"
#include "include/bubble.h"
#include <string.h>
#include <libdl/stdio.h>
#include <libdl/game.h>
#include <libdl/collision.h>
#include <libdl/stdlib.h>
#include <libdl/color.h>
#include <libdl/moby.h>
#include <libdl/sound.h>
#include <libdl/random.h>
#include <libdl/graphics.h>

extern struct SurvivalState State;

struct SurvivalDamageBubble* damageBubbles = NULL;

//--------------------------------------------------------------------------
float bubbleGetScale(struct SurvivalDamageBubble* bubble)
{
  return 0.2 + (bubble->Life / (float)BUBBLE_LIFE_TICKS);
}

//--------------------------------------------------------------------------
u32 bubbleGetColor(int damage, int life)
{
  u32 from = 0x00BFFF;
  u32 to = 0x80C0FF;
  u32 opacity = 0x80;

  // base color
  float t = 1 - clamp((life - 10) / (float)BUBBLE_LIFE_TICKS, 0, 1);
  u32 baseColor = colorLerp(from, to, t);  

  // opacity
  if (life <= 10) {
    opacity = life * 12;
  }

  return baseColor | (opacity << 24);
}

//--------------------------------------------------------------------------
void bubblePush(VECTOR position, float randomRadius, float damage, int isLocal)
{
  int i;
  int lowestLifeIdx = -1;
  int lowestLife = BUBBLE_LIFE_TICKS;
  VECTOR offset;

  if (!damageBubbles) return;

  vector_fromyaw(offset, randRadian());
  vector_scale(offset, offset, randRange(0, randomRadius));

  for (i = 0; i < DAMAGE_BUBBLE_MAX_COUNT; ++i) {
    if (damageBubbles[i].Life == 0) {
      lowestLifeIdx = i;
      break;
    } else if (damageBubbles[i].Life < lowestLife && isLocal >= damageBubbles[i].IsLocal) {
      lowestLife = damageBubbles[i].Life;
      lowestLifeIdx = i;
    }
  }

  if (lowestLifeIdx >= 0) {
    damageBubbles[lowestLifeIdx].Life = BUBBLE_LIFE_TICKS;
    damageBubbles[lowestLifeIdx].Damage = (short)ceilf(damage);
    damageBubbles[lowestLifeIdx].IsLocal = isLocal;
    vector_add(damageBubbles[lowestLifeIdx].Position, position, offset);
  }
}

//--------------------------------------------------------------------------
void bubbleTick(void)
{
  int i;
  int x,y;
  char buf[32];
  float scale;
  VECTOR dt;
  GameCamera* camera = cameraGetGameCamera(0);

  if (!damageBubbles) return;
  if (!camera) return;
  if (playerGetNumLocals() > 1) return; // don't support more than 1 player

  for (i = 0; i < DAMAGE_BUBBLE_MAX_COUNT; ++i) {
    if (damageBubbles[i].Life) {

      if (!damageBubbles[i].IsLocal) {
        vector_subtract(dt, damageBubbles[i].Position, camera->pos);
        if (vector_sqrmag(dt) > (BUBBLE_MAX_NONLOCAL_DIST*BUBBLE_MAX_NONLOCAL_DIST)) continue;
      }

      if (gfxWorldSpaceToScreenSpace(damageBubbles[i].Position, &x, &y)) {
        snprintf(buf, sizeof(buf), "%d", damageBubbles[i].Damage);
        scale = bubbleGetScale(&damageBubbles[i]);
        gfxScreenSpaceText(x, y, scale, scale, bubbleGetColor(damageBubbles[i].Damage, damageBubbles[i].Life), buf, -1, 4);

        damageBubbles[i].Position[2] += 1*MATH_DT;
      }

      damageBubbles[i].Life--;
    }
  }
}

//--------------------------------------------------------------------------
void bubbleInit(void)
{
  if (!damageBubbles) {
    DPRINTF("malloc\n");
    damageBubbles = malloc(sizeof(struct SurvivalDamageBubble) * DAMAGE_BUBBLE_MAX_COUNT);
    if (damageBubbles) {
      memset(damageBubbles, 0, sizeof(struct SurvivalDamageBubble) * DAMAGE_BUBBLE_MAX_COUNT);
    }
  }
}

//--------------------------------------------------------------------------
void bubbleDeinit(void)
{
  if (damageBubbles) {
    DPRINTF("free\n");
    free(damageBubbles);
    damageBubbles = NULL;
  }
}
