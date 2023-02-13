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
#include <libdl/gamesettings.h>
#include <libdl/dialog.h>
#include <libdl/sound.h>
#include <libdl/patch.h>
#include <libdl/ui.h>
#include <libdl/graphics.h>

int State = 0;
Moby* hackerOrbMoby = NULL;
Moby* hiddenMoby = NULL;

#if DEBUG

// 44 

SoundDef BaseSoundDef =
{
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

void playSound(int id)
{
  static short sHandle = 0;

  if (sHandle != 0) {
    soundKillByHandle(sHandle);
    sHandle = 0;
  }

	BaseSoundDef.Index = id;
	short sId = soundPlay(&BaseSoundDef, 0, hiddenMoby, 0, 0x400);
  if (sId >= 0) {
    sHandle = soundCreateHandle(sId);
  }
}

void sound(void)
{
  static int soundId = 0;

  dlPreUpdate();

  if (padGetButtonDown(0, PAD_LEFT) > 0) {
    --soundId;
    playSound(soundId);
    DPRINTF("sound id %d\n", soundId);
  }
  else if (padGetButtonDown(0, PAD_RIGHT) > 0) {
    ++soundId;
    playSound(soundId);
    DPRINTF("sound id %d\n", soundId);
  }

  dlPostUpdate();
}
#endif

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
  if (!hiddenMoby)
    return;

  hiddenMoby->DrawDist = 64;
  hiddenMoby->CollActive = 0;
  mobySetState(hiddenMoby, 1, -1);
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
  Moby* moby = mobyListGetStart();
  Moby* mEnd = mobyListGetEnd();
  while (moby < mEnd)
  {
    if (moby->OClass == 0x2077 && !mobyIsDestroyed(moby) && moby->UID == 0x69) {
      hackerOrbMoby = moby;
      State = 1;
      DPRINTF("found hacker orb moby at %08X\n", (u32)moby);
    }
    else if (moby->OClass == 0x10C3 && !mobyIsDestroyed(moby) && moby->UID == 0x95) {
      hiddenMoby = moby;
      mobySetState(moby, 2, -1);
      moby->DrawDist = 0;
      moby->CollActive = -1;
      DPRINTF("found hidden moby at %08X\n", (u32)moby);
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

  if (State == 1 && hackerOrbMoby && hackerOrbMoby->State == 4) {
    activate();
    State = 2;
  }

#if DEBUG
  sound();
#endif

	return 0;
}
