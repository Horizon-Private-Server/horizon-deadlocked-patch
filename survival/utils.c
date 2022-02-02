#include "include/utils.h"
#include <string.h>
#include <libdl/stdio.h>
#include <libdl/game.h>
#include <libdl/collision.h>
#include <libdl/moby.h>
#include <libdl/sound.h>
#include <libdl/random.h>

/* 
 * Explosion sound def
 */
SoundDef ExplosionSoundDef =
{
	0.0,	// MinRange
	50.0,	// MaxRange
	100,		// MinVolume
	4000,		// MaxVolume
	0,			// MinPitch
	0,			// MaxPitch
	0,			// Loop
	0x10,		// Flags
	0x106,  // 0x123, 0x171, 
	3			  // Bank
};

/* 
 * Explosion sound def
 */
SoundDef UpgradeSoundDef =
{
	0.0,	// MinRange
	20.0,	// MaxRange
	100,		// MinVolume
	2000,		// MaxVolume
	0,			// MinPitch
	0,			// MaxPitch
	0,			// Loop
	0x10,		// Flags
	0x3A,		// Index (0x2C, )
	3			  // Bank
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

void playUpgradeSound(Player* player)
{	
	soundPlay(&UpgradeSoundDef, 0, player->PlayerMoby, 0, 0x400);
}

int playerGetWeaponAlphaModCount(Player* player, int weaponId, int alphaMod)
{
	int i, c = 0;
	if (!player)
		return 0;
	
	GadgetBox* gBox = player->GadgetBox;
	if (!gBox)
		return 0;

	// count
	for (i = 0; i < 10; ++i)
	{
		if (gBox->Gadgets[weaponId].AlphaMods[i] == alphaMod)
			++c;
	}

	return c;
}

int getWeaponIdFromOClass(short oclass)
{
	int weaponId = -1;
	if (oclass > 0) {
		switch (oclass)
		{
			case MOBY_ID_DUAL_VIPER_SHOT: weaponId = WEAPON_ID_VIPERS; break;
			case MOBY_ID_MAGMA_CANNON: weaponId = WEAPON_ID_MAGMA_CANNON; break;
			case MOBY_ID_ARBITER_ROCKET0: weaponId = WEAPON_ID_ARBITER; break;
			case MOBY_ID_FUSION_SHOT: weaponId = WEAPON_ID_FUSION_RIFLE; break;
			case MOBY_ID_MINE_LAUNCHER_MINE: weaponId = WEAPON_ID_MINE_LAUNCHER; break;
			case MOBY_ID_B6_BOMB_EXPLOSION: weaponId = WEAPON_ID_B6; break;
			case MOBY_ID_FLAIL: weaponId = WEAPON_ID_FLAIL; break;
			case MOBY_ID_HOLOSHIELD_LAUNCHER: weaponId = WEAPON_ID_OMNI_SHIELD; break;
			case MOBY_ID_WRENCH: weaponId = WEAPON_ID_WRENCH; break;
		}
	}

	return weaponId;
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
