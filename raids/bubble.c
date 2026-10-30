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
#include <libdl/hud.h>
#include "include/utils.h"
#include "include/game.h"
#include "include/bubble.h"
#include "config.h"
#include "common.h"

extern struct RaidsState State;

struct RaidsDamageBubble* damageBubbles = NULL;

//--------------------------------------------------------------------------
float bubbleGetScale(struct RaidsDamageBubble* bubble)
{
  float scale = 0.2 + (bubble->Life / (float)BUBBLE_LIFE_TICKS);

  // The requested extra pop fades over the first quarter second.
  if (bubble->PopScale > 0) {
    float pop = clamp((bubble->Life - (BUBBLE_LIFE_TICKS - BUBBLE_POP_TICKS))
      / (float)BUBBLE_POP_TICKS, 0, 1);
    scale *= 1 + bubble->PopScale * pop;
  }

  return scale;
}

//--------------------------------------------------------------------------
u32 bubbleGetColor(int damage, int life, int team)
{
  u32 from = hudGetTeamColor(team, 1);
  u32 to = colorScale(from, 2);
  u32 opacity = 0x80;

  // base color
  float t = 1 - clamp((life - 10) / (float)BUBBLE_LIFE_TICKS, 0, 1);
  u32 baseColor = colorLerp(from, to, t) & 0xffffff;

  // opacity
  if (life <= 10) {
    opacity = life * 12;
  }

  return baseColor | (opacity << 24);
}

//--------------------------------------------------------------------------
void bubblePush(VECTOR position, float randomRadius, float damage, int isLocal, int team, float popScale, char suffix)
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
    damageBubbles[lowestLifeIdx].Damage = (int)ceilf(damage);
    damageBubbles[lowestLifeIdx].IsLocal = isLocal;
    damageBubbles[lowestLifeIdx].Team = team;
    damageBubbles[lowestLifeIdx].PopScale = popScale;
    damageBubbles[lowestLifeIdx].Suffix = suffix;
    vector_add(damageBubbles[lowestLifeIdx].Position, position, offset);
  }
}

//--------------------------------------------------------------------------
void bubbleTick(void)
{
  if (!damageBubbles || !cameraGetGameCamera(0) || playerGetNumLocals() > 1) return;

  int i;
  for (i = 0; i < DAMAGE_BUBBLE_MAX_COUNT; ++i) {
    if (damageBubbles[i].Life) {
      damageBubbles[i].Life--;
      damageBubbles[i].Position[2] += MATH_DT;
    }
  }
}

//--------------------------------------------------------------------------
void bubbleDraw(void)
{
  int i;
  char buf[32];
  float scale;
  VECTOR dt;
  GameCamera* camera = cameraGetGameCamera(0);

  if (!damageBubbles) return;
  if (!camera) return;
  if (playerGetNumLocals() > 1) return; // don't support more than 1 player
  
  int dzo = PATCH_INTEROP && PATCH_INTEROP->Client == CLIENT_TYPE_DZO;
  for (i = 0; i < DAMAGE_BUBBLE_MAX_COUNT; ++i) {
    if (damageBubbles[i].Life) {

      if (!damageBubbles[i].IsLocal) {
        vector_subtract(dt, damageBubbles[i].Position, camera->pos);
        if (vector_sqrmag(dt) > (BUBBLE_MAX_NONLOCAL_DIST*BUBBLE_MAX_NONLOCAL_DIST)) continue;
      }

      u32 color = bubbleGetColor(damageBubbles[i].Damage, damageBubbles[i].Life, damageBubbles[i].Team);
      char suffix[2] = { damageBubbles[i].Suffix, 0 };
      snprintf(buf, sizeof(buf), "%d%s", damageBubbles[i].Damage, suffix);
      scale = bubbleGetScale(&damageBubbles[i]);
      gfxHelperDrawText_WS(damageBubbles[i].Position, scale, color, buf, -1, TEXT_ALIGN_MIDDLECENTER, dzo ? COMMON_DZO_DRAW_ONLY : COMMON_DZO_DRAW_NONE);
    }
  }
}

//--------------------------------------------------------------------------
void bubbleInit(void)
{
  if (!damageBubbles) {
    damageBubbles = malloc(sizeof(struct RaidsDamageBubble) * DAMAGE_BUBBLE_MAX_COUNT);
    if (damageBubbles) {
      memset(damageBubbles, 0, sizeof(struct RaidsDamageBubble) * DAMAGE_BUBBLE_MAX_COUNT);
    }
  }
}

//--------------------------------------------------------------------------
void bubbleDeinit(void)
{
  if (damageBubbles) {
    free(damageBubbles);
    damageBubbles = NULL;
  }
}
