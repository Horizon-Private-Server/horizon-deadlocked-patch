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
#include "include/inventory.h"
#include "include/bubble.h"
#include "config.h"
#include "common.h"

extern struct RaidsState State;

// send as binary payload from server
RaidsPlayerBank_t inventoryLocalBank __attribute__((section(".config"))) = {};

u32 inventoryPaintColors[] = {
  0x00FFFFFF,         // white
  0x00FF6000,         // blue
  0x000000FF,         // red
  0x0006FF00,         // green
  0x00008AFF,         // orange
  0x0000EAFF,         // yellow
  0x00FF00E4,         // purple
  0x00F0FF00,         // aqua
  0x00C674FF,         // pink
  0x0000FF9C,         // olive
  0x006000FF,         // maroon
};

char inventoryLevelUpBuf[32];

//--------------------------------------------------------------------------
u32 inventoryGetBolts(void) { return inventoryLocalBank.Account.Bolts; }
u32 inventoryAddBolts(u32 amount) { return inventoryLocalBank.Account.Bolts += amount; }
u64 inventoryGetXP(void) { return inventoryLocalBank.Account.Experience; }
u64 inventoryAddXP(u64 amount)
{
  u64 xp = inventoryLocalBank.Account.Experience;

  int level = getLevelFromXp(xp);
  int nextLevel = getLevelFromXp(xp + amount);
  if (nextLevel > level) {
    inventoryLocalBank.Account.SkillPoints += 1;

    snprintf(inventoryLevelUpBuf, sizeof(inventoryLevelUpBuf), "You have reached level %d", nextLevel + 1);
    uiShowPopup(0, inventoryLevelUpBuf);
  }

  return inventoryLocalBank.Account.Experience += amount;
}

//--------------------------------------------------------------------------
void inventoryRequestFromServer(void)
{
  void* connection = netGetLobbyServerConnection();
  if (!connection) return;

  struct RaidsGetBankRequest msg = { .DestAddress = &inventoryLocalBank };
  netSendCustomAppMessage(NET_DELIVERY_CRITICAL, connection, NET_LOBBY_CLIENT_INDEX, CUSTOM_MSG_ID_GET_RAIDS_BANK_REQUEST, sizeof(msg), &msg);
}

//--------------------------------------------------------------------------
void inventorySendToServer(void)
{
  struct RaidsUpdateBankInventoryRequest msg = {  };

  RaidsPlayerBank_t* localBank = inventoryGetLocalBank();
  if (!localBank) return;
  void* connection = netGetLobbyServerConnection();
  if (!connection) return;

  int i;
  for (i = 0; i < INVENTORY_BANK_MAX_WEAPONS; i += INVENTORY_BANK_UPDATE_WEAPONS_SIZE) {
    msg.Index = i;
    msg.Count = (INVENTORY_BANK_MAX_WEAPONS - i);
    if (msg.Count > INVENTORY_BANK_UPDATE_WEAPONS_SIZE) msg.Count = INVENTORY_BANK_UPDATE_WEAPONS_SIZE;

    memcpy(msg.EquippedWeaponIdxs, localBank->Inventory.EquippedWeaponIdxs, sizeof(msg.EquippedWeaponIdxs));
    memcpy(msg.Weapons, &localBank->Inventory.Weapons[i], sizeof(RaidsInventoryWeapon_t)*msg.Count);
    netSendCustomAppMessage(NET_DELIVERY_CRITICAL, connection, NET_LOBBY_CLIENT_INDEX, CUSTOM_MSG_ID_UPDATE_RAIDS_BANK_INVENTORY_REQUEST, sizeof(msg), &msg);
  }
}

//--------------------------------------------------------------------------
RaidsPlayerBank_t* inventoryGetLocalBank(void)
{
  return &inventoryLocalBank;
}

//--------------------------------------------------------------------------
int inventoryGetEquipSlotFromGadgetId(int gadgetId)
{
  return weaponIdToSlot(gadgetId) - 1;
}

//--------------------------------------------------------------------------
enum RaidsWeaponRarity inventoryGetRarityFromQuality(u8 quality)
{
  if (quality < 64) return RAIDS_WEAPON_RARITY_COMMON;
  if (quality < 128) return RAIDS_WEAPON_RARITY_UNCOMMON;
  if (quality < 196) return RAIDS_WEAPON_RARITY_RARE;
  return RAIDS_WEAPON_RARITY_LEGENDARY;
}

//--------------------------------------------------------------------------
RaidsPlayerEquippedInventory_t* inventoryGetEquippedFromGadgetBox(GadgetBox* gbox)
{
  if (!gbox) return NULL;
  if (gbox->Initialized <= 0) return NULL;

  return &State.PlayerStates[(int)gbox->Initialized - 1].Inventory;
}

