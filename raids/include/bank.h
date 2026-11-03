#ifndef RAIDS_BANK_H
#define RAIDS_BANK_H

//--------------------------------------------------------------------------
// includes
//--------------------------------------------------------------------------
#include <tamtypes.h>
#include <stddef.h>
#include <libdl/moby.h>
#include <libdl/math.h>
#include <libdl/time.h>
#include <libdl/player.h>
#include <libdl/math3d.h>
#include "classes.h"

//--------------------------------------------------------------------------
// defines
//--------------------------------------------------------------------------
// inventory
#define BANK_MAX_ITEMS                 (64)
#define BANK_UPDATE_SIZE               (16)

// badges
#define BANK_BADGE_EFFECT_COUNT        (8)
#define BADGE_HEALTH_BUFF_AMOUNT       (200)
#define BADGE_AMMO_MOD_BUFF_AMOUNT     (6)
#define BADGE_AREA_MOD_BUFF_AMOUNT     (3)
#define BADGE_SPEED_MOD_BUFF_AMOUNT    (4)
#define BADGE_IMPACT_MOD_BUFF_AMOUNT   (4)

// classes
#define BANK_MAX_CLASSES               (RAIDS_MAX_CLASSES)
#define BANK_MAX_CLASS_UNLOCK_RULES    (RAIDS_MAX_CLASS_UNLOCK_RULES)
#define BANK_MAX_CLASS_SKILLS          (RAIDS_MAX_CLASS_SKILLS)

// contracts
#define BANK_MAX_CONTRACTS (3)
#define RAIDS_CONTRACT_MAX_RULES (16)
#define RAIDS_CONTRACT_LIFETIME_SECONDS (21600)

//--------------------------------------------------------------------------
// enums
//--------------------------------------------------------------------------
// inventory
enum RaidsItemTypes
{
  RAIDS_ITEM_NONE = 0,
  RAIDS_ITEM_WEAPON,
  RAIDS_ITEM_BADGE,
};

enum RaidsItemRarity
{
  RAIDS_ITEM_RARITY_COMMON = 0,
  RAIDS_ITEM_RARITY_UNCOMMON,
  RAIDS_ITEM_RARITY_RARE,
  RAIDS_ITEM_RARITY_LEGENDARY,
  RAIDS_ITEM_RARITY_MYTHIC,
  RAIDS_ITEM_RARITY_COUNT,
};

enum RaidsInventoryItemNotify
{
  RAIDS_ITEM_NOTIFY_NONE = 0,
  RAIDS_ITEM_NOTIFY_NEW,
  RAIDS_ITEM_NOTIFY_FAV,
};

enum RaidsItemUpdateAction
{
  RAIDS_ITEM_UPDATE_NONE = 0,
  RAIDS_ITEM_UPDATE_SELL,
  RAIDS_ITEM_UPDATE_SET_NOTIFY,
  RAIDS_ITEM_UPDATE_EQUIP,
  RAIDS_ITEM_UPDATE_UNEQUIP,
  RAIDS_ITEM_UPDATE_UPGRADE,
  RAIDS_ITEM_UPDATE_UPGRADE_RARITY,
  RAIDS_ITEM_UPDATE_DESTROY,
};

// weapons
enum RaidsGadgetPaintSpecialMask
{
  RAIDS_GADGET_PAINTSPECIAL_NONE = 0,
  RAIDS_GADGET_PAINTSPECIAL_GLOW = 0x01,
  RAIDS_GADGET_PAINTSPECIAL_ADDITIVE = 0x02,
};

enum RaidsWeaponModType
{
  RAIDS_WEAPON_MOD_NONE = 0,
  RAIDS_WEAPON_MOD_NAPALM,
  RAIDS_WEAPON_MOD_TIME_BOMB,
  RAIDS_WEAPON_MOD_FREEZE,
  RAIDS_WEAPON_MOD_MINI_BOMB,
  RAIDS_WEAPON_MOD_MORPH,
  RAIDS_WEAPON_MOD_BRAINWASH,
  RAIDS_WEAPON_MOD_ACID,
  RAIDS_WEAPON_MOD_SHOCK,
  RAIDS_WEAPON_MOD_WILL_O_WISP,
  RAIDS_WEAPON_MOD_LIGHTFOOT,
  RAIDS_WEAPON_MOD_COUNT
};

