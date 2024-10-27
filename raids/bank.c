#include <string.h>
#include <libdl/stdio.h>
#include <libdl/game.h>
#include <libdl/collision.h>
#include <libdl/stdlib.h>
#include <libdl/color.h>
#include <libdl/moby.h>
#include <libdl/sound.h>
#include <libdl/random.h>
#include <libdl/hud.h>
#include <libdl/utils.h>
#include <libdl/net.h>
#include <libdl/ui.h>
#include <libdl/graphics.h>
#include "include/utils.h"
#include "include/game.h"
#include "include/bank.h"
#include "include/bubble.h"
#include "config.h"
#include "common.h"

extern struct RaidsState State;

// send as binary payload from server
RaidsPlayerBank_t bankLocalBank __attribute__((section(".config"))) = {};

u32 bankRarityColors[] = {
  [RAIDS_WEAPON_RARITY_COMMON] 0x80D0D0D0,
  [RAIDS_WEAPON_RARITY_UNCOMMON] 0x80000000,
  [RAIDS_WEAPON_RARITY_RARE] 0x80000000,
  [RAIDS_WEAPON_RARITY_LEGENDARY] 0x80000000,
};

u32 bankPaintColors[] = {
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

int bankHasInventory = 0;
int bankHasAccount = 0;
long bankLastInventoryRequestTime = 0;
long bankLastAccountRequestTime = 0;
char bankLevelUpBuf[32];

//--------------------------------------------------------------------------
int bankOnSetPlayerEquippedInventoryRemote(void * connection, void * data)
{
  struct RaidsBankSetPlayerEquippedInventoryMsg msg;
  memcpy(&msg, data, sizeof(msg));

  GameSettings* gs = gameGetSettings();
  if (!gs) return sizeof(msg);

  int i;
  for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
    if (!gs->PlayerClients[i] == msg.ClientId) continue;

    memcpy(&State.PlayerStates[i].Inventory, &msg.EquippedInventory, sizeof(State.PlayerStates[i].Inventory));
  }

  return sizeof(msg);
}

//--------------------------------------------------------------------------
int bankOnSetPlayerAccountRemote(void * connection, void * data)
{
  struct RaidsBankSetPlayerAccountMsg msg;
  memcpy(&msg, data, sizeof(msg));

  GameSettings* gs = gameGetSettings();
  if (!gs) return sizeof(msg);

  int i;
  for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
    if (!gs->PlayerClients[i] == msg.ClientId) continue;

    memcpy(State.PlayerStates[i].State.Skills, msg.Account.Skills, sizeof(State.PlayerStates[i].State.Skills));
  }

  return sizeof(msg);
}

//--------------------------------------------------------------------------
void bankBroadcastEquippedInventory(void)
{
  Player* player = playerGetFromSlot(0);
  if (!player) return;

  int playerId = player->PlayerId;

  // broadcast
  void* connection = netGetDmeServerConnection();
  if (connection) {
    struct RaidsBankSetPlayerEquippedInventoryMsg msg = {
      .ClientId = gameGetMyClientId()
    };
    memcpy(&msg.EquippedInventory, &State.PlayerStates[playerId].Inventory, sizeof(msg.EquippedInventory));
    netBroadcastCustomAppMessage(NET_DELIVERY_CRITICAL, connection, CUSTOM_MSG_SET_PLAYER_EQUIPPED_INVENTORY, sizeof(msg), &msg);
  }
}

//--------------------------------------------------------------------------
void bankBroadcastAccount(void)
{
  // broadcast
  void* connection = netGetDmeServerConnection();
  if (connection) {
    struct RaidsBankSetPlayerAccountMsg msg = {
      .ClientId = gameGetMyClientId()
    };
    memcpy(&msg.Account, &bankLocalBank.Account, sizeof(msg.Account));
    netBroadcastCustomAppMessage(NET_DELIVERY_CRITICAL, connection, CUSTOM_MSG_SET_PLAYER_ACCOUNT, sizeof(msg), &msg);
  }
}

//--------------------------------------------------------------------------
int bankGetHasInventory(void)
{
  return bankHasInventory;
}

//--------------------------------------------------------------------------
int bankHasPendingInventoryRequest(void)
{
  long dtMs = (timerGetSystemTime() - bankLastInventoryRequestTime) / SYSTEM_TIME_TICKS_PER_MS;
  return bankLastInventoryRequestTime && dtMs < (2*TIME_SECOND);
}

