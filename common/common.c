#include <libdl/string.h>
#include <libdl/stdio.h>
#include <libdl/game.h>
#include <libdl/graphics.h>
#include <libdl/math.h>
#include <libdl/camera.h>
#include "common.h"
#include "config.h"
#include "messageid.h"

#define OFFSET_TO_DZO_X(x) (x * (1080.0 / SCREEN_HEIGHT));
#define OFFSET_TO_DZO_Y(y) (y * (1080.0 / SCREEN_HEIGHT));

//------------------------------------------------------------------------------
enum INTERNAL_QUAD_FLAGS
{
  INTERNAL_QUAD_BILLBOARD = 1,
  INTERNAL_QUAD_NO_DEPTH_TEST = 2,
  INTERNAL_QUAD_CLAMP_UV = 4,
  INTERNAL_QUAD_ZWRITE = 8,
};

enum COMMON_DRAW_TARGET gfxHelperDrawTarget = COMMON_DRAW_TARGET_BOTH;

//------------------------------------------------------------------------------
//------------------------------------ MISC ------------------------------------
//------------------------------------------------------------------------------
void helperAlign(float *pX, float *pY, float w, float h, enum TextAlign alignment)
{
  float x = 0, y = 0;
  if (pX)
    x = *pX;
  if (pY)
    y = *pY;

  switch (alignment)
  {
    case TEXT_ALIGN_TOPLEFT:
      break;
    case TEXT_ALIGN_TOPCENTER:
      x -= w * 0.5;
      break;
    case TEXT_ALIGN_TOPRIGHT:
      x -= w;
      break;
    case TEXT_ALIGN_MIDDLELEFT:
      y -= h * 0.5;
      break;
    case TEXT_ALIGN_MIDDLECENTER:
      x -= w * 0.5;
      y -= h * 0.5;
      break;
    case TEXT_ALIGN_MIDDLERIGHT:
      x -= w;
      y -= h * 0.5;
      break;
    case TEXT_ALIGN_BOTTOMLEFT:
      y -= h;
      break;
    case TEXT_ALIGN_BOTTOMCENTER:
      x -= w * 0.5;
      y -= h;
      break;
    case TEXT_ALIGN_BOTTOMRIGHT:
      x -= w;
      y -= h;
      break;
  }

  if (pX)
    *pX = x;
  if (pY)
    *pY = y;
}

//------------------------------------------------------------------------------
void helperRealign(float *pX, float *pY, float w, float h, enum TextAlign fromAlignment, enum TextAlign toAlignment)
{
  if (fromAlignment == toAlignment)
    return;

  float x = 0, y = 0;
  if (pX)
    x = *pX;
  if (pY)
    y = *pY;

  helperAlign(&x, &y, w, h, fromAlignment);
  switch (toAlignment)
  {
    case TEXT_ALIGN_TOPLEFT:
      break;
    case TEXT_ALIGN_TOPCENTER:
      x += w * 0.5;
      break;
    case TEXT_ALIGN_TOPRIGHT:
      x += w;
      break;
    case TEXT_ALIGN_MIDDLELEFT:
      y += h * 0.5;
      break;
    case TEXT_ALIGN_MIDDLECENTER:
      x += w * 0.5;
      y += h * 0.5;
      break;
    case TEXT_ALIGN_MIDDLERIGHT:
      x += w;
      y += h * 0.5;
      break;
    case TEXT_ALIGN_BOTTOMLEFT:
      y += h;
      break;
    case TEXT_ALIGN_BOTTOMCENTER:
      x += w * 0.5;
      y += h;
      break;
    case TEXT_ALIGN_BOTTOMRIGHT:
      x += w;
      y += h;
      break;
  }

  if (pX)
    *pX = x;
  if (pY)
    *pY = y;
}

//------------------------------------------------------------------------------
//------------------------------------ DRAW ------------------------------------
//------------------------------------------------------------------------------
void gfxHelperSetDrawTarget(enum COMMON_DRAW_TARGET target)
{
  if ((unsigned)target > COMMON_DRAW_TARGET_DZO)
    return;

  gfxHelperDrawTarget = target;
}

//------------------------------------------------------------------------------
void gfxHelperResetDrawTarget(void)
{
  gfxHelperDrawTarget = COMMON_DRAW_TARGET_BOTH;
}

