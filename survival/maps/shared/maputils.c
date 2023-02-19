/***************************************************
 * FILENAME :		maputils.c
 * 
 * DESCRIPTION :
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
#include <libdl/stdio.h>
#include <libdl/gamesettings.h>
#include <libdl/dialog.h>
#include <libdl/sound.h>
#include <libdl/patch.h>
#include <libdl/ui.h>
#include <libdl/graphics.h>
#include <libdl/color.h>
#include <libdl/utils.h>
#include "game.h"

extern char LocalPlayerStrBuffer[2][48];
extern struct SurvivalMapConfig MapConfig;


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

//--------------------------------------------------------------------------
void playPaidSound(Player* player)
{
  soundPlay(&PaidSoundDef, 0, player->PlayerMoby, 0, 0x400);
}

//--------------------------------------------------------------------------
int tryPlayerInteract(Moby* moby, Player* player, char* message, int boltCost, int tokenCost, int actionCooldown, float sqrDistance)
{
  VECTOR delta;
  if (!player || !player->PlayerMoby || !player->IsLocal)
    return 0;

  struct SurvivalPlayer* playerData = NULL;
  int localPlayerIndex = player->LocalPlayerIndex;
  int pIndex = player->PlayerId;

  if (MapConfig.State) {
    playerData = &MapConfig.State->PlayerStates[pIndex];
  }
  
  vector_subtract(delta, player->PlayerPosition, moby->Position);
  if (vector_sqrmag(delta) < sqrDistance) {

    // draw help popup
    sprintf(LocalPlayerStrBuffer[localPlayerIndex], message);
    uiShowPopup(player->LocalPlayerIndex, LocalPlayerStrBuffer[localPlayerIndex]);

    // handle pad input
    if (padGetButtonDown(localPlayerIndex, PAD_CIRCLE) > 0 && (!playerData || (playerData->State.Bolts >= boltCost && playerData->State.CurrentTokens >= tokenCost))) {
      if (playerData) {
        //playerData->State.Bolts -= boltCost;
        //playerData->State.CurrentTokens -= tokenCost;
        playerData->ActionCooldownTicks = actionCooldown;
        playerData->MessageCooldownTicks = 5;
      }

      return 1;
    }
  }

  return 0;
}

//--------------------------------------------------------------------------
GuberEvent* guberCreateEvent(Moby* moby, u32 eventType)
{
	GuberEvent * event = NULL;

	// create guber object
	Guber* guber = guberGetObjectByMoby(moby);
	if (guber)
		event = guberEventCreateEvent(guber, eventType, 0, 0);

	return event;
}
