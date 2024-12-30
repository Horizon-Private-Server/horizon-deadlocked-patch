#include "include/utils.h"
#include "include/mob.h"
#include "include/game.h"
#include "common.h"
#include <string.h>
#include <libdl/stdio.h>
#include <libdl/game.h>
#include <libdl/collision.h>
#include <libdl/moby.h>
#include <libdl/sound.h>
#include <libdl/random.h>
#include <libdl/graphics.h>

extern struct RaidsState State;
extern struct RaidsMapConfig* mapConfig;

/* 
 * reusable menu sound def
 */
SoundDef MenuSoundDef =
{
	0.0,	// MinRange
	20.0,	// MaxRange
	100,		// MinVolume
	2000,		// MaxVolume
	0,			// MinPitch
	0,			// MaxPitch
	0,			// Loop
	0x10,		// Flags
	19,		  // Index
	3			  // Bank
};

Moby * spawnExplosion(VECTOR position, float size, u32 color)
{
	// SpawnMoby_5025
	Moby * moby = ((Moby* (*)(u128, float, int, int, int, int, int, short, short, short, short, short, short,
				short, short, float, float, float, int, Moby *, int, int, int, int, int, int, int, int,
				int, short, Moby *, Moby *, u128)) (0x003c3b38))
				(vector_read(position), size / 2.5, 0x2, 0x14, 0x10, 0x10, 0x10, 0x10, 0x2, 0, 1, 0, 0,
				0, 0, 0, 0, 2, 0x00080800, 0, color, color, color, color, color, color, color, color,
				color, 0, 0, 0, 0);
				
  mobyPlaySoundByClass(0, 0, moby, MOBY_ID_ARBITER_ROCKET0);

	return moby;
}

void playLevelUpSound(Player* player)
{	
  MenuSoundDef.Index = 44;
	soundPlay(&MenuSoundDef, 0, player->PlayerMoby, 0, 0x400);
}

void playEquipRejectSound(Player* player)
{	
  MenuSoundDef.Index = 27;
	soundPlay(&MenuSoundDef, 0, player->PlayerMoby, 0, 0x400);
}

void playEquipSound(Player* player)
{	
  MenuSoundDef.Index = 19;
	soundPlay(&MenuSoundDef, 0, player->PlayerMoby, 0, 0x400);
}

void playUpgradeSound(Player* player)
{	
  MenuSoundDef.Index = 58;
	soundPlay(&MenuSoundDef, 0, player->PlayerMoby, 0, 0x400);
}

void playPaidSound(Player* player)
{
  MenuSoundDef.Index = 32;
  soundPlay(&MenuSoundDef, 0, player->PlayerMoby, 0, 0x400);
}

int getWeaponIdFromOClass(short oclass)
{
	int weaponId = -1;
	if (oclass > 0) {
		switch (oclass)
		{
			case MOBY_ID_DUAL_VIPER_SHOT: weaponId = WEAPON_ID_VIPERS; break;
			case MOBY_ID_MAGMA_CANNON: weaponId = WEAPON_ID_MAGMA_CANNON; break;
			case MOBY_ID_ARBITER_ROCKET0: weaponId = WEAPON_ID_ARBITER; break;
			case MOBY_ID_FUSION_SHOT: weaponId = WEAPON_ID_FUSION_RIFLE; break;
			case MOBY_ID_MINE_LAUNCHER_MINE: weaponId = WEAPON_ID_MINE_LAUNCHER; break;
			case MOBY_ID_B6_BOMB_EXPLOSION: weaponId = WEAPON_ID_B6; break;
			case MOBY_ID_FLAIL: weaponId = WEAPON_ID_FLAIL; break;
			case MOBY_ID_HOLOSHIELD_LAUNCHER: weaponId = WEAPON_ID_OMNI_SHIELD; break;
			case MOBY_ID_HOLOSHIELD_SHOT: weaponId = WEAPON_ID_OMNI_SHIELD; break;
			case MOBY_ID_WRENCH: weaponId = WEAPON_ID_WRENCH; break;
		}
	}

	return weaponId;
}

u8 decTimerU8(u8* timeValue)
{
	int value = *timeValue;
	if (value == 0)
		return 0;

	*timeValue = --value;
	return value;
}

u16 decTimerU16(u16* timeValue)
{
	int value = *timeValue;
	if (value == 0)
		return 0;

	*timeValue = --value;
	return value;
}

u32 decTimerU32(u32* timeValue)
{
	long value = *timeValue;
	if (value == 0)
		return 0;

	*timeValue = --value;
	return value;
}

//--------------------------------------------------------------------------
int getLevelFromXp(u64 xp)
{
  if (xp < 0) return 0;

  int level = (int)xp / LEVELUP_PLAYER_LINEAR_FACTOR;
  if (level > LEVELUP_MAX_LEVEL) return LEVELUP_MAX_LEVEL;
  if (level < 0) return 0;
  return level;
}

//--------------------------------------------------------------------------
u64 getXpForLevel(int level)
{
  if (level > LEVELUP_MAX_LEVEL) level = LEVELUP_MAX_LEVEL;
  if (level <= 0) return 0;
  
  return level * LEVELUP_PLAYER_LINEAR_FACTOR;
}