//--------------------------------------------------------------------------
int bankGetHasAccount(void)
{
  return bankHasAccount;
}

//--------------------------------------------------------------------------
int bankHasPendingAccountRequest(void)
{
  long dtMs = (timerGetSystemTime() - bankLastAccountRequestTime) / SYSTEM_TIME_TICKS_PER_MS;
  return bankLastAccountRequestTime && dtMs < (2*TIME_SECOND);
}

//--------------------------------------------------------------------------
void bankRequestInventoryFromServer(void)
{
  void* connection = netGetLobbyServerConnection();
  if (!connection) return;

  bankLastInventoryRequestTime = timerGetSystemTime();
  bankHasInventory = 0;
  struct RaidsGetBankRequest msg = {
    .DestAddress = (u32)&bankLocalBank.Inventory,
    .DestHasFlagAddress = (u32)&bankHasInventory,
    .DestTimeFlagAddress = (u32)&bankLastInventoryRequestTime
  };
  netSendCustomAppMessage(NET_DELIVERY_CRITICAL, connection, NET_LOBBY_CLIENT_INDEX, CUSTOM_MSG_ID_GET_RAIDS_BANK_INVENTORY_REQUEST, sizeof(msg), &msg);
  DPRINTF("request inventory\n");
}

//--------------------------------------------------------------------------
void bankSendInventoryToServer(void)
{
  struct RaidsUpdateBankInventoryRequest msg = {  };

  RaidsPlayerBank_t* localBank = bankGetLocalBank();
  if (!localBank) return;
  void* connection = netGetLobbyServerConnection();
  if (!connection) return;
  if (!bankHasInventory) return;

  int i;
  for (i = 0; i < BANK_MAX_WEAPONS; i += BANK_UPDATE_WEAPONS_SIZE) {
    msg.Index = i;
    msg.Count = (BANK_MAX_WEAPONS - i);
    if (msg.Count > BANK_UPDATE_WEAPONS_SIZE) msg.Count = BANK_UPDATE_WEAPONS_SIZE;

    memcpy(msg.EquippedWeaponIdxs, localBank->Inventory.EquippedWeaponIdxs, sizeof(msg.EquippedWeaponIdxs));
    memcpy(msg.Weapons, &localBank->Inventory.Weapons[i], sizeof(RaidsInventoryWeapon_t)*msg.Count);
    netSendCustomAppMessage(NET_DELIVERY_CRITICAL, connection, NET_LOBBY_CLIENT_INDEX, CUSTOM_MSG_ID_UPDATE_RAIDS_BANK_INVENTORY_REQUEST, sizeof(msg), &msg);
  }
  DPRINTF("sent inventory\n");
}

//--------------------------------------------------------------------------
void bankRequestAccountFromServer(void)
{
  void* connection = netGetLobbyServerConnection();
  if (!connection) return;

  bankLastAccountRequestTime = timerGetSystemTime();
  bankHasAccount = 0;
  struct RaidsGetBankRequest msg = {
    .DestAddress = (u32)&bankLocalBank.Account,
    .DestHasFlagAddress = (u32)&bankHasAccount,
    .DestTimeFlagAddress = (u32)&bankLastAccountRequestTime
  };
  netSendCustomAppMessage(NET_DELIVERY_CRITICAL, connection, NET_LOBBY_CLIENT_INDEX, CUSTOM_MSG_ID_GET_RAIDS_BANK_ACCOUNT_REQUEST, sizeof(msg), &msg);
  DPRINTF("request account\n");
}

//--------------------------------------------------------------------------
void bankSendAccountToServer(void)
{
  RaidsPlayerBank_t* localBank = bankGetLocalBank();
  if (!localBank) return;
  void* connection = netGetLobbyServerConnection();
  if (!connection) return;
  if (!bankHasAccount) return;
  
  netSendCustomAppMessage(NET_DELIVERY_CRITICAL, connection, NET_LOBBY_CLIENT_INDEX, CUSTOM_MSG_ID_UPDATE_RAIDS_BANK_ACCOUNT_REQUEST, sizeof(localBank->Account), &localBank->Account);
  DPRINTF("sent account\n");
}

