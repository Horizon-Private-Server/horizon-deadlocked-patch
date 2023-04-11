

#ifndef SURVIVAL_UTILS_H
#define SURVIVAL_UTILS_H

#include <tamtypes.h>
#include <libdl/moby.h>
#include <libdl/math.h>
#include <libdl/time.h>
#include <libdl/player.h>
#include <libdl/sound.h>

void setFreeze(int isActive);
void setDoublePoints(int isActive);
void setDoubleXP(int isActive);
void playerRevive(Player* player, int fromPlayerId);
short playerGetWeaponAmmo(Player* player, int weaponId);
int playerGetWeaponAlphaModCount(Player* player, int weaponId, int alphaMod);
Moby * spawnExplosion(VECTOR position, float size, u32 color);
void playUpgradeSound(Player* player);
void playPaidSound(Player* player);
int getWeaponIdFromOClass(short oclass);
u8 decTimerU8(u8* timeValue);
u16 decTimerU16(u16* timeValue);
u32 decTimerU32(u32* timeValue);
u32 getXpForNextToken(int counter);
void drawDreadTokenIcon(float x, float y, float scale);
struct PartInstance * spawnParticle(VECTOR position, u32 color, char opacity, int idx);
void destroyParticle(struct PartInstance* particle);

int intArrayContains(int* list, int count, int value);
int charArrayContains(char* list, int count, char value);

void vectorProjectOnVertical(VECTOR output, VECTOR input0);
void vectorProjectOnHorizontal(VECTOR output, VECTOR input0);
float getSignedSlope(VECTOR forward, VECTOR normal);

int mobyIsMob(Moby* moby);

#endif // SURVIVAL_UTILS_H
