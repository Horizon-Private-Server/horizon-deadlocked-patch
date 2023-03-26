/***************************************************
 * FILENAME :		gas.c
 * 
 * DESCRIPTION :
 * 		Handles logic for gaseous part of map.
 * 		
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */

#include <tamtypes.h>

#include <libdl/dl.h>
#include <libdl/player.h>
#include <libdl/pad.h>
#include <libdl/time.h>
#include <libdl/net.h>
#include "module.h"
#include "messageid.h"
#include <libdl/game.h>
#include <libdl/string.h>
#include <libdl/math.h>
#include <libdl/math3d.h>
#include <libdl/stdio.h>
#include <libdl/gamesettings.h>
#include <libdl/dialog.h>
#include <libdl/patch.h>
#include <libdl/ui.h>
#include <libdl/graphics.h>
#include <libdl/color.h>
#include <libdl/utils.h>

#define GAS_DAMAGE_TICKRATE			(60)
#define GAS_DAMAGE_AMOUNT			(1)
#define GAS_QUAD_COLOR				(0x40004000)
#define GAS_QUAD_TEX_ID				(24)
#define GAS_X_DAMAGE_START			(524.4)
#define GAS_X_FOG_FADE_IN_OFFSET	(10)
#define GAS_X_FOG_FADE_IN_LENGTH	(20)
#define GAS_X_FOG_FADE_OUT_OFFSET	(0)
#define GAS_X_FOG_FADE_OUT_LENGTH	(5)

const u32 DefaultFogColor = 0x00002000;
const float DefaultFogDistances[4] = {
	10240.0,
	102400.0,
	255.0,
	102.00
};

const u32 GasFogColor = 0x00002000;
const float GasFogDistances[4] = {
	0.0,
	32768.0,
	255.0,
	2.00
};

struct GasPlane
{
	VECTOR Position;
	float Yaw;
	float Width;
	float Height;
};

struct GasPlane GasPlanes[] = {
	{
		.Position = { 524.4, 682.9, 432.29, 0 },
		.Yaw = MATH_PI / 2,
		.Width = 13.1,
		.Height = 10.2
	},
	{
		.Position = { 524.4, 650.99, 432.29, 0 },
		.Yaw = MATH_PI / 2,
		.Width = 13.1,
		.Height = 10.2
	},
	{
		.Position = { 524.4, 549.06, 432.29, 0 },
		.Yaw = MATH_PI / 2,
		.Width = 13.1,
		.Height = 10.2
	},
	{
		.Position = { 524.4, 516.83, 432.29, 0 },
		.Yaw = MATH_PI / 2,
		.Width = 13.1,
		.Height = 10.2
	},
};
const int GasPlanesCount = sizeof(GasPlanes)/sizeof(struct GasPlane);

// sets the fog colors and distancew
void setFog(u32 color, float distance0, float distance1, float distance2, float distance3)
{
	*(u32*)0x0022254C = color;

	float * rtDistances = (float*)0x00222550;
	rtDistances[0] = distance0;
	rtDistances[1] = distance1;
	rtDistances[2] = distance2;
	rtDistances[3] = distance3;
}

// return non-zero when the given player is inside the gas area
int isPlayerInGasArea(Player* player)
{
	return player && player->PlayerPosition[0] > GAS_X_DAMAGE_START;
}

// return non-zero when the given moby is inside the gas area
int isMobyInGasArea(Moby* moby)
{
	return moby && moby->Position[0] > GAS_X_DAMAGE_START;
}

// returns 0-1, representing the percentage of fog should be gas
float getCameraGasFogFactor(Player* player)
{
	VECTOR gasDotForward = {0,1,0,0};
	if (!player)
		return 0;

	float gasFadeOutX = GAS_X_DAMAGE_START + (GAS_X_FOG_FADE_OUT_OFFSET + GAS_X_FOG_FADE_OUT_LENGTH);
	float gasFadeInX = GAS_X_DAMAGE_START - (GAS_X_FOG_FADE_IN_OFFSET + GAS_X_FOG_FADE_IN_LENGTH);

	// player must be below vertical axis if not in area already
	float v = player->CameraMatrix[12];
	if (v < GAS_X_DAMAGE_START && player->CameraMatrix[14] > 433.5)
		return 0;

	if (v > gasFadeOutX)
		return 1;

  if (v < gasFadeInX)
    return 0;

	float d = vector_innerproduct(&player->CameraMatrix[4], gasDotForward);

	float rOut = 1 - clamp(d * ((v - gasFadeOutX)/GAS_X_FOG_FADE_OUT_LENGTH), 0, 1);
  if (v < GAS_X_DAMAGE_START)
    rOut = 0;
	float rIn = clamp(((v - gasFadeInX)/GAS_X_FOG_FADE_IN_LENGTH), 0, 1);
  if (v > GAS_X_DAMAGE_START)
    rIn = 1;

	return lerpf(rOut, rIn, (d + 1) / 2);
}