//--------------------------------------------------------------------------
u32 bankGetBolts(void) { return bankLocalBank.Account.Bolts; }
u32 bankAddBolts(u32 amount) { return bankLocalBank.Account.Bolts += amount; }
u32 bankSubtractBolts(u32 amount)
{
  if (amount >= bankLocalBank.Account.Bolts) bankLocalBank.Account.Bolts = 0;
  else bankLocalBank.Account.Bolts -= amount;

  return amount;
}

u64 bankGetXP(void) { return bankLocalBank.Account.Experience; }
u64 bankAddXP(u64 amount)
{
  u64 xp = bankLocalBank.Account.Experience;

  int level = getLevelFromXp(xp);
  int nextLevel = getLevelFromXp(xp + amount);
  if (nextLevel > level) {
    bankLocalBank.Account.SkillPoints += 1;

    snprintf(bankLevelUpBuf, sizeof(bankLevelUpBuf), "You have reached level %d", nextLevel + 1);
    uiShowPopup(0, bankLevelUpBuf);
  }

  return bankLocalBank.Account.Experience += amount;
}

//--------------------------------------------------------------------------
RaidsPlayerBank_t* bankGetLocalBank(void)
{
  return &bankLocalBank;
}

//--------------------------------------------------------------------------
int bankGetEquipSlotFromGadgetId(int gadgetId)
{
  return weaponIdToSlot(gadgetId) - 1;
}

//--------------------------------------------------------------------------
enum RaidsWeaponRarity bankGetRarityFromQuality(u8 quality)
{
  if (quality < 64) return RAIDS_WEAPON_RARITY_COMMON;
  if (quality < 128) return RAIDS_WEAPON_RARITY_UNCOMMON;
  if (quality < 196) return RAIDS_WEAPON_RARITY_RARE;
  return RAIDS_WEAPON_RARITY_LEGENDARY;
}

//--------------------------------------------------------------------------
RaidsPlayerEquippedInventory_t* bankGetEquippedFromGadgetBox(GadgetBox* gbox)
{
  if (!gbox) return NULL;
  if (gbox->Initialized <= 0) return NULL;

  return &State.PlayerStates[(int)gbox->Initialized - 1].Inventory;
}

//--------------------------------------------------------------------------
RaidsInventoryWeapon_t* bankGetEquippedWeaponFromGadgetBox(GadgetBox* gbox, int gadgetId)
{
  RaidsPlayerEquippedInventory_t* inventory = bankGetEquippedFromGadgetBox(gbox);
  if (!inventory) return NULL;
  if (gadgetId <= 0) return NULL;

  int idx = weaponIdToSlot(gadgetId)-1;
  if (idx < 0 || idx >= COUNT_OF(inventory->Weapons)) return NULL;
  if (inventory->Weapons[idx].GadgetId != gadgetId) return NULL;

  return &inventory->Weapons[idx];
}

//--------------------------------------------------------------------------
RaidsInventoryWeapon_t* bankGetLocalEquippedWeapon(int gadgetId)
{
  int slotId = bankGetEquipSlotFromGadgetId(gadgetId);
  if (slotId < 0) return NULL;

  int equipIdx = bankLocalBank.Inventory.EquippedWeaponIdxs[slotId];
  if (equipIdx < 0) return NULL;
  RaidsInventoryWeapon_t* weapon = &bankLocalBank.Inventory.Weapons[equipIdx];

  if (weapon->GadgetId != gadgetId) return NULL;
  return weapon;
}

//--------------------------------------------------------------------------
RaidsInventoryWeapon_t* bankGetLocalWeaponFromBank(int index)
{
  if (index < 0) return NULL;
  if (index >= BANK_MAX_WEAPONS) return NULL;

  RaidsInventoryWeapon_t* weapon = &bankLocalBank.Inventory.Weapons[index];
  if (!weapon->GadgetId) return NULL;

  return weapon;
}

//--------------------------------------------------------------------------
u32 bankGetGadgetColor(int localPlayerIndex, int gadgetId)
{
  RaidsInventoryWeapon_t* bankWeapon = bankGetLocalEquippedWeapon(gadgetId);
  if (bankWeapon) return bankRarityColors[bankGetRarityFromQuality(bankWeapon->Quality)];

  return 0x80D0D0D0;
}

