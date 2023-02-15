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
#include <libdl/stdio.h>
#include <libdl/gamesettings.h>
#include <libdl/dialog.h>
#include <libdl/patch.h>
#include <libdl/ui.h>
#include <libdl/graphics.h>
#include <libdl/color.h>
#include <libdl/utils.h>

#define GATE_OCLASS             (0x1F6)

struct GatePVar
{
  VECTOR From;
  VECTOR To;
  float Height;
  float Opacity;
  int Dirty;
};

enum GateEventType {
	GATE_EVENT_SPAWN,
  GATE_EVENT_ACTIVATE,
  GATE_EVENT_DEACTIVATE
};

enum GateState {
	GATE_STATE_DEACTIVATED,
	GATE_STATE_ACTIVATED,
};

VECTOR GateLocations[] = {
  { 383.47, 564.51, 430.44, 6.5 }, { 383.47, 550.51, 430.44, 0 },
  { 425.35, 564.51, 430.44, 6.5 }, { 425.35, 550.51, 430.44, 0 },
  { 425.46, 652.23, 430.44, 6.5 }, { 425.46, 638.23, 430.44, 0 },
  { 383.24, 652.23, 430.44, 6.5 }, { 383.24, 638.23, 430.44, 0 },
  { 359.28, 614.63, 430.44, 6.5 }, { 373.28, 614.63, 430.44, 0 },
  { 359.28, 576.26, 430.44, 6.5 }, { 373.28, 576.26, 430.44, 0 },
  { 470.88, 607.19, 439.14, 9 }, { 470.88, 593.19, 439.14, 0 },
  { 488.89, 523.54, 430.11, 6.5 }, { 488.89, 509.54, 430.11, 0 },
  { 488.89, 690.17, 430.11, 6.5 }, { 488.89, 676.17, 430.11, 0 },
  { 553.5787, 618.6273, 430.11, 6.5 }, { 563.3413, 608.5927, 430.11, 0 },
  { 587.5181, 599.4265, 430.11, 6.5 }, { 598.042, 590.1935, 430.11, 0 },
};
const int GateLocationsCount = sizeof(GateLocations)/sizeof(VECTOR);

//--------------------------------------------------------------------------
void gateDrawQuad(Moby* moby, float direction)
{
  if (!moby || !moby->PVar)
    return;

  struct GatePVar* pvars = (struct GatePVar*)moby->PVar;
  struct QuadDef quad;
	MATRIX m2;
	VECTOR t;
	VECTOR pTL = {0,-0.5,0.5,1};
	VECTOR pTR = {0,0.5,0.5,1};
	VECTOR pBL = {0,-0.5,-0.5,1};
	VECTOR pBR = {0,0.5,-0.5,1};
	VECTOR scale = {1,pvars->Height,pvars->Height,0};

  float uScale = direction * 1.5;
  float vScale = direction * 1.0;
  float uOff = direction * (gameGetTime() / 4000.0);
  float vOff = 0;

	float u0 = fastmodf(uOff*uScale, 1);
	float u1 = uScale + u0;
	float v0 = vOff*vScale;
	float v1 = (1+vOff)*vScale;

  u32 color = colorLerp(0, 0x40404040, pvars->Opacity);

	// init
  gfxResetQuad(&quad);

	// color of each corner?
	vector_copy(quad.VertexPositions[0], pTL);
	vector_copy(quad.VertexPositions[1], pTR);
	vector_copy(quad.VertexPositions[2], pBL);
	vector_copy(quad.VertexPositions[3], pBR);
	quad.VertexColors[0] = quad.VertexColors[1] = quad.VertexColors[2] = quad.VertexColors[3] = color;
	quad.VertexUVs[0] = (struct UV){u0,v0};
	quad.VertexUVs[1] = (struct UV){u1,v0};
	quad.VertexUVs[2] = (struct UV){u0,v1};
	quad.VertexUVs[3] = (struct UV){u1,v1};
	quad.Clamp = 0;
	quad.Tex0 = gfxGetEffectTex(13, 1);
	quad.Tex1 = 0;
	quad.Alpha = 0x8000000058;

	// copy matrix from moby
  matrix_unit(m2);
  matrix_scale(m2, m2, scale);
  matrix_multiply(m2, m2, moby->M0_03);
	memcpy(&m2[12], moby->Position, sizeof(VECTOR));

	// draw
	gfxDrawQuad((void*)0x00222590, &quad, m2, 0);
}

//--------------------------------------------------------------------------
void gateDraw(Moby* moby)
{
  gateDrawQuad(moby, 1);
  gateDrawQuad(moby, -1);
}

