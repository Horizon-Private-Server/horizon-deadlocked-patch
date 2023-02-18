/***************************************************
 * FILENAME :		main.c
 * 
 * DESCRIPTION :
 * 		Custom map logic for Canal City.
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
#include <libdl/stdio.h>
#include <libdl/color.h>
#include <libdl/gamesettings.h>
#include <libdl/dialog.h>
#include <libdl/sound.h>
#include <libdl/patch.h>
#include <libdl/ui.h>
#include <libdl/graphics.h>

int State = 0;
Moby* hackerOrbMoby = NULL;
Moby* hiddenMoby = NULL;

struct PartInstance {	
	char IClass;
	char Type;
	char Tex;
	char Alpha;
	u32 RGBA;
	char Rot;
	char DrawDist;
	short Timer;
	float Scale;
	VECTOR Position;
	int Update[8];
};

struct HiddenMobyPVar {
  Moby* ChildMoby;
	struct PartInstance* Particles[4];
};

SoundDef baseSoundDef = {
	0.0,	  // MinRange
	45.0,	  // MaxRange
	0,		  // MinVolume
	1228,		// MaxVolume
	-635,			// MinPitch
	635,			// MaxPitch
	0,			// Loop
	0x10,		// Flags
	0x17D,		// Index
	3			  // Bank
};

#if DEBUG

void sound(void)
{
  static int soundId = 0;

  dlPreUpdate();

  if (padGetButtonDown(0, PAD_LEFT) > 0) {
    --soundId;
    playSound(hackerOrbMoby, soundId);
    DPRINTF("sound id %d\n", soundId);
  }
  else if (padGetButtonDown(0, PAD_RIGHT) > 0) {
    ++soundId;
    playSound(hackerOrbMoby, soundId);
    DPRINTF("sound id %d\n", soundId);
  }

  dlPostUpdate();
}

#endif

/*
 * NAME :		playSound
 * 
 * DESCRIPTION :
 * 			Plays the given sound id from the given moby.
 * 
 * NOTES :
 * 
 * ARGS : 
 * 
 * RETURN :
 * 
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */
void playSound(Moby* moby, int id)
{
  static short sHandle = 0;

  if (sHandle != 0) {
    soundKillByHandle(sHandle);
    sHandle = 0;
  }

	baseSoundDef.Index = id;
	short sId = soundPlay(&baseSoundDef, 0, moby, 0, 0x400);
  if (sId >= 0) {
    sHandle = soundCreateHandle(sId);
  }
}

/*
 * NAME :		spawnParticle
 * 
 * DESCRIPTION :
 * 			Spawns a particle instance with the given position, color, opacity, and type idx.
 * 
 * NOTES :
 * 
 * ARGS : 
 * 
 * RETURN :
 * 
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */
struct PartInstance * spawnParticle(VECTOR position, u32 color, char opacity, int idx)
{
	u32 a3 = *(u32*)0x002218E8;
	u32 t0 = *(u32*)0x002218E4;
	float f12 = *(float*)0x002218DC;
	float f1 = *(float*)0x002218E0;

	return ((struct PartInstance* (*)(VECTOR, u32, char, u32, u32, int, int, int, float))0x00533308)(position, color, opacity, a3, t0, -1, 0, 0, f12 + (f1 * idx));
}

/*
 * NAME :		destroyParticle
 * 
 * DESCRIPTION :
 * 			Destroys the given particle.
 * 
 * NOTES :
 * 
 * ARGS : 
 * 
 * RETURN :
 * 
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */
void destroyParticle(struct PartInstance* particle)
{
	((void (*)(struct PartInstance*))0x005284d8)(particle);
}

