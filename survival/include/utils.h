

#ifndef SURVIVAL_UTILS_H
#define SURVIVAL_UTILS_H

#include <tamtypes.h>
#include <libdl/moby.h>
#include <libdl/math.h>
#include <libdl/time.h>
#include <libdl/player.h>
#include <libdl/sound.h>

void setFreeze(int isActive);
void setDoublePointsForTeam(int team, int isActive);
void playerRevive(Player* player, int fromPlayerId);
short playerGetWeaponAmmo(Player* player, int weaponId);
int playerGetWeaponAlphaModCount(Player* player, int weaponId, int alphaMod);
Moby * spawnExplosion(VECTOR position, float size);
void playUpgradeSound(Player* player);
int getWeaponIdFromOClass(short oclass);
u8 decTimerU8(u8* timeValue);
u16 decTimerU16(u16* timeValue);
u32 decTimerU32(u32* timeValue);

#endif // SURVIVAL_UTILS_H
