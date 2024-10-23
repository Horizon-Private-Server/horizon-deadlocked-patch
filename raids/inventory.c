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

int aaa = 0;

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

char* inventoryRarityNames[] = {
  "Uncommon",
  "Common",
  "Rare",
  "Legendary"
};

char* inventoryPaintNames[] = {
  "None",
  "Blue",
  "Red",
  "Green",
  "Orange",
  "Yellow",
  "Purple",
  "Aqua",
  "Pink",
  "Olive",
  "Maroon",
};

char* inventoryPaintSpecialNames[] = {
  "None",
  "Glow",
  "Ghost",
  "Glow Ghost"
};

char* inventoryOmegaNames[] = {
  "None",
  "Napalm",
  "Time Bomb",
  "Freeze",
  "Mini Bomb",
  "Morph",
  "Brainwash",
  "Acid",
  "Shock"
};

char inventoryAlphaModSpriteIds[] = {
  [ALPHA_MOD_SPEED] 52,
  [ALPHA_MOD_AMMO] 38,
  [ALPHA_MOD_AIMING] 37,
  [ALPHA_MOD_IMPACT] 44,
  [ALPHA_MOD_AREA] 39,
  [ALPHA_MOD_XP] 41,
  [ALPHA_MOD_JACKPOT] 46,
  [ALPHA_MOD_NANOLEECH] 48,
};

char inventoryWeaponSpriteIds[] = {
  [WEAPON_SLOT_VIPERS] 24,
  [WEAPON_SLOT_MAGMA_CANNON] 28,
  [WEAPON_SLOT_ARBITER] 27,
  [WEAPON_SLOT_FUSION_RIFLE] 29,
  [WEAPON_SLOT_MINE_LAUNCHER] 25,
  [WEAPON_SLOT_B6] 21,
  [WEAPON_SLOT_OMNI_SHIELD] 22,
  [WEAPON_SLOT_FLAIL] 23,
};

char inventoryWeaponSpriteDims[] = {
  [WEAPON_SLOT_VIPERS] 64,
  [WEAPON_SLOT_MAGMA_CANNON] 64,
  [WEAPON_SLOT_ARBITER] 32,
  [WEAPON_SLOT_FUSION_RIFLE] 64,
  [WEAPON_SLOT_MINE_LAUNCHER] 32,
  [WEAPON_SLOT_B6] 32,
  [WEAPON_SLOT_OMNI_SHIELD] 32,
  [WEAPON_SLOT_FLAIL] 32,
};

