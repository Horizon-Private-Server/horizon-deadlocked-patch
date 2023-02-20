#include "include/utils.h"
#include "include/game.h"
#include <string.h>
#include <libdl/stdio.h>
#include <libdl/game.h>
#include <libdl/collision.h>
#include <libdl/moby.h>
#include <libdl/sound.h>
#include <libdl/random.h>
#include <libdl/graphics.h>

extern struct SurvivalState State;

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

/* 
 * paid sound def
 */
SoundDef PaidSoundDef =
{
	0.0,	// MinRange
	20.0,	// MaxRange
	100,		// MinVolume
	2000,		// MaxVolume
	0,			// MinPitch
	0,			// MaxPitch
	0,			// Loop
	0x10,		// Flags
	32,		  // Index
	3			  // Bank
};

Moby * spawnExplosion(VECTOR position, float size, u32 color)
{
	// SpawnMoby_5025
	Moby * moby = ((Moby* (*)(u128, float, int, int, int, int, int, short, short, short, short, short, short,
				short, short, float, float, float, int, Moby *, int, int, int, int, int, int, int, int,
				int, short, Moby *, Moby *, u128)) (0x003c3b38))
				(vector_read(position), size / 2.5, 0x2, 0x14, 0x10, 0x10, 0x10, 0x10, 0x2, 0, 1, 0, 0,
				0, 0, 0, 0, 2, 0x00080800, 0, color, color, color, color, color, color, color, color,
				color, 0, 0, 0, 0);
				
	soundPlay(&ExplosionSoundDef, 0, moby, 0, 0x400);

	return moby;
}

void playUpgradeSound(Player* player)
{	
	soundPlay(&UpgradeSoundDef, 0, player->PlayerMoby, 0, 0x400);
}

void playPaidSound(Player* player)
{
  soundPlay(&PaidSoundDef, 0, player->PlayerMoby, 0, 0x400);
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

//--------------------------------------------------------------------------
u32 getXpForNextToken(int counter)
{
	return (u32)(250 * powf(1.05, counter));
}

//--------------------------------------------------------------------------
void drawDreadTokenIcon(float x, float y, float scale)
{
	float small = scale * 0.75;
	float delta = (scale - small) / 2;

	gfxSetupGifPaging(0);
	u64 dreadzoneSprite = gfxGetFrameTex(32);
	gfxDrawSprite(x+2, y+2, scale, scale, 0, 0, 32, 32, 0x40000000, dreadzoneSprite);
	gfxDrawSprite(x,   y,   scale, scale, 0, 0, 32, 32, 0x80C0C0C0, dreadzoneSprite);
	gfxDrawSprite(x+delta, y+delta, small, small, 0, 0, 32, 32, 0x80000040, dreadzoneSprite);
	gfxDoGifPaging();
}

//--------------------------------------------------------------------------
struct PartInstance * spawnParticle(VECTOR position, u32 color, char opacity, int idx)
{
	u32 a3 = *(u32*)0x002218E8;
	u32 t0 = *(u32*)0x002218E4;
	float f12 = *(float*)0x002218DC;
	float f1 = *(float*)0x002218E0;

	return ((struct PartInstance* (*)(VECTOR, u32, char, u32, u32, int, int, int, float))0x00533308)(position, color, opacity, a3, t0, -1, 0, 0, f12 + (f1 * idx));
}

//--------------------------------------------------------------------------
void destroyParticle(struct PartInstance* particle)
{
	((void (*)(struct PartInstance*))0x005284d8)(particle);
}

//--------------------------------------------------------------------------
int intArrayContains(int* list, int count, int value)
{
	int i;

	for (i = 0; i < count; ++i)
		if (list[i] == value)
			return 1;

	return 0;
}

//--------------------------------------------------------------------------
int charArrayContains(char* list, int count, char value)
{
	int i;

	for (i = 0; i < count; ++i)
		if (list[i] == value)
			return 1;

	return 0;
}

//--------------------------------------------------------------------------
void vectorProjectOnVertical(VECTOR output, VECTOR input0)
{
    asm __volatile__ (
#if __GNUC__ > 3
    "lqc2   $vf1, 0x00(%1)  \n"
    "vmove.xy   $vf1, $vf0   \n"
    "sqc2   $vf1, 0x00(%0)  \n"
#else
    "lqc2		vf1, 0x00(%1)	\n"
    "vmove.xy  vf1, vf0    \n"
    "sqc2		vf1, 0x00(%0)	\n"
#endif
    : : "r" (output), "r" (input0)
  );
}

//--------------------------------------------------------------------------
void vectorProjectOnHorizontal(VECTOR output, VECTOR input0)
{
    asm __volatile__ (
#if __GNUC__ > 3
    "lqc2   $vf1, 0x00(%1)  \n"
    "vmove.z   $vf1, $vf0   \n"
    "sqc2   $vf1, 0x00(%0)  \n"
#else
    "lqc2		vf1, 0x00(%1)	\n"
    "vmove.z  vf1, vf0    \n"
    "sqc2		vf1, 0x00(%0)	\n"
#endif
    : : "r" (output), "r" (input0)
  );
}

//--------------------------------------------------------------------------
float getSignedSlope(VECTOR forward, VECTOR normal)
{
  VECTOR up, hForward;

  vectorProjectOnHorizontal(hForward, forward);
  vector_normalize(hForward, hForward);
  vector_outerproduct(up, hForward, normal);
  float slope = atan2f(vector_length(up), vector_innerproduct(hForward, normal)) - MATH_PI/2;

  /*if (fabsf(slope) > 40*MATH_DEG2RAD) {
    DPRINTF("getSignedSlope:\n\tup:%.2f,%.2f,%.2f\n\thF:%.2f,%.2f,%.2f\n\tn:%.2f,%.2f,%.2f\n\tslope:%f\n"
      , up[0], up[1], up[2]
      , hForward[0], hForward[1], hForward[2]
      , normal[0], normal[1], normal[2]
      , slope * MATH_RAD2DEG);
  }*/
  return slope;
}

//--------------------------------------------------------------------------
int mobyIsMob(Moby* moby)
{
  if (!moby)
    return;

  return moby->OClass == ZOMBIE_MOBY_OCLASS
    || moby->OClass == EXECUTIONER_MOBY_OCLASS
    || moby->OClass == TREMOR_MOBY_OCLASS
    ;
}
