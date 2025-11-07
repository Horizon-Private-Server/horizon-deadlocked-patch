/***************************************************
 * FILENAME :		collectible.c
 * 
 * DESCRIPTION :
 * 		Handles logic for the gold bolts.
 * 		
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */

#include <tamtypes.h>

#include <libdl/dl.h>
#include <libdl/player.h>
#include <libdl/pad.h>
#include <libdl/time.h>
#include <libdl/net.h>
#include <libdl/game.h>
#include <libdl/hud.h>
#include <libdl/string.h>
#include <libdl/math.h>
#include <libdl/math3d.h>
#include <libdl/radar.h>
#include <libdl/stdio.h>
#include <libdl/gamesettings.h>
#include <libdl/dialog.h>
#include <libdl/sound.h>
#include <libdl/patch.h>
#include <libdl/ui.h>
#include <libdl/graphics.h>
#include <libdl/collision.h>
#include <libdl/spawnpoint.h>
#include <libdl/color.h>
#include <libdl/utils.h>
#include <libdl/random.h>
#include "include/collectible.h"
#include "include/game.h"

const u32 COLLECTIBLE_COLORS[] = {
  0x80808080,
  0x8080FF40,
  0x80FF4000,
  0x808040FF,
};

const int COLLECTIBLE_TEAMS[] = {
  TEAM_WHITE,
  TEAM_GREEN,
  TEAM_BLUE,
  TEAM_RED
};

const char COLLECTIBLE_COLOR_CODES[] = {
  '\x0C',
  '\x0A',
  '\x09',
  '\x0E'
};

const char* COLLECTIBLE_VERBS[] = {
  "an Easy Bolt",
  "a Medium Bolt",
  "a Hard Bolt",
  "a Very Hard Bolt"
};

//--------------------------------------------------------------------------
int collectibleHasCollected(u32 uid)
{
  int i;
  for (i = 0; i < MAX_BOLTS; ++i) {
    if (ServerConfig.CollectedBoltsUids[i] == uid) return 1;
  }

  return 0;
}

//--------------------------------------------------------------------------
void collectibleCollect(Moby* moby)
{
  struct CollectiblePVar* pvars = (struct CollectiblePVar*)moby->PVar;
  
  if (!pvars->Collected) {

    // mark collected
    sendCollectedBolt(moby->UID);

    // play sound
    mobyPlaySoundByClass(1, 0, moby, MOBY_ID_WEAPON_PICKUP);

    // count
    int count = 0;
    for (count = 0; count < MAX_BOLTS; ++count)
      if (ServerConfig.CollectedBoltsUids[count] == 0) break;

    // popup
    snprintf(State.PopupMessageBuf, sizeof(State.PopupMessageBuf), "You found %c%s\x08!\x01 Collected %d/%d", COLLECTIBLE_COLOR_CODES[pvars->Difficulty], COLLECTIBLE_VERBS[pvars->Difficulty], count, State.BoltCount);
    uiShowPopup(0, State.PopupMessageBuf);
    uiShowPopup(1, State.PopupMessageBuf);
  }

  mobyDestroy(moby);
}

//--------------------------------------------------------------------------
void collectibleUpdate(Moby* moby)
{
  struct CollectiblePVar* pvars = (struct CollectiblePVar*)moby->PVar;
  pvars->Collected = collectibleHasCollected(moby->UID);
  u32 glow = COLLECTIBLE_COLORS[pvars->Difficulty];

  // check if collected
  if (pvars->Collected) {
    glow = colorLerp(glow, 0, 0.5);
    moby->Opacity = 0x30;
    moby->State = 1;
  } else {
    moby->Opacity = 0xA0;
  }

  moby->PrimaryColor = glow & 0xffffff;
  moby->GlowRGBA = glow;

  // spin
  moby->Rotation[1] = -MATH_PI / 4;
  moby->Rotation[2] = clampAngle(moby->Rotation[2] + MATH_DT*MATH_PI*0.5);

  // bob
  moby->Position[2] = moby->AnimSpeed + 0.25*sinf(gameGetTime() / 500.0);

  // check if player has reached bolt
  int i;
  for (i = 0; i < GAME_MAX_LOCALS; ++i) {
    Player* local = playerGetFromSlot(i);
    if (!local) break;

    if (vector_sqrdistance(moby->Position, local->PlayerPosition) < 6.25) {
      collectibleCollect(moby);
      break;
    }
  }

  if (!pvars->Collected && pvars->Blip) {
    int blipIdx = radarGetBlipIndex(moby);
    if (blipIdx >= 0) {
      RadarBlip* blip = radarGetBlips() + blipIdx;

      blip->Moby = moby;
      blip->Life = 0x1F;
      blip->Team = COLLECTIBLE_TEAMS[pvars->Difficulty];
      blip->Type = 0x11;
			blip->X = moby->Position[0];
			blip->Y = moby->Position[1];
    }
  }
}

//--------------------------------------------------------------------------
void collectibleInit(void)
{
  void* boltMobyClass = mobyGetClass(13);

  // set update
  Moby* moby = mobyListGetStart();
	while ((moby = mobyFindNextByOClass(moby, COLLECTIBLE_OCLASS)))
	{
		if (!mobyIsDestroyed(moby) && moby->PVar) {
      DPRINTF("found collectible %08X\n", (u32)moby);
      moby->PUpdate = &collectibleUpdate;
      moby->PClass = boltMobyClass;
      moby->CollData = *(int*)((u32)boltMobyClass + 0x10);
      moby->MClass = *(u8*)(0x0024a110 + 13);
      moby->AnimSeq = *(void**)((u32)boltMobyClass + 0x48);
      moby->AnimSeqId = 0;
      moby->AnimSpeed = 1;
      moby->JointCnt = *(char*)((u32)boltMobyClass + 0x08);
      moby->Scale = 0.03125;
      moby->ModeBits = 0x0050;
      //((void* (*)(Moby*))0x004fb910)(moby); // init joint cache

      moby->AnimSpeed = moby->Position[2];
      moby->GlowRGBA = 0x80808080;
    }

		++moby;
	}

  DPRINTF("collectible pvar size %d\n", sizeof(struct CollectiblePVar));
}
