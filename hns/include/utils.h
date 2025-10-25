#ifndef HNS_UTILS_H
#define HNS_UTILS_H

#include <tamtypes.h>
#include <libdl/moby.h>
#include <libdl/math.h>
#include <libdl/time.h>
#include <libdl/player.h>
#include <libdl/sound.h>

Moby * spawnExplosion(VECTOR position, float size);
void playTimerTickSound();
u8 decTimerU8(u8* timeValue);
u16 decTimerU16(u16* timeValue);
u32 decTimerU32(u32* timeValue);
void drawRoundMessage(const char * message, float scale);
Player * playerGetRandom(void);
int spawnPointGetNearestTo(VECTOR point, VECTOR out, float minDist);
int spawnGetRandomPlayerPoint(VECTOR out);
void patchVoidFallCameraBug(Player* player);
void unpatchVoidFallCameraBug(Player* player);
int libcRand(void);

#endif // HNS_UTILS_H
