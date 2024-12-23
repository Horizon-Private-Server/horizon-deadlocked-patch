

#ifndef RAIDS_UTILS_H
#define RAIDS_UTILS_H

#define SEQ_DIFF_U8(a, b) (((b - a + 128*3) % (128*2)) - 128)

#if DEBUG
  #define DEBUG_ONLY(s) s
#else
  #define DEBUG_ONLY(s) 
#endif

#if RELEASE
  #define RELEASE_ONLY(s) s
#else
  #define RELEASE_ONLY(s) 
#endif

#include <tamtypes.h>
#include <libdl/moby.h>
#include <libdl/math.h>
#include <libdl/time.h>
#include <libdl/player.h>
#include <libdl/sound.h>

struct RaidsInventoryItem;

Moby * spawnExplosion(VECTOR position, float size, u32 color);
void playEquipRejectSound(Player* player);
void playEquipSound(Player* player);
void playUpgradeSound(Player* player);
void playPaidSound(Player* player);
int getWeaponIdFromOClass(short oclass);
u8 decTimerU8(u8* timeValue);
u16 decTimerU16(u16* timeValue);
u32 decTimerU32(u32* timeValue);
int getProficiencyFromXp(u64 xp);
u64 getXpForProficiency(int level);
int getLevelFromXp(u64 xp);
u64 getXpForLevel(int level);
long getAmmoRefillCost(Player* player);
void drawDreadTokenIcon(float x, float y, float scale);
struct PartInstance * spawnParticle(VECTOR position, u32 color, char opacity, int idx);
void destroyParticle(struct PartInstance* particle);

int intArrayContains(int* list, int count, int value);
int charArrayContains(char* list, int count, char value);

void vectorProjectOnVertical(VECTOR output, VECTOR input0);
void vectorProjectOnHorizontal(VECTOR output, VECTOR input0);

int mobyIsMob(Moby* moby);
int mobyIsNpc(Moby* moby);
Player* mobyGetPlayer(Moby* moby);
Moby* playerGetTargetMoby(Player* player);
int localPlayerHasInput(void);

void transformToSplitscreenPixelCoordinates(int localPlayerIndex, float *x, float *y);
void drawStars(float anchorX, float anchorY, float offsetX, float offsetY, float size, float spacing, u32 color, int alignment, int count);
void drawLives(float anchorX, float anchorY, float offsetX, float offsetY, float size, float spacing, u32 color, int alignment, int count);

int hasPendingWorldHop(void);
int isOnHubWorld(void);
int missionIsFailed(void);
int missionIsComplete(void);
int missionIsActive(void);

void pushSnack(char * str, int ticksAlive, int localPlayerIdx);

#endif // RAIDS_UTILS_H
