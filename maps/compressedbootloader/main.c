/***************************************************
 * FILENAME :		main.c
 * 
 * DESCRIPTION :
 * 
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
#include <libdl/compression.h>

int main (void)
{
  static int init = 0;
  if (init) return 0;

  // decompress
  int len = decompressWad((void*)0x01EF0000, (void*)0x01B10000);
  printf("decompressed %d bytes\n", len);
  init = 1;

  // hook
  HOOK_J(0x00598BA0, 0x01B10000);
	return 0;
}