//------------------------------------------------------------------------------
void gfxHelperDrawNativeBox(float x, float y, float w, float h, u32 color)
{
  if (gfxHelperDrawTarget == COMMON_DRAW_TARGET_DZO)
    return;

  if (w <= 0 || h <= 0)
    return;

  // snap aligned edges together so adjacent boxes keep their spacing
  float edges[4] = { x, y, x + w, y + h };
  int i;
  for (i = 0; i < 4; ++i)
  {
    float rounded = edges[i] + 0.5f;
    int pixel = (int)rounded;
    if (pixel > rounded)
      --pixel;
    edges[i] = pixel;
  }

  // keep positive subpixel boxes visible without padding every height
  if (edges[2] <= edges[0])
    edges[2] = edges[0] + 1;
  if (edges[3] <= edges[1])
    edges[3] = edges[1] + 1;

  // the game truncates scaled vertices; stay clear of float rounding at edges
  for (i = 0; i < 4; ++i)
    edges[i] += edges[i] < 0 ? -0.5f : 0.5f;
  gfxPixelSpaceBox(edges[0], edges[1], edges[2] - edges[0], edges[3] - edges[1], color);
}

//------------------------------------------------------------------------------
void gfxHelperDrawBox(float anchorX, float anchorY, float offsetX, float offsetY, float w, float h, u32 color, enum TextAlign alignment, int dzoStretched)
{
  // pass to dzo
  if (gfxHelperDrawTarget != COMMON_DRAW_TARGET_NATIVE && PATCH_DZO_INTEROP_FUNCS && isInGame())
  {
    CustomDzoCommandDrawBox_t boxCmd;
    boxCmd.X = OFFSET_TO_DZO_X(offsetX);
    boxCmd.Y = OFFSET_TO_DZO_Y(offsetY);
    boxCmd.AnchorX = anchorX / SCREEN_WIDTH;
    boxCmd.AnchorY = anchorY / SCREEN_HEIGHT;
    boxCmd.W = w / SCREEN_HEIGHT;
    boxCmd.H = h / SCREEN_HEIGHT;
    boxCmd.Color = color;
    boxCmd.Alignment = alignment;
    boxCmd.Stretch = dzoStretched != 0;
    PATCH_DZO_INTEROP_FUNCS->SendCustomCommandToClient(CUSTOM_DZO_CMD_ID_DRAW_BOX, sizeof(boxCmd), &boxCmd);
  }

  // draw
  if (gfxHelperDrawTarget != COMMON_DRAW_TARGET_DZO)
  {
    float x = anchorX + offsetX;
    float y = anchorY + offsetY;
    helperAlign(&x, &y, w, h, alignment);
    gfxHelperDrawNativeBox(x, y, w, h, color);
  }
}

//------------------------------------------------------------------------------
void gfxHelperDrawBox_WS(VECTOR worldPosition, float w, float h, u32 color, enum TextAlign alignment)
{
  if (!isInGame())
    return;

  // pass to dzo
  if (gfxHelperDrawTarget != COMMON_DRAW_TARGET_NATIVE && PATCH_DZO_INTEROP_FUNCS)
  {
    CustomDzoCommandDrawWSBox_t boxCmd;
    vector_copy(boxCmd.Position, worldPosition);
    boxCmd.W = w / SCREEN_HEIGHT;
    boxCmd.H = h / SCREEN_HEIGHT;
    boxCmd.Color = color;
    boxCmd.Alignment = alignment;
    PATCH_DZO_INTEROP_FUNCS->SendCustomCommandToClient(CUSTOM_DZO_CMD_ID_DRAW_WS_BOX, sizeof(boxCmd), &boxCmd);
  }

  // draw
  if (gfxHelperDrawTarget != COMMON_DRAW_TARGET_DZO)
  {
    int x, y;
    if (gfxWorldSpaceToScreenSpace(worldPosition, &x, &y))
    {
      float fx = x, fy = y;
      helperAlign(&fx, &fy, w, h, alignment);
      gfxHelperDrawNativeBox(fx, fy, w, h, color);
    }
  }
}

