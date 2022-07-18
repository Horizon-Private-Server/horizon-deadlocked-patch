#include "include/utils.h"
#include <string.h>
#include <libdl/stdio.h>
#include <libdl/game.h>
#include <libdl/collision.h>
#include <libdl/moby.h>
#include <libdl/sound.h>
#include <libdl/random.h>
#include <libdl/spawnpoint.h>
#include <libdl/graphics.h>

/* 
 * Explosion sound def
 */
SoundDef ExplosionSoundDef =
{
	1000.0,		// MinRange
	1000.0,		// MaxRange
	2000,		// MinVolume
	2000,		// MaxVolume
	0,			// MinPitch
	0,			// MaxPitch
	0,			// Loop
	0x10,		// Flags
	0xF4,		// Index
	3			// Bank
};

Moby * spawnExplosion(VECTOR position, float size)
{
	// SpawnMoby_5025
	Moby * moby = ((Moby* (*)(u128, float, int, int, int, int, int, short, short, short, short, short, short,
				short, short, float, float, float, int, Moby *, int, int, int, int, int, int, int, int,
				int, short, Moby *, Moby *, u128)) (0x003c3b38))
				(vector_read(position), size / 2.5, 0x2, 0x14, 0x10, 0x10, 0x10, 0x10, 0x2, 0, 1, 0, 0,
				0, 0, 0, 0, 2, 0x00080800, 0, 0x00388EF7, 0x000063F7, 0x00407FFFF, 0x000020FF, 0x00008FFF, 0x003064FF, 0x7F60A0FF, 0x280000FF,
				0x003064FF, 0, 0, 0, 0);
				
	soundPlay(&ExplosionSoundDef, 0, moby, 0, 0x400);

	return moby;
}

void playCheckpointReachedSound()
{
	((void (*)(Player*, int, int))0x005eb280)((Player*)0x347AA0, 58, 0);
}

u8 decTimerU8(u8* timeValue)
{
	int value = *timeValue;
	if (value == 0)
		return 0;

	*timeValue = --value;
	return value;
}

u16 decTimerU16(u16* timeValue)
{
	int value = *timeValue;
	if (value == 0)
		return 0;

	*timeValue = --value;
	return value;
}

u32 decTimerU32(u32* timeValue)
{
	long value = *timeValue;
	if (value == 0)
		return 0;

	*timeValue = --value;
	return value;
}

//--------------------------------------------------------------------------
void drawRoundMessage(const char * message, float scale)
{
	int fw = gfxGetFontWidth(message, -1, scale);
	float x = 0.5;
	float y = 0.16;

	// draw message
	y *= SCREEN_HEIGHT;
	x *= SCREEN_WIDTH;
	gfxScreenSpaceText(x, y + 5, scale, scale * 1.5, 0x80FFFFFF, message, -1, 1);
}

//--------------------------------------------------------------------------
void drawRing(float yaw, VECTOR position, float scale0, float scale1, u32 color)
{
	float d = sinf(gameGetTime() / 1000.0) * MATH_PI * 0.5;

	VECTOR q1;
	MATRIX m1;
	VECTOR e1 = {MATH_PI / 2,yaw - (MATH_PI * 0.5),0,0};

	quaternion_fromeuler(q1, e1);
	matrix_fromquaternion(m1, q1);

  // draw
  ((void (*)(void*, u128, float, float, int, int, int, int))0x00420c80)(
    m1,
    vector_read(position),
    scale0,
    scale1,
    color,
    22,
    1,
    0
  );
}

//--------------------------------------------------------------------------
Player * playerGetRandom(void)
{
	int r = rand(GAME_MAX_PLAYERS);
	int i = 0, c = 0;
	Player ** players = playerGetAll();

	do {
		if (players[i] && !playerIsDead(players[i]))
			++c;
		
		++i;
		if (i == GAME_MAX_PLAYERS) {
			if (c == 0)
				return NULL;
			i = 0;
		}
	} while (c < r);

	return players[i-1];
}

//--------------------------------------------------------------------------
int spawnPointGetNearestTo(VECTOR point, VECTOR out, float minDist)
{
	VECTOR t;
	int spCount = spawnPointGetCount();
	int i;
	float bestPointDist = 100000;
	float minDistSqr = minDist * minDist;

	for (i = 0; i < spCount; ++i) {
		SpawnPoint* sp = spawnPointGet(i);
		vector_subtract(t, (float*)&sp->M0[12], point);
		float d = vector_sqrmag(t);
		if (d >= minDistSqr) {
			// randomize order a little
			d += randRange(0, 15 * 15);
			if (d < bestPointDist) {
				vector_copy(out, (float*)&sp->M0[12]);
				vector_fromyaw(t, randRadian());
				vector_scale(t, t, 3);
				vector_add(out, out, t);
				bestPointDist = d;
			}
		}
	}

	return spCount > 0;
}

//--------------------------------------------------------------------------
int spawnGetRandomPoint(VECTOR out) {
	// spawn randomly
	SpawnPoint* sp = spawnPointGet(rand(spawnPointGetCount()));
	if (sp) {
		vector_copy(out, &sp->M0[12]);
		return 1;
	}

	return 0;
}
