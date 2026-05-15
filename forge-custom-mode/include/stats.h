#ifndef FORGE_CGM_STATS_H
#define FORGE_CGM_STATS_H

#include <tamtypes.h>
#include "game.h"

void statsFormatValue(char* dest, int size, int value, int format);
void statsBroadcastSync(void);
void statsInit(void);

extern struct CgmStats Stats;

#endif // FORGE_CGM_STATS_H