//------------------------------------------------------------------------------
void gfxHelperDrawText(float anchorX, float anchorY, float offsetX, float offsetY, float scale, u32 color, char *str, int length, enum TextAlign alignment)
{
  // pass to dzo
  if (gfxHelperDrawTarget != COMMON_DRAW_TARGET_NATIVE && PATCH_DZO_INTEROP_FUNCS && isInGame())
  {
    CustomDzoCommandDrawText_t textCmd;
    textCmd.X = OFFSET_TO_DZO_X(offsetX);
    textCmd.Y = OFFSET_TO_DZO_Y(offsetY);
    textCmd.Scale = scale;
    textCmd.Alignment = alignment;
    textCmd.Color = color;
    textCmd.AnchorX = anchorX / SCREEN_WIDTH;
    textCmd.AnchorY = anchorY / SCREEN_HEIGHT;
    memset(textCmd.Text, 0, sizeof(textCmd.Text));
    safe_strcpy(textCmd.Text, str, (length >= 0 && length < 64) ? (length + 1) : 64);
    PATCH_DZO_INTEROP_FUNCS->SendCustomCommandToClient(CUSTOM_DZO_CMD_ID_DRAW_TEXT, sizeof(textCmd), &textCmd);
  }

  // draw
  if (gfxHelperDrawTarget != COMMON_DRAW_TARGET_DZO)
  {
    gfxScreenSpaceText(anchorX + offsetX, anchorY + offsetY, scale, scale, color, str, length, alignment);
  }
}

//------------------------------------------------------------------------------
void gfxHelperDrawText_WS(VECTOR worldPosition, float scale, u32 color, char *str, int length, enum TextAlign alignment)
{
  if (!isInGame())
    return;

  // pass to dzo
  if (gfxHelperDrawTarget != COMMON_DRAW_TARGET_NATIVE && PATCH_DZO_INTEROP_FUNCS)
  {
    CustomDzoCommandDrawWSText_t textCmd;
    vector_copy(textCmd.Position, worldPosition);
    textCmd.Scale = scale;
    textCmd.Alignment = alignment;
    textCmd.Color = color;
    memset(textCmd.Text, 0, sizeof(textCmd.Text));
    safe_strcpy(textCmd.Text, str, (length >= 0 && length < 64) ? (length + 1) : 64);
    PATCH_DZO_INTEROP_FUNCS->SendCustomCommandToClient(CUSTOM_DZO_CMD_ID_DRAW_WS_TEXT, sizeof(textCmd), &textCmd);
  }

  // draw
  if (gfxHelperDrawTarget != COMMON_DRAW_TARGET_DZO)
  {
    int x, y;
    if (gfxWorldSpaceToScreenSpace(worldPosition, &x, &y))
    {
      gfxScreenSpaceText(x, y, scale, scale, color, str, length, alignment);
    }
  }
}