//--------------------------------------------------------------------------
RaidsInventoryWeapon_t* inventoryGetEquippedWeaponFromGadgetBox(GadgetBox* gbox, int gadgetId)
{
  RaidsPlayerEquippedInventory_t* inventory = inventoryGetEquippedFromGadgetBox(gbox);
  if (!inventory) return NULL;
  if (gadgetId <= 0) return NULL;

  int idx = weaponIdToSlot(gadgetId)-1;
  if (idx < 0 || idx >= COUNT_OF(inventory->Weapons)) return NULL;
  if (inventory->Weapons[idx].GadgetId != gadgetId) return NULL;

  return &inventory->Weapons[idx];
}

//--------------------------------------------------------------------------
RaidsInventoryWeapon_t* inventoryGetLocalEquippedWeapon(int gadgetId)
{
  int slotId = inventoryGetEquipSlotFromGadgetId(gadgetId);
  if (slotId < 0) return NULL;

  int equipIdx = inventoryLocalBank.Inventory.EquippedWeaponIdxs[slotId];
  if (equipIdx < 0) return NULL;
  RaidsInventoryWeapon_t* weapon = &inventoryLocalBank.Inventory.Weapons[equipIdx];

  if (weapon->GadgetId != gadgetId) return NULL;
  return weapon;
}

//--------------------------------------------------------------------------
RaidsInventoryWeapon_t* inventoryGetLocalWeaponFromBank(int index)
{
  if (index < 0) return NULL;
  if (index >= INVENTORY_BANK_MAX_WEAPONS) return NULL;

  RaidsInventoryWeapon_t* weapon = &inventoryLocalBank.Inventory.Weapons[index];
  if (!weapon->GadgetId) return NULL;

  return weapon;
}

//--------------------------------------------------------------------------
int inventoryGetAlphaModCount(GadgetBox* gadgetBox, int gadgetId, int alphaModId)
{
  if (!gadgetBox || !gadgetBox->Initialized) return 0;
  if (alphaModId > 8) return 0;
  if (alphaModId <= 0) return 0;

  RaidsInventoryWeapon_t* inventoryWeapon = inventoryGetEquippedWeaponFromGadgetBox(gadgetBox, gadgetId);
  if (!inventoryWeapon) return 0;

  return inventoryWeapon->AlphaModCounts[alphaModId-1];
}

//--------------------------------------------------------------------------
float inventoryGetArbiterSpeed(Player* player)
{
  RaidsInventoryWeapon_t* inventoryWeapon = inventoryGetEquippedWeaponFromGadgetBox(player->GadgetBox, WEAPON_ID_ARBITER);
  if (!inventoryWeapon) return 0;

  return inventoryWeapon->Speed;
}

//--------------------------------------------------------------------------
float inventoryGetGadgetDamage(GadgetBox* gbox, int gadgetId, int damageType, int multiplier)
{
  RaidsInventoryWeapon_t* inventoryWeapon = inventoryGetEquippedWeaponFromGadgetBox(gbox, gadgetId);
  if (inventoryWeapon) return inventoryWeapon->Damage * multiplier;

  int level = gbox->Gadgets[gadgetId].Level;
  return ((float (*)(int gadgetId, int level, int damageType, int multiplier))0x00627520)(gadgetId, level, damageType, multiplier);
}

//--------------------------------------------------------------------------
int inventoryTryAddGadgetToQuickSelect(Player* player, int gadgetId)
{
  int i;

  if (!player || !player->IsLocal) return -1;

  int freeSlot = -1;
  for (i = 0; i < 3; ++i) {
    int gId = playerGetLocalEquipslot(player->LocalPlayerIndex, i);
    if (gId == gadgetId) return i;
    if (gId == 0 && freeSlot < 0) freeSlot = i;
  }

  if (freeSlot >= 0) {
    playerSetLocalEquipslot(player->LocalPlayerIndex, freeSlot, gadgetId);
    if (freeSlot == 0) playerEquipWeapon(player, gadgetId);
  }

  return freeSlot;
}

//--------------------------------------------------------------------------
void inventoryApplyGadgetMoby(Player* player, RaidsInventoryWeapon_t* item, Moby* moby)
{
  u32 glowAlpha = 0xFF000000;
  float glowOpacity = 0.5;
  if (!moby) return;

  if (item->Paint) {
    moby->PrimaryColor = inventoryPaintColors[item->Paint];
  } else {
    moby->PrimaryColor = player->PlayerMoby->PrimaryColor;
    glowOpacity = 1;
  }

  if (item->PaintSpecialMask & RAIDS_GADGET_PAINTSPECIAL_ADDITIVE) {
    moby->Opacity = 0x50;
    moby->ModeBits |= MOBY_MODE_BIT_DRAW_TRANSPARENT_WEIRD;
    glowAlpha = 0x80000000;
    glowOpacity *= 0.5;
  } else {
    moby->Opacity = 0x80;
    moby->ModeBits &= ~MOBY_MODE_BIT_DRAW_TRANSPARENT_WEIRD;
  }
  
  if (item->PaintSpecialMask & RAIDS_GADGET_PAINTSPECIAL_GLOW) {
    moby->PrimaryColor = colorLerp(0, moby->PrimaryColor, glowOpacity) | glowAlpha;
    moby->Lights = 0;
    //moby->ModeBits2 |= 8;
  } else {
    moby->Lights = player->PlayerMoby->Lights;
    //moby->ModeBits2 &= ~8;
  }
}

