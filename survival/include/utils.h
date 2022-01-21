

#ifndef SURVIVAL_UTILS_H
#define SURVIVAL_UTILS_H

#include <tamtypes.h>
#include <libdl/moby.h>
#include <libdl/math.h>
#include <libdl/time.h>
#include <libdl/player.h>
#include <libdl/sound.h>

Moby * spawnExplosion(VECTOR position, float size);
void playUpgradeSound(Player* player);
u8 decTimerU8(u8* timeValue);
u16 decTimerU16(u16* timeValue);
u32 decTimerU32(u32* timeValue);

#endif // SURVIVAL_UTILS_H
