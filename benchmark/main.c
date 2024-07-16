#include <tamtypes.h>
#include <string.h>

#include <libdl/stdio.h>
#include <libdl/math.h>
#include <libdl/math3d.h>
#include <libdl/time.h>
#include <libdl/game.h>
#include <libdl/gamesettings.h>
#include <libdl/player.h>
#include <libdl/cheats.h>
#include <libdl/sha1.h>
#include <libdl/map.h>
#include <libdl/graphics.h>
#include <libdl/dialog.h>
#include <libdl/common.h>
#include <libdl/pad.h>
#include <libdl/utils.h>
#include <libdl/collision.h>
#include <libdl/ui.h>
#include "module.h"

#define MAX_MOB_COUNT                 (75)
#define MAX_MOB_DRAW_FREE             (55)
#define MOB_OCLASS                    (0x20F6)

enum BenchmarkTest
{
  BTEST_NONE = 0,
  BTEST_FREE,
  BTEST_DRAW,
  BTEST_COLLISION,
  BTEST_MOBY_SPAWN
};

void mobUpdate(Moby* moby);

int CurrentTest = BTEST_NONE;
int UpdatesSinceTestBegan = 0;
int CollisionChecksSinceTestBegan = 0;
int FramesSinceTestBegan = 0;
long TestBegan = 0;
long TestEnd = 0;
int MobsDrawn = 0;
int MobysCreated = 0;
Moby* Mobs[MAX_MOB_COUNT];

void spawnMobs(void)
{
  static int spawned = 0;
  if (spawned) return;

  int i;
  for (i = 0; i < MAX_MOB_COUNT; ++i)
  {
    Moby* moby = Mobs[i] = mobySpawn(MOB_OCLASS, 0);
    DPRINTF("spawn %d %08X\n", i, (u32)moby);
    if (!moby) continue;

    int row = i % 12;
    int col = i / 12;
    moby->Position[0] = 390 + (4 * row);
    moby->Position[1] = 603 + (4 * col);
    moby->Position[2] = 434;
    moby->DrawDist = 0x1FF;
    moby->UpdateDist = -1;
    moby->Bangles = 0x1 | 0x20;
    moby->Bolts = i;
    moby->PUpdate = &mobUpdate;
  }

  spawned = 1;
}

void updateCamera_PostHook(void)
{
  VECTOR CAM_POS = {367,618,445,0};
  float CAM_PITCH = 15 * MATH_DEG2RAD;

  if (CurrentTest != BTEST_DRAW && CurrentTest != BTEST_COLLISION) return;

  int i;
  MATRIX m;
  for (i = 0; i < 1; ++i)
  {
    // get game camera
    GameCamera * camera = cameraGetGameCamera(i);

    // write spectate position
    vector_copy(camera->pos, CAM_POS);
    camera->rot[1] = CAM_PITCH;
    camera->rot[2] = 0;
    camera->rot[0] = 0;

    // create and write spectate rotation matrix
    matrix_unit(m);
    matrix_rotate_y(m, m, CAM_PITCH);
    //matrix_rotate_z(m, m, 0);
    memcpy(camera->uMtx, m, sizeof(float) * 12);

    // update render camera
    ((void (*)(int))0x004b2010)(i);
  }

}

void drawTestName(char* name)
{
  char buf[32];
  char* LODs[] = { "Potato", "Low", "Normal", "High" };
  snprintf(buf, sizeof(buf), "LOD %s", LODs[PATCH_INTEROP->Config->levelOfDetail]);
  gfxScreenSpaceText(10, SCREEN_HEIGHT - 40, 1, 1, 0x80FFFFFF, buf, -1, 0);
  gfxScreenSpaceText(10, SCREEN_HEIGHT - 20, 1, 1, 0x80FFFFFF, name, -1, 0);
}

void drawMobCount(void)
{
  char buf[32];
  snprintf(buf, sizeof(buf), "Mobs: %d", MobsDrawn);
  gfxScreenSpaceText(10, SCREEN_HEIGHT - 80, 1, 1, 0x80FFFFFF, buf, -1, 0);
}

void drawMobysCount(void)
{
  char buf[32];
  snprintf(buf, sizeof(buf), "Mobys: %d", MobysCreated);
  gfxScreenSpaceText(10, SCREEN_HEIGHT - 80, 1, 1, 0x80FFFFFF, buf, -1, 0);
}

void drawScore(int score, int bestScore)
{
  char buf[32];
  snprintf(buf, sizeof(buf), "Score: %d / %d (%.2f%%)", score, bestScore, (100.0 * score) / (float)bestScore);
  gfxScreenSpaceText(10, SCREEN_HEIGHT - 60, 1, 1, 0x80FFFFFF, buf, -1, 0);
}

void drawScoreFloat(float score)
{
  char buf[32];
  snprintf(buf, sizeof(buf), "Score: %.2f", score);
  gfxScreenSpaceText(10, SCREEN_HEIGHT - 60, 1, 1, 0x80FFFFFF, buf, -1, 0);
}

long getMsSinceTestBegan(void)
{
  if (TestEnd) return (TestEnd - TestBegan) / SYSTEM_TIME_TICKS_PER_MS;
  return (timerGetSystemTime() - TestBegan) / SYSTEM_TIME_TICKS_PER_MS;
}

void changeTest(int test)
{
  CurrentTest = test;
  UpdatesSinceTestBegan = 0;
  FramesSinceTestBegan = 0;
  CollisionChecksSinceTestBegan = 0;
  TestBegan = timerGetSystemTime();
  TestEnd = 0;
}

void handleTestNone(void)
{
  drawTestName("No Test");
}