//--------------------------------------------------------------------------
void inventoryRemoveGadget(Player* player, int gadgetId)
{
  player->GadgetBox->Gadgets[gadgetId].Level = -1;

  if (player->IsLocal) {
    if (playerGetLocalEquipslot(player->LocalPlayerIndex, 0) == gadgetId) {
      playerSetLocalEquipslot(player->LocalPlayerIndex, 0, playerGetLocalEquipslot(player->LocalPlayerIndex, 1));
      playerSetLocalEquipslot(player->LocalPlayerIndex, 1, playerGetLocalEquipslot(player->LocalPlayerIndex, 2));
      playerSetLocalEquipslot(player->LocalPlayerIndex, 2, 0);
    } else if (playerGetLocalEquipslot(player->LocalPlayerIndex, 1) == gadgetId) {
      playerSetLocalEquipslot(player->LocalPlayerIndex, 1, playerGetLocalEquipslot(player->LocalPlayerIndex, 2));
      playerSetLocalEquipslot(player->LocalPlayerIndex, 2, 0);
    } else if (playerGetLocalEquipslot(player->LocalPlayerIndex, 2) == gadgetId) {
      playerSetLocalEquipslot(player->LocalPlayerIndex, 2, 0);
    }
  }
}

//--------------------------------------------------------------------------
void inventoryApplyItem(Player* player, RaidsInventoryWeapon_t* item)
{
  int gadgetId = item->GadgetId;
  GadgetBox* gbox = player->GadgetBox;

  // give
  if (gbox->Gadgets[gadgetId].Level < 0) {
    playerGiveWeapon(gbox, gadgetId, 0, 1);
    inventoryTryAddGadgetToQuickSelect(player, gadgetId);
  }
  gbox->Gadgets[gadgetId].Level = item->Proficiency;

  // configure mobys
  if (player->Gadgets[0].id == gadgetId) {
    inventoryApplyGadgetMoby(player, item, player->Gadgets[0].pMoby);
    inventoryApplyGadgetMoby(player, item, player->Gadgets[0].pMoby2);
  }

  gbox->Gadgets[gadgetId].OmegaMod = item->OmegaMod;
}

//--------------------------------------------------------------------------
void inventoryUpdateLocalState(Player * player)
{
  if (!player || !player->PlayerMoby || !player->pNetPlayer || !player->IsLocal) return;
  if (!playerIsConnected(player)) return;

  int playerId = player->PlayerId;
  RaidsPlayerBank_t* localBank = inventoryGetLocalBank();

  // save skill points
  memcpy(State.PlayerStates[playerId].State.Skills, localBank->Account.Skills, sizeof(State.PlayerStates[playerId].State.Skills));

  // save to equipped
  int i;
  for (i = 0; i < WEAPON_SLOT_OMNI_SHIELD; ++i) {
    int equippedIdx = localBank->Inventory.EquippedWeaponIdxs[i];
    if (equippedIdx < 0) {
      memset(&State.PlayerStates[playerId].Inventory.Weapons[i], 0, sizeof(RaidsInventoryWeapon_t));
    } else {
      memcpy(&State.PlayerStates[playerId].Inventory.Weapons[i], &localBank->Inventory.Weapons[equippedIdx], sizeof(RaidsInventoryWeapon_t));
    }
  }
}

//--------------------------------------------------------------------------
void inventorySellLocalWeaponAtIndex(int weaponIdx)
{
  RaidsPlayerBank_t* localBank = inventoryGetLocalBank();
  if (!localBank) return;

  if (weaponIdx < 0 || weaponIdx >= INVENTORY_BANK_MAX_WEAPONS) return;
  RaidsInventoryWeapon_t* weapon = &localBank->Inventory.Weapons[weaponIdx];
  if (!weapon || !weapon->GadgetId) return;

  int equipSlot = inventoryGetEquipSlotFromGadgetId(weapon->GadgetId);
  int isEquipped = localBank->Inventory.EquippedWeaponIdxs[equipSlot] == weaponIdx;
  localBank->Account.Bolts += getPriceForWeapon(weapon->Proficiency, weapon->Quality);
  memset(weapon, 0, sizeof(RaidsInventoryWeapon_t));
  if (isEquipped) localBank->Inventory.EquippedWeaponIdxs[equipSlot] = -1;
  localBank->RefreshLocalInventory = 1;
}

