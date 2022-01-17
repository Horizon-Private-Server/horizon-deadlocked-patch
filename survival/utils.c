#include "include/utils.h"
#include <string.h>
#include <libdl/stdio.h>
#include <libdl/game.h>
#include <libdl/collision.h>
#include <libdl/moby.h>
#include <libdl/sound.h>

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