// badges
enum RaidsBadgeType
{
  RAIDS_BADGE_TYPE_NONE = 0,
  RAIDS_BADGE_TYPE_HEALTH_REGEN,
  RAIDS_BADGE_TYPE_AMMO_REGEN,
  RAIDS_BADGE_TYPE_SHARPSHOOTER,
  RAIDS_BADGE_TYPE_BERSERKER,
  RAIDS_BADGE_TYPE_FLINCH_RESISTANCE,
  RAIDS_BADGE_TYPE_HEALTH_BUFF,
  RAIDS_BADGE_TYPE_ALPHA_AMMO_BUFF,
  RAIDS_BADGE_TYPE_ALPHA_AREA_BUFF,
  RAIDS_BADGE_TYPE_ALPHA_SPEED_BUFF,
  RAIDS_BADGE_TYPE_ALPHA_IMPACT_BUFF,
  RAIDS_BADGE_TYPE_EXPLODING_ENEMIES,
  RAIDS_BADGE_TYPE_COUNT
};

// contracts
enum RaidsContractType
{
  RAIDS_CONTRACT_KILL = 0,
  RAIDS_CONTRACT_CLEAR = 1,
};

enum RaidsContractFlags
{
  RAIDS_CONTRACT_COMPLETE = 0x01,
  RAIDS_CONTRACT_EXPIRED = 0x02,
  RAIDS_CONTRACT_CLAIMABLE = 0x04,
  RAIDS_CONTRACT_CLAIMED = 0x08,
  RAIDS_CONTRACT_SPEED_BONUS = 0x10,
};

enum RaidsContractRequestFlags
{
  RAIDS_CONTRACT_REQUEST_PENDING = 0x02,
};

enum RaidsContractAction
{
  RAIDS_CONTRACT_ACTION_REROLL = 0,
  RAIDS_CONTRACT_ACTION_CLAIM = 1,
  RAIDS_CONTRACT_ACTION_DISMISS_EXPIRED = 3,
  RAIDS_CONTRACT_ACTION_CLAIM_PARTY_REWARDS = 4,
};

//--------------------------------------------------------------------------
// structs
//--------------------------------------------------------------------------
// inventory
typedef struct RaidsInventoryItem
{
  char Type;
  char Notify;
  u8 Quality; // determines rarity + values on probability curve
  u32 Price;
  u32 Uid;

  union {
    struct {
      int Damage; // damage
      u8 GadgetId;
      u8 Paint : 4; // 0=none, 1=blue, etc (teams)
      u8 PaintSpecialMask : 4; // RaidsGadgetPaintSpecialMask
      u8 Proficiency; // what proficiency the item was created at (v1-v99)
      u8 CritChance; // 0-255 (0-100%) chance crit
      u8 ModType; // RaidsWeaponModType
      u8 ModQuality; // 0-255
      u8 AlphaModCounts[ALPHA_MOD_COUNT-1];
      u8 Upgrades;
      u8 MaxUpgrades;
    } WeaponData;

    struct {
      u8 Effects[BANK_BADGE_EFFECT_COUNT];
      u8 EffectStrength[BANK_BADGE_EFFECT_COUNT];
    } BadgeData;
  };
} RaidsInventoryItem_t;

typedef struct RaidsPlayerInventoryPage
{
  RaidsInventoryItem_t Items[BANK_MAX_ITEMS];
  u32 Total;
  u16 TotalByFilter[9];
  u16 FilterHasNewMask;
  int RefreshLocalInventory;
  int Filter;
  int Page;
  int Sort;
  int HasFlag;
  long LastRequestTime;
} RaidsPlayerInventoryPage_t;

