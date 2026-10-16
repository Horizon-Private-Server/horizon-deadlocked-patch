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
#define BALL_PHYSICS_RADIUS (0.35)
#define BALL_PHYSICS_SPIN (2)

VECTOR VECTOR_GRAVITY = { 0, 0, -0.005, 0 };
VECTOR VECTOR_ZERO = { 0, 0, 0, 0 };

int (*CollWaterHeight)(VECTOR from, VECTOR to, u64 a2, Moby * damageSource, u64 t0) = 0x00503780;

//--------------------------------------------------------------------------
int ballGetResetCounter(Moby* moby)
{
  if (!moby)
    return -1;
    
  BallPVars_t * pvars = (BallPVars_t*)moby->PVar;
  return pvars->ResetCounter;
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
void ballThrow(Moby* moby, int playerIdx, float power)
{
  VECTOR velocity, temp, position;
  Player* carrier;
  if (!moby || playerIdx < 0 || playerIdx >= GAME_MAX_PLAYERS) return;
  carrier = playerGetFromIndex(playerIdx);
  if (!carrier || !carrier->IsLocal) return;

  // Keep the existing trajectory. Ownership is committed only by the host.
  float carrierVelocityFactor = 0.2 * vector_innerproduct(carrier->Velocity, (float*)((u32)carrier + 0x1A60));
  float speed = carrierVelocityFactor + ((2 + (1 + fabsf(carrier->CameraPitch.Value)) * THROW_COEFF) * power * (1/60.0));
  vector_write(velocity, 0);
  vector_scale(temp, (float*)((u32)carrier + 0x1A60), speed);
  temp[2] += 0.1;
  vector_add(velocity, velocity, temp);
  vector_copy(position, moby->Position);
  if (ballHasPendingPickup(moby, playerIdx) && carrier->PlayerMoby) {
    MATRIX m;
    mobyGetJointMatrix(carrier->PlayerMoby, 18, m);
    vector_copy(position, &m[12]);
  }
  ballRequestAction(moby, playerIdx, BALL_ACTION_DROP, position, velocity);
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
void ballUpdate(Moby * moby)
{
  VECTOR updatePos, toPos, hitNormal, direction;
  if (!moby)
    return;

  ballNetworkUpdate(moby);
  BallPVars_t * pvars = (BallPVars_t*)moby->PVar;

  Player** players = playerGetAll();
  int carrierIdx = pvars->CarrierIdx;
  int visualCarrierIdx = ballGetVisualCarrierIdx(moby);
  Moby* visualMoby = pvars->VisualMoby;
  Player * carrier = NULL;
  Player * visualCarrier = NULL;
  if (carrierIdx >= 0 && carrierIdx < GAME_MAX_PLAYERS)
    carrier = players[carrierIdx];
  if (visualCarrierIdx >= 0 && visualCarrierIdx < GAME_MAX_PLAYERS)
    visualCarrier = players[visualCarrierIdx];

  int blipIdx = radarGetBlipIndex(moby);
  if (blipIdx >= 0) {
    RadarBlip* blip = radarGetBlips() + blipIdx;
    blip->X = moby->Position[0];
    blip->Y = moby->Position[1];
    blip->Life = 0x1F;
    blip->Type = 2;
    blip->Team = visualCarrier ? visualCarrier->Team : TEAM_WHITE;
    blip->Moby = moby;
  }

  // 
  if (pvars->CarrierIdx < 0 && pvars->DieTime && gameGetTime() > pvars->DieTime && gameAmIHost())
  {
    ballReset(moby, BALL_RESET_CENTER);
    return;
  }

  // A temporarily missing carrier object must not turn possession into a drop.
  if (carrierIdx < 0)
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
    float speed = vector_length(pvars->Velocity);

    // determine next update position
    vector_add(updatePos, moby->Position, pvars->Velocity);

    // add radius of model to next position for raycast
    vector_normalize(direction, pvars->Velocity);
    vector_scale(toPos, direction, BALL_PHYSICS_RADIUS);
    vector_add(toPos, updatePos, toPos);

    // spin/roll in direction of velocity
    if (vector_sqrmag(pvars->Velocity) > 0.0001) {
      float yaw = atan2f(pvars->Velocity[1], pvars->Velocity[0]) + (MATH_PI / 2);

      moby->Rotation[0] = clampAngle(moby->Rotation[0] + (speed * BALL_PHYSICS_SPIN));
      moby->Rotation[2] = lerpf(moby->Rotation[2], yaw, 0.1);
      
      moby->ModeBits &= ~MOBY_MODE_BIT_LOCK_ROTATION;
    }

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
        //float exp = powf((dot + 1) / 2, 0.2);
        vector_scale(pvars->Velocity, pvars->Velocity, (clamp(1 - fabsf(dot), 0, 1) + BOUNCINESS_COEFF) * (1 - FRICTION_COEFF));
        //vector_scale(pvars->Velocity, pvars->Velocity, 0.9);

        // correct position based on hit
        //vector_copy()
        
        vector_scale(toPos, direction, BALL_PHYSICS_RADIUS);
        vector_subtract(updatePos, CollLine_Fix_GetHitPosition(), toPos);

        VECTOR from;
        vector_normalize(hitNormal, CollLine_Fix_GetHitNormal());
        vector_scale(hitNormal, hitNormal, BALL_PHYSICS_RADIUS);
        vector_add(from, updatePos, hitNormal);
        vector_subtract(toPos, updatePos, hitNormal);
        if (CollLine_Fix(from, toPos, 2, 0, 0) != 0) {
          vector_add(updatePos, CollLine_Fix_GetHitPosition(), hitNormal);
        }

        // don't move more than the velocity will allow
        VECTOR dt;
        vector_subtract(dt, updatePos, moby->Position);
        float len = vector_length(dt);
        if (len > speed)
          vector_scale(dt, dt, speed / len);

        vector_add(moby->Position, moby->Position, dt);
      }
    }
    else
    {
      vector_copy(moby->Position, updatePos);
    }
  }
  else if (carrier && carrier->PlayerMoby)
  {
    MATRIX m;
    mobyGetJointMatrix(carrier->PlayerMoby, 18, m);
    vector_copy(moby->Position, &m[12]);
    memcpy(moby->M0_03, &m[0], 0x30);
  }
  
  // update ball moby
  if (visualMoby) {
    // Prediction moves only the visual. Keep the confirmed free-ball physics
    // intact so a rejected/expired pickup cannot teleport the real ball.
    vector_copy(visualMoby->Position, moby->Position);
    memcpy(visualMoby->M0_03, moby->M0_03, 0x30);
    if (visualCarrierIdx != carrierIdx && visualCarrier && visualCarrier->PlayerMoby) {
      MATRIX m;
      mobyGetJointMatrix(visualCarrier->PlayerMoby, 18, m);
      vector_copy(visualMoby->Position, &m[12]);
      memcpy(visualMoby->M0_03, &m[0], 0x30);
    }

    // update ball color to match carrier team
    if (visualCarrier && visualCarrier->PlayerMoby)
      visualMoby->PrimaryColor = visualCarrier->PlayerMoby->GlowRGBA;
    else
      visualMoby->PrimaryColor = 0xFFFFFFFF;
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
  VECTOR position;
  BallPVars_t * pvars = (BallPVars_t*)moby->PVar;

  // read
  guberEventRead(event, position, 0x0C);

  // we need to make sure we have our reference moby to
  // copy the model, animation, and collision of
  if (!pvars->VisualMoby)
  {
    Moby* visualMoby = pvars->VisualMoby = mobySpawn(0x1b37, 0);
    if (!visualMoby)
      return 0;

    visualMoby->AnimSpeed = 4;
    visualMoby->Scale = (float)0.02;
    visualMoby->PUpdate = NULL;
    visualMoby->ModeBits = (visualMoby->ModeBits & 0xFF) | MOBY_MODE_BIT_HAS_GLOW | MOBY_MODE_BIT_LOCK_ROTATION;
    visualMoby->Opacity = 0xFF;
    visualMoby->UpdateDist = 0xFF;
    visualMoby->DrawDist = 0x00FF;
  }

  // spawn ball
	DPRINTF("spawning new ball moby: %08X (%08X) %.2f %.2f %.2f\n", (u32)moby, (u32)pvars->VisualMoby, position[0], position[1], position[2]);
  ballNetworkInitialize(moby);

  vector_copy(moby->Position, position);
  moby->UpdateDist = 0xFF;
  moby->Drawn = 0x01;
  moby->DrawDist = 0x00FF;
  moby->Opacity = 0x80;
  moby->State = 1;

  moby->Scale = (float)0.02;
  moby->ModeBits = (moby->ModeBits & 0xFF) | MOBY_MODE_BIT_HIDDEN;
  moby->PUpdate = &ballUpdate;
  moby->CollData = NULL;

  State.BallMoby = moby;
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
			case BALL_EVENT_SPAWN:
        if (gameIsHost(event->NetEvent.OriginClientIdx))
          return ballHandleGuberEvent_Spawn(moby, event);
        return 0;
      case BALL_EVENT_REQUEST:
      case BALL_EVENT_STATE:
      case BALL_EVENT_REPLY:
      case BALL_EVENT_SCORE:
        return ballHandleNetworkEvent(moby, event);
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
	if (moby && moby->OClass == BALL_MOBY_OCLASS && moby->PVar)
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
