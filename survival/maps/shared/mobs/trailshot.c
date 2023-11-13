#include <libdl/stdio.h>
#include <libdl/game.h>
#include <libdl/collision.h>
#include <libdl/graphics.h>
#include <libdl/moby.h>
#include <libdl/random.h>
#include <libdl/radar.h>
#include <libdl/color.h>

#include "../../../include/game.h"
#include "../../../include/mob.h"
#include "../include/maputils.h"
#include "../include/shared.h"

extern int aaa;

typedef struct TrailshotPVar
{
  VECTOR Velocity;
  int LifeTicks;
} TrailshotPVar_t;

//--------------------------------------------------------------------------
void trailshotDrawQuad(Moby* moby, int texId, u32 color)
{
	struct QuadDef quad;
	float size = 1;
	MATRIX m2;
	VECTOR t;
	VECTOR pTL = {0,size,size,1};
	VECTOR pTR = {0,-size,size,1};
	VECTOR pBL = {0,size,-size,1};
	VECTOR pBR = {0,-size,-size,1};

	//u32 color = MobLODColors[pvars->MobVars.Config.MobType] | (moby->Opacity << 24);

	// set draw args
	matrix_unit(m2);

	// init
  gfxResetQuad(&quad);

	// color of each corner?
	vector_copy(quad.VertexPositions[0], pTL);
	vector_copy(quad.VertexPositions[1], pTR);
	vector_copy(quad.VertexPositions[2], pBL);
	vector_copy(quad.VertexPositions[3], pBR);
	quad.VertexColors[0] = quad.VertexColors[1] = quad.VertexColors[2] = quad.VertexColors[3] = color;
  quad.VertexUVs[0] = (struct UV){0,0};
  quad.VertexUVs[1] = (struct UV){1,0};
  quad.VertexUVs[2] = (struct UV){0,1};
  quad.VertexUVs[3] = (struct UV){1,1};
	quad.Clamp = 1;
	quad.Tex0 = gfxGetFrameTex(texId);
	quad.Tex1 = 0xFF9000000260;
	quad.Alpha = 0x8000000044;

	GameCamera* camera = cameraGetGameCamera(0);
	if (!camera)
		return;
	
	// set world matrix by joint
  vector_copy(&m2[12], moby->Position);

	// draw
	gfxDrawQuad((void*)0x00222590, &quad, m2, 1);
}

//--------------------------------------------------------------------------
void trailshotPostDraw(Moby* moby)
{
  int x,y;
  TrailshotPVar_t* pvars = (TrailshotPVar_t*)moby->PVar;
  float rot = fastmodf(gameGetTime() / 1000.0, MATH_TAU) - MATH_PI;

  gfxDrawBillboardQuad(vector_read(moby->Position), 1, moby->PrimaryColor, moby->Opacity >> 1, rot, aaa, 1, 0.001);
  particle
}

//--------------------------------------------------------------------------
void trailshotUpdate(Moby* moby)
{
  TrailshotPVar_t* pvars = (TrailshotPVar_t*)moby->PVar;

  // register draw function
  gfxRegisterDrawFunction((void**)0x0022251C, &trailshotPostDraw, moby);

  // move
  if (pvars->LifeTicks) {
    pvars->LifeTicks--;

    vector_add(moby->Position, moby->Position, pvars->Velocity);
  } else {
    DPRINTF("destroy trailshot\n");
    mobyDestroy(moby);
  }
}

//--------------------------------------------------------------------------
void trailshotSpawn(VECTOR position, VECTOR velocity, u32 color, int lifeTicks)
{
  Moby* moby = mobySpawn(0x01F7, sizeof(TrailshotPVar_t));
  DPRINTF("spawned trailshot %08X\n", moby);
  if (!moby) return;

  TrailshotPVar_t* pvars = (TrailshotPVar_t*)moby->PVar;

  moby->PUpdate = &trailshotUpdate;
  moby->PrimaryColor = color;
  moby->UpdateDist = 0x1FF;
  moby->ModeBits = 0x04;
  moby->Opacity = 0x80;
  vector_copy(moby->Position, position);

  pvars->LifeTicks = lifeTicks;
  vector_copy(pvars->Velocity, velocity);
}
