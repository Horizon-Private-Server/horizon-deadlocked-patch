#include <string.h>
#include <libdl/stdio.h>
#include <libdl/game.h>
#include <libdl/collision.h>
#include <libdl/stdlib.h>
#include <libdl/color.h>
#include <libdl/moby.h>
#include <libdl/sound.h>
#include <libdl/random.h>
#include <libdl/utils.h>
#include <libdl/net.h>
#include <libdl/ui.h>
#include <libdl/graphics.h>
#include "include/utils.h"
#include "include/game.h"
#include "include/bank.h"
#include "include/mob.h"
#include "include/loot.h"
#include "config.h"
#include "common.h"

extern struct RaidsState State;
extern struct RaidsMapConfig* mapConfig;

u32 lootRarityColors[RAIDS_ITEM_RARITY_COUNT] = {
  [RAIDS_ITEM_RARITY_COMMON] 0x80808080,
  [RAIDS_ITEM_RARITY_UNCOMMON] 0x80007000,
  [RAIDS_ITEM_RARITY_RARE] 0x80C01800,
  [RAIDS_ITEM_RARITY_LEGENDARY] 0x80F010F0,
  [RAIDS_ITEM_RARITY_MYTHIC] 0x801010F0,
};

void pushSnack(char * str, int ticksAlive, int localPlayerIdx);

void lootUpdate(Moby* moby)
{
  char buf[64];

  // check if ready to destroy / auto give
  if (moby->State == 2 && moby->StateTimer > 5) {
    int gadgetId = *(int*)(moby->PVar + 0x50);
    int rarity = *(char*)(moby->PVar + 0x54);
    int quality = *(char*)(moby->PVar + 0x55);
    int proficiency = *(char*)(moby->PVar + 0x56);
    if (gadgetId) {
      char rarityCode[] = { '\x08', '\x0A', '\x09', '\x0B', '\x0E' };
      char itemName[64];
      RaidsInventoryItem_t item = { .GadgetId = gadgetId, .Quality = quality, .Proficiency = proficiency };
      bankGetItemName(&item, itemName, sizeof(itemName));
      snprintf(buf, sizeof(buf), "Got %c%s", rarityCode[rarity], itemName);
      pushSnack(buf, 60, 0);
    }

    Moby* gadgetMoby = *(Moby**)(moby->PVar + 0x04);
    if (gadgetMoby && !mobyIsDestroyed(gadgetMoby))
      mobyDestroy(gadgetMoby);

    mobyDestroy(moby);
    return;
  }

  // call base
  ((void (*)(Moby*))0x0043B418)(moby);
}

//--------------------------------------------------------------------------
Moby* lootSpawn(VECTOR position, int gadgetId, int quality, int proficiency)
{
  int pickupId = 3;
  int rarity = bankGetRarityFromQuality(quality);

  switch (gadgetId)
  {
    case WEAPON_ID_VIPERS: pickupId = 2; break;
    case WEAPON_ID_MAGMA_CANNON: pickupId = 3; break;
    case WEAPON_ID_ARBITER: pickupId = 4; break;
    case WEAPON_ID_FUSION_RIFLE: pickupId = 5; break;
    case WEAPON_ID_MINE_LAUNCHER: pickupId = 6; break;
    case WEAPON_ID_B6: pickupId = 7; break;
    case WEAPON_ID_FLAIL: pickupId = 12; break;
    case WEAPON_ID_OMNI_SHIELD: pickupId = 16; break;
    case BANK_BADGE_GADGET_ID: pickupId = 1; break;
  }

  Moby* moby = mobySpawn(0x243E, 0x50 + 0x20);

  vector_copy(moby->Position, position);
  moby->PUpdate = lootUpdate;
  moby->UpdateDist = 255;
  moby->DrawDist = 128;
  *(int*)(moby->PVar + 0x00) = pickupId;
  *(int*)(moby->PVar + 0x08) = 0x0001FFFF;
  *(int*)(moby->PVar + 0x0C) = 0x1E;
  *(int*)(moby->PVar + 0x50) = gadgetId;
  *(char*)(moby->PVar + 0x54) = rarity;
  *(char*)(moby->PVar + 0x55) = quality;
  *(char*)(moby->PVar + 0x56) = proficiency;
  *(int*)(moby->PVar + 0x58) = gameGetTime();
  *(int*)(moby->PVar + 0x60) = colorLerp(0x80000000, lootRarityColors[rarity], 0.8);
  *(int*)(moby->PVar + 0x64) = colorLerp(0x80000000, lootRarityColors[rarity], 0.7);

  DPRINTF("loot spawn %08X\n", (u32)moby);
  return moby;
}

//--------------------------------------------------------------------------
int lootOnGenerateLootResponse(void* connection, void* data)
{
  struct RaidsGenerateLootDropResponse msg;
  memcpy(&msg, data, sizeof(msg));

  lootSpawn(msg.Position, msg.Drop.GadgetId, msg.Drop.Quality, msg.Drop.Proficiency);

  DPRINTF("got loot gen response\n");
  return sizeof(msg);
}

//--------------------------------------------------------------------------
void lootRequestFromMob(Moby* mob, int gadgetId)
{
  struct RaidsGenerateLootDropRequest msg;
  void* connection = netGetLobbyServerConnection();
  if (!connection) return;
  if (!mob || !mob->PVar) return;

  struct MobPVar* pvars = (struct MobPVar*)mob->PVar;
  vector_copy(msg.Position, mob->Position);
  msg.Type = 0;
  msg.KilledWithGadgetId = gadgetId;
  msg.MobDamage = pvars->MobVars.Config.Damage;
  msg.MobSpeed = pvars->MobVars.Config.Speed;
  msg.MobHealth = pvars->MobVars.Config.Health;
  msg.MobDifficulty = State.Difficulty;
  msg.MobMobyOClass = mob->OClass;

  DPRINTF("sent loot gen request (MOB)\n");
  netSendCustomAppMessage(NET_DELIVERY_CRITICAL, connection, NET_LOBBY_CLIENT_INDEX, CUSTOM_MSG_ID_GENERATE_RAIDS_LOOT_REQUEST, sizeof(msg), &msg);
}

//--------------------------------------------------------------------------
void lootRequestFromMissionComplete(VECTOR position)
{
  struct RaidsGenerateLootDropRequest msg;
  void* connection = netGetLobbyServerConnection();
  if (!connection) return;

  vector_copy(msg.Position, position);
  msg.Type = 1;
  msg.KilledWithGadgetId = 0;

  DPRINTF("sent loot gen request (COMPLETE)\n");
  netSendCustomAppMessage(NET_DELIVERY_CRITICAL, connection, NET_LOBBY_CLIENT_INDEX, CUSTOM_MSG_ID_GENERATE_RAIDS_LOOT_REQUEST, sizeof(msg), &msg);
}

//--------------------------------------------------------------------------
void lootTick(void)
{
  
}

//--------------------------------------------------------------------------
void lootInit(void)
{
  netInstallCustomMsgHandler(CUSTOM_MSG_ID_GENERATE_RAIDS_LOOT_RESPONSE, &lootOnGenerateLootResponse);

  // pull aura color from pvars
  POKE_U32(0x0043BF28, 0x8E440060);
  POKE_U32(0x0043BF30, 0x8E450064);

  // allow gadget pickup even if maxed ammo
  POKE_U32(0x0043A824, 0);

  // increase pickup distance (sqrt8)
  POKE_U16(0x0043B690, 0x4100);
}
