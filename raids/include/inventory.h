#ifndef RAIDS_INVENTORY_H
#define RAIDS_INVENTORY_H

#include <tamtypes.h>
#include <libdl/moby.h>
#include <libdl/math.h>
#include <libdl/time.h>
#include <libdl/player.h>
#include <libdl/math3d.h>

#define INVENTORY_BANK_MAX_WEAPONS                 (64)

#define INVENTORY_DRAW_CENTER_X                     (SCREEN_WIDTH / 2.0)
#define INVENTORY_DRAW_CENTER_Y                     (SCREEN_HEIGHT / 2.0)
#define INVENTORY_DRAW_FULL_W                       (SCREEN_WIDTH - 100)
#define INVENTORY_DRAW_FULL_H                       (SCREEN_HEIGHT - 170)
#define INVENTORY_DRAW_FRAME_BORDER_W               (2)

#define INVENTORY_DRAW_WEAPONS_DIM                  (8)
#define INVENTORY_DRAW_WEAPONS_M                    (4)
#define INVENTORY_DRAW_WEAPONS_W                    (INVENTORY_DRAW_FULL_W / 2.0)
#define INVENTORY_DRAW_WEAPONS_H                    (INVENTORY_DRAW_WEAPONS_W)

#define INVENTORY_DRAW_INFO_W                       (INVENTORY_DRAW_FULL_W - (INVENTORY_DRAW_WEAPONS_W))
#define INVENTORY_DRAW_INFO_H                       (INVENTORY_DRAW_INFO_W)

enum RaidsGadgetPaintSpecialMask
{
  RAIDS_GADGET_PAINTSPECIAL_NONE = 0,
  RAIDS_GADGET_PAINTSPECIAL_GLOW = 0x01,
  RAIDS_GADGET_PAINTSPECIAL_ADDITIVE = 0x02,
};

enum RaidsWeaponRarity
{
  RAIDS_WEAPON_RARITY_COMMON = 0,
  RAIDS_WEAPON_RARITY_UNCOMMON,
  RAIDS_WEAPON_RARITY_RARE,
  RAIDS_WEAPON_RARITY_LEGENDARY,
  RAIDS_WEAPON_RARITY_COUNT,
};

enum RaidsSkills
{
  RAIDS_SKILLS_HEALTH = 0,
  RAIDS_SKILLS_DAMAGE = 1,
  RAIDS_SKILLS_SPEED = 2,
  RAIDS_SKILLS_UNUSED = 3,
  RAIDS_SKILLS_COUNT
};

typedef struct InventoryDrawState
{
  int SelectedIdx;
} InventoryDrawState_t;

typedef struct RaidsInventoryWeapon
{
  int Damage; // damage
  float Speed;  // speed of projectile
  u8 GadgetId;
  u8 Paint; // 0=none, 1=blue, etc (teams)
  u8 PaintSpecialMask; // RaidsGadgetPaintSpecialMask
  u8 Proficiency; // what proficiency the item was created at (v1-v99)
  u8 Quality; // determines rarity + values on probability curve
  u8 CritChance; // 0-255 (0-100%) chance crit
  u8 OmegaMod;
  u8 AlphaModCounts[ALPHA_MOD_COUNT-1];
} RaidsInventoryWeapon_t;

typedef struct RaidsPlayerInventory
{
  RaidsInventoryWeapon_t Weapons[INVENTORY_BANK_MAX_WEAPONS];
  u32 TotalWeapons;
  char EquippedWeaponIdxs[WEAPON_SLOT_COUNT-1];
} RaidsPlayerInventory_t;

typedef struct RaidsPlayerAccount
{
  u64 Experience;
  u32 Bolts;
  u32 SkillPoints;
  u16 Skills[RAIDS_SKILLS_COUNT];
  u8 Proficiency[WEAPON_SLOT_COUNT-1];
} RaidsPlayerAccount_t;

typedef struct RaidsPlayerBank
{
  RaidsPlayerInventory_t Inventory;
  RaidsPlayerAccount_t Account;
  int RefreshLocalInventory;
} RaidsPlayerBank_t;

typedef struct RaidsPlayerEquippedInventory
{
  RaidsInventoryWeapon_t Weapons[WEAPON_SLOT_COUNT-1];
} RaidsPlayerEquippedInventory_t;

u32 inventoryGetBolts(void);
u32 inventoryAddBolts(u32 amount);
u64 inventoryGetXP(void);
u64 inventoryAddXP(u64 amount);

RaidsPlayerBank_t* inventoryGetLocalWeaponBank(void);
RaidsPlayerEquippedInventory_t* inventoryGetEquippedFromGadgetBox(GadgetBox* gbox);
RaidsInventoryWeapon_t* inventoryGetEquippedWeaponFromGadgetBox(GadgetBox* gbox, int gadgetId);
enum RaidsWeaponRarity inventoryGetRarityFromQuality(u8 quality);

void inventoryOpen(void);
void inventoryClose(void);

void inventoryTick(void);
void inventoryInit(void);

#endif // RAIDS_INVENTORY_H
