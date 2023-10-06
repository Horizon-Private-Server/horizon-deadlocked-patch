/***************************************************
 * FILENAME :		ball.c
 * 
 * DESCRIPTION :
 * 		Manages ball update and spawning.
 * 
 * NOTES :
 * 		
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */

#include <tamtypes.h>
#include <string.h>

#include <libdl/time.h>
#include <libdl/game.h>
#include <libdl/gamesettings.h>
#include <libdl/graphics.h>
#include <libdl/player.h>
#include <libdl/weapon.h>
#include <libdl/hud.h>
#include <libdl/cheats.h>
#include <libdl/sha1.h>
#include <libdl/dialog.h>
#include <libdl/team.h>
#include <libdl/stdio.h>
#include <libdl/ui.h>
#include <libdl/guber.h>
#include <libdl/color.h>
#include <libdl/radar.h>
#include <libdl/sound.h>
#include <libdl/collision.h>
#include <libdl/net.h>
#include "module.h"
#include "messageid.h"
#include "include/game.h"
#include "include/ball.h"

#define DRAG_COEFF (0.1)
#define BOUNCINESS_COEFF (0.5)
#define THROW_COEFF (1.2)
#define LIFETIME (25)

VECTOR VECTOR_GRAVITY = { 0, 0, -0.005, 0 };
VECTOR VECTOR_ZERO = { 0, 0, 0, 0 };

int (*CollWaterHeight)(VECTOR from, VECTOR to, u64 a2, Moby * damageSource, u64 t0) = 0x00503780;

//--------------------------------------------------------------------------
void ballThrow(Moby * moby)
{
  VECTOR temp;
  if (!moby)
    return;

  Player** players = playerGetAll();
  BallPVars_t * pvars = (BallPVars_t*)moby->PVar;
  int carrierIdx = pvars->CarrierIdx;
  Player * carrier = NULL;
  if (carrierIdx >= 0 && carrierIdx < GAME_MAX_PLAYERS)
    carrier = players[carrierIdx];

  // 
  if (!carrier)
    return;

  // remove carrier
  carrier->HeldMoby = NULL;
  pvars->CarrierIdx = -1;
  pvars->DieTime = gameGetTime() + (TIME_SECOND * LIFETIME);

  // set velocity
  vector_copy(pvars->Velocity, carrier->Velocity);

  // generate lob velocity from forward vector and pitch
  vector_scale(temp, (float*)((u32)carrier + 0x1A60), (1.5 - carrier->CameraPitch.Value) * THROW_COEFF);
  if (temp[2] < 0.1)
    temp[2] = 0.1;
  vector_add(pvars->Velocity, pvars->Velocity, temp);
}

//--------------------------------------------------------------------------
void ballPickup(Moby * moby, int playerIdx)
{
  if (!moby)
    return;

  BallPVars_t * pvars = (BallPVars_t*)moby->PVar;
  Player** players = playerGetAll();
  Player* player = NULL;
  if (playerIdx >= 0 && playerIdx < GAME_MAX_PLAYERS)
    player = players[playerIdx];
  
  // drop from existing carrier
  if (pvars->CarrierIdx >= 0 && pvars->CarrierIdx < GAME_MAX_PLAYERS) {
    Player* carrier = players[pvars->CarrierIdx];
    if (carrier && carrier->HeldMoby == moby)
      carrier->HeldMoby = NULL;
  }

  // set new carrier
  pvars->CarrierIdx = playerIdx;
  if (player)
    player->HeldMoby = moby;
}