//--------------------------------------------------------------------------
int bankGetAlphaModCount(GadgetBox* gadgetBox, int gadgetId, int alphaModId)
{
  if (!gadgetBox || !gadgetBox->Initialized) return 0;
  if (alphaModId > 8) return 0;
  if (alphaModId <= 0) return 0;

  RaidsInventoryWeapon_t* bankWeapon = bankGetEquippedWeaponFromGadgetBox(gadgetBox, gadgetId);
  if (!bankWeapon) return 0;

  return bankWeapon->AlphaModCounts[alphaModId-1];
}

//--------------------------------------------------------------------------
float bankGetArbiterSpeed(Player* player)
{
  RaidsInventoryWeapon_t* bankWeapon = bankGetEquippedWeaponFromGadgetBox(player->GadgetBox, WEAPON_ID_ARBITER);
  if (!bankWeapon) return 0;

  return bankWeapon->Speed;
}

//--------------------------------------------------------------------------
float bankGetGadgetDamage(GadgetBox* gbox, int gadgetId, int damageType, int multiplier)
{
  RaidsInventoryWeapon_t* bankWeapon = bankGetEquippedWeaponFromGadgetBox(gbox, gadgetId);
  if (bankWeapon) return bankWeapon->Damage * multiplier;

  int level = gbox->Gadgets[gadgetId].Level;
  return ((float (*)(int gadgetId, int level, int damageType, int multiplier))0x00627520)(gadgetId, level, damageType, multiplier);
}

