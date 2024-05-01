#include <libdl/graphics.h>
#include <libdl/game.h>
#include <libdl/time.h>
#include <libdl/net.h>
#include <libdl/ui.h>
#include <libdl/common.h>
#include <libdl/player.h>
#include <libdl/sound.h>
#include <libdl/stdio.h>
#include <libdl/string.h>
#include <libdl/stdlib.h>
#include <libdl/weapon.h>
#include <libdl/pad.h>
#include <libdl/collision.h>
#include <libdl/color.h>
#include <libdl/utils.h>
#include "messageid.h"
#include "config.h"

extern int isUnloading;
extern PatchGameConfig_t gameConfig;

/*
typedef struct BaseShotSpawnMessage
{
  short GadgetId;
  union {
    char SourcePlayerId : 4;
    char UNK_02;
  };
  char UNK_03;
  int UNK_04;
  int Timestamp;
  u32 TargetUID;
} BaseShotSpawnMessage_t;

typedef struct FusionShotSpawnMessage
{
  BaseShotSpawnMessage_t Base;
  float From[3];
  float Dir[3];
} FusionShotSpawnMessage_t;

int hasHitTicks = 0;

void playSound(int id)
{
	((void (*)(Player*, int, int))0x005eb280)((Player*)0x347AA0, id, 0);
}

void sentFusionShot(int transport, void * connection, long clientIndex, int msgId, int msgSize, void * payload)
{
  if (payload)
  {
    FusionShotSpawnMessage_t * msg = (FusionShotSpawnMessage_t*)payload;
    Player* p = playerGetFromUID(msg->Base.TargetUID);
    if (p) {
      DPRINTF("%d hit player %d\n", msg->Base.SourcePlayerId, p->PlayerId);

      // force shot to hit on remote clients
      //msg->Base.UNK_02 &= 0x0F;

      playSound(66);
      hasHitTicks = 45;
    }
  }

  netBroadcastMediusAppMessage(transport, connection, msgId, msgSize, payload);
}

void sendFusionShotToTarget(int targetPlayerId)
{
  Player* localPlayer = playerGetFromSlot(0);
  Player* targetPlayer = playerGetAll()[targetPlayerId];
  VECTOR dir;

  vector_subtract(dir, targetPlayer->PlayerPosition, localPlayer->PlayerPosition);
  vector_normalize(dir, dir);
  vector_copy(dir, targetPlayer->PlayerPosition);

  FusionShotSpawnMessage_t msg = {
    .Base.GadgetId = 5,
    .Base.UNK_03 = 0x08,
    .Base.UNK_04 = 0,
    .Base.Timestamp = gameGetTime(),
    .Base.TargetUID = targetPlayer->Guber.Id.UID,
  };
  
  msg.Base.UNK_02 = 0x40;
  msg.Base.SourcePlayerId = localPlayer->PlayerId;
  memcpy(msg.From, localPlayer->PlayerPosition, sizeof(float)*3);
  memcpy(msg.Dir, dir, sizeof(float)*3);

  DPRINTF("%08X\n", &sendFusionShotToTarget);
  netBroadcastMediusAppMessage(NET_DELIVERY_CRITICAL, netGetDmeServerConnection(), 14, sizeof(msg), &msg);
}

void runHitmarkerLogic(void)
{
  if (!isInGame())
    return;

  // hook fusion shot
  HOOK_JAL(0x003FCE9C, &sentFusionShot);

  if (padGetButtonDown(0, PAD_CIRCLE) > 0) {
    sendFusionShotToTarget(1);
  }

  if (hasHitTicks)
  {
    --hasHitTicks;

    u32 color = colorLerp(0x00000000, 0x80000000, powf(clamp(hasHitTicks / 30.0, 0, 1), 2));
    float w = 0.01;
    float h = 0.01;
    gfxScreenSpaceBox(0.49 - w/2, 0.49 - h/2, w, h, color);
    gfxScreenSpaceBox(0.49 - w/2, 0.51 - h/2, w, h, color);
    gfxScreenSpaceBox(0.51 - w/2, 0.49 - h/2, w, h, color);
    gfxScreenSpaceBox(0.51 - w/2, 0.51 - h/2, w, h, color);
  }
}

*/

#define FrameJointMatrixStart   ((u32*)0x00091000)
#define TestMoby   (*(Moby**)0x00091004)
#define Header   ((struct KeysHeader*)0x00091010)
#define MaxKeyframes (64)

struct Keyframe {
  float Time;
  void * Data;
};

struct KeysHeader {
  int FrameCount;
  int FrameOffset;
  int SeqId;
  int JointCount;
  Moby* Moby;
  int State;
  struct Keyframe Frames[MaxKeyframes];
};


int active = 0;
int frame = 0;
float lastTime = 0;
u32 currentFrameJointMatrixAddr = 0;

VECTOR b6ExplosionPositions[64];
int b6ExplosionPositionCount = 0;