void handleTestFree(void)
{
  drawTestName("Free");

  float s = (int)getMsSinceTestBegan() / 1000.0;
  drawScoreFloat(FramesSinceTestBegan / s);
  drawMobCount();
}

void handleTestDraw(void)
{
  drawTestName("Draw");

  float s = (int)getMsSinceTestBegan() / 1000.0;
  if (!TestEnd && s > 25) TestEnd = timerGetSystemTime();
  drawScore(FramesSinceTestBegan, (int)ceilf(59.94 * s));
  drawMobCount();
}

void handleTestCollision(void)
{
  drawTestName("Collision");

  float s = (int)getMsSinceTestBegan() / 1000.0;
  if (!TestEnd && s > 25) TestEnd = timerGetSystemTime();
  drawScore(FramesSinceTestBegan, (int)ceilf(59.94 * s));
  drawMobCount();
}

void handleTestMobySpawn(void)
{
  // spawn and destroy
  int i;
  for (i = 0; i < 1; ++i) {
    Moby* m = mobySpawn(MOB_OCLASS, 0);
    if (m) { mobyDestroy(m); ++MobysCreated; }
  }

  drawTestName("Moby");
  drawMobysCount();
}

void mobUpdate(Moby* moby)
{
  if (moby->Bolts == 0) {
    MobsDrawn = 0;
    if (!TestEnd) ++UpdatesSinceTestBegan;
  }

  long ms = getMsSinceTestBegan();
  int draw = CurrentTest == BTEST_FREE && moby->Bolts < MAX_MOB_DRAW_FREE;
  if (CurrentTest == BTEST_DRAW || CurrentTest == BTEST_COLLISION)
    draw = moby->Bolts <= (ms / 250);

  moby->DrawDist = draw ? 0x1FF : 0;
  if (moby->AnimSeqId != 3) {
    mobyAnimTransition(moby, 3, 0, 0);
  }

  if (draw && !TestEnd) {
    VECTOR from = {0,0,1,0};
    VECTOR to = {0,0,-1,0};
    vector_add(from, from, moby->Position);
    vector_add(to, to, moby->Position);
    CollLine_Fix(from, to, COLLISION_FLAG_IGNORE_NONE, moby, NULL);
    CollLine_Fix(from, to, COLLISION_FLAG_IGNORE_NONE, moby, NULL);
    CollLine_Fix(from, to, COLLISION_FLAG_IGNORE_NONE, moby, NULL);
    ++CollisionChecksSinceTestBegan;
  }

  if (draw) ++MobsDrawn;
}

/*
 * NAME :		gameStart
 * 
 * DESCRIPTION :
 * 			Thousand kills game logic entrypoint.
 * 
 * NOTES :
 * 			This is called only when in game.
 * 
 * ARGS : 
 * 
 * RETURN :
 * 
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */
void gameStart(struct GameModule * module, PatchStateContainer_t * gameState)
{
	GameSettings * gameSettings = gameGetSettings();

	// Ensure in game
	if (!gameSettings || !isInGame())
		return;

  dlPreUpdate();
  
  HOOK_J(0x004b2428, &updateCamera_PostHook);

  // spawn
  spawnMobs();

  // moonjump
  if (padGetButton(0, PAD_L1) > 0) {
    *(float*)0x00347BD8 = 0.125;
  }

  // select test
  if (!gameIsStartMenuOpen(0)) {
    if (padGetButtonDown(0, PAD_UP) > 0) {
      changeTest(BTEST_NONE);
    } else if (padGetButtonDown(0, PAD_RIGHT) > 0) {
      changeTest(BTEST_FREE);
    } else if (padGetButtonDown(0, PAD_DOWN) > 0) {
      changeTest(BTEST_DRAW);
    } else if (padGetButtonDown(0, PAD_LEFT) > 0) {
      changeTest(BTEST_COLLISION);
    } else if (padGetButtonDown(0, PAD_L3) > 0) {
      changeTest(BTEST_MOBY_SPAWN);
    }
  }

  // draw test
  switch (CurrentTest)
  {
    case BTEST_NONE: handleTestNone(); break;
    case BTEST_FREE: handleTestFree(); break;
    case BTEST_DRAW: handleTestDraw(); break;
    case BTEST_COLLISION: handleTestCollision(); break;
    case BTEST_MOBY_SPAWN: handleTestMobySpawn(); break;
  }

  if (!TestEnd) ++FramesSinceTestBegan;
  dlPostUpdate();
}

/*
 * NAME :		lobbyStart
 * 
 * DESCRIPTION :
 * 			Infected lobby logic entrypoint.
 * 
 * NOTES :
 * 			This is called only when in lobby.
 * 
 * ARGS : 
 * 
 * RETURN :
 * 
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */
void lobbyStart(struct GameModule * module, PatchStateContainer_t * gameState)
{
  // disable ranking
  gameSetIsGameRanked(0);
}

/*
 * NAME :		loadStart
 * 
 * DESCRIPTION :
 * 			Load logic entrypoint.
 * 
 * NOTES :
 * 			This is called only when the game has finished reading the level from the disc and before it has started processing the data.
 * 
 * ARGS : 
 * 
 * RETURN :
 * 
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */
void loadStart(struct GameModule * module, PatchStateContainer_t * gameState)
{
	
}

//--------------------------------------------------------------------------
void start(struct GameModule * module, PatchStateContainer_t * gameState, enum GameModuleContext context)
{
  switch (context)
  {
    case GAMEMODULE_LOBBY: lobbyStart(module, gameState); break;
    case GAMEMODULE_LOAD: loadStart(module, gameState); break;
    case GAMEMODULE_GAME: gameStart(module, gameState); break;
    case GAMEMODULE_GAME_UPDATE: break;
  }
}