char inventorySkillSpriteIds[] = {
  [RAIDS_SKILLS_HEALTH] 15,
  [RAIDS_SKILLS_DAMAGE] 9,
  [RAIDS_SKILLS_SPEED] 52,
  [RAIDS_SKILLS_UNUSED] 0
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
RaidsPlayerBank_t* inventoryGetLocalWeaponBank(void)
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
    if (gId == gadgetId) return;
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
  RaidsPlayerBank_t* localBank = inventoryGetLocalWeaponBank();

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
void inventoryEquipLocalWeaponAtIndex(int weaponIdx)
{
  RaidsPlayerBank_t* localBank = inventoryGetLocalWeaponBank();
  if (!localBank) return;

  RaidsInventoryWeapon_t* weapon = &localBank->Inventory.Weapons[weaponIdx];
  if (!weapon || !weapon->GadgetId) return;

  localBank->Inventory.EquippedWeaponIdxs[inventoryGetEquipSlotFromGadgetId(weapon->GadgetId)] = weaponIdx;
  localBank->RefreshLocalInventory = 1;
}

//--------------------------------------------------------------------------
void inventoryOpen(void)
{
  if (State.InventoryOpen) return;
  State.InventoryOpen = 1;
  padDisableInput();
}

//--------------------------------------------------------------------------
void inventoryClose(void)
{
  if (!State.InventoryOpen) return;
  State.InventoryOpen = 0;
  padEnableInput();
}

//--------------------------------------------------------------------------
u32 inventoryDrawGetCompareColor(int compare)
{
  if (compare < 0) return 0x800000FF; // red
  if (compare == 0) return 0x80FFFFFF; // white
  return 0x8000FFFF; // yellow
}

//--------------------------------------------------------------------------
void inventoryDrawAccountInfo(InventoryDrawState_t* drawState)
{
  RaidsPlayerBank_t* localBank = inventoryGetLocalWeaponBank();
  u32 bgColor = 0x70101010; // dark gray
  u32 countTextColor = 0x8000FFFF; // yellow
  u32 equippedColor = 0x80000080; // red
  u32 compareLessColor = 0x800000FF; // red
  u32 compareMoreColor = 0x8000FFFF; // yellow
  u32 textColor = 0x80FFFFFF; // white
  u32 countBgTextColor = 0x80000000; // black
  u32 spriteColor = 0x80808080; // gray
  u32 v10Color = ((u32 (*)(int, int))0x00541ef8)(TEAM_BLUE, 0);
  u32 v99Color = ((u32 (*)(int, int))0x00541ef8)(TEAM_PURPLE, 0);
  float fw = INVENTORY_DRAW_INFO_W;
  float fh = INVENTORY_DRAW_INFO_H;
  float offX = 5;
  float offY = -fh/2 + 5;
  char strBuf[64];

  // draw box
  //gfxHelperDrawBox(INVENTORY_DRAW_CENTER_X, INVENTORY_DRAW_CENTER_Y, offX, offY, fw, fh, bgColor, TEXT_ALIGN_MIDDLECENTER, COMMON_DZO_DRAW_NORMAL);

  // stats
  snprintf(strBuf, sizeof(strBuf), "Bolts: %d", localBank->Account.Bolts);
  gfxHelperDrawText(INVENTORY_DRAW_CENTER_X, INVENTORY_DRAW_CENTER_Y, offX, offY, 0.7, textColor, strBuf, -1, TEXT_ALIGN_TOPLEFT, COMMON_DZO_DRAW_NORMAL);
  offY += 12;
  snprintf(strBuf, sizeof(strBuf), "Level: %d", getLevelFromXp(localBank->Account.Experience) + 1);
  gfxHelperDrawText(INVENTORY_DRAW_CENTER_X, INVENTORY_DRAW_CENTER_Y, offX, offY, 0.7, textColor, strBuf, -1, TEXT_ALIGN_TOPLEFT, COMMON_DZO_DRAW_NORMAL);
  offY += 12;
  snprintf(strBuf, sizeof(strBuf), "Skill Points: %d", localBank->Account.SkillPoints);
  gfxHelperDrawText(INVENTORY_DRAW_CENTER_X, INVENTORY_DRAW_CENTER_Y, offX, offY, 0.7, textColor, strBuf, -1, TEXT_ALIGN_TOPLEFT, COMMON_DZO_DRAW_NORMAL);
  offY += 12;

  // weapon proficiency
  gfxSetupGifPaging(0);
  int i;
  for (i = 1; i < WEAPON_SLOT_COUNT; ++i) {
    int iconSpriteId = inventoryWeaponSpriteIds[i];
    int iconSpriteDim = inventoryWeaponSpriteDims[i];
    gfxHelperDrawSprite(INVENTORY_DRAW_CENTER_X, INVENTORY_DRAW_CENTER_Y, offX, offY, 16, 16, iconSpriteDim, iconSpriteDim, iconSpriteId, spriteColor, TEXT_ALIGN_TOPLEFT, COMMON_DZO_DRAW_NORMAL);
    snprintf(strBuf, sizeof(strBuf), "V%d", localBank->Account.Proficiency[i-1] + 1);
    gfxHelperDrawText(INVENTORY_DRAW_CENTER_X, INVENTORY_DRAW_CENTER_Y, offX + 0, offY + 10, 0.7, textColor, strBuf, -1, TEXT_ALIGN_TOPLEFT, COMMON_DZO_DRAW_NORMAL);
    offX += 25;
  }
  offY += 22;
  
  // skills
  offX = 5;
  for (i = 0; i < RAIDS_SKILLS_UNUSED; ++i) {
    gfxHelperDrawSprite(INVENTORY_DRAW_CENTER_X, INVENTORY_DRAW_CENTER_Y, offX, offY, 16, 16, 32, 32, inventorySkillSpriteIds[i], spriteColor, TEXT_ALIGN_TOPLEFT, COMMON_DZO_DRAW_NORMAL);
    snprintf(strBuf, sizeof(strBuf), "%d", localBank->Account.Skills[i]);
    gfxHelperDrawText(INVENTORY_DRAW_CENTER_X, INVENTORY_DRAW_CENTER_Y, offX + 0, offY + 10, 0.7, textColor, strBuf, -1, TEXT_ALIGN_TOPLEFT, COMMON_DZO_DRAW_NORMAL);
    offX += 25;
  }
  offY += 18;
  gfxDoGifPaging();
}

//--------------------------------------------------------------------------
void inventoryDrawWeaponInfo(InventoryDrawState_t* drawState)
{
  RaidsPlayerBank_t* localBank = inventoryGetLocalWeaponBank();
  u32 bgColor = 0x70101010; // dark gray
  u32 countTextColor = 0x8000FFFF; // yellow
  u32 equippedColor = 0x80000080; // red
  u32 compareLessColor = 0x800000FF; // red
  u32 compareMoreColor = 0x8000FFFF; // yellow
  u32 textColor = 0x80FFFFFF; // white
  u32 countBgTextColor = 0x80000000; // black
  u32 spriteColor = 0x80808080; // gray
  u32 v10Color = ((u32 (*)(int, int))0x00541ef8)(TEAM_BLUE, 0);
  u32 v99Color = ((u32 (*)(int, int))0x00541ef8)(TEAM_PURPLE, 0);
  float fw = INVENTORY_DRAW_INFO_W;
  float fh = INVENTORY_DRAW_INFO_H;
  float offX = fw/2;
  float offY = 0;
  char strBuf[64];

  // draw box
  gfxHelperDrawBox(INVENTORY_DRAW_CENTER_X, INVENTORY_DRAW_CENTER_Y, offX, offY, fw, fh, bgColor, TEXT_ALIGN_MIDDLECENTER, COMMON_DZO_DRAW_NORMAL);

  RaidsInventoryWeapon_t* selectedWeapon = &localBank->Inventory.Weapons[drawState->SelectedIdx];
  if (!selectedWeapon->GadgetId) return;
  RaidsInventoryWeapon_t* equippedWeapon = inventoryGetLocalEquippedWeapon(selectedWeapon->GadgetId);
  int hasComparison = equippedWeapon && equippedWeapon != selectedWeapon;
  RaidsInventoryWeapon_t* baseWeapon = hasComparison ? equippedWeapon : selectedWeapon;

  // name
  offX = 5;
  offY = -10;
  struct GadgetDef* gadgetDef = weaponGetDef(selectedWeapon->GadgetId, 0);
  snprintf(strBuf, sizeof(strBuf), "%s V%d", uiMsgString(gadgetDef->quickSelectTag), selectedWeapon->Proficiency + 1);
  gfxHelperDrawText(INVENTORY_DRAW_CENTER_X, INVENTORY_DRAW_CENTER_Y, offX, offY, 0.9, textColor, strBuf, -1, TEXT_ALIGN_TOPLEFT, COMMON_DZO_DRAW_NORMAL);
  offY += 20;

  // stats
  snprintf(strBuf, sizeof(strBuf), "Damage: %d", baseWeapon->Damage);
  gfxHelperDrawText(INVENTORY_DRAW_CENTER_X, INVENTORY_DRAW_CENTER_Y, offX, offY, 0.7, textColor, strBuf, -1, TEXT_ALIGN_TOPLEFT, COMMON_DZO_DRAW_NORMAL);
  if (hasComparison) {
    float strW = gfxGetFontWidth(strBuf, -1, 0.7);
    snprintf(strBuf, sizeof(strBuf), "=> %d", selectedWeapon->Damage);
    gfxHelperDrawText(INVENTORY_DRAW_CENTER_X, INVENTORY_DRAW_CENTER_Y, offX + strW + 5, offY, 0.7, inventoryDrawGetCompareColor(selectedWeapon->Damage - baseWeapon->Damage), strBuf, -1, TEXT_ALIGN_TOPLEFT, COMMON_DZO_DRAW_NORMAL);
  }
  offY += 12;
  snprintf(strBuf, sizeof(strBuf), "Speed: %.2f", baseWeapon->Speed);
  gfxHelperDrawText(INVENTORY_DRAW_CENTER_X, INVENTORY_DRAW_CENTER_Y, offX, offY, 0.7, textColor, strBuf, -1, TEXT_ALIGN_TOPLEFT, COMMON_DZO_DRAW_NORMAL);
  if (hasComparison) {
    float strW = gfxGetFontWidth(strBuf, -1, 0.7);
    snprintf(strBuf, sizeof(strBuf), "=> %.2f", selectedWeapon->Speed);
    gfxHelperDrawText(INVENTORY_DRAW_CENTER_X, INVENTORY_DRAW_CENTER_Y, offX + strW + 5, offY, 0.7, inventoryDrawGetCompareColor((selectedWeapon->Speed - baseWeapon->Speed)*100), strBuf, -1, TEXT_ALIGN_TOPLEFT, COMMON_DZO_DRAW_NORMAL);
  }
  offY += 12;
  snprintf(strBuf, sizeof(strBuf), "Crit %%: %.f%%", (baseWeapon->CritChance / 255.0) * 100);
  gfxHelperDrawText(INVENTORY_DRAW_CENTER_X, INVENTORY_DRAW_CENTER_Y, offX, offY, 0.7, textColor, strBuf, -1, TEXT_ALIGN_TOPLEFT, COMMON_DZO_DRAW_NORMAL);
  if (hasComparison) {
    float strW = gfxGetFontWidth(strBuf, -1, 0.7);
    snprintf(strBuf, sizeof(strBuf), "=> %.f%%", (selectedWeapon->CritChance / 255.0) * 100);
    gfxHelperDrawText(INVENTORY_DRAW_CENTER_X, INVENTORY_DRAW_CENTER_Y, offX + strW + 5, offY, 0.7, inventoryDrawGetCompareColor(selectedWeapon->CritChance - baseWeapon->CritChance), strBuf, -1, TEXT_ALIGN_TOPLEFT, COMMON_DZO_DRAW_NORMAL);
  }
  offY += 12;
  // int baseRarity = inventoryGetRarityFromQuality(baseWeapon->Quality);
  // snprintf(strBuf, sizeof(strBuf), "Rarity: %s", inventoryRarityNames[baseRarity]);
  // gfxHelperDrawText(INVENTORY_DRAW_CENTER_X, INVENTORY_DRAW_CENTER_Y, offX, offY, 0.7, textColor, strBuf, -1, TEXT_ALIGN_TOPLEFT, COMMON_DZO_DRAW_NORMAL);
  // if (hasComparison) {
  //   float strW = gfxGetFontWidth(strBuf, -1, 0.7);
  //   int selRarity = inventoryGetRarityFromQuality(selectedWeapon->Quality);
  //   snprintf(strBuf, sizeof(strBuf), "=> %s", inventoryRarityNames[selRarity]);
  //   gfxHelperDrawText(INVENTORY_DRAW_CENTER_X, INVENTORY_DRAW_CENTER_Y, offX + strW + 5, offY, 0.7, inventoryDrawGetCompareColor(selRarity - baseRarity), strBuf, -1, TEXT_ALIGN_TOPLEFT, COMMON_DZO_DRAW_NORMAL);
  // }
  // offY += 12;
  snprintf(strBuf, sizeof(strBuf), "Color: %s", inventoryPaintNames[baseWeapon->Paint]);
  gfxHelperDrawText(INVENTORY_DRAW_CENTER_X, INVENTORY_DRAW_CENTER_Y, offX, offY, 0.7, textColor, strBuf, -1, TEXT_ALIGN_TOPLEFT, COMMON_DZO_DRAW_NORMAL);
  if (hasComparison) {
    float strW = gfxGetFontWidth(strBuf, -1, 0.7);
    snprintf(strBuf, sizeof(strBuf), "=> %s", inventoryPaintNames[selectedWeapon->Paint]);
    gfxHelperDrawText(INVENTORY_DRAW_CENTER_X, INVENTORY_DRAW_CENTER_Y, offX + strW + 5, offY, 0.7, textColor, strBuf, -1, TEXT_ALIGN_TOPLEFT, COMMON_DZO_DRAW_NORMAL);
  }
  offY += 12;
  snprintf(strBuf, sizeof(strBuf), "Special: %s", inventoryPaintSpecialNames[baseWeapon->PaintSpecialMask]);
  gfxHelperDrawText(INVENTORY_DRAW_CENTER_X, INVENTORY_DRAW_CENTER_Y, offX, offY, 0.7, textColor, strBuf, -1, TEXT_ALIGN_TOPLEFT, COMMON_DZO_DRAW_NORMAL);
  if (hasComparison) {
    float strW = gfxGetFontWidth(strBuf, -1, 0.7);
    snprintf(strBuf, sizeof(strBuf), "=> %s", inventoryPaintSpecialNames[selectedWeapon->PaintSpecialMask]);
    gfxHelperDrawText(INVENTORY_DRAW_CENTER_X, INVENTORY_DRAW_CENTER_Y, offX + strW + 5, offY, 0.7, textColor, strBuf, -1, TEXT_ALIGN_TOPLEFT, COMMON_DZO_DRAW_NORMAL);
  }
  offY += 12;
  snprintf(strBuf, sizeof(strBuf), "Omega: %s", inventoryOmegaNames[baseWeapon->OmegaMod]);
  gfxHelperDrawText(INVENTORY_DRAW_CENTER_X, INVENTORY_DRAW_CENTER_Y, offX, offY, 0.7, textColor, strBuf, -1, TEXT_ALIGN_TOPLEFT, COMMON_DZO_DRAW_NORMAL);
  if (hasComparison) {
    float strW = gfxGetFontWidth(strBuf, -1, 0.7);
    snprintf(strBuf, sizeof(strBuf), "=> %s", inventoryOmegaNames[selectedWeapon->OmegaMod]);
    gfxHelperDrawText(INVENTORY_DRAW_CENTER_X, INVENTORY_DRAW_CENTER_Y, offX + strW + 5, offY, 0.7, textColor, strBuf, -1, TEXT_ALIGN_TOPLEFT, COMMON_DZO_DRAW_NORMAL);
  }
  offY += 12;

  // alpha mods
  gfxSetupGifPaging(0);
  int i;
  for (i = 1; i < ALPHA_MOD_COUNT; ++i) {
    gfxHelperDrawSprite(INVENTORY_DRAW_CENTER_X, INVENTORY_DRAW_CENTER_Y, offX, offY, 16, 16, 32, 32, inventoryAlphaModSpriteIds[i], spriteColor, TEXT_ALIGN_TOPLEFT, COMMON_DZO_DRAW_NORMAL);
    snprintf(strBuf, sizeof(strBuf), "%d", baseWeapon->AlphaModCounts[i-1]);
    gfxHelperDrawText(INVENTORY_DRAW_CENTER_X, INVENTORY_DRAW_CENTER_Y, offX + 0, offY + 10, 0.7, textColor, strBuf, -1, TEXT_ALIGN_TOPLEFT, COMMON_DZO_DRAW_NORMAL);
    if (hasComparison) {
      float strW = gfxGetFontWidth(strBuf, -1, 0.7);
      snprintf(strBuf, sizeof(strBuf), ">%d", selectedWeapon->AlphaModCounts[i-1]);
      gfxHelperDrawText(INVENTORY_DRAW_CENTER_X, INVENTORY_DRAW_CENTER_Y, offX + 2 + strW, offY + 11, 0.6, inventoryDrawGetCompareColor(selectedWeapon->AlphaModCounts[i-1] - baseWeapon->AlphaModCounts[i-1]), strBuf, -1, TEXT_ALIGN_TOPLEFT, COMMON_DZO_DRAW_NORMAL);
    }
    
    offX += 25;
  }
  gfxDoGifPaging();

}

//--------------------------------------------------------------------------
void inventoryDrawWeapon(InventoryDrawState_t* drawState, int row, int col, RaidsInventoryWeapon_t* weapon)
{
  RaidsPlayerBank_t* localBank = inventoryGetLocalWeaponBank();
  u32 bgColor = 0x70101010; // dark gray
  u32 selectedColor = 0x40008080; // yellow
  u32 equippedColor = 0x40000080; // red
  u32 v10Color = ((u32 (*)(int, int))0x00541ef8)(TEAM_BLUE, 0);
  u32 v99Color = ((u32 (*)(int, int))0x00541ef8)(TEAM_PURPLE, 0);
  float fw = (INVENTORY_DRAW_WEAPONS_W / INVENTORY_DRAW_WEAPONS_DIM);
  float fh = (INVENTORY_DRAW_WEAPONS_H / INVENTORY_DRAW_WEAPONS_DIM);
  float w = fw - INVENTORY_DRAW_WEAPONS_M*2.0;
  float h = fh - INVENTORY_DRAW_WEAPONS_M*2.0;
  float offX = -(INVENTORY_DRAW_FULL_W/2.0) + fw/2 + col*fw;
  float offY = -(INVENTORY_DRAW_WEAPONS_H/2.0) + fh/2 + row*fh ;
  int idx = row*8 + col;
  int isSelected = idx == drawState->SelectedIdx;
  int isEquipped = idx == localBank->Inventory.EquippedWeaponIdxs[inventoryGetEquipSlotFromGadgetId(weapon->GadgetId)];

  // draw bg on first item
  if (col == 0 && row == 0) {
    gfxHelperDrawBox(INVENTORY_DRAW_CENTER_X, INVENTORY_DRAW_CENTER_Y, -(INVENTORY_DRAW_WEAPONS_W/2.0), 0, INVENTORY_DRAW_WEAPONS_W, INVENTORY_DRAW_WEAPONS_H, 0x60101010, TEXT_ALIGN_MIDDLECENTER, COMMON_DZO_DRAW_NORMAL);
  }

  // draw equipped
  if (isEquipped) {
    gfxHelperDrawBox(INVENTORY_DRAW_CENTER_X, INVENTORY_DRAW_CENTER_Y, offX, offY, fw, fh, equippedColor, TEXT_ALIGN_MIDDLECENTER, COMMON_DZO_DRAW_NORMAL);
  }

  // draw box
  if (isSelected) {
    gfxHelperDrawBox(INVENTORY_DRAW_CENTER_X, INVENTORY_DRAW_CENTER_Y, offX, offY, fw, fh, selectedColor, TEXT_ALIGN_MIDDLECENTER, COMMON_DZO_DRAW_NORMAL);
  }

  // no weapon info
  if (!weapon) return;

  int slotId = weaponIdToSlot(weapon->GadgetId);
  int iconSpriteId = inventoryWeaponSpriteIds[slotId];
  int iconSpriteDim = inventoryWeaponSpriteDims[slotId];
  
  // draw icon
  u32 iconColor = colorLerp(0x80FFFFFF, v10Color, (weapon->Proficiency+1)/10.0);
  if (weapon->Proficiency > 9)
    iconColor = colorLerp(v10Color, v99Color, (weapon->Proficiency-9) / 90.0);
  gfxSetupGifPaging(0);
  gfxHelperDrawSprite(INVENTORY_DRAW_CENTER_X, INVENTORY_DRAW_CENTER_Y, offX-1, offY-1, w+2, h+2, iconSpriteDim, iconSpriteDim, iconSpriteId, 0x80000000, TEXT_ALIGN_MIDDLECENTER, COMMON_DZO_DRAW_NORMAL);
  gfxHelperDrawSprite(INVENTORY_DRAW_CENTER_X, INVENTORY_DRAW_CENTER_Y, offX, offY, w, h, iconSpriteDim, iconSpriteDim, iconSpriteId, iconColor, TEXT_ALIGN_MIDDLECENTER, COMMON_DZO_DRAW_NORMAL);

  // draw equipped
  if (isEquipped) {
    //gfxHelperDrawSprite(INVENTORY_DRAW_CENTER_X, INVENTORY_DRAW_CENTER_Y, offX-w/2, offY-h/2, 12, 12, 32, 32, 80, equippedColor, TEXT_ALIGN_MIDDLECENTER, COMMON_DZO_DRAW_NORMAL);
  }

  // draw paint
  if (weapon->Paint || weapon->PaintSpecialMask) {
    gfxHelperDrawSprite(INVENTORY_DRAW_CENTER_X, INVENTORY_DRAW_CENTER_Y, offX-w/2+1, offY+h/2-1, 8, 8, 32, 32, 80 + (weapon->PaintSpecialMask>0?1:0), inventoryPaintColors[weapon->Paint] | 0x80000000, TEXT_ALIGN_MIDDLECENTER, COMMON_DZO_DRAW_NORMAL);
  }

  // draw omega
  if (weapon->OmegaMod) {
    u32 omegaColor = ((u32 (*)(int))0x00541fd0)(weapon->OmegaMod);
    gfxHelperDrawSprite(INVENTORY_DRAW_CENTER_X, INVENTORY_DRAW_CENTER_Y, offX+w/2-2, offY+h/2-2, 12, 12, 32, 32, 79, omegaColor | 0x80000000, TEXT_ALIGN_MIDDLECENTER, COMMON_DZO_DRAW_NORMAL);
  }

  gfxDoGifPaging();
}

//--------------------------------------------------------------------------
void inventoryDraw(void)
{
  static int selIdx = 0;
  u32 bgColor = 0x60000000;
  u32 borderColor = 0x80000020;
  u32 textColor = 0x80FFFFFF;
  float borderSizeH = INVENTORY_DRAW_FRAME_BORDER_W * SCREEN_RATIO_INV;
  float borderSizeV = INVENTORY_DRAW_FRAME_BORDER_W;
  
  GameSettings* gs = gameGetSettings();
  Player* localPlayer = playerGetFromSlot(0);
  RaidsPlayerBank_t* localBank = inventoryGetLocalWeaponBank();
  RaidsInventoryWeapon_t* selectedWeapon = &localBank->Inventory.Weapons[selIdx];
  RaidsInventoryWeapon_t* equippedWeapon = inventoryGetLocalEquippedWeapon(selectedWeapon->GadgetId);
  int canEquip = selectedWeapon->GadgetId && equippedWeapon != selectedWeapon; // already equipped
  int canSell = selectedWeapon->GadgetId && equippedWeapon != selectedWeapon; // can't sell equipped

  InventoryDrawState_t drawState = {
    .SelectedIdx = selIdx
  };

  // draw frame
  gfxHelperDrawBox(INVENTORY_DRAW_CENTER_X, INVENTORY_DRAW_CENTER_Y, 0, 0, INVENTORY_DRAW_FULL_W, INVENTORY_DRAW_FULL_H, bgColor, TEXT_ALIGN_MIDDLECENTER, COMMON_DZO_DRAW_NORMAL);

  // draw title text
  gfxHelperDrawText(INVENTORY_DRAW_CENTER_X, INVENTORY_DRAW_CENTER_Y, INVENTORY_DRAW_INFO_W/2, -INVENTORY_DRAW_FULL_H/2 + 2, 1.1, textColor, gs->PlayerNames[localPlayer->PlayerId], -1, TEXT_ALIGN_TOPCENTER, COMMON_DZO_DRAW_NORMAL);
  
  // draw grid
  int i,j; // col,row
  for (i = 0; i < INVENTORY_DRAW_WEAPONS_DIM; ++i) {
    for (j = 0; j < INVENTORY_DRAW_WEAPONS_DIM; ++j) {
      int idx = (i*INVENTORY_DRAW_WEAPONS_DIM)+j;
      RaidsInventoryWeapon_t* weapon = &localBank->Inventory.Weapons[idx];
      inventoryDrawWeapon(&drawState, i, j, weapon);
    }
  }

  inventoryDrawWeaponInfo(&drawState);
  inventoryDrawAccountInfo(&drawState);

  // draw footer text
  char strBuf[64];
  strcpy(strBuf, "\x14 \x15 FILTER    ");
  if (canEquip) strcat(strBuf, "\x11 EQUIP    ");
  if (canSell) strcat(strBuf, "\x13 SELL    ");
  strcat(strBuf, "\x12 CLOSE");
  gfxHelperDrawText(INVENTORY_DRAW_CENTER_X, INVENTORY_DRAW_CENTER_Y, -INVENTORY_DRAW_FULL_W/2 + 5, INVENTORY_DRAW_FULL_H/2 - 5, 0.8, textColor, strBuf, -1, TEXT_ALIGN_BOTTOMLEFT, COMMON_DZO_DRAW_NORMAL);
  
  // draw frame borders
  gfxHelperDrawBox(INVENTORY_DRAW_CENTER_X, INVENTORY_DRAW_CENTER_Y, INVENTORY_DRAW_FULL_W/2, 0, borderSizeH, INVENTORY_DRAW_FULL_H + borderSizeV, borderColor, TEXT_ALIGN_MIDDLECENTER, COMMON_DZO_DRAW_NORMAL);
  gfxHelperDrawBox(INVENTORY_DRAW_CENTER_X, INVENTORY_DRAW_CENTER_Y, -INVENTORY_DRAW_FULL_W/2, 0, borderSizeH, INVENTORY_DRAW_FULL_H + borderSizeV, borderColor, TEXT_ALIGN_MIDDLECENTER, COMMON_DZO_DRAW_NORMAL);
  gfxHelperDrawBox(INVENTORY_DRAW_CENTER_X, INVENTORY_DRAW_CENTER_Y, 0, INVENTORY_DRAW_FULL_H/2, INVENTORY_DRAW_FULL_W + borderSizeH, borderSizeV, borderColor, TEXT_ALIGN_MIDDLECENTER, COMMON_DZO_DRAW_NORMAL);
  gfxHelperDrawBox(INVENTORY_DRAW_CENTER_X, INVENTORY_DRAW_CENTER_Y, 0, -INVENTORY_DRAW_FULL_H/2, INVENTORY_DRAW_FULL_W + borderSizeH, borderSizeV, borderColor, TEXT_ALIGN_MIDDLECENTER, COMMON_DZO_DRAW_NORMAL);

  // handle input
  int selMod = selIdx % INVENTORY_DRAW_WEAPONS_DIM;
  if (padGetButtonDown(0, PAD_LEFT) > 0) {
    selIdx = (selIdx-1) % INVENTORY_BANK_MAX_WEAPONS;
    if (selIdx < 0) selIdx += INVENTORY_BANK_MAX_WEAPONS;
  } else if (padGetButtonDown(0, PAD_RIGHT) > 0) {
    selIdx = (selIdx+1) % INVENTORY_BANK_MAX_WEAPONS;
  } else if (padGetButtonDown(0, PAD_DOWN) > 0) {
    selIdx = (selIdx+INVENTORY_DRAW_WEAPONS_DIM) % INVENTORY_BANK_MAX_WEAPONS;
  } else if (padGetButtonDown(0, PAD_UP) > 0) {
    selIdx = (selIdx-INVENTORY_DRAW_WEAPONS_DIM) % INVENTORY_BANK_MAX_WEAPONS;
    if (selIdx < 0) selIdx += INVENTORY_BANK_MAX_WEAPONS;
  } else if (padGetButtonDown(0, PAD_CIRCLE) > 0) { // EQUIP
    inventoryEquipLocalWeaponAtIndex(selIdx);
  }
  
  //gfxSetupGifPaging(0);
  //gfxHelperDrawSprite(60, SCREEN_HEIGHT - 60, 0, 0, 100, 100, 64, 64, aaa, 0x80808080, TEXT_ALIGN_MIDDLECENTER, COMMON_DZO_DRAW_NORMAL);
  //gfxDoGifPaging();
}

//--------------------------------------------------------------------------
void inventoryTickPlayer(Player * player)
{
  if (!player || !player->PlayerMoby || !player->pNetPlayer) return;
  if (!playerIsConnected(player)) return;

  RaidsPlayerBank_t* localBank = inventoryGetLocalWeaponBank();
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
  int refreshLocalInv = inventoryLocalBank.RefreshLocalInventory;
  int i;

  if (padGetButtonDown(0, PAD_LEFT) > 0) {
    --aaa;
    DPRINTF("%d\n", aaa);
  } else if (padGetButtonDown(0, PAD_RIGHT) > 0) {
    ++aaa;
    DPRINTF("%d\n", aaa);
  }

  // check if we need to request our bank
  if (!isInGame()) {
    hasBank = 0;
    inventoryClose();
    return;
  } else if (!hasBank) {
    struct RaidsGetBankRequest msg = { .DestAddress = &inventoryLocalBank };
    netSendCustomAppMessage(NET_DELIVERY_CRITICAL, netGetLobbyServerConnection(), NET_LOBBY_CLIENT_INDEX, CUSTOM_MSG_ID_GET_RAIDS_BANK_REQUEST, sizeof(msg), &msg);
    hasBank = 1;
  }

  // handle new inventory change
  if (refreshLocalInv) {
    inventoryLocalBank.RefreshLocalInventory = 0;
    for (i = 0; i < GAME_MAX_LOCALS; ++i) {
      inventoryUpdateLocalState(playerGetFromSlot(i));
    }
  }

  // process players
  Player** players = playerGetAll();
  for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
    inventoryTickPlayer(players[i]);
  }

  // draw
  if (!State.InventoryOpen && padGetButtonDown(0, PAD_L3) > 0) {
    inventoryOpen();
  } else if (State.InventoryOpen) {
    inventoryDraw();
    if (gameIsAnyStartMenuOpen() || padGetButtonDown(0, PAD_TRIANGLE) > 0) {
      inventoryClose();
    }
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