//--------------------------------------------------------------------------
int getProficiencyFromXp(u64 xp)
{
  if (xp < 0) return 0;

  // (500 (2/3)^(1/3))/(sqrt(3) sqrt(27 x^2 + 500000000) - 9 x)^(1/3) - (sqrt(3) sqrt(27 x^2 + 500000000) - 9 x)^(1/3)/(2^(1/3) 3^(2/3))
  // Constants
  const double c1 = 0.87358046;                 // (2/3)^(1/3)
  const double c2 = 1.25992104;                 // 2^(1/3)
  const double c3 = 2.08008382;                 // 3^(2/3)
  const double sqrt3 = 1.73205080;              // sqrt(3)

  // Calculate the inner term
  double inner = sqrt3 * sqrt((double)27.0 * xp * xp + 500000000.0) - (double)9.0 * xp;
  
  // Compute the two terms
  double term1 = (double)500.0 * c1 / pow(inner, (double)1.0 / (double)3.0);
  double term2 = pow(inner, (double)1.0 / (double)3.0) / (c2 * c3);

  // Final result
  double level = term1 - term2;
  
  if (level < 0) return 0;
  if (level > LEVELUP_MAX_LEVEL) return LEVELUP_MAX_LEVEL;
  return (int)level;
}

//--------------------------------------------------------------------------
u64 getXpForProficiency(int proficiency)
{
  if (proficiency > LEVELUP_MAX_LEVEL) proficiency = LEVELUP_MAX_LEVEL;
  if (proficiency <= 0) return 0;
  return (u64)((double)powf(1*proficiency, 3) + 500*proficiency);
}

//--------------------------------------------------------------------------
long getAmmoRefillCost(Player* player)
{
  if (!player || !player->GadgetBox) return -1;
    
  float ammoRefillCostPerShot[WEAPON_SLOT_COUNT] = {
    [WEAPON_SLOT_VIPERS] 50,
    [WEAPON_SLOT_MAGMA_CANNON] 200,
    [WEAPON_SLOT_ARBITER] 1000,
    [WEAPON_SLOT_FUSION_RIFLE] 1000,
    [WEAPON_SLOT_MINE_LAUNCHER] 1000,
    [WEAPON_SLOT_B6] 1000,
    [WEAPON_SLOT_OMNI_SHIELD] 1000,
    [WEAPON_SLOT_FLAIL] 500,
  };

  int j;
  u32 cost = 0;
  int needsAmmo = 0;
  for (j = WEAPON_SLOT_VIPERS; j < WEAPON_SLOT_COUNT; ++j) {
    int gadgetId = weaponSlotToId(j);
    int maxAmmo = playerGetWeaponMaxAmmo(player->GadgetBox, gadgetId);
    int ammo = player->GadgetBox->Gadgets[gadgetId].Ammo;
    if (player->GadgetBox->Gadgets[gadgetId].Level >= 0 && ammo < maxAmmo) {
      needsAmmo = 1;
      cost += (u32)(ammoRefillCostPerShot[j] * (maxAmmo - ammo) * State.AmmoRefillCostMultiplier);
    }
  }

  if (!needsAmmo) return -1;

  return cost;
}

//--------------------------------------------------------------------------
void drawDreadTokenIcon(float x, float y, float scale)
{
	float small = scale * 0.75;
	float delta = (scale - small) / 2;

	gfxSetupGifPaging(0);
	u64 dreadzoneSprite = gfxGetFrameTex(32);
	gfxDrawSprite(x+2, y+2, scale, scale, 0, 0, 32, 32, 0x40000000, dreadzoneSprite);
	gfxDrawSprite(x,   y,   scale, scale, 0, 0, 32, 32, 0x80C0C0C0, dreadzoneSprite);
	gfxDrawSprite(x+delta, y+delta, small, small, 0, 0, 32, 32, 0x80000040, dreadzoneSprite);
	gfxDoGifPaging();
}

//--------------------------------------------------------------------------
struct PartInstance * spawnParticle(VECTOR position, u32 color, char opacity, int idx)
{
	u32 a3 = *(u32*)0x002218E8;
	u32 t0 = *(u32*)0x002218E4;
	float f12 = *(float*)0x002218DC;
	float f1 = *(float*)0x002218E0;

	return ((struct PartInstance* (*)(VECTOR, u32, char, u32, u32, int, int, int, float))0x00533308)(position, color, opacity, a3, t0, -1, 0, 0, f12 + (f1 * idx));
}

//--------------------------------------------------------------------------
void destroyParticle(struct PartInstance* particle)
{
	((void (*)(struct PartInstance*))0x005284d8)(particle);
}

//--------------------------------------------------------------------------
int intArrayContains(int* list, int count, int value)
{
	int i;

	for (i = 0; i < count; ++i)
		if (list[i] == value)
			return 1;

	return 0;
}

//--------------------------------------------------------------------------
int charArrayContains(char* list, int count, char value)
{
	int i;

	for (i = 0; i < count; ++i)
		if (list[i] == value)
			return 1;

	return 0;
}

