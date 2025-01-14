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

#define RA_COUNTERS_SIZE  (64)

#define GUBER_BACKUP_SIZE                            (64)
#define GUBER_BACKUP_START_AT                        (10)

typedef struct RollingValue {
  float Value;
  float BackValue;
  float Low;
  float High;
  int BackTicks;
} RollingValue_t;

typedef struct RACounter {
  u32 Address;
  int Count;
  int Ticks;
} RACounter_t;

#if LOG_PERF_STATS
RollingValue_t statsGuberEventAlloc_Calls = { .Low = -1 };
RollingValue_t statsCollLineFix_Calls = { .Low = -1 };
RACounter_t statsCollLineFix_RA[RA_COUNTERS_SIZE] = {};
#endif

GuberEvent statsGuberSafeBackup[GUBER_BACKUP_SIZE];
int statsGuberSafeBackupIdx = 0;

//--------------------------------------------------------------------------
int statsGuberEventBackupPop(void)
{
  if (statsGuberSafeBackupIdx <= 0) return 0;

  GuberEvent* event = guberEventAlloc();
  if (!event) return 0;

  DPRINTF("statsGuberEventBackupPop() %d\n", statsGuberSafeBackupIdx);
  memcpy(event, &statsGuberSafeBackup[0], 0x58); // copy all but NextEvent ptr
  memmove(&statsGuberSafeBackup[0], &statsGuberSafeBackup[1], sizeof(GuberEvent) * (statsGuberSafeBackupIdx - 1));
  --statsGuberSafeBackupIdx;
  return 1;
}

//--------------------------------------------------------------------------
GuberEvent* statsGuberEventAllocSafe(void)
{
  // max allocated is 256
  // we want to avoid spamming clients with guber events
  // so backup old events
  int free = guberCountFreeEvents();
  while (free > GUBER_BACKUP_START_AT && statsGuberEventBackupPop()) free--;
  if (free <= GUBER_BACKUP_START_AT) {

    DPRINTF("statsGuberEventAllocSafe() called with %d free GuberEvents\n", free);

    // we don't have enough space, try to pop the latest event off the queue
    // if that fails then abort (return null)
    if (statsGuberSafeBackupIdx >= (GUBER_BACKUP_SIZE-1) && !statsGuberEventBackupPop())
      return NULL;

    GuberEvent* gEvent = &statsGuberSafeBackup[statsGuberSafeBackupIdx++];
    memset(gEvent, 0, sizeof(GuberEvent));
    return gEvent;
  }

  return guberEventAlloc();
}


//--------------------------------------------------------------------------
int statsRollingValueTick(char* name, RollingValue_t* rollingValue, int rolloverAtTicks)
{
  rollingValue->BackTicks++;
  if (rollingValue->BackTicks >= rolloverAtTicks) {
    float normalizedValue = (TPS * rollingValue->BackValue) / (float)rollingValue->BackTicks;
    if (normalizedValue > rollingValue->High) rollingValue->High = normalizedValue;
    if (normalizedValue < rollingValue->Low || rollingValue->Low < 0) rollingValue->Low = normalizedValue;
    rollingValue->Value = rollingValue->BackValue;
    printf("%s: %f (%f per sec) | range:<%f,%f>\n", name, rollingValue->Value, normalizedValue, rollingValue->Low, rollingValue->High);
    rollingValue->BackTicks = 0;
    rollingValue->BackValue = 0;
    return 1;
  }

  return 0;
}

//--------------------------------------------------------------------------
void statsRATick(char* name, RACounter_t* counters, int countersSize, int rolloverAtTicks)
{
  int i;
  for (i = 0; i < countersSize; ++i) {
    if (counters[i].Address) {
      ++counters[i].Ticks;
      if (counters[i].Ticks >= rolloverAtTicks) {
        float normalizedValue = (TPS * counters[i].Count) / (float)counters[i].Ticks;
        printf("%s: %08X called %d (%f per sec)\n", name, counters[i].Address, counters[i].Count, normalizedValue);
        memset(&counters[i], 0, sizeof(RACounter_t));
      }
    }
  }
}

//--------------------------------------------------------------------------
void statsCountRA(RACounter_t* counters, int countersSize, u32 address)
{
  int i;
  for (i = 0; i < countersSize; ++i) {
    if (counters[i].Address == address) {
      ++counters[i].Count;
      return;
    }

    if (counters[i].Address == 0) {
      counters[i].Address = address;
      counters[i].Count = 1;
      counters[i].Ticks = 0;
      return;
    }
  }
}

//--------------------------------------------------------------------------
GuberEvent* statsOnGuberAllocEvent(void)
{
#if LOG_PERF_STATS
  statsGuberEventAlloc_Calls.BackValue++;
#endif
  return statsGuberEventAllocSafe();
}

//--------------------------------------------------------------------------
int statsOnCollLineFix(int a0)
{
  volatile void* ra;
	asm volatile (
    "vnop;"
		"sw $ra, 0(%0)"
		: : "r" (&ra)
	);

#if LOG_PERF_STATS
  statsCountRA(statsCollLineFix_RA, RA_COUNTERS_SIZE, (u32)ra);
  statsCollLineFix_Calls.BackValue++;
#endif
  return a0;
}

//--------------------------------------------------------------------------
void statsTick(void)
{
  int free = guberCountFreeEvents();
  while (free > GUBER_BACKUP_START_AT && statsGuberEventBackupPop())
    free--;

#if LOG_PERF_STATS
  //printf("FREE GUBER EVENTS %d\n", guberCountFreeEvents());
  statsRollingValueTick("GuberEvent Alloc", &statsGuberEventAlloc_Calls, TPS);
  statsRollingValueTick("CollLine_Fix", &statsCollLineFix_Calls, TPS);
  statsRATick("CollLine_Fix", statsCollLineFix_RA, RA_COUNTERS_SIZE, TPS);
#endif
}

//--------------------------------------------------------------------------
void statsInit(void)
{
  //printf("FREE GUBER EVENTS %d\n", guberCountFreeEvents());
  
  // guber events
  HOOK_JAL(0x00611360, &statsOnGuberAllocEvent);
  HOOK_JAL(0x006116d4, &statsOnGuberAllocEvent);

#if LOG_PERF_STATS
  // collision
  HOOK_J_OP(0x004b8e28, statsOnCollLineFix, 0x0040202D);
#endif
}
