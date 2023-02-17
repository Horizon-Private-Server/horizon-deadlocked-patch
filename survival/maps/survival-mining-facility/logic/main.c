/***************************************************
 * FILENAME :		logic.c
 * 
 * DESCRIPTION :
 * 		Custom map logic for Survival V2's Mining Facility.
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
#include <libdl/collision.h>
#include <libdl/stdio.h>
#include <libdl/gamesettings.h>
#include <libdl/dialog.h>
#include <libdl/patch.h>
#include <libdl/ui.h>
#include <libdl/graphics.h>
#include <libdl/color.h>
#include <libdl/utils.h>

Moby* gateCreate(VECTOR start, VECTOR end, float height);
void powerNodeUpdate(Moby* moby);
void gateInit(void);
void gateSpawn(void);
void gasTick(void);

int aaa = 2;

void initialize(void)
{
  static int initialized = 0;
  if (initialized)
    return;

  // create gates
  gateInit();

  initialized = 1;
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
	int i;
	if (!isInGame())
		return;

  // init
  initialize();

  // disable jump pad effect
  POKE_U32(0x0042608C, 0);

#if DEBUG
  dlPreUpdate();
  if (padGetButtonDown(0, PAD_LEFT) > 0) {
    --aaa;
    DPRINTF("%d\n", aaa);
  }
  else if (padGetButtonDown(0, PAD_RIGHT) > 0) {
    ++aaa;
    DPRINTF("%d\n", aaa);
  }
  dlPostUpdate();
#endif

  gasTick();
	return 0;
}

void nodeUpdate(Moby* moby)
{
  static int initialized = 0;
  if (!initialized) {
    DPRINTF("node %08X\n", (u32)moby);
    initialized = 1;
  }

  // init
  initialize();
  
  // enable cq
  GameOptions* gameOptions = gameGetOptions();
  if (gameOptions) {
    gameOptions->GameFlags.MultiplayerGameFlags.NodeType = 0;
    //gameOptions->GameFlags.MultiplayerGameFlags.Lockdown = 1;
    gameOptions->GameFlags.MultiplayerGameFlags.UNK_09 = 1;
  }

  //
  powerNodeUpdate(moby);

  // disable deleting node if not CQ
  POKE_U32(0x003D16DC, 0x1000001D);

  // disable node captured popup
  POKE_U32(0x003D2E6C, 0);

  // call base node base update
  ((void (*)(Moby*))0x003D13C0)(moby);

  gateSpawn();
}
