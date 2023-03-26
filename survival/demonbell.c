#include "include/demonbell.h"
#include "include/drop.h"
#include "include/mob.h"
#include "include/utils.h"
#include "include/game.h"
#include <string.h>
#include <libdl/stdio.h>
#include <libdl/game.h>
#include <libdl/color.h>
#include <libdl/collision.h>
#include <libdl/moby.h>
#include <libdl/ui.h>
#include <libdl/graphics.h>
#include <libdl/random.h>
#include <libdl/radar.h>

extern struct SurvivalState State;
extern SoundDef BaseSoundDef;

void pushSnack(char * str, int ticksAlive, int localPlayerIdx);
GuberEvent* demonbellCreateEvent(Moby* moby, u32 eventType);

//--------------------------------------------------------------------------
void demonbellPlayActivateSound(Moby* moby)
{
	BaseSoundDef.Index = 366;
	soundPlay(&BaseSoundDef, 0, moby, 0, 0x400);
}	

//--------------------------------------------------------------------------
void demonbellDestroy(Moby* moby)
{
	// create event
	GuberEvent * guberEvent = demonbellCreateEvent(moby, DEMONBELL_EVENT_DESTROY);
}

//--------------------------------------------------------------------------
void demonbellUpdate(Moby* moby)
{
	int i;
	struct DemonBellPVar* pvars = (struct DemonBellPVar*)moby->PVar;
	if (!pvars)
		return;

  switch (moby->State)
  {
    case 1: // activated
    {
      // check if new round and deactivate
      if (State.RoundNumber != pvars->RoundActivated && gameAmIHost()) {
        demonbellCreateEvent(moby, DEMONBELL_EVENT_DEACTIVATE);
        return;
      }

      // rotate
      moby->Rotation[2] = clampAngle(moby->Rotation[2] - pvars->HitAmount*7*MATH_DT*MATH_PI);

      // pulse
      float t = (sinf(DEMONBELL_PULSE_SPEED * gameGetTime() / 1000.0)+1) / 2.0;
      moby->GlowRGBA = colorLerp(DEMONBELL_PULSE_COLOR_1, DEMONBELL_PULSE_COLOR_2, t);
      break;
    }
    case 0: // awaiting activation
    {
      // hit
      int damageIndex = moby->CollDamage;
      if (damageIndex >= 0) {
        MobyColDamage* colDamage = mobyGetDamage(moby, 0x00481C40, 0);
        if (colDamage && pvars->HitAmount < 1) {
          pvars->HitAmount = clamp(pvars->HitAmount + colDamage->DamageHp * DEMONBELL_DAMAGE_SCALE, 0, 1);
          pvars->RecoverCooldownTicks = DEMONBELL_HIT_COOLDOWN_TICKS;

          // activate
          if (pvars->HitAmount == 1 && gameAmIHost()) {
            GuberEvent * guberEvent = demonbellCreateEvent(moby, DEMONBELL_EVENT_ACTIVATE);
            Player* sourcePlayer = guberMobyGetPlayerDamager(colDamage->Damager);
            int activatedByPlayerId = -1;
            if (sourcePlayer) {
              activatedByPlayerId = sourcePlayer->PlayerId;
            }

            if (guberEvent) {
              guberEventWrite(guberEvent, &activatedByPlayerId, 4);
            }
          }
        }

        moby->CollDamage = -1;
      }

      // cooldown
      if (pvars->RecoverCooldownTicks) {
        --pvars->RecoverCooldownTicks;
      } else if (pvars->HitAmount > 0) {
        pvars->HitAmount = clamp(pvars->HitAmount - 1*MATH_DT, 0, 1);
      }

      // rotate
      moby->Rotation[2] = clampAngle(moby->Rotation[2] - pvars->HitAmount*3*MATH_DT*MATH_PI);

      // default color
      moby->GlowRGBA = DEMONBELL_DEFAULT_COLOR;
      break;
    }
  }
}

//--------------------------------------------------------------------------
GuberEvent* demonbellCreateEvent(Moby* moby, u32 eventType)
{
	GuberEvent * event = NULL;

	// create guber object
	Guber* guber = guberGetObjectByMoby(moby);
	if (guber)
		event = guberEventCreateEvent(guber, eventType, 0, 0);

	return event;
}

