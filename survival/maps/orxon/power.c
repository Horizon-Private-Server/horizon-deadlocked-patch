/***************************************************
 * FILENAME :		power.c
 * 
 * DESCRIPTION :
 * 		Handles logic for power.
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
#include "module.h"
#include "messageid.h"
#include "../../shared/include/maputils.h"

#define MAX_TEMP_POWER_ON_MOBYS         (10)

int powerTimeOff = -1;
int powerForcedOn = 0;

struct MobyTempPowerOn
{
  Moby* pMoby;
  int TimeDeactivate;
};

struct MobyTempPowerOn TempPowerOnList[MAX_TEMP_POWER_ON_MOBYS];

//--------------------------------------------------------------------------
int powerMobyHasTempPowerOn(Moby* moby)
{
  int i;

  for (i = 0; i < MAX_TEMP_POWER_ON_MOBYS; ++i) {
    if (TempPowerOnList[i].pMoby == moby)
      return 1;
  }

  return 0;
}

//--------------------------------------------------------------------------
int powerIsMobyOn(Moby* moby)
{
  switch (moby->OClass)
  {
    case MOBY_ID_BOLT_CRANK_MP: return powerTimeOff >= 0;
    case MOBY_ID_JUMP_PAD: return moby->State == 3;
    case MOBY_ID_PICKUP_PAD: return (*(u32*)moby->PVar & 0x10000000) == 0;
  }

  return 0;
}

//--------------------------------------------------------------------------
void powerUpdateMoby(Moby* moby, int powerOn)
{
  switch (moby->OClass)
  {
    case MOBY_ID_JUMP_PAD:
    {
      moby->State = powerOn ? 3 : 1;
      break;
    }
    case MOBY_ID_PICKUP_PAD:
    {
      u32 v = *(u32*)moby->PVar;
      if (powerOn) {
        *(u32*)moby->PVar = v &= 0xFF;
        *(u32*)(moby->PVar + 0x7C) = 0;
      } else {
        *(u32*)moby->PVar = v |= 0x10000000;
      }
      break;
    }
  }
}

//--------------------------------------------------------------------------
void powerUpdateMobies(int powerOn)
{
  Moby* m = mobyListGetStart();
  Moby* mEnd = mobyListGetEnd();

  while (m < mEnd)
  {
    if (!mobyIsDestroyed(m)) {
      powerUpdateMoby(m, powerOn);
    }

    ++m;
  }
}

//--------------------------------------------------------------------------
void powerActivateMobyTempPower(Moby* moby, int durationMs)
{
  int i;
  int idx = -1;

  for (i = 0; i < MAX_TEMP_POWER_ON_MOBYS; ++i) {
    struct MobyTempPowerOn* tempPowerOn = &TempPowerOnList[i];
    if (tempPowerOn->pMoby == moby) {
      idx = i;
      break;
    } else if (!tempPowerOn->pMoby) {
      idx = i;
    }
  }

  // activate
  if (idx >= 0) {
    struct MobyTempPowerOn* tempPowerOn = &TempPowerOnList[idx];
    tempPowerOn->TimeDeactivate = gameGetTime() + durationMs;
    tempPowerOn->pMoby = moby;
    powerUpdateMoby(moby, 1);
    DPRINTF("temp activate %08X for %d ms\n", (u32)moby, durationMs);
  }
}

//--------------------------------------------------------------------------
void powerDeactivateMobyTempPower(void)
{
  int i;
  int time = gameGetTime();

  for (i = 0; i < MAX_TEMP_POWER_ON_MOBYS; ++i) {
    struct MobyTempPowerOn* tempPowerOn = &TempPowerOnList[i];
    if (tempPowerOn->pMoby && time > tempPowerOn->TimeDeactivate) {

      // turn off only if power is off
      if (powerTimeOff < 0) {
        powerUpdateMoby(tempPowerOn->pMoby, 0);
        DPRINTF("temp deactivate %08X\n", (u32)tempPowerOn->pMoby);
      }
      tempPowerOn->pMoby = 0;
    }
  }
}

//--------------------------------------------------------------------------
void powerOnMysteryBoxActivatePower(void)
{
  powerForcedOn = 1;
  powerTimeOff = gameGetTime() + 5*TIME_MINUTE;

  pushSnack(-1, "Power supercharged", 60);
}

//--------------------------------------------------------------------------
void powerOnSurgeMoby(Moby* moby)
{
  if (!moby)
    return;

  switch (moby->OClass)
  {
    case MOBY_ID_BOLT_CRANK_MP:
    {
      // supercharge once
      if (!powerForcedOn) {
        powerOnMysteryBoxActivatePower();
      }
      break;
    }
    case MOBY_ID_JUMP_PAD:
    {
      powerActivateMobyTempPower(moby, TIME_SECOND * 5);
      break;
    }
    case MOBY_ID_PICKUP_PAD:
    {
      powerActivateMobyTempPower(moby, TIME_SECOND * 5);
      break;
    }
  }
}

//--------------------------------------------------------------------------
void powerNodeUpdate(Moby* moby)
{
  static int initialized = 0;
  float baseTurnSpeed = 0.061666;
  int i;
  if (!moby || !moby->PVar)
    return;

  // default to power off
  if (!initialized) {

    // disable jump pad from setting jump height to 0 when deactivated
    POKE_U32(0x004257D8, 0);
    POKE_U32(0x00425930, 0);

    // disable pickup pad not being pickupable in CQ
    POKE_U32(0x00446638, 0);

    // 
    memset(TempPowerOnList, 0, sizeof(TempPowerOnList));

    //
    powerUpdateMobies(0);
    initialized = 1;
  }

  // get bolt crank moby from node pvars
  Moby* boltCrank = *(Moby**)((u32)moby->PVar + 0xC);
  if (!boltCrank || !boltCrank->PVar)
    return;

  // adjust crank speed by player speed
  Player** players = playerGetAll();
  int playerOnCrank = *(int*)(boltCrank->PVar + 0x10);
  if (playerOnCrank >= 0)
  {
    Player* p = players[playerOnCrank];
    baseTurnSpeed *= p->Speed;
  }

  // update crank speed
  u32 turnSpeedU32 = *(u32*)&baseTurnSpeed;
  POKE_U16(0x003D8B2C, (turnSpeedU32 >> 16));
  POKE_U16(0x003D8B30, turnSpeedU32);
  POKE_U16(0x003D8028, (turnSpeedU32 >> 16));
  POKE_U16(0x003D802C, turnSpeedU32);

  // get team from bolt crank pvars
  // if team is 10 then power is waiting to be activated
  // otherwise power is active and we want to turn it off after a period of time
  int *team = (int*)((u32)boltCrank->PVar + 4);
  if (*team == 10) {
    if (powerForcedOn && powerTimeOff >= 0) {
      powerUpdateMobies(1);
      Player* lp = playerGetFromSlot(0);
      if (lp)
        *team = lp->Team;
      else
        *team = 0;
    }
    else {
      powerTimeOff = -1;
    }
  } else if (powerTimeOff >= 0) {
    int timeSincePowerOn = gameGetTime() - powerTimeOff;
    if (timeSincePowerOn > 0) {
      powerForcedOn = 0;
      *team = 10;
      powerUpdateMobies(0);
      pushSnack(-1, "Power's out", 60);
    }
  } else {
    powerTimeOff = gameGetTime() + TIME_SECOND*90;
    powerUpdateMobies(1);
    pushSnack(-1, "Power activated!", 60);
  }

  powerDeactivateMobyTempPower();
}