//--------------------------------------------------------------------------
int bankTryAddGadgetToQuickSelect(Player* player, int gadgetId)
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
void bankApplyGadgetMoby(Player* player, RaidsInventoryWeapon_t* item, Moby* moby)
{
  u32 glowAlpha = 0xFF000000;
  float glowOpacity = 0.5;
  if (!moby) return;

  if (item->Paint) {
    moby->PrimaryColor = bankPaintColors[item->Paint];
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
void bankRemoveGadget(Player* player, int gadgetId)
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
void bankApplyItem(Player* player, RaidsInventoryWeapon_t* item)
{
  int gadgetId = item->GadgetId;
  GadgetBox* gbox = player->GadgetBox;

  // give
  if (gbox->Gadgets[gadgetId].Level < 0) {
    playerGiveWeapon(gbox, gadgetId, 0, 1);
    bankTryAddGadgetToQuickSelect(player, gadgetId);
  }
  gbox->Gadgets[gadgetId].Level = bankGetRarityFromQuality(item->Quality) == RAIDS_WEAPON_RARITY_LEGENDARY ? 9 : 0; //item->Proficiency;

  // configure mobys
  if (player->Gadgets[0].id == gadgetId) {
    bankApplyGadgetMoby(player, item, player->Gadgets[0].pMoby);
    bankApplyGadgetMoby(player, item, player->Gadgets[0].pMoby2);
  }

  gbox->Gadgets[gadgetId].OmegaMod = item->OmegaMod;
}

//--------------------------------------------------------------------------
void bankUpdateLocalState(Player * player)
{
  if (!player || !player->PlayerMoby || !player->pNetPlayer || !player->IsLocal) return;
  if (!playerIsConnected(player)) return;

  int playerId = player->PlayerId;
  RaidsPlayerBank_t* localBank = bankGetLocalBank();

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

  if (player->LocalPlayerIndex == 0) {
    bankBroadcastEquippedInventory();
    bankBroadcastAccount();
  }
}

//--------------------------------------------------------------------------
void bankSellLocalWeaponAtIndex(int weaponIdx)
{
  RaidsPlayerBank_t* localBank = bankGetLocalBank();
  if (!localBank) return;

  if (weaponIdx < 0 || weaponIdx >= BANK_MAX_WEAPONS) return;
  RaidsInventoryWeapon_t* weapon = &localBank->Inventory.Weapons[weaponIdx];
  if (!weapon || !weapon->GadgetId) return;

  int equipSlot = bankGetEquipSlotFromGadgetId(weapon->GadgetId);
  int isEquipped = localBank->Inventory.EquippedWeaponIdxs[equipSlot] == weaponIdx;
  localBank->Account.Bolts += getPriceForWeapon(weapon->Proficiency, weapon->Quality);
  memset(weapon, 0, sizeof(RaidsInventoryWeapon_t));
  if (isEquipped) localBank->Inventory.EquippedWeaponIdxs[equipSlot] = -1;
  localBank->Inventory.RefreshLocalInventory = 1;
}

//--------------------------------------------------------------------------
void bankEquipLocalWeaponAtIndex(int weaponIdx)
{
  RaidsPlayerBank_t* localBank = bankGetLocalBank();
  if (!localBank) return;

  if (weaponIdx < 0 || weaponIdx >= BANK_MAX_WEAPONS) return;
  RaidsInventoryWeapon_t* weapon = &localBank->Inventory.Weapons[weaponIdx];
  if (!weapon || !weapon->GadgetId) return;

  localBank->Inventory.EquippedWeaponIdxs[bankGetEquipSlotFromGadgetId(weapon->GadgetId)] = weaponIdx;
  localBank->Inventory.RefreshLocalInventory = 1;
}

//--------------------------------------------------------------------------
void bankTickPlayer(Player * player)
{
  if (!player || !player->PlayerMoby || !player->pNetPlayer) return;
  if (!playerIsConnected(player)) return;

  GadgetBox* gbox = player->GadgetBox;
  if (!gbox) return;

  // default equipslots
  if (player->IsLocal && !playerGetLocalEquipslot(player->LocalPlayerIndex, 0) && State.PlayerStates[player->PlayerId].LastEquipslots[0]) {
    playerSetLocalEquipslot(player->LocalPlayerIndex, 0, State.PlayerStates[player->PlayerId].LastEquipslots[0]);
    playerSetLocalEquipslot(player->LocalPlayerIndex, 1, State.PlayerStates[player->PlayerId].LastEquipslots[1]);
    playerSetLocalEquipslot(player->LocalPlayerIndex, 2, State.PlayerStates[player->PlayerId].LastEquipslots[2]);
  }

  // apply items
  int i;
  for (i = 0; i < WEAPON_SLOT_OMNI_SHIELD; ++i) {
    int gadgetId = weaponSlotToId(i+1);
    RaidsInventoryWeapon_t* weapon = bankGetEquippedWeaponFromGadgetBox(gbox, gadgetId);
    if (weapon) {
      bankApplyItem(player, weapon);
    } else if (gbox->Gadgets[gadgetId].Level >= 0)  {
      bankRemoveGadget(player, gadgetId);
    }
  }
}

//--------------------------------------------------------------------------
void bankTick(void)
{
  int i;

  // handle new inventory change
  if (bankLocalBank.Inventory.RefreshLocalInventory) {
    for (i = 0; i < GAME_MAX_LOCALS; ++i) {
      bankUpdateLocalState(playerGetFromSlot(i));
    }
  }

  // process players
  Player** players = playerGetAll();
  for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
    bankTickPlayer(players[i]);
  }
}

//--------------------------------------------------------------------------
void bankInit(void)
{
  int i;

  netInstallCustomMsgHandler(CUSTOM_MSG_SET_PLAYER_EQUIPPED_INVENTORY, &bankOnSetPlayerEquippedInventoryRemote);
  netInstallCustomMsgHandler(CUSTOM_MSG_SET_PLAYER_ACCOUNT, &bankOnSetPlayerAccountRemote);

  // hooks
  POKE_U32(0x005DDF98, 0); // disable player ambient color affecting child mobys
  POKE_U8(0x00171b66, 1); // challenge mode
  HOOK_J_OP(0x00627600, &bankGetGadgetDamage, 0);
  HOOK_J_OP(0x00542078, &bankGetGadgetColor, 0);
  HOOK_JAL(0x003F29AC, &bankGetArbiterSpeed);
  POKE_U32(0x003F2984, 0x0240202D);
  HOOK_J(0x006299A8, &bankGetAlphaModCount);
  //HOOK_J_OP(0x00626d98, &bankGetGadgetMaxLevel, 0);
  //HOOK_J_OP(0x00626fb8, &bankGetGadgetMaxAmmo, 0);
  //HOOK_JAL_OP(0x0060f780, &bankGetGadgetRefireRate, 0x0200282D);

  bankRarityColors[RAIDS_WEAPON_RARITY_UNCOMMON] = hudGetTeamColor(TEAM_GREEN, 0);
  bankRarityColors[RAIDS_WEAPON_RARITY_RARE] = hudGetTeamColor(TEAM_BLUE, 0);
  bankRarityColors[RAIDS_WEAPON_RARITY_LEGENDARY] = hudGetTeamColor(TEAM_PURPLE, 0);

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

  bankTick();
}