void setActive(int v) {
  if (v == active)
    return;

  active = v;

  if (active) {
    frame = -1;
    Header->FrameCount = 0;
    Header->FrameOffset = 0;
    Header->JointCount = 0;
    Header->State = 0;
    currentFrameJointMatrixAddr = *FrameJointMatrixStart;
  }
  else {
    Header->State = -1;
    Header->JointCount = 0;
  }
  DPRINTF("active:%d\n", active);
}

void w() {
  if (Header->Moby) {
    int j = 8;
    if (Header->Moby->JointCache && j < Header->Moby->JointCnt) {
      MATRIX m;
      matrix_rotate_x(m, m, MATH_PI / 2);
      memcpy((void*)((u32)Header->Moby->JointCache + 0x40*j), m, sizeof(float)*12);
    }
  }
}

u32 h(u32 a0, u32 a1, u32 a2) {
  //w();
  u32 v = ((u32 (*)(u32, u32, u32))0x004FB5A8)(a0, a1, a2);
  MATRIX m;

  matrix_unit(m);
  //memcpy((void*)v, m, sizeof(MATRIX));


  return v;
}

void * getBoneMatrices(Moby* moby, int jointIdx)
{
  return (void*)((u32)moby->JointCache + (jointIdx * 0x40));
}

void getMobyJointMatrix(Moby* moby, int jointIdx, MATRIX out)
{
  u32 v = *(u32*)0x004f8178;
  HOOK_JAL(0x004f8178, &getBoneMatrices);
  ((void (*)(Moby*,int,float*))0x004F8150)(moby, jointIdx, out);
  POKE_U32(0x004f8178, v);
}

void drawRing(MATRIX m3, VECTOR position, float scale0, float scale1, u32 color)
{
  // draw
  ((void (*)(void*, u128, float, float, int, int, int, int))0x00420c80)(
    m3,
    vector_read(position),
    scale0,
    scale1,
    color,
    22,
    1,
    0
  );
}

void drawAxisGizmos(MATRIX m)
{
  MATRIX t;
  VECTOR vs;

  // Z
  matrix_unit(t);
  matrix_multiply(t, t, m);
  vector_scale(t, t, 0.1);
  vector_scale(&t[4], &t[4], 0.1);
  vector_scale(vs, &t[8], 0.1);
  vector_add(&t[12], &t[12], vs);
  drawRing(t, &t[12], 0.1, 0.1, 0x80FF0000);

  // Y
  matrix_unit(t);
  matrix_multiply(t, t, m);
  vector_scale(t, t, 0.1);
  vector_scale(&t[8], &t[8], 0.1);
  vector_scale(vs, &t[4], 0.1);
  vector_add(&t[12], &t[12], vs);
  drawRing(t, &t[12], 0.1, 0.1, 0x8000FF00);
  
  // X
  matrix_unit(t);
  matrix_multiply(t, t, m);
  vector_scale(&t[4], &t[4], 0.1);
  vector_scale(&t[8], &t[8], 0.1);
  vector_scale(vs, &t[0], 0.1);
  vector_add(&t[12], &t[12], vs);
  drawRing(t, &t[12], 0.1, 0.1, 0x800000FF);
}

void drawJoints(Moby* moby)
{
  static int selectedJoint = 0;
  moby = Header->Moby;
  if (!moby)
    return;

  int j = 0;
  int count = moby->JointCnt;
  int x,y;
  MATRIX m;
  char buf[12];
  moby->Opacity = 0x80;
  
  if (padGetButtonDown(0, PAD_LEFT) > 0) {
    if (selectedJoint > 0)
      --selectedJoint;
    else
      selectedJoint = count - 1;
  }
  else if (padGetButtonDown(0, PAD_RIGHT) > 0) {
    if (selectedJoint < count)
      ++selectedJoint;
    else
      selectedJoint = 0;
  }

  for (j = 0; j < moby->JointCnt; ++j) {
    if (j != selectedJoint)
      continue;

    getMobyJointMatrix(moby, j, m);
    if (gfxWorldSpaceToScreenSpace((float*)(&m[12]), &x, &y)) {
      sprintf(buf, "%d", j);
      gfxScreenSpaceText(x, y+15, 0.5, 0.5, 0x80FFFFFF, buf, -1, 4);
    }

    drawAxisGizmos(m);

    // 
    memset(Header->Moby->Rotation, 0, sizeof(VECTOR));
    memset(Header->Moby->Position, 0, sizeof(VECTOR));
    getMobyJointMatrix(moby, j, m);

    if (padGetButtonDown(0, PAD_R3) > 0) {
      vector_print(&m[0]); printf("\n");
      vector_print(&m[4]); printf("\n");
      vector_print(&m[8]); printf("\n");
      vector_print(&m[12]); printf("\n");
    }
  }

}