typedef struct RaidsPlayerEquippedInventory
{
  RaidsInventoryItem_t Items[WEAPON_SLOT_COUNT-1];
  RaidsInventoryItem_t Badge;
  int RefreshLocalInventory;
} RaidsPlayerEquippedInventory_t;

struct RaidsGetBankRequest
{
  u32 DestAddress;
  u32 DestHasFlagAddress;
  u32 DestTimeFlagAddress;
  int Filter;
  int Page;
};

struct RaidsGetBankInventoryRequest
{
  u32 DestAddress;
  u32 DestHasFlagAddress;
  u32 DestTimeFlagAddress;
  int Filter;
  int Page;
  int Sort;
};

struct RaidsUpdateBankInventoryItemRequest
{
  RaidsInventoryItem_t Item;
  char Action;
};

struct RaidsUpdateBankInventoryRequest
{
  int Index;
  int Count;
  RaidsInventoryItem_t Items[BANK_UPDATE_SIZE];
  char EquippedBadgeIdx;
  char EquippedWeaponIdxs[WEAPON_SLOT_COUNT-1];
};

struct RaidsBankSetPlayerEquippedInventoryMsg
{
  int ClientId;
  RaidsPlayerEquippedInventory_t EquippedInventory;
};

// account progression
typedef struct RaidsPlayerAccount
{
  u64 Experience;
  u32 Bolts;
  int EquippedClass;
  u64 ClassXp[BANK_MAX_CLASSES];
  double WeaponExperience;
} RaidsPlayerAccount_t;

struct RaidsGetBankAccountRequest
{
  u32 DestAddress;
  u32 DestHasFlagAddress;
  u32 DestTimeFlagAddress;
  u32 AccountSize;
};

struct RaidsBankSetPlayerAccountMsg
{
  int ClientId;
  int Level;
  u32 Bolts;
};

typedef char RaidsAccountSizeCheck[sizeof(RaidsPlayerAccount_t) == 152 ? 1 : -1];

// classes
typedef struct RaidsPlayerClassSkill
{
  u8 SkillId;
  u8 AppliedRank;
} RaidsPlayerClassSkill_t;

typedef struct RaidsPlayerClassProgress
{
  int ClassType;
  RaidsPlayerClassSkill_t Skills[BANK_MAX_CLASS_SKILLS];
} RaidsPlayerClassProgress_t;

struct RaidsGetEquippedClassRequest
{
  u32 DestAddress;
  u32 DestHasFlagAddress;
  u32 DestTimeFlagAddress;
};

struct RaidsGetClassesRequest
{
  u32 DestAddress;
  u32 DestHasFlagAddress;
  u32 DestTimeFlagAddress;
  int Offset;
  int Count;
};

struct RaidsClassSkillRequest
{
  int ClassType;
  int SkillId;
};

struct RaidsBankSetPlayerEquippedClassMsg
{
  int ClientId;
  int AccountId;
  RaidsPlayerClassProgress_t EquippedClassProgress;
  u64 EquippedClassXp;
};

// contracts
typedef struct RaidsContractReward
{
  u32 Bolts;
  u32 PlayerXp;
  u32 WeaponXp;
  u32 ClassXp;
} RaidsContractReward_t;

typedef struct RaidsContractDifficulty
{
  u16 TargetTimeSeconds[5];
  u16 Reserved;
  RaidsContractReward_t TotalReward;
  float BonusRewardFactor;
} RaidsContractDifficulty_t;

// table records share this header, followed by their type's payload
struct RaidsContractRuleHeader
{
  u16 Type;
  u16 MobDefMask;
  char Name[32];
};

struct RaidsKillContractRule
{
  float XpMult;
  float BoltMult;
  float ClassXpPerMob;
  u16 MinCount;
  u16 MaxCount;
  u32 MinHeroLevel;
  u32 CountIncrement;
  float RewardRamp;
  float PartyHeroLevelRamp;
};

