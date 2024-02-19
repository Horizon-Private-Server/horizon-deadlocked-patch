# Scrapped pieces of code

## Team Colored Shots

This code changes the fusion shot color to be the same color as source player's team.

```cpp
u32 GetShotColor(Player* player)
{
	if (!player)
		return 0x80007F7F;

	return TEAM_COLORS[player->Team]; //colorLerp(TEAM_COLORS[player->Team], 0, 0.5);
}

u32 GetShotColor2(Moby* shot)
{
	Player * player = guberGetObjectByUID((u32)shot->NetObject);
	return GetShotColor(player);
}

void FusionShotDrawFunction(Moby* fusionShotMoby)
{
	u32 color = GetShotColor2(fusionShotMoby);
	color = colorLerp(color, 0, 0.25);
	u32 colorDark = color;
	u16 colorUpper = color >> 16;
	u16 colorLower = color & 0xFFFF;

	// change colors to team
	POKE_U32(0x003fd46c, 0x3C020000 | colorUpper);
	POKE_U16(0x003fd470, colorLower);
	POKE_U32(0x003fd8cc, 0x3C020000 | colorUpper);
	POKE_U16(0x003fd8d4, colorLower);

	POKE_U32(0x003fde68, 0x3C080000 | colorUpper);
	POKE_U32(0x003fdeb8, 0x3C080000 | colorUpper);
	POKE_U32(0x003fda44, 0x351E0000 | colorLower);
	POKE_U32(0x003fde6c, 0x3C090000 | (colorDark >> 16));
	POKE_U32(0x003fdebc, 0x3C090000 | (colorDark >> 16));
	POKE_U32(0x003fda4c, 0x35370000 | (u16)colorDark);

	// call base
	((void (*)(Moby*))0x003fdcb0)(fusionShotMoby);
}

void writeDrawFunctionPatch(u32 addr1, u32 addr2, u32 value)
{
	POKE_U16(addr1, (u16)(value >> 16));
	POKE_U16(addr2, (u16)value);
}

void patchTeamColoredShots(void)
{
	if (!isInGame())
		return;

	writeDrawFunctionPatch(0x003fdc88, 0x003fdc9c, &FusionShotDrawFunction);
}
```

## Attempts to fix lag

```cpp
// removes anti latency variation updates when
// client thinks that its average latency is 0
// which has only happened when client timeout has been disabled
// and emu hits a breakpoint for 30+ seconds multiple times
int pp = 0;
((void (*)(int*))0x01eabda0)(&pp);
if (pp == 0) {
	POKE_U32(0x01eabd60, 0);
}	else {
	POKE_U32(0x01eabd60, 0x14600003);
}

// disables timebase anti latency variation updates
// ie accepts all timebase updates from server regardless if 
// client suddenly starts lagging
POKE_U32(0x01eabd60, 0);

```

This does stuff like
 - Spam server with packets to simulate bandwidth constraints on PS2.
 - Simulate client inputs on a loop to help test certain cases
 - Send false position to remote clients
 - Offset game time by some number of seconds to simulate timebase desync