void runFixedAnimation(void)
{
  int x,y;
  if (!isInGame()) {
    Header->Moby = NULL;
    Header->SeqId = 0;
    return;
  }

  gameSetDeathHeight(-100);

  if (!(*FrameJointMatrixStart)) {
    int size = 16 * sizeof(float) * MaxKeyframes * 0x45;
    *FrameJointMatrixStart = (u32)malloc(size);
    DPRINTF("malloc at %08X size %X\n", *FrameJointMatrixStart, size);
    return;
  }

  Player* p = playerGetFromSlot(0);
  if (p)
    Header->Moby = p->Gadgets[0].pMoby;
  
  if (!Header->Moby)
    return;
  
  Moby* mobyStart = mobyListGetStart();
  Moby* mobyEnd = mobyListGetEnd();
  if (!Header->Moby) {
    Header->Moby = mobyFindNextByOClass(mobyStart, 0x108A);
    //Header->Moby = mobyStart;
    DPRINTF("bp at %08X\n", (u32)&getBoneMatrices);
    if (Header->Moby) {
      DPRINTF("moby at %08X\n", (u32)Header->Moby);
    }
    Header->SeqId = 0;
    setActive(0);
    return;
  }

  /*
  // move moby
  if (padGetButtonDown(0, PAD_RIGHT) > 0) {
    while (Header->Moby < mobyEnd) {
      ++Header->Moby;
      if (Header->Moby->JointCnt)
        break;
    }

    setActive(0);
    Header->SeqId = 0;
    DPRINTF("moby:%08X oclass:%04X joints:%d \n", (u32)Header->Moby, Header->Moby->OClass, Header->Moby->JointCnt);
  }
  else if (padGetButtonDown(0, PAD_LEFT) > 0) {
    while (Header->Moby > mobyStart) {
      --Header->Moby;
      if (Header->Moby->JointCnt)
        break;
    }

    setActive(0);
    Header->SeqId = 0;
    DPRINTF("moby:%08X oclass:%04X joints:%d \n", (u32)Header->Moby, Header->Moby->OClass, Header->Moby->JointCnt);
  }
  */

  if (Header->Moby) {
    Header->Moby->Opacity = 0;
    gfxRegisterDrawFunction((void**)0x0022251C, &drawJoints, TestMoby);
    //drawJoints(Header->Moby);
  }

  return;

  // move animation
  if (Header->Moby && Header->Moby->PClass) {
    int seqCount = *(char*)((u32)Header->Moby->PClass + 0xC);

    if (padGetButtonDown(0, PAD_UP) > 0) {
      if (Header->SeqId < seqCount)
        ++Header->SeqId;

      setActive(0);
      DPRINTF("anim:%d seqCnt:%d\n", Header->SeqId, seqCount);
    }
    else if (padGetButtonDown(0, PAD_DOWN) > 0) {
      if (Header->SeqId > 0)
        --Header->SeqId;

      setActive(0);
      DPRINTF("anim:%d seqCnt:%d\n", Header->SeqId, seqCount);
    }
  }

  // enable disable
  if (padGetButtonDown(0, PAD_L3) > 0) {
    setActive(!active);
  }

  if (Header->Moby && !active) {
    //mobyAnimTransition(Header->Moby, Header->SeqId, 0, 0);
  }

  if (active && Header->Moby) {
    // draw marker over moby
    if (gfxWorldSpaceToScreenSpace(Header->Moby->Position, &x, &y)) {
      gfxScreenSpaceText(x, y, 1, 1, 0x80FFFFFF, "+", -1, 1);
    }

    // 
    Header->JointCount = Header->Moby->JointCnt;

    // patch to prevent 
    POKE_U32(0x00606BA8, 0);
    POKE_U32(0x00606C9C, 0);

    // 
    memset(Header->Moby->Rotation, 0, sizeof(VECTOR));
    memset(Header->Moby->Position, 0, sizeof(VECTOR));

    // lock anim
    if (Header->Moby->AnimSeqId != Header->SeqId || frame < 0) {
      mobyAnimTransition(Header->Moby, Header->SeqId, 0, 0);
      lastTime = 0;
      frame = 0;
      Header->FrameCount = 0;
      Header->JointCount = Header->Moby->JointCnt;
      currentFrameJointMatrixAddr = *FrameJointMatrixStart;
    }

    if (Header->Moby->AnimSeqId == Header->SeqId)
    {
      int storeFrame = 0;

      if (Header->State == 4) {
        setActive(0); // reached end
      }
      else if (Header->State == 0) {
        if (Header->Moby->AnimSeqT < lastTime) {
          Header->State = 3;
        }
        else if (Header->Moby->AnimSeqT > lastTime) {
          storeFrame = 1;
        }

        if (storeFrame) {
          // store joint matrix in keyframe
          //int j = 0;
          const int matrix4_size = 16 * sizeof(float);
          //for (j = 0; j < Header->Moby->JointCnt; ++j) {
          //  getMobyJointMatrix(Header->Moby, j, (void*)(currentFrameJointMatrixAddr + (j * matrix4_size)));
          //}
          memcpy((void*)currentFrameJointMatrixAddr, Header->Moby->JointCache, Header->Moby->JointCnt * sizeof(float) * 16);

          // save frame
          Header->Frames[Header->FrameCount].Time = Header->Moby->AnimSeqT;
          Header->Frames[Header->FrameCount].Data = (void*)currentFrameJointMatrixAddr;

          DPRINTF("frame %d:%f\n", Header->FrameCount + Header->FrameOffset, Header->Moby->AnimSeqT);

          // increment
          Header->FrameCount++;
          currentFrameJointMatrixAddr += Header->Moby->JointCnt * matrix4_size;

          // check for end
          if (Header->FrameCount >= MaxKeyframes) {
            DPRINTF("frame limit hit.. waiting for controller to reset\n");
            Header->State = 1;
          }
        }

        ++frame;
        lastTime = Header->Moby->AnimSeqT;
      }
      else if (Header->State == 1) {
        // waiting
      }
      else if (Header->State == 2) {
        if (Header->Moby->AnimSeqT == lastTime) {
          Header->FrameOffset += Header->FrameCount;
          Header->FrameCount = 0;
          Header->State = 0;
          currentFrameJointMatrixAddr = *FrameJointMatrixStart;
        }
      }
    }
  }
  else {
  }
}