//--------------------------------------------------------------------------
void gateUpdate(Moby* moby)
{
  VECTOR delta;
  if (!moby || !moby->PVar)
    return;
    
  struct GatePVar* pvars = (struct GatePVar*)moby->PVar;

	// draw gate
  if (pvars->Opacity > 0)
    gfxRegisterDrawFunction((void**)0x0022251C, &gateDraw, moby);

  // handle state
  if (moby->State == GATE_STATE_ACTIVATED) {
    pvars->Opacity = clamp(pvars->Opacity + MATH_DT, 0, 1);
  } else if (moby->State == GATE_STATE_DEACTIVATED) {
    pvars->Opacity = clamp(pvars->Opacity - MATH_DT, 0, 1);
  }
  
  // collision only if visible
  if (pvars->Opacity > 0)
    moby->CollActive = 0;
  else
    moby->CollActive = -1;

  // update moby to fit from position to target
  if (pvars->Dirty)
  {
    moby->Scale = pvars->Height * 0.02083333395;

    vector_subtract(delta, pvars->To, pvars->From);
    vector_scale(delta, delta, 0.5);
    vector_add(moby->Position, pvars->From, delta);

    vector_scale(delta, delta, 2 / pvars->Height);

    matrix_unit(moby->M0_03);
    vector_copy(moby->M1_03, delta);
    vector_outerproduct(moby->M0_03, moby->M1_03, moby->M2_03);

    // allow game to update BSphere
    moby->ModeBits &= ~4;
    pvars->Dirty = 0;
  }
  else
  {
    // prevent game from updating BSphere
    moby->ModeBits |= 4;
  }

  if (padGetButton(0, PAD_UP) > 0) {
    mobySetState(moby, GATE_STATE_ACTIVATED, -1);
  } else if (padGetButton(0, PAD_DOWN) > 0) {
    mobySetState(moby, GATE_STATE_DEACTIVATED, -1);
  }

  // force BSphere radius to something larger so that entire collision registers
  moby->BSphere[3] = 14444;
}

//--------------------------------------------------------------------------
int gateHandleEvent_Spawned(Moby* moby, GuberEvent* event)
{
	int i;
  
  DPRINTF("gate spawned: %08X\n", (u32)moby);
  struct GatePVar* pvars = (struct GatePVar*)moby->PVar;
  if (!pvars)
    return 0;
  
	// read event
	guberEventRead(event, pvars->From, 12);
	guberEventRead(event, pvars->To, 12);
	guberEventRead(event, &pvars->Height, 4);

	// set update
	moby->PUpdate = &gateUpdate;

  // set dirty
  pvars->Dirty = 1;

  // don't render
  moby->ModeBits |= 0x101;

  // set default state
	mobySetState(moby, GATE_STATE_ACTIVATED, -1);
  return 0;
}

//--------------------------------------------------------------------------
struct GuberMoby* gateGetGuber(Moby* moby)
{
	if (moby->OClass == 0x1F6 && moby->PVar)
		return moby->GuberMoby;
	
	return 0;
}

//--------------------------------------------------------------------------
int gateHandleEvent(Moby* moby, GuberEvent* event)
{
	if (!moby || !event || !isInGame() || moby->OClass != 0x1F6)
		return 0;

	if (isInGame() && !mobyIsDestroyed(moby) && moby->OClass == GATE_OCLASS && moby->PVar) {
		u32 upgradeEvent = event->NetEvent.EventID;

		switch (upgradeEvent)
		{
			case GATE_EVENT_SPAWN: return gateHandleEvent_Spawned(moby, event);
      case GATE_EVENT_ACTIVATE: { mobySetState(moby, GATE_STATE_ACTIVATED, -1); return 0; }
      case GATE_EVENT_DEACTIVATE: { mobySetState(moby, GATE_STATE_DEACTIVATED, -1); return 0; }
			default:
			{
				DPRINTF("unhandle gate event %d\n", upgradeEvent);
				break;
			}
		}
	}

	return 0;
}

//--------------------------------------------------------------------------
int gateCreate(VECTOR start, VECTOR end, float height)
{
	// create guber object
	GuberEvent * guberEvent = 0;
	guberMobyCreateSpawned(GATE_OCLASS, sizeof(struct GatePVar), &guberEvent, NULL);
	if (guberEvent)
	{
		guberEventWrite(guberEvent, start, 12);
		guberEventWrite(guberEvent, end, 12);
    guberEventWrite(guberEvent, &height, 4);
	}
	else
	{
		DPRINTF("failed to guberevent gate\n");
	}
  
  return guberEvent != NULL;
}

void gateInit(void)
{
  int i;
  Moby* temp = mobySpawn(GATE_OCLASS, 0);
  if (!temp)
    return;

  // set vtable callbacks
  u32 mobyFunctionsPtr = (u32)mobyGetFunctions(temp);
  if (mobyFunctionsPtr) {
    *(u32*)(mobyFunctionsPtr + 0x04) = (u32)&gateGetGuber;
    *(u32*)(mobyFunctionsPtr + 0x14) = (u32)&gateHandleEvent;
    DPRINTF("mClass:%02X func:%08X getGuber:%08X handleEvent:%08X\n", temp->MClass, mobyFunctionsPtr, *(u32*)(mobyFunctionsPtr + 0x04), *(u32*)(mobyFunctionsPtr + 0x14));
  }
  mobyDestroy(temp);
  
  // create gates
  if (gameAmIHost()) {
    for (i = 0; i < GateLocationsCount; i += 2) {
      gateCreate(GateLocations[i], GateLocations[i+1], GateLocations[i][3]);
    }
  }
}