typedef struct RaidsContract
{
  u32 Uid;
  u32 OwnerAccountId;
  RaidsContractReward_t Reward;
  union {
    struct {
      u32 RequiredKills;
      u32 Kills;
    } Kill;

    struct {
      u32 CompletedTimeMs;
      u32 SpeedTargetMs;
      RaidsContractReward_t SpeedBonus;
      float BonusRewardFactor;
    } Completion;
  };
  u16 MobDefMask;
  u16 RemainingSeconds;
  u8 Type;
  u8 Flags;
  s8 CompletedDifficulty;
  s8 CompletedClassType;
  char Name[32];
} RaidsContract_t;

typedef struct RaidsContractPage
{
  u32 BoardId; // per-map id in owner's account, 0 if no board
  u32 OwnerAccountId;
  u32 Sequence; // host progress counter used to discard stale updates
  u32 RequestId; // echoed page request id used to discard stale responses
  u16 PendingCount; // total unclaimed rewards for this player across all maps
  u8 Difficulty;
  u8 Count;
  char MapFilename[64];
  char MapName[32];
  RaidsContract_t Contracts[BANK_MAX_CONTRACTS];
} RaidsContractPage_t;

struct RaidsGetContractsRequest
{
  u32 RequestId;
  u16 PendingOffset;
  u8 Difficulty;
  u8 Flags;
  char MapFilename[64];
  u32 PartySize;
};

struct RaidsContractProgress
{
  u32 ContractUid;
  union {
    u32 Kills;
    u32 CompletedTimeMs;
  };
  u8 Flags;
  s8 Difficulty;
  u8 PartySize;
  u8 Reserved;
};

struct RaidsUpdateContractStatsRequest
{
  u32 BoardId;
  u32 OwnerAccountId;
  u32 Sequence;
  u32 Count;
  struct RaidsContractProgress Contracts[BANK_MAX_CONTRACTS];
};

struct RaidsActionContractRequest
{
  u32 ContractUid;
  u32 OwnerAccountId;
  u32 RequestId;
  u32 Action;
};

typedef char RaidsContractRuleHeaderSizeCheck[sizeof(struct RaidsContractRuleHeader) == 36 ? 1 : -1];
typedef char RaidsKillContractRuleSizeCheck[sizeof(struct RaidsKillContractRule) == 32 ? 1 : -1];
typedef char RaidsContractRewardSizeCheck[sizeof(RaidsContractReward_t) == 16 ? 1 : -1];
typedef char RaidsContractDifficultySizeCheck[sizeof(RaidsContractDifficulty_t) == 32 ? 1 : -1];
typedef char RaidsContractSizeCheck[sizeof(RaidsContract_t) == 92 ? 1 : -1];
typedef char RaidsContractDataOffsetCheck[offsetof(RaidsContract_t, Kill) == 24 && offsetof(RaidsContract_t, Completion) == 24 ? 1 : -1];
typedef char RaidsContractNameOffsetCheck[offsetof(RaidsContract_t, Name) == 60 ? 1 : -1];
typedef char RaidsContractPageSizeCheck[sizeof(RaidsContractPage_t) == 392 ? 1 : -1];
typedef char RaidsContractRequestSizeCheck[sizeof(struct RaidsGetContractsRequest) == 76 ? 1 : -1];
typedef char RaidsContractProgressEntrySizeCheck[sizeof(struct RaidsContractProgress) == 12 ? 1 : -1];
typedef char RaidsContractProgressDataOffsetCheck[offsetof(struct RaidsContractProgress, Kills) == 4 && offsetof(struct RaidsContractProgress, CompletedTimeMs) == 4 ? 1 : -1];
typedef char RaidsContractProgressSizeCheck[sizeof(struct RaidsUpdateContractStatsRequest) == 52 ? 1 : -1];
typedef char RaidsContractActionSizeCheck[sizeof(struct RaidsActionContractRequest) == 16 ? 1 : -1];