//------------------------------------------------------------------------------
float gfxHelperDrawTextWindow(float anchorX, float anchorY, float offsetX, float offsetY, float width, float height, float textOffsetX, float textOffsetY, float scale, u32 color, char *str, int length, enum TextAlign alignment, enum FontWindowFlags flags)
{
  float fx = anchorX + offsetX;
  float fy = anchorY + offsetY;
  //helperAlign(&fx, &fy, width, height, alignment);

  if (alignment >= TEXT_ALIGN_MIDDLELEFT && alignment <= TEXT_ALIGN_MIDDLERIGHT)
    flags |= FONT_WINDOW_FLAGS_V_ALIGN_CENTER;
  if ((alignment % 3) == 1)
    flags |= FONT_WINDOW_FLAGS_H_ALIGN_CENTER;

  struct FontWindow fontWindow = {
    .windowLeft = fx,
    .windowRight = fx + width,
    .windowTop = fy,
    .windowBottom = fy + height,
    .textX = fx + textOffsetX,
    .textY = fy + textOffsetY,
    .maxWidth = width,
    .maxHeight = height,
    .lineSpacing = 16 * scale,
    .flags = flags,
    //.subPixelX = (short)((fx + textOffsetX)*2) % 2,
    //.subPixelY = (short)((fy + textOffsetY)*2) % 2,
    .shadowOffsetX = 1,
    .shadowOffsetY = 1
  };

  if ((flags & FONT_WINDOW_FLAGS_V_ALIGN_CENTER))
  {
    fontWindow.windowTop = fy - height * 0.5;
    fontWindow.windowBottom = fy + height * 0.5;
  }

  if ((flags & FONT_WINDOW_FLAGS_H_ALIGN_CENTER))
  {
    fontWindow.windowLeft = fx - width * 0.5;
    fontWindow.windowRight = fx + width * 0.5;
  }

  // pass to dzo
  if (gfxHelperDrawTarget != COMMON_DRAW_TARGET_NATIVE && PATCH_DZO_INTEROP_FUNCS && isInGame())
  {
    char msgBuf[2048];
    CustomDzoCommandDrawTextWindowVarStr_t textCmd;

    int maxStrLen = sizeof(msgBuf) - sizeof(textCmd);
    int strLen = length < 0 ? strlen(str) : length;
    if (strLen > maxStrLen)
      strLen = maxStrLen;

    textCmd.X = OFFSET_TO_DZO_X(offsetX);
    textCmd.Y = OFFSET_TO_DZO_Y(offsetY);
    textCmd.TextX = OFFSET_TO_DZO_X(textOffsetX);
    textCmd.TextY = OFFSET_TO_DZO_Y(textOffsetY);
    textCmd.Width = width / SCREEN_HEIGHT;
    textCmd.Height = height / SCREEN_HEIGHT;
    textCmd.Scale = scale;
    textCmd.Alignment = alignment;
    textCmd.Flags = flags;
    textCmd.Color = color;
    textCmd.AnchorX = anchorX / SCREEN_WIDTH;
    textCmd.AnchorY = anchorY / SCREEN_HEIGHT;
    textCmd.TextLen = strLen;
    memcpy(msgBuf, &textCmd, sizeof(textCmd));
    memcpy(&msgBuf[sizeof(textCmd)], str, strLen);
    PATCH_DZO_INTEROP_FUNCS->SendCustomCommandToClient(CUSTOM_DZO_CMD_ID_DRAW_TEXT_WINDOW_VARSTR, sizeof(textCmd) + strLen, msgBuf);
  }

  // keep layout measurements when native painting is disabled
  if (gfxHelperDrawTarget == COMMON_DRAW_TARGET_DZO)
    fontWindow.flags |= FONT_WINDOW_FLAGS_NO_DRAW;
  gfxScreenSpaceTextWindow(&fontWindow, scale, scale, color, str, length, 0x80000000);

  return fontWindow.maxHeight;
}

//------------------------------------------------------------------------------
void gfxHelperDrawSprite(float anchorX, float anchorY, float offsetX, float offsetY, float w, float h, int texWidth, int texHeight, int texId, u32 color, enum TextAlign alignment, int dzoStretched)
{
  // pass to dzo
  if (gfxHelperDrawTarget != COMMON_DRAW_TARGET_NATIVE && PATCH_DZO_INTEROP_FUNCS && isInGame())
  {
    CustomDzoCommandDrawSprite_t spriteCmd;
    spriteCmd.X = OFFSET_TO_DZO_X(offsetX);
    spriteCmd.Y = OFFSET_TO_DZO_Y(offsetY);
    spriteCmd.Alignment = alignment;
    spriteCmd.AnchorX = anchorX / SCREEN_WIDTH;
    spriteCmd.AnchorY = anchorY / SCREEN_HEIGHT;
    spriteCmd.W = w / SCREEN_HEIGHT;
    spriteCmd.H = h / SCREEN_HEIGHT;
    spriteCmd.SpriteId = texId;
    spriteCmd.CustomSpriteId = 0;
    spriteCmd.Stretch = dzoStretched != 0;
    spriteCmd.Color = color;
    PATCH_DZO_INTEROP_FUNCS->SendCustomCommandToClient(CUSTOM_DZO_CMD_ID_DRAW_SPRITE, sizeof(spriteCmd), &spriteCmd);
  }

  // draw
  if (gfxHelperDrawTarget != COMMON_DRAW_TARGET_DZO)
  {
    float x = anchorX + offsetX;
    float y = anchorY + offsetY;
    helperAlign(&x, &y, w, h, alignment);
    gfxDrawSprite(x, y, w, h, 0, 0, texWidth, texHeight, color, gfxGetFrameTex(texId));
  }
}