```cpp

/*
int sampleCount = 0;
int isSampling = 0;
int deltaTotal = 0;
int deltaCount = 0;
int counts[200];
const int SAMPLE_SIZE = 250;

int dot_callback(void * connection, void * data)
{
	int time = *(int*)data;
	int delta = gameGetTime() - time;

	//printf("delta %d\n", delta);
	counts[delta]++;
	deltaTotal += delta;
	deltaCount++;

	return sizeof(int);
}

int dot_callback2(void * connection, void * data)
{
	int time = *(int*)data;
	int delta = gameGetTime() - time;

	printf("delta %d\n", delta);

	return sizeof(int);
}

void dot(void)
{

	netInstallCustomMsgHandler(101, &dot_callback);
	netInstallCustomMsgHandler(102, &dot_callback2);

	int i = 0;
	int gameTime = gameGetTime();
	GameSettings* gs = gameGetSettings();
	if (!gs)
		return;

	void * dmeConnection = netGetDmeServerConnection();
	if (!dmeConnection)
		return;

	if (!isSampling && padGetButtonDown(0, PAD_L1) > 0) {
		printf("sending %d samples\n", SAMPLE_SIZE);
		isSampling = 1;
		sampleCount = 0;
		deltaCount = 0;
		deltaTotal = 0;
		memset(counts, 0, sizeof(counts));
	}
	else if (!isSampling && padGetButtonDown(0, PAD_R1) > 0) {
		netBroadcastCustomAppMessage(NET_LATENCY_CRITICAL, dmeConnection, 102, sizeof(gameTime), &gameTime);
	}

	if (isSampling) {
		if (sampleCount >= SAMPLE_SIZE) {
			if (isSampling < 200) {
				isSampling++;
				return;
			}
			isSampling = 0;

			float avgDt = (float)deltaTotal / (float)deltaCount;
			int minDt = (int)avgDt;
			int maxDt = (int)avgDt;
			for (i = 0; i < 200; ++i) {
				if (i < minDt && counts[i] > 0)
					minDt = i;
				if (i > maxDt && counts[i] > 0)
					maxDt = i;
			}

			printf("received %d samples with average latency %f, min:%d max:%d\n", deltaCount, avgDt, minDt, maxDt);
			for (i = 0; i < 200; ++i) {
				if (counts[i] > 0) {
					printf("\t%d: %d\n", i, counts[i]);
				}
			}
		} else {
			netBroadcastCustomAppMessage(NET_LATENCY_CRITICAL, dmeConnection, 101, sizeof(gameTime), &gameTime);
			++sampleCount;
		}
	}
}
*/

int aaa_tick = 0;
int aaa_active = 0;
VECTOR aaa_pos;
VECTOR aaa_rot;

int r(void)
{
	if (aaa_active == 3) {
		*(int*)0x00172378 = *(int*)0x00172378 + (TIME_SECOND * 1.1);
	}

	return 0;
}

void copyposstateupdatehook(void * dest, void * src, int size)
{
	if (aaa_active == 4) {
		memcpy(dest, aaa_pos, size);
	} else {
		memcpy(dest, src, size);
	}
}

void aaa()
{
	VECTOR t;
	if (!isInGame())
		return;

	//HOOK_J(0x0015B290, &r);
	//HOOK_JAL(0x0060ed9c, &copyposstateupdatehook);

	Player * p = playerGetFromSlot(0);
	vector_write(t, 0);

	// toggle
	if (padGetButtonDown(0, PAD_L3) > 0) {
		aaa_active = 0;
		DPRINTF("deactivated\n");
	}

	// toggle
	if (padGetButtonDown(0, PAD_L1 | PAD_UP) > 0) {
		aaa_active = 1;
		DPRINTF("activated\n");
		uiShowPopup(0, "activated");
		aaa_tick = 0;
		vector_copy(aaa_pos, p->PlayerPosition);
		aaa_rot[0] = p->PlayerYaw;
		aaa_rot[1] = p->CameraPitch.Value;
		aaa_rot[2] = p->CameraYaw.Value;
	}

	// toggle
	if (padGetButtonDown(0, PAD_L1 | PAD_LEFT) > 0) {
		aaa_active = 2;
		DPRINTF("activated\n");
		aaa_tick = 0;
		vector_copy(aaa_pos, p->PlayerPosition);
		aaa_rot[0] = p->PlayerYaw;
		aaa_rot[1] = p->CameraPitch.Value;
		aaa_rot[2] = p->CameraYaw.Value;
	}

	// toggle
	if (padGetButtonDown(0, PAD_L1 | PAD_DOWN) > 0) {
		aaa_active = 3;
		DPRINTF("activated\n");
		aaa_tick = 0;
	}

	// toggle
	if (padGetButtonDown(0, PAD_L1 | PAD_RIGHT) > 0) {
		aaa_active = 4;
		DPRINTF("activated\n");
		aaa_tick = 0;
		vector_copy(aaa_pos, p->PlayerPosition);
	}

	if (!aaa_active || gameIsStartMenuOpen() || isConfigMenuActive)
		return;

	if (aaa_active == 1) {
		if (aaa_tick > 100) {
			// reset
			t[2] = aaa_rot[0];
			playerSetPosRot(p, aaa_pos, t);
			p->CameraPitch.Value = aaa_rot[1];
			p->CameraYaw.Value = aaa_rot[2];
			aaa_tick = 0;
		} else if (aaa_tick < 20 && (aaa_tick % 3) != 0) {
			// first few frames try and chargeboot
			p->Paddata->btns &= ~PAD_L2;
		} else if (aaa_tick >= 25 && aaa_tick < 30) {
			// then try and shoot
			p->Paddata->btns &= ~PAD_R1;
		}
	} else if (aaa_active == 2) {
		if (aaa_tick > 100) {
			p->Paddata->btns &= ~PAD_R1;
			aaa_tick = 0;
		}
	} else if (aaa_tick == 3) {
		//*(int*)0x00172378 = *(int*)0x00172378 + (TIME_SECOND);
	} else if (aaa_tick == 4) {
		POKE_U32(0x0034AA94, 0x007F007F);
		POKE_U32(0x0034AA98, 0x007F007F);
	}
	++aaa_tick;
}

```