//--------------------------------------------------------------------------
void inventoryEquipLocalWeaponAtIndex(int weaponIdx)
{
  RaidsPlayerBank_t* localBank = inventoryGetLocalBank();
  if (!localBank) return;

  if (weaponIdx < 0 || weaponIdx >= INVENTORY_BANK_MAX_WEAPONS) return;
  RaidsInventoryWeapon_t* weapon = &localBank->Inventory.Weapons[weaponIdx];
  if (!weapon || !weapon->GadgetId) return;

  localBank->Inventory.EquippedWeaponIdxs[inventoryGetEquipSlotFromGadgetId(weapon->GadgetId)] = weaponIdx;
  localBank->RefreshLocalInventory = 1;
}

//--------------------------------------------------------------------------
void inventoryTickPlayer(Player * player)
{
  if (!player || !player->PlayerMoby || !player->pNetPlayer) return;
  if (!playerIsConnected(player)) return;

  RaidsPlayerBank_t* localBank = inventoryGetLocalBank();
  GadgetBox* gbox = player->GadgetBox;
  if (!gbox) return;

  // apply items
  int i;
  for (i = 0; i < WEAPON_SLOT_OMNI_SHIELD; ++i) {
    int gadgetId = weaponSlotToId(i+1);
    RaidsInventoryWeapon_t* weapon = inventoryGetEquippedWeaponFromGadgetBox(gbox, gadgetId);
    if (weapon) {
      inventoryApplyItem(player, weapon);
    } else if (gbox->Gadgets[gadgetId].Level >= 0)  {
      inventoryRemoveGadget(player, gadgetId);
    }
  }
}

//--------------------------------------------------------------------------
void inventoryTick(void)
{
  static int hasBank = 0;
  int i;

  // check if we need to request our bank
  if (!isInGame()) {
    hasBank = 0;
    return;
  } else if (!hasBank) {
    inventoryRequestFromServer();
    hasBank = 1;
  }

  // handle new inventory change
  if (inventoryLocalBank.RefreshLocalInventory) {
    for (i = 0; i < GAME_MAX_LOCALS; ++i) {
      inventoryUpdateLocalState(playerGetFromSlot(i));
    }
  }

  // process players
  Player** players = playerGetAll();
  for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
    inventoryTickPlayer(players[i]);
  }
}

//--------------------------------------------------------------------------
void inventoryInit(void)
{
  int i;
  static int initialized = 0;

  if (!initialized) {

    // hooks
    POKE_U32(0x005DDF98, 0); // disable player ambient color affecting child mobys
    POKE_U8(0x00171b66, 1); // challenge mode
    HOOK_J_OP(0x00627600, &inventoryGetGadgetDamage, 0);
    HOOK_JAL(0x003F29AC, &inventoryGetArbiterSpeed);
    POKE_U32(0x003F2984, 0x0240202D);
    HOOK_J(0x006299A8, &inventoryGetAlphaModCount);
    //HOOK_J_OP(0x00626d98, &inventoryGetGadgetMaxLevel, 0);
    //HOOK_J_OP(0x00626fb8, &inventoryGetGadgetMaxAmmo, 0);
    //HOOK_JAL_OP(0x0060f780, &inventoryGetGadgetRefireRate, 0x0200282D);

    // clear inventory
    Player** players = playerGetAll();
    for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
      if (players[i] && players[i]->GadgetBox) {
        playerStripWeapons(players[i]);
        playerGiveWeapon(players[i]->GadgetBox, 17, 0, 0); // give cboots
        players[i]->GadgetBox->Initialized = i+1;
      }
    }

#if DEBUG1
    GadgetBox* gbox = playerGetFromSlot(0)->GadgetBox;
    playerGiveWeapon(gbox, WEAPON_ID_VIPERS, 97, 1);
    playerGiveWeapon(gbox, WEAPON_ID_MAGMA_CANNON, 9, 1);
    playerGiveWeapon(gbox, WEAPON_ID_ARBITER, 98, 1);
    playerGiveWeapon(gbox, WEAPON_ID_B6, 9, 1);
    playerGiveWeapon(gbox, WEAPON_ID_MINE_LAUNCHER, 9, 1);
    playerGiveWeapon(gbox, WEAPON_ID_FUSION_RIFLE, 9, 1);
    playerGiveWeapon(gbox, WEAPON_ID_OMNI_SHIELD, 9, 1);
    playerGiveWeapon(gbox, WEAPON_ID_FLAIL, 9, 1);
#endif

    initialized = 1;
  }

  inventoryTick();
}