//------------------------------------------------------------------------------
void gfxHelperDrawSprite_WS(VECTOR worldPosition, float w, float h, int texWidth, int texHeight, int texId, u32 color, enum TextAlign alignment)
{
  if (!isInGame())
    return;

  // pass to dzo
  if (gfxHelperDrawTarget != COMMON_DRAW_TARGET_NATIVE && PATCH_DZO_INTEROP_FUNCS)
  {
    CustomDzoCommandDrawWSSprite_t spriteCmd;
    vector_copy(spriteCmd.Position, worldPosition);
    spriteCmd.Alignment = alignment;
    spriteCmd.W = w / SCREEN_HEIGHT;
    spriteCmd.H = h / SCREEN_HEIGHT;
    spriteCmd.SpriteId = texId;
    spriteCmd.CustomSpriteId = 0;
    spriteCmd.Color = color;
    PATCH_DZO_INTEROP_FUNCS->SendCustomCommandToClient(CUSTOM_DZO_CMD_ID_DRAW_WS_SPRITE, sizeof(spriteCmd), &spriteCmd);
  }

  // draw
  if (gfxHelperDrawTarget != COMMON_DRAW_TARGET_DZO)
  {
    int x, y;
    if (gfxWorldSpaceToScreenSpace(worldPosition, &x, &y))
    {
      float fx = x, fy = y;
      helperAlign(&fx, &fy, w, h, alignment);
      gfxDrawSprite(fx, fy, w, h, 0, 0, texWidth, texHeight, color, gfxGetFrameTex(texId));
    }
  }
}

//------------------------------------------------------------------------------
void gfxHelperDrawNativeQuad_WS(const VECTOR positions[4], const u32 colors[4], const struct UV uvs[4], int texId, enum COMMON_QUAD_TEXTURE textureSource, enum COMMON_QUAD_BLEND blend, unsigned short flags, float offsetX, float offsetY)
{
  if (gfxHelperDrawTarget == COMMON_DRAW_TARGET_DZO)
    return;

  // draw textured and untextured quads through the native fx strip renderer
  // initialize the strip context before applying our gs registers
  void *fx = (void *)0x00222590;
  void (*setRegister)(void *, int, u64, int) = (void *)0x004CA6C8;
  void (*initStrip)(void) = (void *)0x004C6DC8;
  void (*drawStrip)(int, VECTOR *, const u32 *, const struct UV *, int, int) = (void *)0x004C9290;
  static const struct
  {
    int Register;
    const u64 *Cached;
  } states[] = {
    { 0x06, (u64 *)0x0023F740 }, // TEX0_1
    { 0x08, (u64 *)0x0023F720 }, // CLAMP_1
    { 0x42, (u64 *)0x0023F718 }, // ALPHA_1
    { 0x47, (u64 *)0x0023F730 }, // TEST_1
    { 0x4E, (u64 *)0x0023F748 }, // ZBUF_1
  };
  u64 saved[5];
  VECTOR vertices[4];
  VECTOR center = { 0, 0, 0, 1 };
  GameCamera *camera = NULL;
  int i, j;

  if (flags & INTERNAL_QUAD_BILLBOARD)
  {
    // get the current render camera, including split-screen views
    camera = cameraGetGameCamera(*(int *)0x0021DD9C);
    if (!camera)
      return;

    // find the billboard center
    for (i = 0; i < 4; ++i)
    {
      for (j = 0; j < 3; ++j)
        center[j] += positions[i][j] * 0.25f;
    }
  }

  // build world vertices
  for (i = 0; i < 4; ++i)
  {
    for (j = 0; j < 3; ++j)
    {
      if (camera)
      {
        // rotate canonical right/toward/up onto the camera axes around the center
        // camera axes are forward, left, up
        vertices[i][j] = center[j] - (positions[i][0] - center[0] + offsetX) * camera->uMtx[1][j] - (positions[i][1] - center[1]) * camera->uMtx[0][j] + (positions[i][2] - center[2] + offsetY) * camera->uMtx[2][j];
      }
      else
      {
        vertices[i][j] = positions[i][j];
      }
    }
    vertices[i][3] = 1;
  }

  // back up the active fx state before changing registers
  int tracked = *(u8 *)0x0023F700;
  if (tracked)
  {
    for (i = 0; i < 5; ++i)
      saved[i] = *states[i].Cached;
  }

  // resolve texture
  u64 texture = 0;
  if (texId >= 0)
    texture = textureSource == COMMON_QUAD_TEXTURE_FRAME ? gfxGetFrameTex(texId) : gfxGetEffectTex(texId, 1);

  // blend equation: Cs for opaque, (Cs-Cd)*As+Cd for alpha, Cs*As+Cd for additive
  u64 alpha = 0x44;
  if (blend == COMMON_QUAD_BLEND_OPAQUE)
    alpha = 0x80000000A8ULL;
  else if (blend == COMMON_QUAD_BLEND_ADDITIVE)
    alpha = 0x48;

  // depth writes are independent of the blend mode
  u64 test = flags & INTERNAL_QUAD_NO_DEPTH_TEST ? 0x30000 : 0x50000;
  u64 zbuf = (tracked ? saved[4] : *(u64 *)0x00171520) & ~(1ULL << 32);
  if (!(flags & INTERNAL_QUAD_ZWRITE))
    zbuf |= 1ULL << 32;

  // DrawStrip can reset ZBUF_1 during initialization, after register writes
  // prepare first and use its strip writer directly
  if (!tracked || *(int *)0x00222618 != 8)
    initStrip();

  // draw
  setRegister(fx, 0x06, texture, 1);
  setRegister(fx, 0x08, flags & INTERNAL_QUAD_CLAMP_UV ? 5 : 0, 1);
  setRegister(fx, 0x42, alpha, 1);
  setRegister(fx, 0x47, test, 1);
  setRegister(fx, 0x4E, zbuf, 1);
  drawStrip(4, vertices, colors, uvs, 1, texId >= 0);

  // restore cached fx state, or normal gs defaults for subsequent hud draws
  if (tracked)
  {
    for (i = 0; i < 5; ++i)
      setRegister(fx, states[i].Register, saved[i], 1);
  }
  else
  {
    gfxResetGsRegisters();
  }
}

