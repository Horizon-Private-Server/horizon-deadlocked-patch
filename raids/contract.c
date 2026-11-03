#include <libdl/game.h>
#include <libdl/string.h>
#include "include/game.h"
#include "include/contract.h"
#include "include/utils.h"

//--------------------------------------------------------------------------
void contractHandleKill(int mobDefIdx)
{
  if (!hasMapConfig() || !mapConfig->BankVTable || !mapConfig->BankVTable->GetHasContracts()) return;
  if (mobDefIdx < 0 || mobDefIdx >= 16 || mobDefIdx >= mapConfig->MobSpawnParamsCount) return;

  // only track contracts for the loaded map
  RaidsPlayerBank_t* bank = mapConfig->BankVTable->GetLocalBank();
  if (!bank || !State.CurrentMapDef
      || strncmp(bank->ContractPage.MapFilename, State.CurrentMapDef->Filename, 64) != 0) return;

  int i;
  for (i = 0; i < bank->ContractPage.Count; ++i) {
    RaidsContract_t* contract = &bank->ContractPage.Contracts[i];
    if (!contract->Uid || contract->Type != RAIDS_CONTRACT_KILL || !contract->RemainingSeconds
        || (contract->Flags & (RAIDS_CONTRACT_COMPLETE | RAIDS_CONTRACT_EXPIRED))
        || !(contract->MobDefMask & (1u << mobDefIdx))) continue;

    // guests predict kills and wait for the server to confirm completion
    if (contract->Kill.Kills < contract->Kill.RequiredKills) {
      ++contract->Kill.Kills;
    }
    if (gameAmIHost() && contract->Kill.Kills >= contract->Kill.RequiredKills) {
      contract->Flags |= RAIDS_CONTRACT_COMPLETE;
    }
  }
}

//--------------------------------------------------------------------------
void contractMissionComplete(int timeMs)
{
  if (!gameAmIHost() || timeMs <= 0 || !hasMapConfig() || !mapConfig->BankVTable
      || !mapConfig->BankVTable->GetHasContracts()) return;

  // only track contracts for the loaded map
  RaidsPlayerBank_t* bank = mapConfig->BankVTable->GetLocalBank();
  if (!bank || !State.CurrentMapDef
      || strncmp(bank->ContractPage.MapFilename, State.CurrentMapDef->Filename, 64) != 0) return;

  int i;
  for (i = 0; i < bank->ContractPage.Count; ++i) {
    RaidsContract_t* contract = &bank->ContractPage.Contracts[i];
    if (!contract->Uid || contract->Type != RAIDS_CONTRACT_CLEAR || !contract->RemainingSeconds
        || (contract->Flags & (RAIDS_CONTRACT_COMPLETE | RAIDS_CONTRACT_EXPIRED))) continue;

    contract->Completion.CompletedTimeMs = timeMs;
    contract->CompletedDifficulty = State.DifficultyStars;
    contract->Flags |= RAIDS_CONTRACT_COMPLETE;
  }

  // flush completion immediately
  mapConfig->BankVTable->SendContractStatsToServer();
}

//--------------------------------------------------------------------------
void contractTick(void)
{
  // map-owned bank code batches progress and owns its network handlers
}