// map progression
struct RaidsBankMapStats
{
  int Invalid;
  int CollectiblesCount;
  u32 CollectiblesMask;
  int ChallengesCount;
  u32 ChallengesMask;
  float PercentageComplete;
  u32 BestTimeMsPerDifficulty[5];
  char MapFilename[64];
};

struct RaidsBankGetMapStatsRequest
{
  u32 ResponseAddress;
  int CollectiblesCount;
  int ChallengesCount;
  int MissionType;
  char MapFilename[64];
  char MapName[32];
};

struct RaidsBankSetMapStatsRequest
{
  int CollectiblesCount;
  u32 CollectiblesMask;
  int ChallengesCount;
  u32 ChallengesMask;
  char MapFilename[64];
};

// local bank
typedef struct RaidsPlayerBank
{
  RaidsPlayerAccount_t Account;
  RaidsPlayerClassProgress_t EquippedClassProgress;
  RaidsPlayerEquippedInventory_t EquippedInventory;
  RaidsContractPage_t ContractPage;
} RaidsPlayerBank_t;

//--------------------------------------------------------------------------
// bank vtable and function types
//--------------------------------------------------------------------------
// local bank
typedef RaidsPlayerBank_t* (*BankGetLocalBank_func)(void);

// inventory
typedef void (*BankGetItemName_func)(RaidsInventoryItem_t* item, char* buf, int bufSize);
typedef enum RaidsItemRarity (*BankGetRarityFromQuality_func)(int quality);
typedef void (*BankRequestInventoryFromServer_func)(RaidsPlayerInventoryPage_t* inventory, int filter, int page, int sort);
typedef void (*BankSendInventoryItemToServer_func)(RaidsInventoryItem_t* item, enum RaidsItemUpdateAction action);
typedef void (*BankRequestEquippedInventoryFromServer_func)(void);
typedef int (*BankGetHasEquippedInventory_func)(void);
typedef int (*BankHasPendingEquippedInventoryRequest_func)(void);

// weapons
typedef RaidsInventoryItem_t* (*BankGetEquippedWeaponFromGadgetBox_func)(GadgetBox* gbox, int gadgetId);
typedef enum RaidsItemRarity (*BankGetEquippedWeaponModRarity_func)(int playerId, int gadgetId, enum RaidsWeaponModType modType);
typedef double (*BankGetWeaponXP_func)(void);
typedef double (*BankAddWeaponXP_func)(double amt);

// badges
typedef float (*BankGetEquippedBadgeEffectStrength_func)(int playerId, enum RaidsBadgeType effect);

// account progression
typedef void (*BankRequestAccountFromServer_func)(void);
typedef void (*BankSendAccountToServer_func)(void);
typedef int (*BankGetHasAccount_func)(void);
typedef int (*BankHasPendingAccountRequest_func)(void);
typedef u64 (*BankGetXP_func)(void);
typedef u64 (*BankAddXP_func)(u64 amt);
typedef int (*BankGetLevel_func)(void);
typedef u32 (*BankGetBolts_func)(void);
typedef u32 (*BankAddBolts_func)(u32 amt);
typedef u32 (*BankSubBolts_func)(u32 amt);

// classes
typedef RaidsPlayerClassProgress_t* (*BankGetPlayerEquippedClass_func)(int playerId);
typedef int (*BankGetPlayerClassSkillRank_func)(int playerId, enum RaidsClassSkillId skillId);
typedef float (*BankGetPlayerClassSkillMultiplier_func)(int playerId, enum RaidsClassSkillId skillId);
typedef void (*BankRequestEquippedClassProgressFromServer_func)(void);
typedef void (*BankRequestAllClassProgressFromServer_func)(void);
typedef int (*BankSpendClassSkillPoint_func)(enum RaidsClassSkillId skillId);
typedef int (*BankResetClassSkills_func)(void);
typedef int (*BankGetHasEquippedClassProgress_func)(void);
typedef int (*BankHasPendingEquippedClassProgressRequest_func)(void);
typedef int (*BankGetHasAllClassProgress_func)(void);
typedef int (*BankHasPendingAllClassProgressRequest_func)(void);
typedef u64 (*BankGetClassXP_func)(int classType);
typedef u64 (*BankAddClassXP_func)(u64 amount, int classType);
typedef int (*BankGetClassLevel_func)(RaidsPlayerClassProgress_t* playerClass, u64 xp);
typedef const RaidsClassDef_t* (*BankGetClassDefinition_func)(int classType);