// draw gas quad
void drawGasQuad(VECTOR position, float yaw, float width, float height, u32 color, float uOff, float vOff, float uScale, float vScale)
{
	struct QuadDef quad;
	MATRIX m2;
	VECTOR t;
	VECTOR pTL = {0.5,0,0.5,1};
	VECTOR pTR = {-0.5,0,0.5,1};
	VECTOR pBL = {0.5,0,-0.5,1};
	VECTOR pBR = {-0.5,0,-0.5,1};
	VECTOR scale = {width,1,height,0};

	float u0 = fastmodf(uOff*uScale, 1);
	float u1 = uScale + u0;
	float v0 = fastmodf(vOff*vScale, 1);
	float v1 = vScale + v0;

	// init
  gfxResetQuad(&quad);

	// color of each corner?
	vector_copy(quad.VertexPositions[0], pTL);
	vector_copy(quad.VertexPositions[1], pTR);
	vector_copy(quad.VertexPositions[2], pBL);
	vector_copy(quad.VertexPositions[3], pBR);
	quad.VertexColors[0] = quad.VertexColors[1] = quad.VertexColors[2] = quad.VertexColors[3] = color;
	quad.VertexUVs[0] = (struct UV){u0,v0};
	quad.VertexUVs[1] = (struct UV){u1,v0};
	quad.VertexUVs[2] = (struct UV){u0,v1};
	quad.VertexUVs[3] = (struct UV){u1,v1};
	quad.Clamp = 0;
	quad.Tex0 = gfxGetEffectTex(32, 1);
	quad.Tex1 = 0xFF9000000260;
	quad.Alpha = 0x8000000048;

	// set draw args
	matrix_unit(m2);
	matrix_scale(m2, m2, scale);
	matrix_rotate_z(m2, m2, yaw);

	// copy from moby
	memcpy(&m2[12], position, sizeof(VECTOR));

	// draw
	gfxDrawQuad((void*)0x00222590, &quad, m2, 0);
}

// render all gas planes at entrances to gas area
void drawGasQuads(void)
{
	int i;
	float time = gameGetTime() / 1000.0;
	Player* p = playerGetFromSlot(0);

	float gasFactor = getCameraGasFogFactor(p);
	u32 color = colorLerp(0x80008000, 0x80002000, gasFactor * gasFactor);

	for (i = 0; i < GasPlanesCount; ++i) {
		struct GasPlane* plane = &GasPlanes[i];

		drawGasQuad(plane->Position, plane->Yaw, plane->Width, plane->Height, color, 0, fastmodf(time / 10, 1), 1, 1);
	}
}

/*
 * NAME :		gasTick
 * 
 * DESCRIPTION :
 * 			
 * 
 * NOTES :
 * 
 * ARGS : 
 * 
 * RETURN :
 * 
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */
int gasTick(void)
{
	static int gasDamageTicker = GAS_DAMAGE_TICKRATE;
	int i;
	if (!isInGame())
		return;

	// draw gas mobies
  gfxRegisterDrawFunction((void**)0x0022251C, &drawGasQuads, NULL);

	// handle player damage and fog
	Player* p = playerGetFromSlot(0);
	if (p) {

		// set fog based on camera position
		float gasFactor = getCameraGasFogFactor(p);
#if NOGAS
    gasFactor = 0;
#endif
		setFog(
			colorLerp(DefaultFogColor, GasFogColor, gasFactor),
			lerpf(DefaultFogDistances[0], GasFogDistances[0], gasFactor),
			lerpf(DefaultFogDistances[1], GasFogDistances[1], gasFactor),
			lerpf(DefaultFogDistances[2], GasFogDistances[2], gasFactor),
			lerpf(DefaultFogDistances[3], GasFogDistances[3], gasFactor)
			);

		// damage player if in gas
    // and player doesn't have shield on
#if !DEBUG
		if (!playerIsDead(p) && isPlayerInGasArea(p) && p->timers.armorLevelTimer <= 0) {
			
			if (gasDamageTicker) {
				--gasDamageTicker;
			} else {
				gasDamageTicker = GAS_DAMAGE_TICKRATE;
				p->Health = maxf(0, p->Health - GAS_DAMAGE_AMOUNT);

				// update remote health
				if (p->pNetPlayer && p->pNetPlayer->pNetPlayerData)
					p->pNetPlayer->pNetPlayerData->hitPoints = p->Health;
			}
		} else {
			gasDamageTicker = GAS_DAMAGE_TICKRATE;
		}
#endif
	}

	return 0;
}
