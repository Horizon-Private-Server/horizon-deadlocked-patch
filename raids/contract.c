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
#include "config.h"
#include "common.h"

struct ContractStatCache {
  u32 ContractUid;
  u32 Kills;
};

struct ContractState {
  int SendStatsCounter[BANK_MAX_CONTRACTS];
  struct ContractStatCache StatCache[BANK_MAX_CONTRACTS];
} contractState;

//--------------------------------------------------------------------------
int contractCheckForCompletion(int contractIdx)
{
  if (!hasMapConfig() || !mapConfig->BankVTable) return 0;
  
  RaidsPlayerBank_t* bank = mapConfig->BankVTable->GetLocalBank();
  if (!bank) return 0;
    
  RaidsContract_t* contract = &bank->Contracts[contractIdx];
  if (contract->Uid == 0 || !contract->Activated) return 0;

  // raid completed in time
  if (contract->RequiredRaidTimeMs > 0 && contract->CompletedTimeMs > 0 && contract->CompletedTimeMs < contract->RequiredRaidTimeMs)
    return 1;

  // kills reached
  if (contract->RequiredKills > 0 && contract->Kills >= contract->RequiredKills)
    return 1;

  return 0;
}

//--------------------------------------------------------------------------
void contractHandleKill(int mobOClass, int gadgetId, u32 weaponXp, u32 playerXp)
{
  if (!hasMapConfig() || !mapConfig->BankVTable) return;
  
  RaidsPlayerBank_t* bank = mapConfig->BankVTable->GetLocalBank();
  if (!bank) return;

  int i;
  for (i = 0; i < BANK_MAX_CONTRACTS; ++i) {
    RaidsContract_t* contract = &bank->Contracts[i];
    if (contract->Uid == 0) continue;
    if (!contract->Activated) continue;
    if (contract->ExpiresInMinutes <= 0) continue; // expired

    // already complete
    if (contractCheckForCompletion(i)) continue;

    // check map
    if (contract->MapFilename[0] && strncmp(contract->MapFilename, State.CurrentMapDef->Filename, sizeof(State.CurrentMapDef->Filename)) != 0) continue;

    // check difficulty
    //if (contract->RequiredDifficultyStars >= 0 && contract->RequiredDifficultyStars != State.DifficultyStars) continue;

    // check mob
    if (contract->RequiredKillsMobOClass != 0 && contract->RequiredKillsMobOClass != mobOClass) continue;

    // check weapon
    if (contract->RequiredKillsGadgetId != 0 && contract->RequiredKillsGadgetId != gadgetId) continue;

    // update cache
    struct ContractStatCache* cache = &contractState.StatCache[i];
    if (cache->ContractUid != contract->Uid) {
      cache->ContractUid = contract->Uid;
      cache->Kills = contract->Kills;
    }

    // record
    contract->Kills = ++cache->Kills;
    
    // delay sending stats
    // so that we don't send on every kill
    //mapConfig->BankVTable->SendContractStatsToServer(contract);
    if (contractState.SendStatsCounter[i] == 0) {
      contractState.SendStatsCounter[i] = TPS * 3;
    }

    // check for completion
    if (contractCheckForCompletion(i)) {
      char buf[64];
      snprintf(buf, sizeof(buf), "Contract #%d Completed", i + 1);
      playUpgradeSound(playerGetFromSlot(0));
      pushSnack(buf, 240, 0);
    }
  }
}

//--------------------------------------------------------------------------
void contractMissionComplete(int timeMs)
{
  if (!hasMapConfig() || !mapConfig->BankVTable) return;
  if (timeMs <= 0) return;
  
  RaidsPlayerBank_t* bank = mapConfig->BankVTable->GetLocalBank();
  if (!bank) return;

  int i;
  for (i = 0; i < BANK_MAX_CONTRACTS; ++i) {
    RaidsContract_t* contract = &bank->Contracts[i];
    if (contract->Uid == 0) continue;
    if (!contract->Activated) continue;
    if (contract->ExpiresInMinutes <= 0) continue; // expired

    // check map
    if (contract->MapFilename[0] && strncmp(contract->MapFilename, State.CurrentMapDef->Filename, sizeof(State.CurrentMapDef->Filename)) != 0) continue;

    // check difficulty
    if (contract->RequiredDifficultyStars >= 0 && contract->RequiredDifficultyStars != State.DifficultyStars) continue;

    // check time
    if (contract->RequiredRaidTimeMs > 0 && timeMs < contract->RequiredRaidTimeMs) {
      contract->CompletedTimeMs = (u32)timeMs;
      mapConfig->BankVTable->SendContractStatsToServer(contract);

      char buf[64];
      snprintf(buf, sizeof(buf), "Contract #%d Completed", i + 1);
      pushSnack(buf, 120, 0);
    }
  }
}

//--------------------------------------------------------------------------
void contractTick(void)
{
  if (!hasMapConfig() || !mapConfig->BankVTable) return;

  RaidsPlayerBank_t* bank = mapConfig->BankVTable->GetLocalBank();
  if (!bank) return;

  int i;
  for (i = 0; i < BANK_MAX_CONTRACTS; ++i) {
    if (contractState.SendStatsCounter[i] > 0) {
      if (--contractState.SendStatsCounter[i] == 0) {
        
        RaidsContract_t* contract = &bank->Contracts[i];
        if (contract->Uid == 0) continue;

        // send stats
        mapConfig->BankVTable->SendContractStatsToServer(contract);
      }
    }
  }
}