This intercepts the fusion shot direction processing to force all shots that hit to aim in the exact direction of the player.
This is a predecessor of one of the fusion shot lag fixes that removes the shot direction variance on the remote client.
Which happens even if the shot hit on the source client's screen.
```cpp

u128 aaa_fusionhook(Player* p, u64 a1, u128 from, u128 to, u64 t0, u64 t1)
{
	//t0 |= 1;
	u128 r = ((u128 (*)(Player*, u64, u128, u128, u64, u64))0x003f9fc0)(p, a1, from, to, t0, t1);

	VECTOR dir, playerPos = {0,0,1,0};
	vector_add(playerPos, playerPos, p->PlayerPosition);
	vector_write(dir, from);
	vector_subtract(dir, playerPos, dir);
	vector_normalize(dir, dir);

	return r; //vector_read(dir);
}

HOOK_JAL(0x003FA5C8, &aaa_fusionhook);

```

## Player Collider Drawing

```cpp
VECTOR hits[250];
u32 hitsColor[250];
int hitsCount = 0;
u32 hitsColorCurrent = 0x80FFFFFF;

void testPlayerCollider(int pid)
{
  VECTOR pos,t,o = {0,0,0.7,0};
  int i,j = 0;
  int x,y;
  char buf[12];
  Player * p = playerGetAll()[pid];

  const int steps = 5 * 2;
  const float radius = 2;

  for (i = 0; i < steps; ++i) {
    for (j = 0; j < steps; ++j) {
      if (hitsCount >= 250) break;

      float theta = (i / (float)steps) * MATH_TAU;
      float omega = (j / (float)steps) * MATH_TAU;  

      vector_copy(pos, p->PlayerPosition);
      pos[0] += radius * sinf(theta) * cosf(omega);
      pos[1] += radius * sinf(theta) * sinf(omega);
      pos[2] += radius * cosf(theta);

      vector_add(t, p->PlayerPosition, o);

      if (CollLine_Fix(pos, t, COLLISION_FLAG_IGNORE_STATIC, NULL, 0)) {
        vector_copy(t, CollLine_Fix_GetHitPosition());
        hitsColor[hitsCount] = hitsColorCurrent;
        vector_copy(hits[hitsCount++], CollLine_Fix_GetHitPosition());
        if (gfxWorldSpaceToScreenSpace(t, &x, &y)) {
          gfxScreenSpaceText(x, y, 1, 1, 0x80FFFFFF, "+", -1, 4);
        }
      }
    }
  }
}

void drawPlayerCollider()
{
  int i,x,y;
  for (i = 0; i < hitsCount; ++i) {
    if (gfxWorldSpaceToScreenSpace(hits[i], &x, &y)) {
      gfxScreenSpaceText(x, y, 0.5, 0.5, hitsColor[i], "+", -1, 4);
    }
  }
}
```