void runAnimJointThing(void)
{
  if (isInGame()) {
    Player* localPlayer = playerGetFromSlot(0);
    VECTOR off = {0,0,-10,0};

    if (!TestMoby) {
      Moby *m = TestMoby = mobySpawn(MOBY_ID_BETA_BOX, 0);
      m->Scale *= 0.1;
    }

    if (isUnloading && TestMoby) {
      TestMoby->PUpdate = 0;
      setActive(0);
      POKE_U32(0x004F8178, 0x0C13ED6A);
    }
    else if (TestMoby && localPlayer) {
      TestMoby->DrawDist = 0xFF;
      TestMoby->UpdateDist = 0xFF;
      vector_add(TestMoby->Position, localPlayer->PlayerPosition, off);
      TestMoby->PUpdate = &runFixedAnimation;
      //runFixedAnimation();
      HOOK_JAL(0x004F8178, &h);
    }
  }
}

//--------------------------------------------------------------------------
void drawEffectQuad(VECTOR position, int texId, float scale, u32 color)
{
	struct QuadDef quad;
	MATRIX m2;
	VECTOR t;
	VECTOR pTL = {0.5,0,0.5,1};
	VECTOR pTR = {-0.5,0,0.5,1};
	VECTOR pBL = {0.5,0,-0.5,1};
	VECTOR pBR = {-0.5,0,-0.5,1};
  
	// set draw args
	matrix_unit(m2);

	// init
  gfxResetQuad(&quad);

	// color of each corner?
	vector_copy(quad.VertexPositions[0], pTL);
	vector_copy(quad.VertexPositions[1], pTR);
	vector_copy(quad.VertexPositions[2], pBL);
	vector_copy(quad.VertexPositions[3], pBR);
	quad.VertexColors[0] = quad.VertexColors[1] = quad.VertexColors[2] = quad.VertexColors[3] = color;
  quad.VertexUVs[0] = (struct UV){0,0};
  quad.VertexUVs[1] = (struct UV){1,0};
  quad.VertexUVs[2] = (struct UV){0,1};
  quad.VertexUVs[3] = (struct UV){1,1};
	quad.Clamp = 0;
	quad.Tex0 = gfxGetEffectTex(texId, 1);
	quad.Tex1 = 0xFF9000000260;
	quad.Alpha = 0x8000000044;

	GameCamera* camera = cameraGetGameCamera(0);
	if (!camera)
		return;

	// get forward vector
	vector_subtract(t, camera->pos, position);
	t[2] = 0;
	vector_normalize(&m2[4], t);

	// up vector
	m2[8 + 2] = 1;

	// right vector
	vector_outerproduct(&m2[0], &m2[4], &m2[8]);

  t[0] = t[1] = t[2] = scale;
  t[3] = 1;
  matrix_scale(m2, m2, t);

	// position
	memcpy(&m2[12], position, sizeof(VECTOR));

	// draw
	gfxDrawQuad((void*)0x00222590, &quad, m2, 1);
}

//--------------------------------------------------------------------------
void dropPostDraw(Moby* moby)
{
	struct QuadDef quad;
	MATRIX m2;
	VECTOR t;
  
	// determine color
	u32 color = 0x70FFFFFF;

	// set draw args
	matrix_unit(m2);

	// init
  gfxResetQuad(&quad);

	// color of each corner?
	quad.VertexColors[0] = quad.VertexColors[1] = quad.VertexColors[2] = quad.VertexColors[3] = color;
  quad.VertexUVs[0] = (struct UV){0,0};
  quad.VertexUVs[1] = (struct UV){1,0};
  quad.VertexUVs[2] = (struct UV){0,1};
  quad.VertexUVs[3] = (struct UV){1,1};
	quad.Clamp = 0;
	quad.Tex0 = gfxGetFrameTex(43);
	quad.Tex1 = gfxGetFrameTex(44); //0xFF9000000260;
	quad.Alpha = 0x8000000044;

	// position
	memcpy(&m2[12], moby->Position, sizeof(VECTOR));
	m2[14] += 2;

	// draw
	gfxDrawQuad((void*)0x00222590, &quad, m2, 1);
}

