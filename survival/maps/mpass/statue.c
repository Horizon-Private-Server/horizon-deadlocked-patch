#include <libdl/stdio.h>
#include <libdl/game.h>
#include <libdl/collision.h>
#include <libdl/graphics.h>
#include <libdl/moby.h>
#include <libdl/random.h>
#include <libdl/radar.h>
#include <libdl/camera.h>
#include <libdl/guber.h>
#include <libdl/color.h>
#include <libdl/math.h>
#include <libdl/player.h>

#include "game.h"
#include "mpass.h"
#include "maputils.h"

extern int aaa;

SoundDef StatueSoundDef =
{
	0.0,	// MinRange
	50.0,	// MaxRange
	100,		// MinVolume
	1000,		// MaxVolume
	0,			// MinPitch
	0,			// MaxPitch
	0,			// Loop
	0x10,		// Flags
	158,    // 298, 158
	3 		  // Bank
};


//--------------------------------------------------------------------------
void statueSetState(Moby* moby, enum StatueMobyState state)
{
	GuberEvent* event = guberCreateEvent(moby, STATUE_EVENT_STATE_UPDATE);
	if (event) {
		guberEventWrite(event, &state, sizeof(enum StatueMobyState));
	}
}

//--------------------------------------------------------------------------
void statueUpdate(Moby* moby)
{
  struct StatuePVar* pvars = (struct StatuePVar*)moby->PVar;
  if (!pvars)
    return;

  float colorT = (sinf(5 * (gameGetTime() / (float)TIME_SECOND)) + 1) / 2;
  u32 activatedGlowColor = colorLerp(0xFFFF8000, 0xFFFFFF00, colorT);

  // handle orb
  Moby* orb = pvars->ActivatedOrbMoby;
  if (orb) {
    orb->PrimaryColor = orb->GlowRGBA = activatedGlowColor;
    orb->Opacity = (moby->State == STATUE_STATE_DEACTIVATED) ? 0 : 0x80;
  }

  // 
  moby->GlowRGBA = (moby->State == STATUE_STATE_DEACTIVATED) ? 0x80C0C0C0 : activatedGlowColor;
}

//--------------------------------------------------------------------------
int statueHandleEvent_Spawned(Moby* moby, GuberEvent* event)
{
	int i;
  
  DPRINTF("statue spawned: %08X\n", (u32)moby);
  struct StatuePVar* pvars = (struct StatuePVar*)moby->PVar;
  if (!pvars)
    return 0;
  
	// read event
	guberEventRead(event, moby->Position, 12);
	guberEventRead(event, moby->Rotation, 12);

	// set update
	moby->PUpdate = &statueUpdate;

  // create orb
  Moby* orb = mobySpawn(0x2034, 0);
  pvars->ActivatedOrbMoby = orb;
  if (orb) {
    MATRIX m;
    VECTOR f = {0,-4.333,1.461,0};
    matrix_unit(m);
    matrix_rotate_x(m, m, moby->Rotation[0]);
    matrix_rotate_y(m, m, moby->Rotation[1]);
    matrix_rotate_z(m, m, moby->Rotation[2]);
    vector_apply(f, f, m);
    vector_add(orb->Position, moby->Position, f);
    vector_copy(orb->Rotation, moby->Rotation);
    orb->Rotation[0] = (MATH_PI/2) - orb->Rotation[0];

    orb->DrawDist = 0xFF;
    orb->UpdateDist = 0x80;
    orb->Scale = 0.012;
    orb->PUpdate = NULL;
  }

  DPRINTF("orb: %08X\n", (u32)orb);

  // set default state
	mobySetState(moby, STATUE_STATE_DEACTIVATED, -1);
  return 0;
}

//--------------------------------------------------------------------------
int statueHandleEvent_StateUpdate(Moby* moby, GuberEvent* event)
{
	int i;
  enum StatueMobyState state;
  
	// read event
	guberEventRead(event, &state, sizeof(enum StatueMobyState));

  // set state
	mobySetState(moby, state, -1);
  DPRINTF("statue state update: %08X => %d\n", (u32)moby, state);
  return 0;
}

//--------------------------------------------------------------------------
struct GuberMoby* statueGetGuber(Moby* moby)
{
	if (moby->OClass == STATUE_MOBY_OCLASS && moby->PVar)
		return moby->GuberMoby;
	
	return 0;
}

//--------------------------------------------------------------------------
int statueHandleEvent(Moby* moby, GuberEvent* event)
{
	if (!moby || !event)
		return 0;

	if (isInGame() && !mobyIsDestroyed(moby) && moby->OClass == STATUE_MOBY_OCLASS && moby->PVar) {
		u32 eventId = event->NetEvent.EventID;
		switch (eventId)
		{
			case STATUE_EVENT_SPAWN: return statueHandleEvent_Spawned(moby, event);
      case STATUE_EVENT_STATE_UPDATE: return statueHandleEvent_StateUpdate(moby, event);
			default:
			{
				DPRINTF("unhandle statue event %d\n", eventId);
				break;
			}
		}
	}

	return 0;
}

//--------------------------------------------------------------------------
int statueCreate(VECTOR position, VECTOR rotation)
{
	// create guber object
	GuberEvent * guberEvent = 0;
	guberMobyCreateSpawned(STATUE_MOBY_OCLASS, sizeof(struct StatuePVar), &guberEvent, NULL);
	if (guberEvent)
	{
		guberEventWrite(guberEvent, position, 12);
		guberEventWrite(guberEvent, rotation, 12);
	}
	else
	{
		DPRINTF("failed to guberevent statue\n");
	}
  
  return guberEvent != NULL;
}

//--------------------------------------------------------------------------
void statueSpawn(void)
{
  static int spawned = 0;
  int i;
  
  if (spawned)
    return;

  // spawn
  if (gameAmIHost()) {
    for (i = 0; i < statueSpawnPositionRotationsCount; ++i) {
      statueCreate(statueSpawnPositionRotations[i*2 + 0], statueSpawnPositionRotations[i*2 + 1]);
    }
  }

  spawned = 1;
}

//--------------------------------------------------------------------------
void statueInit(void)
{
  int i;
  Moby* temp = mobySpawn(STATUE_MOBY_OCLASS, 0);
  if (!temp)
    return;

  // set vtable callbacks
  u32 mobyFunctionsPtr = (u32)mobyGetFunctions(temp);
  if (mobyFunctionsPtr) {
    *(u32*)(mobyFunctionsPtr + 0x04) = (u32)&statueGetGuber;
    *(u32*)(mobyFunctionsPtr + 0x14) = (u32)&statueHandleEvent;
    DPRINTF("STATUE oClass:%04X mClass:%02X func:%08X getGuber:%08X handleEvent:%08X\n", temp->OClass, temp->MClass, mobyFunctionsPtr, *(u32*)(mobyFunctionsPtr + 0x04), *(u32*)(mobyFunctionsPtr + 0x14));
  }
  mobyDestroy(temp);
}
