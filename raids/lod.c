#include <libdl/game.h>
#include <libdl/math.h>
#include <libdl/string.h>
#include <libdl/utils.h>
#include "config.h"
#include "include/game.h"
#include "include/lod.h"
#include "include/settings.h"
#include "include/utils.h"

#define SHRUB_RENDER_DISTANCE (*(float*)0x0022308C)
#define DRAW_SHADOW_FUNC ((u32*)0x00587b30)

extern struct RaidsState State;
extern float _raidsLodScale;
extern void _raidsCorrectTieLod(void);

int raidsLastLod = -1;
short raidsMinShrubRenderDistance = 0;
int raidsForcePotatoLod = 0;

const int raidsLodPatchesPotato[][2] = {
  { 0x0050e318, 0x03E00008 }, // disable corn
  { 0x0050e31c, 0x00000000 },
  { 0x0041d400, 0x03E00008 }, // disable small weapon explosion
  { 0x0041d404, 0x00000000 },
  { 0x003F7154, 0x00000000 }, // disable b6 ball shadow
  { 0x003A18F0, 0x24020000 }, // disable b6 particles
  { 0x0042e9f0, 0x03E00008 }, // disable mag particles
  { 0x0042e9f4, 0x0000102D }, // disable mag particles
  { 0x0043C150, 0x24020000 }, // disable mag shells
  { 0x003A18F0, 0x24020000 }, // disable fusion shot particles
  { 0x0042608C, 0x00000000 }, // disable jump pad blur effect
};

const int raidsLodPatchesNormal[][2] = {
  { 0x0050e318, 0x27BDFE40 }, // enable corn
  { 0x0050e31c, 0x7FB40100 },
  { 0x0041d400, 0x27BDFF00 }, // enable small weapon explosion
  { 0x0041d404, 0x7FB00060 },
  { 0x003F7154, 0x0C140F40 }, // enable b6 ball shadow
  { 0x003A18F0, 0x0C13DC80 }, // enable b6 particles
  { 0x0042e9f0, 0x27BDFF80 }, // enable mag particles
  { 0x0042e9f4, 0x7FB30010 }, // enable mag particles
  { 0x0043C150, 0x0C13DC80 }, // enable mag shells
  { 0x003A18F0, 0x0C13DC80 }, // enable fusion shot particles
  { 0x0042608C, 0x0C131194 }, // enable jump pad blur effect
};


//--------------------------------------------------------------------------
void raidsLodMapInit(void)
{
  raidsLastLod = -1;
  raidsMinShrubRenderDistance = 0;
  raidsForcePotatoLod = 0;
  if (PATCH_INTEROP && PATCH_INTEROP->MapLoaderFilename) {
    raidsForcePotatoLod = strcmp(PATCH_INTEROP->MapLoaderFilename, "canal city") == 0;
    if (PATCH_INTEROP->GetCustomMapDefCount && PATCH_INTEROP->GetCustomMapDef) {
      int i;
      int count = PATCH_INTEROP->GetCustomMapDefCount();
      for (i = 0; i < count; ++i) {
        CustomMapDef_t* def = PATCH_INTEROP->GetCustomMapDef(i);
        if (def && strcmp(def->Filename, PATCH_INTEROP->MapLoaderFilename) == 0) {
          raidsMinShrubRenderDistance = def->ShrubMinRenderDistance;
          break;
        }
      }
    }
  }

  HOOK_J(0x005930b8, &_raidsCorrectTieLod);
  raidsLodTick();
}

//--------------------------------------------------------------------------
void raidsLodTick(void)
{
  if (!isInGame()) return;

  int lod = raidsForcePotatoLod ? 0 : raidsUserSettings.MapLod;
  if (lod == raidsLastLod) return;

  int i;
  int terrainDistance = lod == 0 ? 120 : lod == 1 ? 480 : 960;
  float shrubDistance = lod < 2 ? 50 : lod == 2 ? 500 : 5000;
  _raidsLodScale = lod < 2 ? 0.2 : lod == 2 ? 1.0 : 10.0;
  SHRUB_RENDER_DISTANCE = maxf(raidsMinShrubRenderDistance, shrubDistance);
  DRAW_SHADOW_FUNC[0] = lod < 2 ? 0x03E00008 : 0x27BDFF90;
  DRAW_SHADOW_FUNC[1] = lod < 2 ? 0 : 0xFFB30038;
  POKE_U16(0x00223158, terrainDistance);
  POKE_F32(0x002230f0, terrainDistance * 1024);

  const int (*patches)[2] = lod == 0 ? raidsLodPatchesPotato : raidsLodPatchesNormal;
  int count = lod == 0 ? sizeof(raidsLodPatchesPotato) / sizeof(raidsLodPatchesPotato[0])
    : sizeof(raidsLodPatchesNormal) / sizeof(raidsLodPatchesNormal[0]);
  for (i = 0; i < count; ++i) POKE_U32(patches[i][0], patches[i][1]);
  if (lod == 1) POKE_U32(0x0042608c, 0);
  raidsLastLod = lod;
}
