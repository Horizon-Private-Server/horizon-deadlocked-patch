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
#include <libdl/utils.h>

#define NAPALM_DAMAGE       (10.0)

int State = 0;
Moby* hackerOrbMoby = NULL;

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
 * NAME :		applyViperNapalm
 * 
 * DESCRIPTION :
 * 			Spawns napalm on projectile weapon shots.
 * 
 * NOTES :
 * 
 * ARGS : 
 * 
 * RETURN :
 * 
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */
void applyViperNapalm(void)
{
  VECTOR offset = {0,0,-0.1,0};
  Moby* m = mobyListGetStart();
  Moby* mEnd = mobyListGetEnd();

  // disable napalm explosion
  POKE_U32(0x00450FE4, 0);
  POKE_U16(0x00450dfc, 2);

  while (m < mEnd)
  {
    if (mobyIsDestroyed(m)) {
      ++m;
      continue;
    }

    int fire = 0;
    int rate = 10;
    switch (m->OClass)
    {
      case MOBY_ID_DUAL_VIPER_SHOT:
      case MOBY_ID_ARBITER_ROCKET0:
      case MOBY_ID_B6_BALL0:
      {
        rate = 5;
        fire = 1;
        break;
      }
      case MOBY_ID_HOLOSHIELD_SHOT:
      case MOBY_ID_MINE_LAUNCHER_MINE:
      {
        rate = 10;
        fire = 1;
        break;
      }
      case MOBY_ID_FLAIL_HEAD:
      {
        rate = 3;

        Moby* flail = m->PParent;
        if (flail && flail->State == 2)
          fire = 1;
        break;
      }
    }

    // napalm
    if (fire) {
      if (m->Mission >= rate) {
        Player* player = guberMobyGetPlayerDamager(m);
        if (player) {
          Moby* napalm = ((Moby* (*)(u128 pos, u128 a1, u128 a2, Player* player, u32 t0, int t1, u32 t2, u32 t3))0x00450380)(
            vector_read(m->Position),
            0,
            0,
            player,
            0x1801,
            4,
            0,
            0
          );

          if (napalm && napalm->PVar) {
            ((float*)napalm->PVar)[12] = NAPALM_DAMAGE;
          }
        }

        m->Mission = 0;
      } else {
        m->Mission++;
      }
    }

    ++m;
  }
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
    if (moby->OClass == 0x2077 && !mobyIsDestroyed(moby)) {
      hackerOrbMoby = moby;
      State = 1;
      DPRINTF("found hacker orb moby at %08X\n", (u32)moby);
    }

    ++moby;
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

  // activate effect when bolt crank is activated
  if (State == 1 && hackerOrbMoby && hackerOrbMoby->State == 4) {
    playSound(hackerOrbMoby, 386);
    State = 2;
  }

  if (State == 2) {
    applyViperNapalm();
  }

#if DEBUG
  sound();
#endif

	return 0;
}