// contracts
typedef void (*BankRequestContractsFromServer_func)(void);
typedef void (*BankSendContractStatsToServer_func)(void);
typedef int (*BankGetHasContracts_func)(void);
typedef int (*BankHasPendingContractsRequest_func)(void);

// map progression
typedef void (*BankRequestMapStats_func)(char* mapFilename, char* mapName, struct RaidsBankMapStats* dest, int missionType);

// keep shared callback offsets stable
struct BankVTable
{
  // bank, class and inventory access
  BankGetLocalBank_func GetLocalBank;
  BankGetPlayerEquippedClass_func GetPlayerEquippedClass;
  BankGetPlayerClassSkillRank_func GetPlayerClassSkillRank;
  BankGetPlayerClassSkillMultiplier_func GetPlayerClassSkillMultiplier;
  BankGetItemName_func GetItemName;
  BankGetRarityFromQuality_func GetRarityFromQuality;
  BankGetEquippedBadgeEffectStrength_func GetEquippedBadgeEffectStrength;
  BankGetEquippedWeaponModRarity_func GetEquippedWeaponModRarity;
  BankGetEquippedWeaponFromGadgetBox_func GetEquippedWeaponFromGadgetBox;

  // server requests
  BankRequestInventoryFromServer_func RequestInventoryFromServer;
  BankSendInventoryItemToServer_func SendInventoryItemToServer;
  BankRequestEquippedInventoryFromServer_func RequestEquippedInventoryFromServer;
  BankRequestAccountFromServer_func RequestAccountFromServer;
  BankSendAccountToServer_func SendAccountToServer;
  BankRequestEquippedClassProgressFromServer_func RequestEquippedClassProgressFromServer;
  BankRequestAllClassProgressFromServer_func RequestAllClassProgressFromServer;
  BankSpendClassSkillPoint_func SpendClassSkillPoint;
  BankResetClassSkills_func ResetClassSkills;
  BankRequestContractsFromServer_func RequestContractsFromServer;
  BankSendContractStatsToServer_func SendContractStatsToServer;
  BankRequestMapStats_func RequestMapStats;

  // request state
  BankGetHasEquippedInventory_func GetHasEquippedInventory;
  BankHasPendingEquippedInventoryRequest_func HasPendingEquippedInventoryRequest;
  BankGetHasAccount_func GetHasAccount;
  BankHasPendingAccountRequest_func HasPendingAccountRequest;
  BankGetHasEquippedClassProgress_func GetHasEquippedClassProgress;
  BankHasPendingEquippedClassProgressRequest_func HasPendingEquippedClassProgressRequest;
  BankGetHasAllClassProgress_func GetHasAllClassProgress;
  BankHasPendingAllClassProgressRequest_func HasPendingAllClassProgressRequest;
  BankGetHasContracts_func GetHasContracts;
  BankHasPendingContractsRequest_func HasPendingContractsRequest;

  // account, weapon and class progression
  BankGetXP_func GetXP;
  BankAddXP_func AddXP;
  BankGetLevel_func GetLevel;
  BankGetBolts_func GetBolts;
  BankAddBolts_func AddBolts;
  BankSubBolts_func SubBolts;
  BankGetWeaponXP_func GetWeaponXP;
  BankAddWeaponXP_func AddWeaponXP;
  BankGetClassXP_func GetClassXP;
  BankAddClassXP_func AddClassXP;
  BankGetClassLevel_func GetClassLevel;
  BankGetClassDefinition_func GetClassDefinition;
};

#endif // RAIDS_BANK_H
