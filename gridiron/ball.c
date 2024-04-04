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

#define DRAG_COEFF (0.05)
#define BOUNCINESS_COEFF (0.2)
#define FRICTION_COEFF (0.2)
#define THROW_COEFF (1.5)
#define LIFETIME (25)

VECTOR VECTOR_GRAVITY = { 0, 0, -0.005, 0 };
VECTOR VECTOR_ZERO = { 0, 0, 0, 0 };

int (*CollWaterHeight)(VECTOR from, VECTOR to, u64 a2, Moby * damageSource, u64 t0) = 0x00503780;

//--------------------------------------------------------------------------
void ballRemoveFromHeldPlayer(Moby* moby)
{
  Player** players = playerGetAll();
  int i;
  for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
    Player* player = players[i];
    if (player && player->HeldMoby == moby) {
      player->HeldMoby = NULL;
      State.PlayerStates[i].TimeLastCarrier = gameGetTime();
    }
  }
}

//--------------------------------------------------------------------------
int ballGetCarrierIdx(Moby* moby)
{
  if (!moby)
    return -1;
    
  BallPVars_t * pvars = (BallPVars_t*)moby->PVar;
  return pvars->CarrierIdx;
}

//--------------------------------------------------------------------------
void ballDrop(Moby * moby)
{
  if (!moby)
    return;

	Guber* guber = guberGetObjectByMoby(moby);
  if (!guber) return;

  GuberEvent* event = guberEventCreateEvent(guber, BALL_EVENT_DROP, 0, 0);
  if (!event) return;

  BallPVars_t * pvars = (BallPVars_t*)moby->PVar;
  int time = gameGetTime();
  guberEventWrite(event, moby->Position, 12);
  guberEventWrite(event, pvars->Velocity, 12);
  guberEventWrite(event, &time, 4);
}

//--------------------------------------------------------------------------
void ballThrow(Moby * moby, float power)
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
  vector_scale(temp, (float*)((u32)carrier + 0x1A60), (2 + (1 + fabsf(carrier->CameraPitch.Value)) * THROW_COEFF) * power * (1/60.0));
  temp[2] += 0.1;
  vector_add(pvars->Velocity, pvars->Velocity, temp);
  ballDrop(moby);
}

//--------------------------------------------------------------------------
void ballPickup(Moby * moby, int playerIdx)
{
  if (!moby)
    return;

	Guber* guber = guberGetObjectByMoby(moby);
  if (!guber) return;

  GuberEvent* event = guberEventCreateEvent(guber, BALL_EVENT_PICKUP, 0, 0);
  if (!event) return;

  guberEventWrite(event, &playerIdx, 4);
}

//--------------------------------------------------------------------------
void ballResendPickup(Moby * moby)
{
  if (!moby)
    return;

  BallPVars_t * pvars = (BallPVars_t*)moby->PVar;

	Guber* guber = guberGetObjectByMoby(moby);
  if (!guber) return;

  GuberEvent* event = guberEventCreateEvent(guber, BALL_EVENT_PICKUP, 0, 0);
  if (!event) return;

  guberEventWrite(event, &pvars->CarrierIdx, 4);
}

//--------------------------------------------------------------------------
void ballMoveToResetPosition(Moby* moby, int resetType)
{
  // move ball to reset position
  switch (resetType)
  {
    case BALL_RESET_CENTER:
    {
      vector_copy(moby->Position, State.BallSpawnPosition);
      break;
    }
    case BALL_RESET_TEAM0:
    {
      vector_copy(moby->Position, State.Teams[0].BallSpawnPosition);
      break;
    }
    case BALL_RESET_TEAM1:
    {
      vector_copy(moby->Position, State.Teams[1].BallSpawnPosition);
      break;
    }
  }
  
}