//------------------------------------------------------------------------------
void gfxHelperDrawQuadEx_WS(const VECTOR positions[4], const u32 colors[4], const struct UV uvs[4], int texId, enum COMMON_QUAD_TEXTURE textureSource, enum COMMON_QUAD_BLEND blend, unsigned short flags, float offsetX, float offsetY)
{
  if (!isInGame())
    return;
  if (!positions || !colors || !uvs || texId < -1)
    return;
  if ((unsigned)textureSource > COMMON_QUAD_TEXTURE_FRAME || (unsigned)blend > COMMON_QUAD_BLEND_ADDITIVE)
    return;
  if (flags & ~(INTERNAL_QUAD_BILLBOARD | INTERNAL_QUAD_NO_DEPTH_TEST | INTERNAL_QUAD_CLAMP_UV | INTERNAL_QUAD_ZWRITE))
    return;

  // pass to dzo
  if (gfxHelperDrawTarget != COMMON_DRAW_TARGET_NATIVE && PATCH_DZO_INTEROP_FUNCS)
  {
    CustomDzoCommandDrawWSQuad_t cmd;
    int i;
    for (i = 0; i < 4; ++i)
    {
      memcpy(cmd.Positions[i], positions[i], sizeof(cmd.Positions[i]));
      cmd.Colors[i] = colors[i];
      cmd.UVs[i][0] = uvs[i].U;
      cmd.UVs[i][1] = uvs[i].V;
    }
    cmd.TextureId = texId;
    cmd.TextureSource = textureSource;
    cmd.Blend = blend;
    cmd.Flags = flags;
    cmd.BillboardOffset[0] = offsetX;
    cmd.BillboardOffset[1] = offsetY;
    PATCH_DZO_INTEROP_FUNCS->SendCustomCommandToClient(CUSTOM_DZO_CMD_ID_DRAW_WS_QUAD, sizeof(cmd), &cmd);
  }

  // draw
  gfxHelperDrawNativeQuad_WS(positions, colors, uvs, texId, textureSource, blend, flags, offsetX, offsetY);
}