//--------------------------------------------------------------------------
Player* mobyGetPlayer(Moby* moby)
{
  if (!moby) return 0;
  
  Player** players = playerGetAll();
  int i;

  for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
    Player* player = players[i];
    if (!player) continue;

    if (player->PlayerMoby == moby) return player;
    if (player->SkinMoby == moby) return player;
  }

  return NULL;
}

//--------------------------------------------------------------------------
Moby* playerGetTargetMoby(Player* player)
{
  if (!player) return NULL;
  return player->SkinMoby;
}

//--------------------------------------------------------------------------
int localPlayerHasInput(void)
{
  Player* localPlayer = playerGetFromSlot(0);
  if (!localPlayer) return 0;

  return !localPlayer->timers.noInput && !gameIsStartMenuOpen(0);
}

//--------------------------------------------------------------------------
void transformToSplitscreenPixelCoordinates(int localPlayerIndex, float *x, float *y)
{
  int localCount = playerGetNumLocals();

  //
  switch (localCount)
  {
    case 0: // 1 player
    case 1: return;
    case 2: // 2 players
    {
      // vertical split
      *y *= 0.5;
      if (localPlayerIndex == 1)
        *y += 0.5 * SCREEN_HEIGHT;

      break;
    }
    case 3: // 3 players
    {
      // player 1 on top
      // player 2/3 horizontal split on bottom
      *y *= 0.5;
      if (localPlayerIndex > 0) {
        *x *= 0.5;
        *y += 0.5 * SCREEN_HEIGHT;
        if (localPlayerIndex == 2)
          *x += 0.5 * SCREEN_WIDTH;
      }
      break;
    }
    case 4: // 4 players
    {
      // player 1/2 horizontal split on top
      // player 2/3 horizontal split on bottom
      *x *= 0.5;
      *y *= 0.5;
      if ((localPlayerIndex % 2) == 1)
        *x += 0.5 * SCREEN_WIDTH;
      if ((localPlayerIndex / 2) == 1)
        *y += 0.5 * SCREEN_HEIGHT;

      break;
    }
  }
}

//--------------------------------------------------------------------------
int mobyIsMob(Moby* moby)
{
  if (!moby) return 0;
  
  int i;
  if (mapConfig) {
    for (i = 0; i < mapConfig->MobSpawnParamsCount; ++i) {
      if (mapConfig->MobSpawnParams[i].OClass == moby->OClass)
        return 1;
    }
  }

  return moby->OClass == NPC_MOBY_OCLASS;
}

//--------------------------------------------------------------------------
int mobyIsNpc(Moby* moby)
{
  return moby && moby->OClass == NPC_MOBY_OCLASS;
}

//--------------------------------------------------------------------------
int hasPendingWorldHop(void)
{
  return State.PendingWorldHopMapDef && State.PendingWorldHopAtTime > 0;
}

//--------------------------------------------------------------------------
int isOnHubWorld(void)
{
  return State.OnHubWorld;
}

//--------------------------------------------------------------------------
int missionIsFailed(void)
{
  return State.MissionStatus == RAIDS_MISSION_FAILED;
}

//--------------------------------------------------------------------------
int missionIsComplete(void)
{
  return State.MissionStatus == RAIDS_MISSION_COMPLETED;
}

//--------------------------------------------------------------------------
int missionIsActive(void)
{
  return !isOnHubWorld() && !hasPendingWorldHop() && State.MissionStatus == RAIDS_MISSION_ACTIVE;
}

//--------------------------------------------------------------------------
void drawSprites(float anchorX, float anchorY, float offsetX, float offsetY, float size, float spacing, u32 color, int alignment, int count, int texId, int texDim)
{
  if (count <= 0) return;
  
  float w = (size * count) + (spacing * (count - 1));
  helperAlign(&offsetX, &offsetY, w, size, alignment);
  
  gfxSetupGifPaging(0);
  int i;
  for (i = 0; i < count; ++i) {
    gfxHelperDrawSprite(anchorX, anchorY, offsetX + i * (size + spacing), offsetY, size, size, texDim, texDim, texId, color, TEXT_ALIGN_TOPLEFT, COMMON_DZO_DRAW_NORMAL);
  }
  gfxDoGifPaging();
}

//--------------------------------------------------------------------------
void drawStars(float anchorX, float anchorY, float offsetX, float offsetY, float size, float spacing, u32 color, int alignment, int count)
{
  drawSprites(anchorX, anchorY, offsetX, offsetY, size, spacing, color, alignment, count, 88, 32);
}

//--------------------------------------------------------------------------
void drawLives(float anchorX, float anchorY, float offsetX, float offsetY, float size, float spacing, u32 color, int alignment, int count)
{
  drawSprites(anchorX, anchorY, offsetX, offsetY, size, spacing, color, alignment, count, 0, 64);
}

//--------------------------------------------------------------------------
int hasMapConfig(void)
{
  return mapConfig && mapConfig->Magic == MAP_CONFIG_MAGIC;
}