void drawQuadTestMobyUpdate(Moby* moby)
{
  moby->Opacity = 0;
  gfxRegisterDrawFunction((void**)0x0022251C, &dropPostDraw, TestMoby);
}

void runDrawQuad(void)
{
  if (isInGame()) {
    Player* localPlayer = playerGetFromSlot(0);
    VECTOR off = {0,0,-10,0};

    if (!TestMoby && padGetButtonDown(0, PAD_LEFT) > 0) {
      Moby *m = TestMoby = mobySpawn(0x1F5, 0);
      //m->Scale = 1;
      vector_copy(m->Position, localPlayer->PlayerPosition);
      DPRINTF("spawned moby at %08X\n", (u32)m);
    }

    if (isUnloading && TestMoby) {
      TestMoby->PUpdate = 0;
    }
    else if (TestMoby && localPlayer) {
      TestMoby->DrawDist = 0xFF;
      TestMoby->UpdateDist = 0xFF;
      TestMoby->PUpdate = &drawQuadTestMobyUpdate;
    }
  }
}

void runCameraHeight(void)
{
  char buf[32];

  GameCamera* camera = cameraGetGameCamera(0);
  if (camera) {

    sprintf(buf, "%f\n", camera->pos[2]);
    gfxScreenSpaceText(15, SCREEN_HEIGHT - 30, 1, 1, 0x80FFFFFF, buf, -1, 0);
  }
}

static inline void nopdelay(void)
{
    int i = 0xfffff;

    do {
        __asm__("nop\nnop\nnop\nnop\nnop\n");
    } while (i-- != -1);
}

void runSystemTime(void)
{
  static int aaa = 0;
  static int d = 0;
  char buf[32];

  long t = timerGetSystemTime();
	long tMs = t / SYSTEM_TIME_TICKS_PER_MS;

  sprintf(buf, "%d: %.2f\n", aaa, (int)tMs / 1000.0);
  gfxScreenSpaceText(15, SCREEN_HEIGHT - 30, 1, 1, 0x80FFFFFF, buf, -1, 0);

  if (padGetButton(0, PAD_LEFT) > 0) {
    if (!d) {
      --aaa;
      if (aaa < 0)
        aaa = 0;
    }
    d = 1;
  }
  else if (padGetButton(0, PAD_RIGHT) > 0) {
    if (!d) {
      ++aaa;
    }
    d = 1;
  }
  else if (padGetButton(0, PAD_DOWN) > 0) {
    if (!d) {
      POKE_U32(0x0021df60, *(int*)0x0021df60 == 60 ? 30 : 60);
    }
    d = 1;
  }
  else {
    d = 0;
  }

  int i = 0;
  while (i) {
    nopdelay();
    --i;
  }
}

void drawCBoot(Moby* moby)
{
  MATRIX m;
  int i = 0;
  int x,y;
  char buf[12];

  matrix_unit(m);
  vector_copy(&m[12], moby->Position);
  m[12 + 0] += 2;
  m[12 + 2] += 0;

  for (i = 0; i < 0x40; ++i) {
    vector_copy(&m[12], moby->Position);
    m[12 + 0] += 2 * (i / 0x10);
    m[12 + 1] += 2 * (i % 0x10);

    float gt = (gameGetTime() % 1000) / 1000.0f;
    ((void (*)(float, u32, MATRIX, u32, u64, int, int))0x004CCF98)(gt, 0x002225A8, m, i, 0xFFFFFFFF97396BC0, 5, 2);
    if (gfxWorldSpaceToScreenSpace(&m[12], &x, &y)) {
      sprintf(buf, "%d", i);
      gfxScreenSpaceText(x, y, 1, 1, 0xFFFFFFFF, buf, -1, 4);
    }

    drawEffectQuad(&m[12], i, 1, 0x80FFFFFF);
  }
}

VECTOR * drawColliderFrom = (VECTOR*)0x000A0000;
VECTOR * drawColliderTo = (VECTOR*)0x000A0100;

