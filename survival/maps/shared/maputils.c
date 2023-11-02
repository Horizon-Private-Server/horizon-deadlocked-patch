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

//--------------------------------------------------------------------------
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

//--------------------------------------------------------------------------
float getSignedSlope(VECTOR forward, VECTOR normal)
{
  VECTOR up, hForward;

  vector_projectonhorizontal(hForward, forward);
  vector_normalize(hForward, hForward);
  vector_outerproduct(up, hForward, normal);
  return atan2f(vector_length(up), vector_innerproduct(hForward, normal)) - MATH_PI/2;
}

//--------------------------------------------------------------------------
u8 decTimerU8(u8* timeValue)
{
	int value = *timeValue;
	if (value == 0)
		return 0;

	*timeValue = --value;
	return value;
}

//--------------------------------------------------------------------------
u16 decTimerU16(u16* timeValue)
{
	int value = *timeValue;
	if (value == 0)
		return 0;

	*timeValue = --value;
	return value;
}

//--------------------------------------------------------------------------
u32 decTimerU32(u32* timeValue)
{
	long value = *timeValue;
	if (value == 0)
		return 0;

	*timeValue = --value;
	return value;
}

//--------------------------------------------------------------------------
void pushSnack(int localPlayerIdx, char* string, int ticksAlive)
{
  if (MapConfig.PushSnackFunc)
    MapConfig.PushSnackFunc(string, ticksAlive, localPlayerIdx);
  else
    uiShowPopup(localPlayerIdx, string);
}

//--------------------------------------------------------------------------
int isInDrawDist(Moby* moby)
{
  int i;
  VECTOR delta;
  if (!moby)
    return 0;

  int drawDistSqr = moby->DrawDist*moby->DrawDist;

  for (i = 0; i < 2; ++i) {
    GameCamera* camera = cameraGetGameCamera(i);
    if (!camera)
      continue;
    
    // check if in range of camera
    vector_subtract(delta, camera->pos, moby->Position);
    if (vector_sqrmag(delta) < drawDistSqr)
      return 1;
  }

  return 0;
}

//--------------------------------------------------------------------------
int mobyIsMob(Moby* moby)
{
  if (!moby)
    return;

  return moby->OClass == ZOMBIE_MOBY_OCLASS
    || moby->OClass == EXECUTIONER_MOBY_OCLASS
    || moby->OClass == TREMOR_MOBY_OCLASS
    || moby->OClass == SWARMER_MOBY_OCLASS
    || moby->OClass == REACTOR_MOBY_OCLASS
    ;
}
