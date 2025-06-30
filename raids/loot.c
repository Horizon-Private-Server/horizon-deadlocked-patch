#include <string.h>
#include <libdl/stdio.h>
#include <libdl/game.h>
#include <libdl/collision.h>
#include <libdl/stdlib.h>
#include <libdl/color.h>
#include <libdl/moby.h>
#include <libdl/radar.h>
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
#include "include/tracker.h"
#include "config.h"
#include "common.h"

u32 lootRarityColors[RAIDS_ITEM_RARITY_COUNT] = {
  [RAIDS_ITEM_RARITY_COMMON] 0x80808080,
  [RAIDS_ITEM_RARITY_UNCOMMON] 0x80007000,
  [RAIDS_ITEM_RARITY_RARE] 0x80C01800,
  [RAIDS_ITEM_RARITY_LEGENDARY] 0x80F010F0,
  [RAIDS_ITEM_RARITY_MYTHIC] 0x801010F0,
};

void pushSnack(char * str, int ticksAlive, int localPlayerIdx);

//--------------------------------------------------------------------------
void lootSetStatePickedUp(Moby* moby)
{
  mobySetState(moby, 2, -1);
  ((void (*)(Moby*))0x0043b330)(moby);
  ((void (*)(Moby*))0x0043c088)(moby);

  // play pickup sound
  Player* player = playerGetFromSlot(0);
  if (player && player->PlayerMoby)
    mobyPlaySoundByClass(1, 0, player->PlayerMoby, MOBY_ID_WEAPON_PICKUP);
}

//--------------------------------------------------------------------------
void lootUpdate(Moby* moby)
{
  char buf[64];

  // fall
  char* hit = (char*)(moby->PVar + 0x5C);
  if (!*hit) {
    VECTOR vel = {0,0,-1*MATH_DT,0};
    VECTOR nextPos;
    vector_add(nextPos, vel, moby->Position);
    if (CollLine_Fix(moby->Position, nextPos, COLLISION_FLAG_IGNORE_DYNAMIC, moby, NULL)) {
      vector_copy(nextPos, CollLine_Fix_GetHitPosition());
      *hit = 1;
    }

    vector_subtract(vel, nextPos, moby->Position);
    vector_copy(moby->Position, nextPos);


  }

  // draw on radar
  int blipIdx = radarGetBlipIndex(moby);
  if (blipIdx >= 0) {
    RadarBlip* blip = radarGetBlips() + blipIdx;
    blip->X = moby->Position[0];
    blip->Y = moby->Position[1];
    blip->Life = 0x1F;
    blip->Type = 14;
    blip->Team = TEAM_BLUE;
  }

  // auto despawn after a period of time
  // or if the user opens their inventory
  // as in that case they will already see the item
  int timeCreated = *(int*)(moby->PVar + 0x50);
  if (moby->State != 2 && ((gameGetTime() - timeCreated) > TIME_MINUTE || State.MenuOpen == RAIDS_CUSTOM_MENU_INVENTORY)) {
    lootSetStatePickedUp(moby);
  }

  // check if ready to destroy / auto give
  if (moby->State == 2 && moby->StateTimer > 5) {
    RaidsInventoryItem_t item;
    memcpy(&item, moby->PVar + 0x60, sizeof(RaidsInventoryItem_t));

    if (item.Type) {
      char itemName[64];
      mapConfig->BankVTable->GetItemName(&item, itemName, sizeof(itemName));
      snprintf(buf, sizeof(buf), "Got %s", itemName);
      DPRINTF("pickup %d (gadget %d)\n", item.Type, item.WeaponData.GadgetId);
      pushSnack(buf, 60, 0);
    }

    // destroy gadget
    Moby* gadgetMoby = *(Moby**)(moby->PVar + 0x04);
    if (gadgetMoby && !mobyIsDestroyed(gadgetMoby))
      mobyDestroy(gadgetMoby);

    // destroy self
    mobyDestroy(moby);
    return;
  }

  // call base
  ((void (*)(Moby*))0x0043B418)(moby);
}

//--------------------------------------------------------------------------
Moby* lootSpawn(VECTOR position, RaidsInventoryItem_t* item)
{
  int pickupId = 3;
  int rarity = mapConfig->BankVTable->GetRarityFromQuality(item->Quality);

  if (item->Type == RAIDS_ITEM_WEAPON) {
    switch (item->WeaponData.GadgetId)
    {
      case WEAPON_ID_VIPERS: pickupId = 2; break;
      case WEAPON_ID_MAGMA_CANNON: pickupId = 3; break;
      case WEAPON_ID_ARBITER: pickupId = 4; break;
      case WEAPON_ID_FUSION_RIFLE: pickupId = 5; break;
      case WEAPON_ID_MINE_LAUNCHER: pickupId = 6; break;
      case WEAPON_ID_B6: pickupId = 7; break;
      case WEAPON_ID_FLAIL: pickupId = 12; break;
      case WEAPON_ID_OMNI_SHIELD: pickupId = 16; break;
    }
  } else {
    pickupId = 1;
  }


  Moby* moby = mobySpawn(0x243E, 0x50 + 0x10 + sizeof(RaidsInventoryItem_t));

  vector_copy(moby->Position, position);
  moby->PUpdate = lootUpdate;
  moby->UpdateDist = 255;
  moby->DrawDist = 128;
  *(int*)(moby->PVar + 0x00) = pickupId;
  *(int*)(moby->PVar + 0x08) = 0x0001FFFF;
  *(int*)(moby->PVar + 0x0C) = 0x1E;
  *(int*)(moby->PVar + 0x50) = gameGetTime();
  *(int*)(moby->PVar + 0x54) = colorLerp(0x80000000, lootRarityColors[rarity], 0.8);
  *(int*)(moby->PVar + 0x58) = colorLerp(0x80000000, lootRarityColors[rarity], 0.7);
  memcpy(moby->PVar + 0x60, item, sizeof(RaidsInventoryItem_t));

  DPRINTF("loot spawn %08X (type %d, gadget %d)\n", (u32)moby, item->Type, item->WeaponData.GadgetId);
  return moby;
}