void drawCollider(Moby* moby)
{
  VECTOR pos,t,o = {0,0,0.7,0};
  int i,j = 0;
  int x,y;
  char buf[12];
  Player * p = playerGetAll()[moby->Bolts];

  if (gfxWorldSpaceToScreenSpace(p->Ground.point, &x, &y)) {
    gfxScreenSpaceText(x, y, 1, 1, 0x80FFFFFF, "+", -1, 4);
  }
  return;

  const int steps = 10 * 2;
  const float radius = 2;

  for (i = 0; i < steps; ++i) {
    for (j = 0; j < steps; ++j) {
      float theta = (i / (float)steps) * MATH_TAU;
      float omega = (j / (float)steps) * MATH_TAU;  

      vector_copy(pos, p->PlayerPosition);
      pos[0] += radius * sinf(theta) * cosf(omega);
      pos[1] += radius * sinf(theta) * sinf(omega);
      pos[2] += radius * cosf(theta);

      vector_add(t, p->PlayerPosition, o);

      if (CollLine_Fix(pos, t, COLLISION_FLAG_IGNORE_STATIC, NULL, 0)) {
        vector_copy(t, CollLine_Fix_GetHitPosition());
        drawEffectQuad(t, 22, 0.05, 0x80FFFFFF);
      }
    }
  }

  if (0) {
    VECTOR dt, offset;
    for (j = 0; j < 4; ++j) {

      u32 color = 0x800000FF;
      if (j == 1) color = 0x8000FFFF;
      if (j == 2) color = 0x80FF00FF;
      if (j == 3) color = 0x80FF0000;

      vector_subtract(dt, drawColliderTo[j], drawColliderFrom[j]);
      for (i = 0; i < 20; ++i) {
        float t = i / 20.0;
        vector_scale(offset, dt, t);
        vector_add(offset, offset, drawColliderFrom[j]);
        drawEffectQuad(offset, 21, 0.1, color);
      }

      if (CollLine_Fix(drawColliderFrom[j], drawColliderTo[j], 0, NULL, NULL)) {
        drawEffectQuad(CollLine_Fix_GetHitPosition(), 21, 0.3, 0x8000FF00);
        //DPRINTF("%08X\n", CollLine_Fix_GetHitMoby());
      }
    }
  }
}

void drawPlayerCollider(void);

renderCBootPUpdate(Moby* m)
{
  gfxRegisterDrawFunction((void**)0x0022251C, &drawCollider, m);
}

void runRenderCboot(int localPlayerIndex)
{
  VECTOR off = {0,0,2,0};

  if (isUnloading) {
    if (TestMoby) {
      mobyDestroy(TestMoby);
      TestMoby = NULL;
    }
    return;
  }

  if (!TestMoby) {
    Moby *m = TestMoby = mobySpawn(MOBY_ID_BETA_BOX, 0);
    m->Scale *= 0.1;
    m->PUpdate = &renderCBootPUpdate;
    m->CollActive = 0;
    m->Bolts = localPlayerIndex;
    vector_add(m->Position, playerGetAll()[localPlayerIndex]->PlayerPosition, off);
  }

  TestMoby->DrawDist = 0xFF;
  TestMoby->UpdateDist = 0xFF;
}

void drawB6Hits(void);

drawB6Visualizer(void)
{
  int i;

  for (i = 0; i < b6ExplosionPositionCount; ++i) {
    drawEffectQuad(b6ExplosionPositions[i], 4, 0.2, 0x80FFFFFF);
  }

  drawB6Hits();
}

renderB6Visualizer(Moby* m)
{
  gfxRegisterDrawFunction((void**)0x0022251C, &drawB6Visualizer, m);
}

void runB6HitVisualizer(void)
{
  VECTOR off = {0,0,2,0};

  if (isUnloading) {
    if (TestMoby) {
      mobyDestroy(TestMoby);
      TestMoby = NULL;
    }
    return;
  }

  if (!TestMoby) {
    Moby *m = TestMoby = mobySpawn(MOBY_ID_BETA_BOX, 0);
    m->Scale *= 0.1;
    m->PUpdate = &renderB6Visualizer;
    m->CollActive = 0;
    vector_add(m->Position, playerGetFromSlot(0)->PlayerPosition, off);
  }

  // 
  TestMoby->DrawDist = 0xFF;
  TestMoby->UpdateDist = 0xFF;

  // check for b6
  Moby* m = mobyListGetStart();
  Moby* mEnd = mobyListGetEnd();
  while (m < mEnd)
  {
    if (!mobyIsDestroyed(m) && m->OClass == MOBY_ID_B6_BALL0 && m->State == 4) {
      DPRINTF("%08X\n", (u32)m->PUpdate);
      vector_copy(b6ExplosionPositions[b6ExplosionPositionCount], m->Position);
      b6ExplosionPositionCount = (b6ExplosionPositionCount + 1) % 64;
    }

    ++m;
  }
}

void drawPositionYaw(void)
{
  if (isInGame()) {
    Player * p = playerGetFromSlot(0);

    if (padGetButtonDown(0, PAD_UP) > 0) {
      p->PlayerState = PLAYER_STATE_WAIT_FOR_RESURRECT;
      p->timers.resurrectWait = 1;
    }

    if (p) {
      char buf[64];
      sprintf(buf, "%.2f %.2f %.2f   %.3f", p->PlayerPosition[0], p->PlayerPosition[1], p->PlayerPosition[2], p->PlayerRotation[2]);
      gfxScreenSpaceText(15, SCREEN_HEIGHT - 40, 1, 1, 0x80FFFFFF, buf, -1, 0);
    }
  }
}

