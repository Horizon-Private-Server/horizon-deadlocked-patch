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
#include <libdl/pad.h>
#include <libdl/collision.h>
#include <libdl/color.h>
#include <libdl/utils.h>
#include "messageid.h"

extern int isUnloading;

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

void bp(void) {

}

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

void runTestLogic(void)
{
  //runHitmarkerLogic();

  if (isInGame()) {
    //runAnimJointThing();
    runDrawQuad();
  } else {
    TestMoby = NULL;
  }
}