//--------------------------------------------------------------------------
void ballUpdate(Moby * moby)
{
  VECTOR updatePos, toPos, hitNormal, direction;
  float * hitPos = (float*)0x0023f930;
  if (!moby)
    return;

  Player** players = playerGetAll();
  BallPVars_t * pvars = (BallPVars_t*)moby->PVar;
  int carrierIdx = pvars->CarrierIdx;
  Player * carrier = NULL;
  if (carrierIdx >= 0 && carrierIdx < GAME_MAX_PLAYERS)
    carrier = players[carrierIdx];

  int blipIdx = radarGetBlipIndex(moby);
  if (blipIdx >= 0) {
    RadarBlip* blip = radarGetBlips() + blipIdx;
    blip->X = moby->Position[0];
    blip->Y = moby->Position[1];
    blip->Life = 0x1F;
    blip->Type = 4;
    blip->Team = 0;
    blip->Moby = moby;
  }

  // 
  if (!carrier && pvars->DieTime && gameGetTime() > pvars->DieTime)
  {
    mobyDestroy(moby);
    return;
  }

  // detect drop
  if (carrier && carrier->HeldMoby != moby)
  {
    vector_copy(pvars->Velocity, carrier->Velocity);
    pvars->CarrierIdx = -1;
    carrier = NULL;
    pvars->DieTime = gameGetTime() + (TIME_SECOND * LIFETIME);
  }

  // if not held do physics
  if (!carrier)
  {
    // detect if fell under map
    if (moby->Position[2] < gameGetDeathHeight())
    {
      vector_copy(moby->Position, State.BallSpawnPosition);
      vector_copy(pvars->Velocity, VECTOR_ZERO);
      return;
    }

    // add gravity
    vector_add(pvars->Velocity, pvars->Velocity, VECTOR_GRAVITY);

    // determine next update position
    vector_add(updatePos, moby->Position, pvars->Velocity);

    // add radius of model to next position for raycast
    vector_normalize(direction, pvars->Velocity);
    vector_scale(toPos, direction, 0.5);
    vector_add(toPos, updatePos, toPos);

    // hit detect
    if (CollLine_Fix(moby->Position, toPos, 2, 0, 0) != 0)
    {
      // get collision type
      u8 cType = *(u8*)0x0023F91C;
      if (cType == 0x0B || cType == 0x01 || cType == 0x04 || cType == 0x05 || cType == 0x0D || cType == 0x03)
      {
        vector_copy(moby->Position, State.BallSpawnPosition);
        vector_copy(pvars->Velocity, VECTOR_ZERO);
      }
      else
      {
        // change velocity based on hit
        vector_normalize(hitNormal, (float*)0x0023f940);
        vector_reflect(pvars->Velocity, pvars->Velocity, hitNormal);

        float dot = vector_innerproduct(direction, hitNormal);
        float exp = powf((dot + 1) / 2, 0.2);
        vector_scale(pvars->Velocity, pvars->Velocity, (clamp(1 - fabsf(dot), 0, 1) + BOUNCINESS_COEFF) * (1 - DRAG_COEFF));
        //vector_scale(pvars->Velocity, pvars->Velocity, 0.9);

        // correct position based on hit
        //vector_copy()
        
        //vector_copy(moby->Position, updatePos);
      }
    }
    else
    {
      vector_copy(moby->Position, updatePos);
    }
  }
}

//--------------------------------------------------------------------------
void ballCreate(VECTOR position)
{
	GuberEvent* guberEvent = NULL;

	// create guber object
	GuberMoby * guberMoby = guberMobyCreateSpawned(BALL_MOBY_OCLASS, sizeof(BallPVars_t), &guberEvent, NULL);
	if (guberEvent)
	{
		guberEventWrite(guberEvent, position, 0x0C);
	}
	else
	{
		DPRINTF("failed to guberevent ball\n");
	}
}

//--------------------------------------------------------------------------
int ballHandleGuberEvent_Spawn(Moby* moby, GuberEvent* event)
{
  static Moby * BallVisualRefMoby = NULL;
  VECTOR position;

  // read
  guberEventRead(event, position, 0x0C);

  // we need to make sure we have our reference moby to
  // copy the model, animation, and collision of
  if (!BallVisualRefMoby)
  {
    BallVisualRefMoby = mobySpawn(0x1b37, 0);
    if (!BallVisualRefMoby)
      return 0;
  }

  // spawn ball
	DPRINTF("spawning new ball moby: %08X %.2f %.2f %.2f\n", (u32)moby, position[0], position[1], position[2]);
  BallPVars_t * pvars = (BallPVars_t*)moby->PVar;
  pvars->DieTime = 0;
  pvars->CarrierIdx = -1;

  vector_copy(moby->Position, position);
  moby->UpdateDist = 0xFF;
  moby->Drawn = 0x01;
  moby->DrawDist = 0x00FF;
  moby->Opacity = 0x80;
  moby->State = 1;
  moby->MClass = 0x37;

  moby->Scale = (float)0.02;
  moby->Lights = 0x202;
  moby->ModeBits = (moby->ModeBits & 0xFF) | 0x10;
  moby->PrimaryColor = 0xFFFF4040;
  moby->PUpdate = &ballUpdate;

  // animation stuff
  memcpy(&moby->AnimSeq, &BallVisualRefMoby->AnimSeq, 0x20);
  moby->AnimSpeed = 4;

  moby->PClass = BallVisualRefMoby->PClass;
  moby->CollData = BallVisualRefMoby->CollData;

  return 0;
}

//--------------------------------------------------------------------------
int ballHandleGuberEvent(Moby* moby, GuberEvent* event)
{
	if (!moby || !event || !isInGame())
		return 0;

  if (isInGame() && !mobyIsDestroyed(moby) && moby->OClass == BALL_MOBY_OCLASS && moby->PVar) {
		u32 eventId = event->NetEvent.EventID;

		switch (eventId)
		{
			case BALL_EVENT_SPAWN: return ballHandleGuberEvent_Spawn(moby, event);
			default:
			{
				DPRINTF("unhandle ball event %d\n", eventId);
				break;
			}
		}
	}

	return 0;
}

//--------------------------------------------------------------------------
void ballInitialize(void)
{
	Moby* testMoby = mobySpawn(BALL_MOBY_OCLASS, 0);
	if (testMoby) {
		u32 mobyFunctionsPtr = (u32)mobyGetFunctions(testMoby);
		if (mobyFunctionsPtr) {
			// set vtable callbacks
			*(u32*)(mobyFunctionsPtr + 0x14) = (u32)&ballHandleGuberEvent;
		}

		mobyDestroy(testMoby);
	}
}