void sendFusionShot(void)
{
  if (padGetButton(0, PAD_DOWN) <= 0) return;
  
  Player* player = playerGetFromSlot(0);
  void * connection = netGetDmeServerConnection();

  VECTOR from={0,0,1,0};
  VECTOR to;
  vector_add(from, from, player->PlayerPosition);
  vector_add(from, from, player->CameraDir);
  vector_scale(to, player->CameraDir, 50);
  vector_add(to, to, player->PlayerPosition);

  struct tNW_GadgetEventMessage msg;
  msg.PlayerIndex = player->PlayerId;
  msg.GadgetId = WEAPON_ID_FUSION_RIFLE;
  msg.GadgetEventType = 8;
  msg.ActiveTime = gameGetTime() - (TIME_SECOND * 60);
  msg.TargetUID = -1;
  memcpy(msg.FiringLoc, from, 12);
  memcpy(msg.TargetDir, to, 12);
  netBroadcastMediusAppMessage(0, connection, 14, sizeof(msg), &msg);
}

struct LatencyPing
{
  int FromClientIdx;
  int ToClientIdx;
  long Ticks;
};

int runLatencyPing_OnRemotePing(void* connection, void* data)
{
  struct LatencyPing msg;
  memcpy(&msg, data, sizeof(msg));

  if (msg.FromClientIdx == gameGetMyClientId()) {
    long dticks = timerGetSystemTime() - msg.Ticks;
    int dms = dticks / SYSTEM_TIME_TICKS_PER_MS;
    DPRINTF("RTT to CLIENT %d : %dms\n", msg.ToClientIdx, dms);
  } else {
    // send back
    netSendCustomAppMessage(NET_DELIVERY_CRITICAL, connection, msg.FromClientIdx, 250, sizeof(msg), &msg);
  }

  return sizeof(struct LatencyPing);
}

void runLatencyPing(void)
{
  static int pingCooldownTicks[GAME_MAX_PLAYERS] = {0,0,0,0,0,0,0,0,0,0};

  netInstallCustomMsgHandler(250, &runLatencyPing_OnRemotePing);
  void * connection = netGetDmeServerConnection();
  if (!connection) return;

  patchAggTime(5);
  long ticks = timerGetSystemTime();
  Player** players = playerGetAll();
  int i;
  for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
    Player* player = players[i];
    if (!player || !player->pNetPlayer) continue;
    if (pingCooldownTicks[i]) { pingCooldownTicks[i]--; continue; }

    // send
    struct LatencyPing msg;
    msg.FromClientIdx = gameGetMyClientId();
    msg.ToClientIdx = player->pNetPlayer->netClientIndex;
    msg.Ticks = ticks;
    netSendCustomAppMessage(NET_DELIVERY_CRITICAL, connection, msg.ToClientIdx, 250, sizeof(msg), &msg);

    // reset cooldown
    pingCooldownTicks[i] = 60;
  }
}

int onSniperCollLineFix(VECTOR from, VECTOR to, int hitFlag, Moby* ignore, MobyColDamageIn* damageIn)
{
  static int idx = 0;
  vector_copy(drawColliderFrom[idx], from);
  vector_copy(drawColliderTo[idx], to);

  //((void (*)(int))0x0059b700)(1);

  int r = CollLine_Fix(from, to, hitFlag, ignore, damageIn);
  vector_print(from);
  printf(" ");
  vector_print(to);
  printf(" %d %08X %08X => %d (%08X)\n", hitFlag, ignore, damageIn, r, CollLine_Fix_GetHitMoby());

  idx = idx + 1;
  if (idx > 4) idx = 0;

  return r;
}

void rotEffects(Player* player)
{
  if (player->PlayerState == PLAYER_STATE_CHARGE) {
    //player->PlayerMoby->Rotation[1] += MATH_PI / 2;
  }

  ((void (*)(Player*))0x005d2388)(player);
}

void onSetMobyCollPill(Moby* moby, int index, VECTOR pos, float radius, float height)
{
  //((void (*)(Moby*, int, VECTOR, float, float))0x004F79A8)(moby, index, pos, radius, height);
}