//------------------------------------------------------------------------------
void gfxHelperDrawRectQuad_WS(MATRIX worldMatrix, float width, float height, u32 color, int texId, enum COMMON_QUAD_TEXTURE textureSource, enum COMMON_QUAD_BLEND blend, unsigned short flags, float offsetX, float offsetY)
{
  if (!worldMatrix)
    return;

  VECTOR positions[4];
  u32 colors[4] = { color, color, color, color };
  const struct UV uvs[4] = { { 0, 0 }, { 1, 0 }, { 0, 1 }, { 1, 1 } };
  int i;

  // transform the centered rectangle from local x/z into world space
  for (i = 0; i < 4; ++i)
  {
    VECTOR corner = {
      (i & 1 ? 0.5f : -0.5f) * width,
      0,
      (i & 2 ? -0.5f : 0.5f) * height,
      1
    };
    vector_apply(positions[i], corner, worldMatrix);
    positions[i][3] = 1;
  }
  gfxHelperDrawQuadEx_WS((const VECTOR *)positions, colors, uvs, texId, textureSource, blend, flags, offsetX, offsetY);
}

//------------------------------------------------------------------------------
int gfxHelperGetInternalQuadFlags(enum COMMON_QUAD_FLAGS flags)
{
  if (flags & ~(COMMON_QUAD_ZWRITE | COMMON_QUAD_CLAMP))
    return -1;

  // map public draw options to the native and dzo flags
  int internalFlags = 0;
  if (flags & COMMON_QUAD_ZWRITE)
    internalFlags |= INTERNAL_QUAD_ZWRITE;
  if (flags & COMMON_QUAD_CLAMP)
    internalFlags |= INTERNAL_QUAD_CLAMP_UV;
  return internalFlags;
}

//------------------------------------------------------------------------------
void gfxHelperDrawQuad_WS(MATRIX worldMatrix, u32 color, int texId, enum COMMON_QUAD_TEXTURE textureSource, enum COMMON_QUAD_BLEND blend, enum COMMON_QUAD_FLAGS flags)
{
  int internalFlags = gfxHelperGetInternalQuadFlags(flags);
  if (internalFlags < 0)
    return;

  gfxHelperDrawRectQuad_WS(worldMatrix, 1, 1, color, texId, textureSource, blend, internalFlags, 0, 0);
}

//------------------------------------------------------------------------------
void gfxHelperDrawQuadFromEuler_WS(VECTOR position, float width, float height, VECTOR rotation, u32 color, int texId, enum COMMON_QUAD_TEXTURE textureSource, enum COMMON_QUAD_BLEND blend, enum COMMON_QUAD_FLAGS flags)
{
  if (!position || !rotation)
    return;

  // size the local x/z rectangle before applying rotation and translation
  VECTOR scale = { width, 1, height, 0 };
  MATRIX worldMatrix;
  matrix_trs(worldMatrix, position, rotation, scale);
  gfxHelperDrawQuad_WS(worldMatrix, color, texId, textureSource, blend, flags);
}

//------------------------------------------------------------------------------
void gfxHelperDrawBillboard_WS(const VECTOR position, float offsetX, float offsetY, float width, float height, float rotation, u32 color, int texId, enum COMMON_QUAD_TEXTURE textureSource, enum COMMON_QUAD_BLEND blend, enum COMMON_QUAD_FLAGS flags)
{
  int internalFlags = gfxHelperGetInternalQuadFlags(flags);
  if (internalFlags < 0)
    return;

  if (!position)
    return;

  // rotate in the canonical x/z plane before facing the camera
  float c = cosf(rotation), s = sinf(rotation);
  MATRIX worldMatrix = {
    c, 0, s, 0,
    0, 1, 0, 0,
    -s, 0, c, 0,
    position[0], position[1], position[2], 1
  };
  gfxHelperDrawRectQuad_WS(worldMatrix, width, height, color, texId, textureSource, blend, internalFlags | INTERNAL_QUAD_BILLBOARD, offsetX, offsetY);
}

//------------------------------------------------------------------------------
int intArrayContains(int *list, int count, int value)
{
  int i;

  for (i = 0; i < count; ++i)
    if (list[i] == value)
      return 1;

  return 0;
}

//------------------------------------------------------------------------------
int charArrayContains(char *list, int count, char value)
{
  int i;

  for (i = 0; i < count; ++i)
    if (list[i] == value)
      return 1;

  return 0;
}

//------------------------------------------------------------------------------
int libcRand(void)
{
  return ((int (*)(void))0x001633d0)();
}