/*
 * NAME :		hiddenMobyUpdate
 * 
 * DESCRIPTION :
 * 			Handles hidden moby update logic.
 * 
 * NOTES :
 * 
 * ARGS : 
 * 
 * RETURN :
 * 
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */
void hiddenMobyUpdate(Moby* moby)
{
	const float rotSpeeds[] = { 0.05, 0.02, -0.03, -0.1 };
	const int opacities[] = { 64, 32, 44, 51 };
  int i;
  VECTOR t;
  Player** players = playerGetAll();

  if (!moby || !moby->PVar || moby->OClass != 0x1F4)
    return;

  struct HiddenMobyPVar* pvars = (struct HiddenMobyPVar*)moby->PVar;

	// handle particles
	u32 color = colorLerp(0xc7027f, 0xffa8df, 0);
	color |= 0x40000000;
	for (i = 0; i < 4; ++i) {
		struct PartInstance * particle = pvars->Particles[i];
		if (!particle) {
			pvars->Particles[i] = particle = spawnParticle(moby->Position, color, opacities[i], i);
		}

		// update
		if (particle) {
			particle->Rot = (int)((gameGetTime() + (i * 100)) / (TIME_SECOND * rotSpeeds[i])) & 0xFF;
		}
	}

  // handle child moby
  Moby* childMoby = pvars->ChildMoby;
  if (!childMoby) {
    pvars->ChildMoby = childMoby = mobySpawn(0x1098, 0);
    vector_copy(childMoby->Position, moby->Position);
    childMoby->Position[2] -= 0.15;
    childMoby->PUpdate = 0;
    childMoby->OClass = 0x1F4;
    childMoby->DrawDist = 64;
    childMoby->UpdateDist = 0xFF;
    childMoby->Rotation[0] = -MATH_PI / 2;
    mobyAnimTransition(childMoby, 1, 0, 0);
    DPRINTF("child moby %08X\n", (u32)childMoby);
  }

  if (childMoby) {
    childMoby->Rotation[2] = clampAngle(fastmodf(gameGetTime() / 1000.0, MATH_TAU));
  }

	// handle pickup
	for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
		Player * player = players[i];
		if (player && !playerIsDead(player) && player->PlayerMoby && player->IsLocal && player->GadgetBox->Gadgets[9].Level < 0) {
			vector_subtract(t, player->PlayerPosition, moby->Position);
			if (vector_sqrmag(t) < 4) {
				uiShowPopup(player->LocalPlayerIndex, uiMsgString(0x25BB));
        player->GadgetBox->Gadgets[9].Level = 0;
        playSound(moby, 55);
				break;
			}
		}
	}
}

/*
 * NAME :		activate
 * 
 * DESCRIPTION :
 * 			Activates the hidden moby.
 * 
 * NOTES :
 * 
 * ARGS : 
 * 
 * RETURN :
 * 
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */
void activate(void)
{
  if (hiddenMoby)
    return;

  hiddenMoby = mobySpawn(0x1F4, sizeof(struct HiddenMobyPVar));
  if (!hiddenMoby)
    return;

  hiddenMoby->PUpdate = &hiddenMobyUpdate;
  hiddenMoby->Position[0] = 275.5110474;
  hiddenMoby->Position[1] = 178.5585632;
  hiddenMoby->Position[2] = 51.25;
  hiddenMoby->DrawDist = 64;
  hiddenMoby->UpdateDist = 0xFF;
  hiddenMoby->ModeBits |= 1;
  hiddenMoby->CollActive = -1;

  DPRINTF("hidden moby %08X\n", (u32)hiddenMoby);
}

/*
 * NAME :		initialize
 * 
 * DESCRIPTION :
 * 			Finds the hacker orb that activates the hidden moby.
 * 
 * NOTES :
 * 
 * ARGS : 
 * 
 * RETURN :
 * 
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */
void initialize(void)
{
  GameOptions* gameOptions = gameGetOptions();
  Moby* moby = mobyListGetStart();
  Moby* mEnd = mobyListGetEnd();
  while (moby < mEnd)
  {
    if (moby->OClass == 0x2077 && !mobyIsDestroyed(moby) && moby->UID == 0x69) {
      hackerOrbMoby = moby;
      State = 1;
      DPRINTF("found hacker orb moby at %08X\n", (u32)moby);
    }

    ++moby;
  }

  if (gameOptions) {
    // enable mini turret
    gameOptions->WeaponFlags.UNK_09 = 1;
  }
}

/*
 * NAME :		main
 * 
 * DESCRIPTION :
 * 			Entrypoint.
 * 
 * NOTES :
 * 
 * ARGS : 
 * 
 * RETURN :
 * 
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */
int main (void)
{
  if (!State)
  {
    initialize();
    return;
  }

  if (State == 1 && hackerOrbMoby && hackerOrbMoby->State == 4) {
    playSound(hackerOrbMoby, 386);
    activate();
    State = 2;
  }

#if DEBUG
  sound();
#endif

	return 0;
}