void runLocalPlayerChargeboot(void)
{
  static VECTOR pos;
  int pid = 1;

  if (!isInGame()) return;
  Player* player = playerGetAll()[pid];
  if (!player || !player->PlayerMoby) return;

  if (isUnloading) {
    //POKE_U32(0x003FC66C, 0x0C12DF94);
    //POKE_U32(0x005d6318, 0x0C13DE6A);
    //POKE_U32(0x005D638C, 0x0C13DE6A);
    //POKE_U32(0x005d6250, 0x0C1748E2);
  } else {
    //HOOK_JAL(0x003FC66C, &onSniperCollLineFix);
    //HOOK_JAL(0x005d6318, &onSetMobyCollPill);
    //HOOK_JAL(0x005D638C, &onSetMobyCollPill);
    //HOOK_JAL(0x005d6250, &rotEffects);
  }

  if (0) {
    VECTOR pfixed = { 352.42, 208.53, 107.75, 0 };
    VECTOR dt;
    vector_subtract(dt, pfixed, player->PlayerPosition);
    if (player->PlayerState != PLAYER_STATE_CHARGE && vector_sqrmag(dt) > 0.1) {
      playerSetPosRot(player, pfixed, player->PlayerRotation);
    }
  }

  player->Health = 50;
  if (player->PlayerState == PLAYER_STATE_CHARGE && player->timers.state <= 21) {
    player->timers.state = 20;
    vector_copy(player->PlayerPosition, pos);
    vector_copy(player->PlayerMoby->Position, pos);
    //player->Tweakers[0].rot[1] = -MATH_PI / 2;
  } else {
    vector_copy(pos, player->PlayerPosition);
  }
  
  runRenderCboot(pid);
}

int runSendMonitor_Hook(void* a0, void* a1, int a2)
{
  u32 ra;
  
	// pointer to gameplay data is stored in $s1
	asm volatile (
		"move %0, $ra"
		: : "r" (ra)
	);


  POKE_U32(0x01EA1270, 0x27BDFFE0);
  POKE_U32(0x01EA1274, 0x0080482D);

  int msgId = *(int*)((u32)a0 + 0x08);
  int msgLen = *(int*)((u32)a0 + 0x38);
  void* msgPtr = *(void**)((u32)a0 + 0x3C);
  if (a2 == 2 && msgId == 15) {
    
    DPRINTF("SEND msgClass:%d msgId:%d msgLen:%d msgBuf:%08X ra:%08X\n", a2, msgId, msgLen, msgPtr, ra);
  }
  int result = ((int (*)(void*, void*, int))0x01ea1270)(a0, a1, a2);
  
  HOOK_J(0x01EA1270, &runSendMonitor_Hook);
  POKE_U32(0x01EA1274, 0);

  return result;
}

void runSendMonitor(void)
{
  if (isUnloading) {
    POKE_U32(0x01EA1270, 0x27BDFFE0);
    POKE_U32(0x01EA1274, 0x0080482D);
    return;
  }
  
  HOOK_J(0x01EA1270, &runSendMonitor_Hook);
  POKE_U32(0x01EA1274, 0);
}

void runTestLogic(void)
{
  int i;

  //runHitmarkerLogic();
  //runDrawTex();
  //runAllow4Locals();

  //drawPositionYaw();
  //gameConfig.grBetterFlags = 1;

  //runSystemTime();
  if (isInGame()) {
    // if (padGetButtonDown(0, PAD_DOWN) > 0) {
    //   Player* p = playerGetFromSlot(0);
    //   VECTOR to = {5,5,5,0};
    //   vector_add(to, to, p->PlayerPosition);
    //   Moby* m = gfxDrawSimpleTwoPointLightning(
    //     (void*)0x002225A0,
    //     p->PlayerPosition,
    //     to,
    //     3000,
    //     1,
    //     1,
    //     NULL,
    //     NULL,
    //     NULL,
    //     0x80804020
    //   );

    //   DPRINTF("lightning moby %08X\n", m);
    // }

    //runLatencyPing();
    //sendFusionShot();
    //runAnimJointThing();
    //runDrawQuad();
    //runCameraHeight();
    //runRenderCboot(0);
    //runB6HitVisualizer();
    //runLocalPlayerChargeboot();
    //runSendMonitor();

    // if (padGetButtonDown(0, PAD_DOWN) > 0) {
    //   Player* p = playerGetFromSlot(0);
    //   VECTOR pos;
    //   vector_scale(pos, p->CameraForward, 4);
    //   vector_add(pos, pos, p->PlayerPosition);
    //   pos[2] += 1;
    //   u128 vPos = vector_read(pos); 
    //   u32 clear = 0;
    //   u32 red = 0x800000FF;
    //   u32 green = 0x8000FF00;
    //   u32 white = 0x80FFFFFF;
    //   mobySpawnExplosion(vPos, 0,
    //     0, 0, 0, 0, 0, 10, 1, 1, 0, 0, 1, 1,
    //     0, 0, 
    //     white, white, white, white, white, white, white, white, white,
    //     0, 0, 0, 0, 1, 0, 0, 0);
    // }

    for (i = 0; i < GAME_MAX_LOCALS; ++i) {
      Player* p = playerGetFromSlot(i);
      if (!p) continue;
      if (p->Health < 10)
        p->Health = 50;
    }

    if (padGetButton(0, PAD_L1 | PAD_CIRCLE) > 0) {
      *(float*)0x00347BD8 = 0.125;
    }
    else if (padGetButton(0, PAD_L1 | PAD_R1 | PAD_TRIANGLE) > 0) {
      playerGetFromSlot(0)->Health = 0;
    }
  } else {
    TestMoby = NULL;
  }
}