//--------------------------------------------------------------------------
int demonbellHandleEvent_Spawn(Moby* moby, GuberEvent* event)
{
	VECTOR p;
	int i;

	// read event
	guberEventRead(event, p, 12);

	// set position
	vector_copy(moby->Position, p);

	// set update
	moby->PUpdate = &demonbellUpdate;

	// 
	//moby->ModeBits |= 0x30;
	//moby->GlowRGBA = MobSecondaryColors[(int)args.MobType];
	//moby->PrimaryColor = MobPrimaryColors[(int)args.MobType];
	//moby->CollData = NULL;
	//moby->DrawDist = 0;
	//moby->PClass = NULL;

  moby->Scale = 0.05;
  moby->ModeBits = 0x5050;

	// update pvars
	struct DemonBellPVar* pvars = (struct DemonBellPVar*)moby->PVar;

	// set team
	Guber* guber = guberGetObjectByMoby(moby);
	if (guber)
		((GuberMoby*)guber)->TeamNum = 10;
	
	// 
	mobySetState(moby, 0, -1);
	DPRINTF("demonbell spawned at %08X\n", (u32)moby);
	return 0;
}

//--------------------------------------------------------------------------
int demonbellHandleEvent_Destroy(Moby* moby, GuberEvent* event)
{
	char killedByPlayerId, weaponId;
	int i;
	struct DemonBellPVar* pvars = (struct DemonBellPVar*)moby->PVar;
	if (!pvars)
		return 0;

	guberMobyDestroy(moby);
	return 0;
}

//--------------------------------------------------------------------------
int demonbellHandleEvent_Activate(Moby* moby, GuberEvent* event)
{
  int activatedByPlayerId = -1;
	struct DemonBellPVar* pvars = (struct DemonBellPVar*)moby->PVar;
	if (!pvars)
		return 0;


  guberEventRead(event, &activatedByPlayerId, 4);

  // increment demon bell stat
  if (activatedByPlayerId >= 0) {
    State.PlayerStates[activatedByPlayerId].State.TimesActivatedDemonBell += 1;
  }

  pvars->RoundActivated = State.RoundNumber;
  State.RoundDemonBellCount += 1;
  mobySetState(moby, 1, -1);
  demonbellPlayActivateSound(moby);
  pushSnack("Demon Bell Activated!", 120, 0);
	DPRINTF("demonbell activated at %08X by \n", (u32)moby, activatedByPlayerId);
	return 0;
}

//--------------------------------------------------------------------------
int demonbellHandleEvent_Deactivate(Moby* moby, GuberEvent* event)
{
	struct DemonBellPVar* pvars = (struct DemonBellPVar*)moby->PVar;
	if (!pvars)
		return 0;
    
  mobySetState(moby, 0, -1);
  pvars->HitAmount = 0;
  pvars->RecoverCooldownTicks = 0;
	DPRINTF("demonbell deactivated at %08X\n", (u32)moby);
	return 0;
}

//--------------------------------------------------------------------------
int demonbellHandleEvent(Moby* moby, GuberEvent* event)
{
	struct UpgradePVar* pvars = (struct UpgradePVar*)moby->PVar;

	if (isInGame() && !mobyIsDestroyed(moby) && moby->OClass == DEMONBELL_MOBY_OCLASS && pvars) {
		u32 upgradeEvent = event->NetEvent.EventID;

		switch (upgradeEvent)
		{
			case DEMONBELL_EVENT_SPAWN: return demonbellHandleEvent_Spawn(moby, event);
			case DEMONBELL_EVENT_DESTROY: return demonbellHandleEvent_Destroy(moby, event);
			case DEMONBELL_EVENT_ACTIVATE: return demonbellHandleEvent_Activate(moby, event);
			case DEMONBELL_EVENT_DEACTIVATE: return demonbellHandleEvent_Deactivate(moby, event);
			default:
			{
				DPRINTF("unhandle demonbell event %d\n", upgradeEvent);
				break;
			}
		}
	}

	return 0;
}

//--------------------------------------------------------------------------
int demonbellCreate(VECTOR position)
{
	GameSettings* gs = gameGetSettings();

	// create guber object
	GuberEvent * guberEvent = 0;
	guberMobyCreateSpawned(DEMONBELL_MOBY_OCLASS, sizeof(struct DemonBellPVar), &guberEvent, NULL);
	if (guberEvent)
	{
		guberEventWrite(guberEvent, position, 12);
	}
	else
	{
		DPRINTF("failed to guberevent demonbell\n");
	}
  
  return guberEvent != NULL;
}

//--------------------------------------------------------------------------
void demonbellInitialize(void)
{
	Moby* testMoby = mobySpawn(DEMONBELL_MOBY_OCLASS, 0);
	if (testMoby) {
		u32 mobyFunctionsPtr = (u32)mobyGetFunctions(testMoby);
		if (mobyFunctionsPtr) {
			// set vtable callbacks
			*(u32*)(mobyFunctionsPtr + 0x04) = (u32)&getGuber;
			*(u32*)(mobyFunctionsPtr + 0x14) = (u32)&handleEvent;
		}

		mobyDestroy(testMoby);
	}
}