//--------------------------------------------------------------------------
int lootOnGenerateLootResponse(void* connection, void* data)
{
  struct RaidsGenerateLootDropResponse msg;
  memcpy(&msg, data, sizeof(msg));

  lootSpawn(msg.Position, &msg.Drop);
  trackerLogLootDrop();

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
  msg.Type = LOOT_DROP_TYPE_MOB_DEATH;
  msg.KilledWithGadgetId = gadgetId;
  msg.DifficultyStars = State.DifficultyStars;
  msg.MobDamage = pvars->MobVars.Config.Damage;
  msg.MobSpeed = pvars->MobVars.Config.Speed;
  msg.MobHealth = pvars->MobVars.Config.Health;
  msg.MobMobyOClass = mob->OClass;
  msg.QuickSelectGadgets[0] = playerGetLocalEquipslot(0, 0);
  msg.QuickSelectGadgets[1] = playerGetLocalEquipslot(0, 1);
  msg.QuickSelectGadgets[2] = playerGetLocalEquipslot(0, 2);

  DPRINTF("sent loot gen request (MOB)\n");
  netSendCustomAppMessage(NET_DELIVERY_CRITICAL, connection, NET_LOBBY_CLIENT_INDEX, CUSTOM_MSG_ID_GENERATE_RAIDS_LOOT_REQUEST, sizeof(msg), &msg);
}

//--------------------------------------------------------------------------
void lootRequestFromPrestige(int gadgetId)
{
  struct RaidsGenerateLootDropRequest msg;
  void* connection = netGetLobbyServerConnection();
  if (!connection) return;

  memset(&msg, 0, sizeof(msg));
  vector_copy(msg.Position, playerGetFromSlot(0)->PlayerPosition);
  msg.Type = LOOT_DROP_TYPE_PRESTIGE;
  msg.KilledWithGadgetId = gadgetId;
  msg.DifficultyStars = State.DifficultyStars;
  memset(msg.QuickSelectGadgets, 0, sizeof(msg.QuickSelectGadgets));

  DPRINTF("sent loot gen request (PRESTIGE)\n");
  netSendCustomAppMessage(NET_DELIVERY_CRITICAL, connection, NET_LOBBY_CLIENT_INDEX, CUSTOM_MSG_ID_GENERATE_RAIDS_LOOT_REQUEST, sizeof(msg), &msg);
}

//--------------------------------------------------------------------------
void lootRequestFromMissionComplete(VECTOR position)
{
  struct RaidsGenerateLootDropRequest msg;
  void* connection = netGetLobbyServerConnection();
  if (!connection) return;

  vector_copy(msg.Position, position);
  msg.Type = LOOT_DROP_TYPE_MISSION_COMPLETE;
  msg.KilledWithGadgetId = 0;
  msg.DifficultyStars = State.DifficultyStars;
  msg.QuickSelectGadgets[0] = playerGetLocalEquipslot(0, 0);
  msg.QuickSelectGadgets[1] = playerGetLocalEquipslot(0, 1);
  msg.QuickSelectGadgets[2] = playerGetLocalEquipslot(0, 2);

  DPRINTF("sent loot gen request (COMPLETE)\n");
  netSendCustomAppMessage(NET_DELIVERY_CRITICAL, connection, NET_LOBBY_CLIENT_INDEX, CUSTOM_MSG_ID_GENERATE_RAIDS_LOOT_REQUEST, sizeof(msg), &msg);
}

//--------------------------------------------------------------------------
void lootTick(void)
{
#if DEBUG && 0
  if (padGetButtonDown(0, PAD_L1) > 0) {
    struct RaidsGenerateLootDropRequest msg;
    void* connection = netGetLobbyServerConnection();
    if (!connection) return;

    VECTOR offset={0,5,0,0};
    vector_add(msg.Position, playerGetFromSlot(0)->PlayerPosition, offset);
    msg.Type = LOOT_DROP_TYPE_MOB_DEATH;
    msg.KilledWithGadgetId = weaponSlotToId(1 + rand(8));
    msg.DifficultyStars = State.DifficultyStars;

    DPRINTF("sent loot gen request (MOB)\n");
    netSendCustomAppMessage(NET_DELIVERY_CRITICAL, connection, NET_LOBBY_CLIENT_INDEX, CUSTOM_MSG_ID_GENERATE_RAIDS_LOOT_REQUEST, sizeof(msg), &msg);
  }
#endif
}

//--------------------------------------------------------------------------
void lootInit(void)
{
  netInstallCustomMsgHandler(CUSTOM_MSG_ID_GENERATE_RAIDS_LOOT_RESPONSE, &lootOnGenerateLootResponse);

  // pull aura color from pvars
  POKE_U32(0x0043BF28, 0x8E440054);
  POKE_U32(0x0043BF30, 0x8E450058);

  // allow gadget pickup even if maxed ammo
  POKE_U32(0x0043A824, 0);

  // increase pickup distance (sqrt8)
  POKE_U16(0x0043B690, 0x4100);
}