//--------------------------------------------------------------------------
void ballReset(Moby * moby, int resetType)
{
  if (!moby)
    return;

  // reset locally first
  BallPVars_t * pvars = (BallPVars_t*)moby->PVar;
  vector_write(pvars->Velocity, 0);
  pvars->SyncTicks = 0;
  pvars->CarrierIdx = -1;
  pvars->DieTime = gameGetTime() + (TIME_SECOND * LIFETIME);
  ballRemoveFromHeldPlayer(moby);
  ballMoveToResetPosition(moby, resetType);

  // only reset if host
  if (!gameAmIHost()) return;

	Guber* guber = guberGetObjectByMoby(moby);
  if (!guber) return;

  GuberEvent* event = guberEventCreateEvent(guber, BALL_EVENT_RESET, 0, 0);
  if (!event) return;

  guberEventWrite(event, &resetType, 4);
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
    blip->Type = 2;
    blip->Team = carrier ? carrier->Team : TEAM_WHITE;
    blip->Moby = moby;
  }

  // 
  if (!carrier && pvars->DieTime && gameGetTime() > pvars->DieTime)
  {
    ballReset(moby, BALL_RESET_CENTER);
    return;
  }

  // detect drop
  if (carrier && carrier->HeldMoby != moby && carrier->IsLocal)
  {
    // carry velocity of carrier on drop
    vector_copy(pvars->Velocity, carrier->Velocity);
    ballDrop(moby);
  }

  // if not held do physics
  if (!carrier)
  {
    // detect if fell under map
    if (moby->Position[2] < gameGetDeathHeight())
    {
      if (gameAmIHost()) {
        ballReset(moby, BALL_RESET_CENTER);
      }
      return;
    }

    // add gravity
    vector_add(pvars->Velocity, pvars->Velocity, VECTOR_GRAVITY);
    vector_scale(pvars->Velocity, pvars->Velocity, (1 - DRAG_COEFF/TPS));

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
      u8 cType = CollLine_Fix_GetHitCollisionId() & 0x0F;
      if (cType == 0x0B || cType == 0x01 || cType == 0x04 || cType == 0x05 || cType == 0x0D || cType == 0x03)
      {
        ballReset(moby, BALL_RESET_CENTER);
      }
      else
      {
        // change velocity based on hit
        vector_normalize(hitNormal, (float*)0x0023f940);
        vector_reflect(pvars->Velocity, pvars->Velocity, hitNormal);

        float dot = vector_innerproduct(direction, hitNormal);
        float exp = powf((dot + 1) / 2, 0.2);
        vector_scale(pvars->Velocity, pvars->Velocity, (clamp(1 - fabsf(dot), 0, 1) + BOUNCINESS_COEFF) * (1 - FRICTION_COEFF));
        //vector_scale(pvars->Velocity, pvars->Velocity, 0.9);

        // correct position based on hit
        //vector_copy()
        
        vector_scale(toPos, direction, 0.5);
        vector_subtract(moby->Position, CollLine_Fix_GetHitPosition(), toPos);
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
  //moby->MClass = 0x37;

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

  State.BallMoby = moby;
  return 0;
}

//--------------------------------------------------------------------------
int ballHandleGuberEvent_Reset(Moby* moby, GuberEvent* event)
{
  int resetType;

  // read
  guberEventRead(event, &resetType, 4);

  BallPVars_t * pvars = (BallPVars_t*)moby->PVar;
  vector_copy(moby->Position, State.BallSpawnPosition);
  vector_write(pvars->Velocity, 0);
  pvars->SyncTicks = 0;

  // drop from existing carrier
  ballRemoveFromHeldPlayer(moby);
  pvars->CarrierIdx = -1;
  pvars->DieTime = gameGetTime() + (TIME_SECOND * LIFETIME);
  ballMoveToResetPosition(moby, resetType);
  
  // popup
  if (resetType == BALL_RESET_CENTER) uiShowPopup(0, "The ball has returned to the center!");
  return 0;
}

//--------------------------------------------------------------------------
int ballHandleGuberEvent_Pickup(Moby* moby, GuberEvent* event)
{
  // read
  int playerIdx;
  guberEventRead(event, &playerIdx, sizeof(playerIdx));

  // pickup
  BallPVars_t * pvars = (BallPVars_t*)moby->PVar;
  Player** players = playerGetAll();
  Player* player = NULL;
  if (playerIdx >= 0 && playerIdx < GAME_MAX_PLAYERS)
    player = players[playerIdx];
  
  // drop from existing carrier
  ballRemoveFromHeldPlayer(moby);

  // set new carrier
  pvars->CarrierIdx = playerIdx;
  pvars->SyncTicks = 0;
  if (player)
    player->HeldMoby = moby;
    
  // popup and play sound
  if (playerIdx >= 0) {
    mobyPlaySoundByClass(0, 0, playerGetFromSlot(0)->PlayerMoby, MOBY_ID_RED_FLAG);
    uiShowPopup(0, "The ball has been picked up!");
  }
  return 0;
}

//--------------------------------------------------------------------------
int ballHandleGuberEvent_Drop(Moby* moby, GuberEvent* event)
{
  VECTOR position, velocity;
  int time;

  // read
  guberEventRead(event, position, 12);
  guberEventRead(event, velocity, 12);
  guberEventRead(event, &time, 4);
  
  BallPVars_t * pvars = (BallPVars_t*)moby->PVar;
  if (pvars->CarrierIdx >= 0) {
    Player* player = playerGetAll()[pvars->CarrierIdx];
    if (player && player->HeldMoby == moby) {
      player->HeldMoby = NULL;
    }
      
    State.PlayerStates[pvars->CarrierIdx].TimeLastCarrier = time;

    // popup and play sound
    mobyPlaySoundByClass(1, 0, playerGetFromSlot(0)->PlayerMoby, MOBY_ID_RED_FLAG);
    uiShowPopup(0, "The ball has been dropped!");
  }
  
  pvars->DieTime = time + (TIME_SECOND * LIFETIME);
  pvars->CarrierIdx = -1;
  pvars->SyncTicks = 0;
  
  // set
  vector_copy(moby->Position, position);
  vector_copy(pvars->Velocity, velocity);
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
      case BALL_EVENT_RESET: return ballHandleGuberEvent_Reset(moby, event);
      case BALL_EVENT_PICKUP: return ballHandleGuberEvent_Pickup(moby, event);
      case BALL_EVENT_DROP: return ballHandleGuberEvent_Drop(moby, event);
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
struct GuberMoby* ballGetGuber(Moby* moby)
{
	if (moby->OClass == BALL_MOBY_OCLASS && moby->PVar)
		return moby->GuberMoby;
	
  DPRINTF("get guber miss %08X %04X\n", (u32)moby, moby->OClass);
	return 0;
}

//--------------------------------------------------------------------------
void ballInitialize(void)
{
  static int init = 0;
  if (init) return;

	Moby* testMoby = mobySpawn(BALL_MOBY_OCLASS, 0);
	if (testMoby) {
		u32 mobyFunctionsPtr = (u32)mobyGetFunctions(testMoby);
		if (mobyFunctionsPtr) {
			// set vtable callbacks
      *(u32*)(mobyFunctionsPtr + 0x04) = (u32)&ballGetGuber;
			*(u32*)(mobyFunctionsPtr + 0x14) = (u32)&ballHandleGuberEvent;
      DPRINTF("ball vtable %04X %08X\n", testMoby->OClass, mobyFunctionsPtr);
		}

		mobyDestroy(testMoby);
    init = 1;
	}
}
