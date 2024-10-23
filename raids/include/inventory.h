#ifndef RAIDS_INVENTORY_H
#define RAIDS_INVENTORY_H

#include <tamtypes.h>
#include <libdl/moby.h>
#include <libdl/math.h>
#include <libdl/time.h>
#include <libdl/player.h>
#include <libdl/math3d.h>

#define INVENTORY_BANK_MAX_WEAPONS                 (64)
#define INVENTORY_BANK_UPDATE_WEAPONS_SIZE         (16)

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
  char Notify;
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

struct RaidsGetBankRequest
{
  u32 DestAddress;
};

struct RaidsUpdateBankInventoryRequest
{
  int Index;
  int Count;
  RaidsInventoryWeapon_t Weapons[INVENTORY_BANK_UPDATE_WEAPONS_SIZE];
  char EquippedWeaponIdxs[WEAPON_SLOT_COUNT-1];
};

u32 inventoryGetBolts(void);
u32 inventoryAddBolts(u32 amount);
u64 inventoryGetXP(void);
u64 inventoryAddXP(u64 amount);

void inventoryRequestFromServer(void);
void inventorySendToServer(void);

RaidsPlayerBank_t* inventoryGetLocalBank(void);
RaidsInventoryWeapon_t* inventoryGetLocalWeaponFromBank(int index);
void inventoryEquipLocalWeaponAtIndex(int weaponIdx);
void inventorySellLocalWeaponAtIndex(int weaponIdx);
RaidsInventoryWeapon_t* inventoryGetLocalEquippedWeapon(int gadgetId);
RaidsPlayerEquippedInventory_t* inventoryGetEquippedFromGadgetBox(GadgetBox* gbox);
RaidsInventoryWeapon_t* inventoryGetEquippedWeaponFromGadgetBox(GadgetBox* gbox, int gadgetId);
enum RaidsWeaponRarity inventoryGetRarityFromQuality(u8 quality);

void inventoryOpen(void);
void inventoryClose(void);

void inventoryTick(void);
void inventoryInit(void);

#endif // RAIDS_INVENTORY_H
