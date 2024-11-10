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
#include "include/hop.h"
#include "include/game.h"
#include "include/bank.h"
#include "include/inventory.h"
#include "include/bubble.h"
#include "config.h"
#include "common.h"

extern struct RaidsState State;
extern u32 bankRarityColors[];
extern u32 bankPaintColors[];
char inventoryFilterMapping[BANK_MAX_WEAPONS];
char inventoryTabCounts[INVENTORY_TAB_COUNT];
char inventoryTabHasNew[INVENTORY_TAB_COUNT];

InventoryDrawState_t inventoryDrawState = {
  .SelectedIdx = 0,
  .FilterIdx = 0,
  .ShowSellDialog = 0,
};

char* inventoryRarityNames[] = {
  "Common",
  "Uncommon",
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
  [WEAPON_SLOT_WRENCH] 99,
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
  [WEAPON_SLOT_WRENCH] 32,
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

//--------------------------------------------------------------------------
void inventoryOpen(void)
{
  if (gameHasEnded()) return;
  if (State.MenuOpen != RAIDS_CUSTOM_MENU_NONE) return;

  State.MenuOpen = RAIDS_CUSTOM_MENU_INVENTORY;
  bankRequestInventoryFromServer();
  inventorySetFilter(inventoryDrawState.FilterIdx);
  padDisableInput();
}

//--------------------------------------------------------------------------
void inventoryClose(void)
{
  if (State.MenuOpen != RAIDS_CUSTOM_MENU_INVENTORY) return;
  
  State.MenuOpen = RAIDS_CUSTOM_MENU_NONE;
  padEnableInput();
  bankSendInventoryToServer();
}

//--------------------------------------------------------------------------
u32 inventoryDrawGetCompareColor(int compare)
{
  if (compare < 0) return 0x800000FF; // red
  if (compare == 0) return 0x80FFFFFF; // white
  return 0x8000FFFF; // yellow
}

//--------------------------------------------------------------------------
void inventorySetFilter(int filter)
{
  int i;
  RaidsPlayerBank_t* localBank = bankGetLocalBank();

  // reset (no filter)
  for (i = 0; i < BANK_MAX_WEAPONS; ++i)
    inventoryFilterMapping[i] = i;
 
  if (!localBank) return;
  
  // filter by gadget
  int filterGadgetId = weaponSlotToId(filter);
  int idx = 0;
  memset(inventoryFilterMapping, -1, sizeof(inventoryFilterMapping));
  memset(inventoryTabCounts, 0, sizeof(inventoryTabCounts));
  memset(inventoryTabHasNew, 0, sizeof(inventoryTabHasNew));
  for (i = 0; i < BANK_MAX_WEAPONS; ++i) {
    // count
    int weaponGadgetId = localBank->Inventory.Weapons[i].GadgetId;
    int slotId = weaponIdToSlot(weaponGadgetId);
    if (weaponGadgetId) {
      inventoryTabCounts[0]++; // total
      if (slotId) inventoryTabCounts[slotId]++;
      if (localBank->Inventory.Weapons[i].Notify) inventoryTabHasNew[slotId] = 1;
    }

    // add to mapping if matches filter
    if (!filter || weaponGadgetId == filterGadgetId) {
      inventoryFilterMapping[idx++] = i;
    }
  }
}

//--------------------------------------------------------------------------
int inventoryDrawDialog(InventoryDrawState_t* drawState, char* titleStr, char* bodyStr)
{
  float offX = -(INVENTORY_DRAW_WEAPONS_W/2.0);
  float offY = 0;
  u32 textColor = 0x80FFFFFF; // white
  char* buttonStr = "\x10 CONFIRM        \x12 CANCEL";

  // determine width of dialog
  float fw = gfxGetFontWidth(buttonStr, -1, 0.9);
  float fh = 20;
  if (titleStr) { fw = maxf(fw, gfxGetFontWidth(titleStr, -1, 0.9)); fh += 20; }
  if (bodyStr) { fw = maxf(fw, gfxGetFontWidth(bodyStr, -1, 0.9)); fh += 20; }

  // draw frame
  gfxHelperDrawBox(INVENTORY_DRAW_CENTER_X, INVENTORY_DRAW_CENTER_Y, offX, offY, fw, fh, 0x80101030, TEXT_ALIGN_MIDDLECENTER, COMMON_DZO_DRAW_NORMAL);

  // draw title text
  offY = -fh/2.0;
  if (titleStr) {
    gfxHelperDrawText(INVENTORY_DRAW_CENTER_X, INVENTORY_DRAW_CENTER_Y, offX, offY, 0.9, textColor, titleStr, -1, TEXT_ALIGN_TOPCENTER, COMMON_DZO_DRAW_NORMAL);
    offY += 20;
  }
  
  // draw price
  if (bodyStr) {
    gfxHelperDrawText(INVENTORY_DRAW_CENTER_X, INVENTORY_DRAW_CENTER_Y, offX, offY, 0.9, textColor, bodyStr, -1, TEXT_ALIGN_TOPCENTER, COMMON_DZO_DRAW_NORMAL);
    offY += 20;
  }
  
  // footer buttons
  gfxHelperDrawText(INVENTORY_DRAW_CENTER_X, INVENTORY_DRAW_CENTER_Y, offX, offY, 0.9, textColor, buttonStr, -1, TEXT_ALIGN_TOPCENTER, COMMON_DZO_DRAW_NORMAL);
  offY += 20;

  // input
  if (padGetButtonDown(0, PAD_CROSS) > 0) {
    return 1;
  } else if (padGetButtonDown(0, PAD_TRIANGLE) > 0) {
    return 0;
  }

  return -1;
}

//--------------------------------------------------------------------------
void inventoryDrawSpinner(InventoryDrawState_t* drawState, float x, float y)
{
  gfxSetupGifPaging(0);

  x -= 16; // middle align (16*(3-1))/2
  float t = (int)(4*fastmodf(gameGetTime() / 1000.0, 1.0));
  int i = 0;
  while (i < t) {
    gfxHelperDrawSprite(INVENTORY_DRAW_CENTER_X, INVENTORY_DRAW_CENTER_Y, x, y, 16, 16, 64, 64, 80, 0x80FFFFFF, TEXT_ALIGN_MIDDLECENTER, COMMON_DZO_DRAW_NORMAL);
    ++i;
    x += 16;
  }
  
  gfxDoGifPaging();
}

//--------------------------------------------------------------------------
void inventoryDrawAccountInfo(InventoryDrawState_t* drawState)
{
  RaidsPlayerBank_t* localBank = bankGetLocalBank();
  u32 textColor = 0x80FFFFFF; // white
  u32 spriteColor = 0x80808080; // gray
  float fw = (INVENTORY_DRAW_FULL_W - INVENTORY_DRAW_WEAPONS_W);
  float fh = INVENTORY_DRAW_INFO_H;
  float offX = 5;
  float offY = -fh/2 + 3;
  char strBuf[64];

  if (!bankGetHasAccount()) {
    inventoryDrawSpinner(drawState, fw/2.0, -fh/4.0);
    return;
  }

  // draw box
  //gfxHelperDrawBox(INVENTORY_DRAW_CENTER_X, INVENTORY_DRAW_CENTER_Y, offX, offY, fw, fh, bgColor, TEXT_ALIGN_MIDDLECENTER, COMMON_DZO_DRAW_NORMAL);

  // stats
  snprintf(strBuf, sizeof(strBuf), "Bolts: %'d", localBank->Account.Bolts);
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
    snprintf(strBuf, sizeof(strBuf), "P%d", getProficiencyFromXp(localBank->Account.WeaponXp[i-1]) + 1);
    gfxHelperDrawText(INVENTORY_DRAW_CENTER_X, INVENTORY_DRAW_CENTER_Y, offX + 0, offY + 10, 0.6, textColor, strBuf, -1, TEXT_ALIGN_TOPLEFT, COMMON_DZO_DRAW_NORMAL);
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
  u32 bgColor = 0x70101010; // dark gray
  u32 textColor = 0x80FFFFFF; // white
  u32 spriteColor = 0x80808080; // gray
  float fw = INVENTORY_DRAW_INFO_W;
  float fh = INVENTORY_DRAW_INFO_H;
  float offX = fw/2;
  float offY = 0;
  char strBuf[64];

  // draw box
  gfxHelperDrawBox(INVENTORY_DRAW_CENTER_X, INVENTORY_DRAW_CENTER_Y, offX, offY, fw, fh, bgColor, TEXT_ALIGN_MIDDLECENTER, COMMON_DZO_DRAW_NORMAL);

  RaidsInventoryWeapon_t* selectedWeapon = bankGetLocalWeaponFromBank(inventoryFilterMapping[drawState->SelectedIdx]);
  if (!selectedWeapon) return;
  RaidsInventoryWeapon_t* equippedWeapon = bankGetLocalEquippedWeapon(selectedWeapon->GadgetId);
  int hasComparison = equippedWeapon && equippedWeapon != selectedWeapon;
  RaidsInventoryWeapon_t* baseWeapon = hasComparison ? equippedWeapon : selectedWeapon;
  int rarity = bankGetRarityFromQuality(selectedWeapon->Quality);

  // name
  offX = 5;
  offY = -19;
  struct GadgetDef* gadgetDef = weaponGetDef(selectedWeapon->GadgetId, 0);
  snprintf(strBuf, sizeof(strBuf), "%s P%d", uiMsgString(rarity == RAIDS_WEAPON_RARITY_LEGENDARY ? gadgetDef->upgQSTag : gadgetDef->quickSelectTag), selectedWeapon->Proficiency + 1);
  gfxHelperDrawText(INVENTORY_DRAW_CENTER_X, INVENTORY_DRAW_CENTER_Y, offX, offY, 0.95, textColor, strBuf, -1, TEXT_ALIGN_TOPLEFT, COMMON_DZO_DRAW_NORMAL);
  offY += 17;

  // stats
  offX = 10;
  snprintf(strBuf, sizeof(strBuf), "Damage: %d", baseWeapon->Damage);
  gfxHelperDrawText(INVENTORY_DRAW_CENTER_X, INVENTORY_DRAW_CENTER_Y, offX, offY, 0.7, textColor, strBuf, -1, TEXT_ALIGN_TOPLEFT, COMMON_DZO_DRAW_NORMAL);
  if (hasComparison) {
    float strW = gfxGetFontWidth(strBuf, -1, 0.7);
    snprintf(strBuf, sizeof(strBuf), "> %d", selectedWeapon->Damage);
    gfxHelperDrawText(INVENTORY_DRAW_CENTER_X, INVENTORY_DRAW_CENTER_Y, offX + strW + 5, offY, 0.7, inventoryDrawGetCompareColor(selectedWeapon->Damage - baseWeapon->Damage), strBuf, -1, TEXT_ALIGN_TOPLEFT, COMMON_DZO_DRAW_NORMAL);
  }
  offY += 12;
  // snprintf(strBuf, sizeof(strBuf), "Speed: %.2f", baseWeapon->Speed);
  // gfxHelperDrawText(INVENTORY_DRAW_CENTER_X, INVENTORY_DRAW_CENTER_Y, offX, offY, 0.7, textColor, strBuf, -1, TEXT_ALIGN_TOPLEFT, COMMON_DZO_DRAW_NORMAL);
  // if (hasComparison) {
  //   float strW = gfxGetFontWidth(strBuf, -1, 0.7);
  //   snprintf(strBuf, sizeof(strBuf), "> %.2f", selectedWeapon->Speed);
  //   gfxHelperDrawText(INVENTORY_DRAW_CENTER_X, INVENTORY_DRAW_CENTER_Y, offX + strW + 5, offY, 0.7, inventoryDrawGetCompareColor((selectedWeapon->Speed - baseWeapon->Speed)*100), strBuf, -1, TEXT_ALIGN_TOPLEFT, COMMON_DZO_DRAW_NORMAL);
  // }
  // offY += 12;
  snprintf(strBuf, sizeof(strBuf), "Critical Hit: %.f%%", (baseWeapon->CritChance / 255.0) * 100);
  gfxHelperDrawText(INVENTORY_DRAW_CENTER_X, INVENTORY_DRAW_CENTER_Y, offX, offY, 0.7, textColor, strBuf, -1, TEXT_ALIGN_TOPLEFT, COMMON_DZO_DRAW_NORMAL);
  if (hasComparison) {
    float strW = gfxGetFontWidth(strBuf, -1, 0.7);
    snprintf(strBuf, sizeof(strBuf), "> %.f%%", (selectedWeapon->CritChance / 255.0) * 100);
    gfxHelperDrawText(INVENTORY_DRAW_CENTER_X, INVENTORY_DRAW_CENTER_Y, offX + strW + 5, offY, 0.7, inventoryDrawGetCompareColor(selectedWeapon->CritChance - baseWeapon->CritChance), strBuf, -1, TEXT_ALIGN_TOPLEFT, COMMON_DZO_DRAW_NORMAL);
  }
  offY += 12;
  int baseRarity = bankGetRarityFromQuality(baseWeapon->Quality);
  snprintf(strBuf, sizeof(strBuf), "Rarity: %s", inventoryRarityNames[baseRarity]);
  gfxHelperDrawText(INVENTORY_DRAW_CENTER_X, INVENTORY_DRAW_CENTER_Y, offX, offY, 0.7, textColor, strBuf, -1, TEXT_ALIGN_TOPLEFT, COMMON_DZO_DRAW_NORMAL);
  if (hasComparison) {
    float strW = gfxGetFontWidth(strBuf, -1, 0.7);
    int selRarity = bankGetRarityFromQuality(selectedWeapon->Quality);
    snprintf(strBuf, sizeof(strBuf), "=> %s", inventoryRarityNames[selRarity]);
    gfxHelperDrawText(INVENTORY_DRAW_CENTER_X, INVENTORY_DRAW_CENTER_Y, offX + strW + 5, offY, 0.7, inventoryDrawGetCompareColor(selRarity - baseRarity), strBuf, -1, TEXT_ALIGN_TOPLEFT, COMMON_DZO_DRAW_NORMAL);
  }
  offY += 12;
  snprintf(strBuf, sizeof(strBuf), "Paint: %s", inventoryPaintNames[baseWeapon->Paint]);
  gfxHelperDrawText(INVENTORY_DRAW_CENTER_X, INVENTORY_DRAW_CENTER_Y, offX, offY, 0.7, textColor, strBuf, -1, TEXT_ALIGN_TOPLEFT, COMMON_DZO_DRAW_NORMAL);
  if (hasComparison) {
    float strW = gfxGetFontWidth(strBuf, -1, 0.7);
    snprintf(strBuf, sizeof(strBuf), "> %s", inventoryPaintNames[selectedWeapon->Paint]);
    gfxHelperDrawText(INVENTORY_DRAW_CENTER_X, INVENTORY_DRAW_CENTER_Y, offX + strW + 5, offY, 0.7, textColor, strBuf, -1, TEXT_ALIGN_TOPLEFT, COMMON_DZO_DRAW_NORMAL);
  }
  offY += 12;
  snprintf(strBuf, sizeof(strBuf), "Special: %s", inventoryPaintSpecialNames[baseWeapon->PaintSpecialMask]);
  gfxHelperDrawText(INVENTORY_DRAW_CENTER_X, INVENTORY_DRAW_CENTER_Y, offX, offY, 0.7, textColor, strBuf, -1, TEXT_ALIGN_TOPLEFT, COMMON_DZO_DRAW_NORMAL);
  if (hasComparison) {
    float strW = gfxGetFontWidth(strBuf, -1, 0.7);
    snprintf(strBuf, sizeof(strBuf), "> %s", inventoryPaintSpecialNames[selectedWeapon->PaintSpecialMask]);
    gfxHelperDrawText(INVENTORY_DRAW_CENTER_X, INVENTORY_DRAW_CENTER_Y, offX + strW + 5, offY, 0.7, textColor, strBuf, -1, TEXT_ALIGN_TOPLEFT, COMMON_DZO_DRAW_NORMAL);
  }
  offY += 12;
  snprintf(strBuf, sizeof(strBuf), "Omega: %s", inventoryOmegaNames[baseWeapon->OmegaMod]);
  gfxHelperDrawText(INVENTORY_DRAW_CENTER_X, INVENTORY_DRAW_CENTER_Y, offX, offY, 0.7, textColor, strBuf, -1, TEXT_ALIGN_TOPLEFT, COMMON_DZO_DRAW_NORMAL);
  if (hasComparison) {
    float strW = gfxGetFontWidth(strBuf, -1, 0.7);
    snprintf(strBuf, sizeof(strBuf), "> %s", inventoryOmegaNames[selectedWeapon->OmegaMod]);
    gfxHelperDrawText(INVENTORY_DRAW_CENTER_X, INVENTORY_DRAW_CENTER_Y, offX + strW + 5, offY, 0.7, textColor, strBuf, -1, TEXT_ALIGN_TOPLEFT, COMMON_DZO_DRAW_NORMAL);
  }
  offY += 12;
  offY += 12; // since Speed is hidden

  // alpha mods
  gfxSetupGifPaging(0);
  int i;
  offX = 5;
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
  RaidsPlayerBank_t* localBank = bankGetLocalBank();
  u32 selectedColor = 0x40008080; // yellow
  u32 equippedColor = 0x40000080; // red
  float fw = (INVENTORY_DRAW_WEAPONS_W / INVENTORY_DRAW_WEAPONS_DIM);
  float fh = (INVENTORY_DRAW_WEAPONS_H / INVENTORY_DRAW_WEAPONS_DIM);
  float w = fw - INVENTORY_DRAW_WEAPONS_M*2.0;
  float h = fh - INVENTORY_DRAW_WEAPONS_M*2.0;
  float offX = -(INVENTORY_DRAW_FULL_W/2.0) + fw/2 + col*fw;
  float offY = -(INVENTORY_DRAW_WEAPONS_H/2.0) + fh/2 + row*fh ;
  int idx = row*8 + col;
  int isSelected = idx == drawState->SelectedIdx;

  // draw bg on first item
  if (col == 0 && row == 0) {
    gfxHelperDrawBox(INVENTORY_DRAW_CENTER_X, INVENTORY_DRAW_CENTER_Y, -(INVENTORY_DRAW_WEAPONS_W/2.0), 0, INVENTORY_DRAW_WEAPONS_W, INVENTORY_DRAW_WEAPONS_H, 0x60101010, TEXT_ALIGN_MIDDLECENTER, COMMON_DZO_DRAW_NORMAL);
  }

  // draw box
  if (isSelected) {
    gfxHelperDrawBox(INVENTORY_DRAW_CENTER_X, INVENTORY_DRAW_CENTER_Y, offX, offY, fw, fh, selectedColor, TEXT_ALIGN_MIDDLECENTER, COMMON_DZO_DRAW_NORMAL);
  }

  // no weapon info
  if (!weapon) return;

  // seen
  if (isSelected && weapon->Notify == RAIDS_WEAPON_NOTIFY_NEW) {
    weapon->Notify = RAIDS_WEAPON_NOTIFY_NONE;
    inventorySetFilter(drawState->FilterIdx);
  }
  
  // draw equipped
  int isEquipped = inventoryFilterMapping[idx] == localBank->Inventory.EquippedWeaponIdxs[bankGetEquipSlotFromGadgetId(weapon->GadgetId)];
  if (isEquipped) {
    gfxHelperDrawBox(INVENTORY_DRAW_CENTER_X, INVENTORY_DRAW_CENTER_Y, offX, offY, fw, fh, equippedColor, TEXT_ALIGN_MIDDLECENTER, COMMON_DZO_DRAW_NORMAL);
  }

  int slotId = weaponIdToSlot(weapon->GadgetId);
  int iconSpriteId = inventoryWeaponSpriteIds[slotId];
  int iconSpriteDim = inventoryWeaponSpriteDims[slotId];
  
  // draw icon
  u32 iconColor = bankRarityColors[bankGetRarityFromQuality(weapon->Quality)];
  gfxSetupGifPaging(0);
  gfxHelperDrawSprite(INVENTORY_DRAW_CENTER_X, INVENTORY_DRAW_CENTER_Y, offX-1, offY-1, w+2, h+2, iconSpriteDim, iconSpriteDim, iconSpriteId, 0x80000000, TEXT_ALIGN_MIDDLECENTER, COMMON_DZO_DRAW_NORMAL);
  gfxHelperDrawSprite(INVENTORY_DRAW_CENTER_X, INVENTORY_DRAW_CENTER_Y, offX, offY, w, h, iconSpriteDim, iconSpriteDim, iconSpriteId, iconColor, TEXT_ALIGN_MIDDLECENTER, COMMON_DZO_DRAW_NORMAL);

  // draw equipped
  if (isEquipped) {
    //gfxHelperDrawSprite(INVENTORY_DRAW_CENTER_X, INVENTORY_DRAW_CENTER_Y, offX-w/2, offY-h/2, 12, 12, 32, 32, 80, equippedColor, TEXT_ALIGN_MIDDLECENTER, COMMON_DZO_DRAW_NORMAL);
  }

  // draw notify
  if (weapon->Notify) {
    u32 notifyColor = weapon->Notify == RAIDS_WEAPON_NOTIFY_NEW ? INVENTORY_NOTIFY_NEW_COLOR : INVENTORY_NOTIFY_FAV_COLOR;
    gfxHelperDrawSprite(INVENTORY_DRAW_CENTER_X, INVENTORY_DRAW_CENTER_Y, offX+w/2+1, offY-h/2-1, 8, 8, 32, 32, INVENTORY_NOTIFY_SPRITE_ID, notifyColor, TEXT_ALIGN_MIDDLECENTER, COMMON_DZO_DRAW_NORMAL);
  }

  // draw paint
  if (weapon->Paint || weapon->PaintSpecialMask) {
    gfxHelperDrawSprite(INVENTORY_DRAW_CENTER_X, INVENTORY_DRAW_CENTER_Y, offX-w/2+1, offY+h/2-1, 8, 8, 32, 32, 80 + (weapon->PaintSpecialMask>0?1:0), bankPaintColors[weapon->Paint] | 0x80000000, TEXT_ALIGN_MIDDLECENTER, COMMON_DZO_DRAW_NORMAL);
  }

  // draw omega
  if (weapon->OmegaMod) {
    u32 omegaColor = ((u32 (*)(int))0x00541fd0)(weapon->OmegaMod);
    gfxHelperDrawSprite(INVENTORY_DRAW_CENTER_X, INVENTORY_DRAW_CENTER_Y, offX+w/2-2, offY+h/2-2, 12, 12, 32, 32, 79, omegaColor | 0x80000000, TEXT_ALIGN_MIDDLECENTER, COMMON_DZO_DRAW_NORMAL);
  }

  gfxDoGifPaging();
}

//--------------------------------------------------------------------------
void inventoryDrawWeapons(InventoryDrawState_t* drawState)
{
  int i,j;
  char strBuf[128];

  if (!bankGetHasInventory()) {
    inventoryDrawSpinner(drawState, -INVENTORY_DRAW_WEAPONS_W/2.0, 0);
    return;
  }

  // draw tabs
  gfxSetupGifPaging(0);
  for (i = 0; i < INVENTORY_TAB_COUNT; ++i) {
    int isTabSelected = inventoryDrawState.FilterIdx == i;
    float tabW = (INVENTORY_DRAW_WEAPONS_W / INVENTORY_TAB_COUNT);
    float tabH = tabW-2;
    float tabD = tabW - INVENTORY_TAB_SPRITE_PADDING;
    float offX = (tabW * i);
    int spriteId = inventoryWeaponSpriteIds[i];
    int spriteDim = inventoryWeaponSpriteDims[i];
    gfxHelperDrawBox(INVENTORY_DRAW_CENTER_X, INVENTORY_DRAW_CENTER_Y, -(INVENTORY_DRAW_FULL_W/2) + offX, -(INVENTORY_DRAW_FULL_H/2), tabW, tabH, isTabSelected ? 0x40004040 : 0x40101010, TEXT_ALIGN_TOPLEFT, COMMON_DZO_DRAW_NORMAL);
    gfxHelperDrawSprite(INVENTORY_DRAW_CENTER_X, INVENTORY_DRAW_CENTER_Y, -(INVENTORY_DRAW_FULL_W/2) + offX + (INVENTORY_TAB_SPRITE_PADDING/2), -(INVENTORY_DRAW_FULL_H/2)+2, tabD, tabD, spriteDim, spriteDim, spriteId, isTabSelected ? 0x80FFFFFF : 0x80808080, TEXT_ALIGN_TOPLEFT, COMMON_DZO_DRAW_NORMAL);
    if (inventoryTabHasNew[i]) {
      gfxHelperDrawSprite(INVENTORY_DRAW_CENTER_X, INVENTORY_DRAW_CENTER_Y, -(INVENTORY_DRAW_FULL_W/2) + offX + tabW, -(INVENTORY_DRAW_FULL_H/2), 8, 8, 32, 32, INVENTORY_NOTIFY_SPRITE_ID, INVENTORY_NOTIFY_NEW_COLOR, TEXT_ALIGN_TOPRIGHT, COMMON_DZO_DRAW_NORMAL);
    }
    snprintf(strBuf, sizeof(strBuf), "%d", inventoryTabCounts[i]);
    gfxHelperDrawText(INVENTORY_DRAW_CENTER_X, INVENTORY_DRAW_CENTER_Y, -(INVENTORY_DRAW_FULL_W/2) + offX + tabW, -(INVENTORY_DRAW_FULL_H/2)+2+tabD, 0.6, 0x80FFFFFF, strBuf, -1, TEXT_ALIGN_BOTTOMRIGHT, COMMON_DZO_DRAW_NORMAL);
  }
  gfxDoGifPaging();

  // draw grid
  for (i = 0; i < INVENTORY_DRAW_WEAPONS_DIM; ++i) {
    for (j = 0; j < INVENTORY_DRAW_WEAPONS_DIM; ++j) {
      int idx = (i*INVENTORY_DRAW_WEAPONS_DIM)+j;
      int remappedIdx = inventoryFilterMapping[idx];
      RaidsInventoryWeapon_t* weapon = bankGetLocalWeaponFromBank(remappedIdx);
      inventoryDrawWeapon(drawState, i, j, weapon);
    }
  }

  inventoryDrawWeaponInfo(drawState);
}

//--------------------------------------------------------------------------
void inventoryDrawFooter(InventoryDrawState_t* drawState)
{
  u32 textColor = 0x80FFFFFF;
  char strBuf[128];
  int canEquip = 0;
  int selectedTooStrong = 0;
  int canSell = 0;
  int isLoading = !bankGetHasInventory();
  u32 sellPrice = 0;

  RaidsPlayerBank_t* localBank = bankGetLocalBank();
  RaidsInventoryWeapon_t* selectedWeapon = bankGetLocalWeaponFromBank(inventoryFilterMapping[inventoryDrawState.SelectedIdx]);
  RaidsInventoryWeapon_t* equippedWeapon = NULL;
  if (selectedWeapon) {
    int accountProf = getProficiencyFromXp(localBank->Account.WeaponXp[bankGetEquipSlotFromGadgetId(selectedWeapon->GadgetId)]);
    equippedWeapon = bankGetLocalEquippedWeapon(selectedWeapon->GadgetId);
    selectedTooStrong = selectedWeapon->GadgetId && selectedWeapon->Proficiency > accountProf;
    canEquip = !selectedTooStrong && selectedWeapon->GadgetId && equippedWeapon != selectedWeapon; // already equipped
    canSell = selectedWeapon->GadgetId && equippedWeapon != selectedWeapon && selectedWeapon->Notify != RAIDS_WEAPON_NOTIFY_FAV; // can't sell equipped
    sellPrice = getPriceForWeapon(selectedWeapon->Proficiency, selectedWeapon->Quality);
  }
  
  // draw footer text
  char sellPriceStrBuf[32];
  strBuf[0] = 0;
  if (!isLoading) {
    strcat(strBuf, "\x14 \x15 FILTER    ");
    if (canEquip) strcat(strBuf, "\x10 EQUIP    ");
    if (selectedTooStrong) { snprintf(sellPriceStrBuf, sizeof(sellPriceStrBuf), "MUST BE P%d TO EQUIP    ", selectedWeapon->Proficiency+1); strcat(strBuf, sellPriceStrBuf); }
    if (selectedWeapon) strcat(strBuf, "\x11 FAV    ");
    if (canSell) { strcat(strBuf, "\x13 SELL    "); }
  }
  strcat(strBuf, "\x12 CLOSE");
  gfxHelperDrawText(INVENTORY_DRAW_CENTER_X, INVENTORY_DRAW_CENTER_Y, -INVENTORY_DRAW_FULL_W/2 + 5, INVENTORY_DRAW_FULL_H/2 - 5, 0.8, textColor, strBuf, -1, TEXT_ALIGN_BOTTOMLEFT, COMMON_DZO_DRAW_NORMAL);
}

//--------------------------------------------------------------------------
void inventoryDraw(void)
{
  u32 bgColor = 0x60000000;
  u32 borderColor = 0x80000020;
  u32 textColor = 0x80FFFFFF;
  float borderSizeH = INVENTORY_DRAW_FRAME_BORDER_W * SCREEN_RATIO_INV;
  float borderSizeV = INVENTORY_DRAW_FRAME_BORDER_W;

  GameSettings* gs = gameGetSettings();
  Player* localPlayer = playerGetFromSlot(0);
  RaidsPlayerBank_t* localBank = bankGetLocalBank();
  RaidsInventoryWeapon_t* selectedWeapon = bankGetLocalWeaponFromBank(inventoryFilterMapping[inventoryDrawState.SelectedIdx]);
  RaidsInventoryWeapon_t* equippedWeapon = NULL;
  int isLoadingInventory = bankHasPendingInventoryRequest();
  int isLoadingAccount = bankHasPendingAccountRequest();
  int canEquip = 0;
  int selectedTooStrong = 0;
  int canSell = 0;
  if (selectedWeapon) {
    int accountProf = getProficiencyFromXp(localBank->Account.WeaponXp[bankGetEquipSlotFromGadgetId(selectedWeapon->GadgetId)]);
    equippedWeapon = bankGetLocalEquippedWeapon(selectedWeapon->GadgetId);
    selectedTooStrong = selectedWeapon->GadgetId && selectedWeapon->Proficiency > accountProf;
    canEquip = !selectedTooStrong && selectedWeapon->GadgetId && equippedWeapon != selectedWeapon; // already equipped
    canSell = selectedWeapon->GadgetId && equippedWeapon != selectedWeapon && selectedWeapon->Notify != RAIDS_WEAPON_NOTIFY_FAV; // can't sell equipped
  }

#if DEBUG
  canEquip = selectedWeapon != NULL && equippedWeapon != selectedWeapon;
#endif

  // bad state
  if ((!isLoadingInventory && !bankGetHasInventory()) || (!isLoadingAccount && !bankGetHasAccount())) {
    inventoryClose();
    return;
  }

  // reset filter when inventory changes while menu is open
  if (localBank->Inventory.RefreshLocalInventory) {
    inventorySetFilter(inventoryDrawState.FilterIdx);
  }

  // draw frame
  gfxHelperDrawBox(INVENTORY_DRAW_CENTER_X, INVENTORY_DRAW_CENTER_Y, 0, 0, INVENTORY_DRAW_FULL_W, INVENTORY_DRAW_FULL_H, bgColor, TEXT_ALIGN_MIDDLECENTER, COMMON_DZO_DRAW_NORMAL);

  // draw title text
  gfxHelperDrawText(INVENTORY_DRAW_CENTER_X, INVENTORY_DRAW_CENTER_Y, INVENTORY_DRAW_INFO_W/2, -INVENTORY_DRAW_FULL_H/2 + 2, 1.1, textColor, gs->PlayerNames[localPlayer->PlayerId], -1, TEXT_ALIGN_TOPCENTER, COMMON_DZO_DRAW_NORMAL);
  
  // draw inventory section
  inventoryDrawWeapons(&inventoryDrawState);

  // draw account section
  inventoryDrawAccountInfo(&inventoryDrawState);

  // draw footer
  inventoryDrawFooter(&inventoryDrawState);

  // draw frame borders
  gfxHelperDrawBox(INVENTORY_DRAW_CENTER_X, INVENTORY_DRAW_CENTER_Y, INVENTORY_DRAW_FULL_W/2, 0, borderSizeH, INVENTORY_DRAW_FULL_H + borderSizeV, borderColor, TEXT_ALIGN_MIDDLECENTER, COMMON_DZO_DRAW_NORMAL);
  gfxHelperDrawBox(INVENTORY_DRAW_CENTER_X, INVENTORY_DRAW_CENTER_Y, -INVENTORY_DRAW_FULL_W/2, 0, borderSizeH, INVENTORY_DRAW_FULL_H + borderSizeV, borderColor, TEXT_ALIGN_MIDDLECENTER, COMMON_DZO_DRAW_NORMAL);
  gfxHelperDrawBox(INVENTORY_DRAW_CENTER_X, INVENTORY_DRAW_CENTER_Y, 0, INVENTORY_DRAW_FULL_H/2, INVENTORY_DRAW_FULL_W + borderSizeH, borderSizeV, borderColor, TEXT_ALIGN_MIDDLECENTER, COMMON_DZO_DRAW_NORMAL);
  gfxHelperDrawBox(INVENTORY_DRAW_CENTER_X, INVENTORY_DRAW_CENTER_Y, 0, -INVENTORY_DRAW_FULL_H/2, INVENTORY_DRAW_FULL_W + borderSizeH, borderSizeV, borderColor, TEXT_ALIGN_MIDDLECENTER, COMMON_DZO_DRAW_NORMAL);

  // draw sell dialog
  if (inventoryDrawState.ShowSellDialog) {
        
    // invalid
    if (!selectedWeapon) {
      inventoryDrawState.ShowSellDialog = 0;
      return;
    }

    u32 sellPrice = getPriceForWeapon(selectedWeapon->Proficiency, selectedWeapon->Quality);
    int rarity = bankGetRarityFromQuality(selectedWeapon->Quality);
    struct GadgetDef* gadgetDef = weaponGetDef(selectedWeapon->GadgetId, 0);
    char sellStrBuf[64];
    char sellPriceBuf[64];
    snprintf(sellStrBuf, sizeof(sellStrBuf), "Sell %s?", uiMsgString(rarity == RAIDS_WEAPON_RARITY_LEGENDARY ? gadgetDef->upgQSTag : gadgetDef->quickSelectTag));
    snprintf(sellPriceBuf, sizeof(sellPriceBuf), "\x0A%'d", sellPrice);
    int selResult = inventoryDrawDialog(&inventoryDrawState, sellStrBuf, sellPriceBuf);

    if (selResult == 1) {
      bankSellLocalWeaponAtIndex(inventoryFilterMapping[inventoryDrawState.SelectedIdx]);
      inventoryDrawState.ShowSellDialog = 0;
    } else if (selResult == 0) {
      inventoryDrawState.ShowSellDialog = 0;
    }
    return;
  }

  // handle close input
  if (State.MenuOpen == RAIDS_CUSTOM_MENU_INVENTORY && (gameIsAnyStartMenuOpen() || padGetButtonDown(0, PAD_TRIANGLE) > 0)) {
    inventoryClose();
  }

  // handle input
  if (bankGetHasInventory()) {
    int selIdx = inventoryDrawState.SelectedIdx;
    if (padGetButtonDown(0, PAD_LEFT) > 0) {                              // NAV LEFT
      int row = (selIdx-1)%INVENTORY_DRAW_WEAPONS_DIM;
      if (row < 0 || row > (selIdx%INVENTORY_DRAW_WEAPONS_DIM)) selIdx += INVENTORY_DRAW_WEAPONS_DIM-1;
      else selIdx--;
    } else if (padGetButtonDown(0, PAD_RIGHT) > 0) {                      // NAV RIGHT
      int row = (selIdx+1)%INVENTORY_DRAW_WEAPONS_DIM;
      if (row < (selIdx%INVENTORY_DRAW_WEAPONS_DIM)) selIdx -= INVENTORY_DRAW_WEAPONS_DIM-1;
      else selIdx++;
    } else if (padGetButtonDown(0, PAD_DOWN) > 0) {                       // NAV DOWN
      selIdx = (selIdx+INVENTORY_DRAW_WEAPONS_DIM) % BANK_MAX_WEAPONS;
    } else if (padGetButtonDown(0, PAD_UP) > 0) {                         // NAV UP
      selIdx = (selIdx-INVENTORY_DRAW_WEAPONS_DIM) % BANK_MAX_WEAPONS;
      if (selIdx < 0) selIdx += BANK_MAX_WEAPONS;
    } else if (padGetButtonDown(0, PAD_L1) > 0) {                         // TAB LEFT
      inventoryDrawState.FilterIdx--;
      if (inventoryDrawState.FilterIdx < 0) inventoryDrawState.FilterIdx = INVENTORY_TAB_COUNT - 1;
      inventorySetFilter(inventoryDrawState.FilterIdx);
    } else if (padGetButtonDown(0, PAD_R1) > 0) {                         // TAB RIGHT
      inventoryDrawState.FilterIdx = (inventoryDrawState.FilterIdx + 1) % INVENTORY_TAB_COUNT;
      inventorySetFilter(inventoryDrawState.FilterIdx);
    } else if (canEquip && padGetButtonDown(0, PAD_CROSS) > 0) {          // EQUIP
      bankEquipLocalWeaponAtIndex(inventoryFilterMapping[selIdx]);
    } else if (canSell && padGetButtonDown(0, PAD_SQUARE) > 0) {          // SELL
      inventoryDrawState.ShowSellDialog = 1;
    } else if (selectedWeapon && padGetButtonDown(0, PAD_CIRCLE) > 0) {   // FAVORITE
      if (selectedWeapon->Notify == RAIDS_WEAPON_NOTIFY_FAV) selectedWeapon->Notify = RAIDS_WEAPON_NOTIFY_NONE;
      else selectedWeapon->Notify = RAIDS_WEAPON_NOTIFY_FAV;
    }
    inventoryDrawState.SelectedIdx = selIdx;
  }
  
  //gfxSetupGifPaging(0);
  //gfxHelperDrawSprite(60, SCREEN_HEIGHT - 60, 0, 0, 100, 100, 64, 64, aaa, 0x80808080, TEXT_ALIGN_MIDDLECENTER, COMMON_DZO_DRAW_NORMAL);
  //gfxDoGifPaging();
}

//--------------------------------------------------------------------------
void inventoryTick(void)
{
  // check if we need to request our bank
  if (gameHasEnded()) {
    inventoryClose();
    return;
  }
  
  // draw
  if (State.MenuOpen == RAIDS_CUSTOM_MENU_NONE && PATCH_POINTERS_PATCHMENU == 0 && !gameIsAnyStartMenuOpen() && padGetButtonDown(0, PAD_LEFT) > 0) {
    inventoryOpen();
  } else if (State.MenuOpen == RAIDS_CUSTOM_MENU_INVENTORY) {
    inventoryDraw();
  }
}

//--------------------------------------------------------------------------
void inventoryInit(void)
{

}
