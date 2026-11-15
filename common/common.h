#ifndef _PATCH_COMMON_
#define _PATCH_COMMON_

#include <libdl/graphics.h>
#include <libdl/math3d.h>

enum COMMON_DRAW_TARGET
{
  COMMON_DRAW_TARGET_BOTH = 0,
  COMMON_DRAW_TARGET_NATIVE = 1,
  COMMON_DRAW_TARGET_DZO = 2,
};

enum COMMON_QUAD_TEXTURE
{
  COMMON_QUAD_TEXTURE_FX = 0,
  COMMON_QUAD_TEXTURE_FRAME = 1,
};

enum COMMON_QUAD_BLEND
{
  COMMON_QUAD_BLEND_OPAQUE = 0,
  COMMON_QUAD_BLEND_SEMITRANSPARENT = 1,
  COMMON_QUAD_BLEND_ADDITIVE = 2,
};

enum COMMON_QUAD_FLAGS
{
  COMMON_QUAD_ZWRITE = 1,
  COMMON_QUAD_CLAMP = 2,
};

/*
 * NAME :    randomUnit
 *
 * DESCRIPTION :
 *       Combines two native 15-bit random draws into a 30-bit sample.
 *
 * NOTES :
 *       Returns double to preserve the low bits when scaling to the unit range.
 *
 * ARGS :
 *
 * RETURN :
 *       A random value greater than or equal to 0 and less than 1.
 *
 */
double randomUnit(void);

/*
 * NAME :    rollChance
 *
 * DESCRIPTION :
 *       Tests a probability using randomUnit.
 *
 * NOTES :
 *       Chances at or below 0 always fail; chances at or above 1 always succeed.
 *       NaN fails. These cases do not consume random draws.
 *
 * ARGS :
 *      chance:         Normalized probability; 0.01f represents 1%.
 *
 * RETURN :
 *       1 if the roll succeeds, otherwise 0.
 *
 */
int rollChance(float chance);

/*
 * NAME :    helperAlign
 *
 * DESCRIPTION :
 *       Transforms the point by the given alignment.
 *
 * NOTES :
 *
 * ARGS :
 *
 * RETURN :
 *
 * AUTHOR :      Daniel "Dnawrkshp" Gerendasy
 */
void helperAlign(float *pX, float *pY, float w, float h, enum TextAlign alignment);

/*
 * NAME :    helperRealign
 *
 * DESCRIPTION :
 *       Transforms the point from one alignment to another.
 *
 * NOTES :
 *
 * ARGS :
 *
 * RETURN :
 *
 * AUTHOR :      Daniel "Dnawrkshp" Gerendasy
 */
void helperRealign(float *pX, float *pY, float w, float h, enum TextAlign fromAlignment, enum TextAlign toAlignment);

//------------------------------------------------------------------------------
//------------------------------------ DRAW ------------------------------------
//------------------------------------------------------------------------------

/*
 * NAME :    gfxHelperSetDrawTarget
 *
 * DESCRIPTION :
 *       Selects the draw destination for subsequent gfx helper calls.
 *
 * NOTES :
 *       BOTH is the default. The setting persists until changed or reset.
 *       The setting applies to this linked copy of common.
 *
 * ARGS :
 *      target:         BOTH, NATIVE, or DZO. Invalid targets are ignored.
 *
 * RETURN :
 *
 */
void gfxHelperSetDrawTarget(enum COMMON_DRAW_TARGET target);

/*
 * NAME :    gfxHelperResetDrawTarget
 *
 * DESCRIPTION :
 *       Restores COMMON_DRAW_TARGET_BOTH for subsequent gfx helper calls.
 *
 * RETURN :
 *
 */
void gfxHelperResetDrawTarget(void);

/*
 * NAME :    gfxHelperDrawBox
 *
 * DESCRIPTION :
 *       Draws a box on the screen.
 *
 * NOTES :
 *       Native bounds snap to pixels; positive subpixel sizes draw at least one pixel.
 *       DZO retains the original floating point bounds.
 *       Draw destinations are selected by gfxHelperSetDrawTarget.
 *
 * ARGS :
 *      x:              Screen X position (0-SCREEN_WIDTH).
 *      y:              Screen Y position (0-SCREEN_HEIGHT).
 *      w:              Width (0-SCREEN_WIDTH).
 *      h:              Height (0-SCREEN_HEIGHT).
 *      color:          Color of box.
 *      alignment:
 *      dzoStretched:   Nonzero stretches the DZO box to the screen aspect ratio.
 *
 * RETURN :
 *
 * AUTHOR :      Daniel "Dnawrkshp" Gerendasy
 */
void gfxHelperDrawBox(float x, float y, float offsetX, float offsetY, float w, float h, u32 color, enum TextAlign alignment, int dzoStretched);
void gfxHelperDrawBox_WS(VECTOR worldPosition, float w, float h, u32 color, enum TextAlign alignment);

/*
 * NAME :    gfxHelperDrawText
 *
 * DESCRIPTION :
 *       Draws text on the screen.
 *
 * NOTES :
 *       Draw destinations are selected by gfxHelperSetDrawTarget.
 *
 * ARGS :
 *      x:              Screen X position (0-SCREEN_WIDTH).
 *      y:              Screen Y position (0-SCREEN_HEIGHT).
 *      scale:          Text scale.
 *      color:          color of box.
 *      str:            String to draw. Max 64 characters for DZO.
 *      length:         Number of characters to draw. -1 to draw full length of string.
 *      alignment:      Text alignment.
 *
 * RETURN :
 *
 * AUTHOR :      Daniel "Dnawrkshp" Gerendasy
 */
void gfxHelperDrawText(float x, float y, float offsetX, float offsetY, float scale, u32 color, char *str, int length, enum TextAlign alignment);
void gfxHelperDrawText_WS(VECTOR worldPosition, float scale, u32 color, char *str, int length, enum TextAlign alignment);

/*
 * NAME :    gfxHelperDrawTextWindow
 *
 * DESCRIPTION :
 *       Draws text on the screen constrained to a window.
 *
 * NOTES :
 *       Native layout is still measured when drawing only to DZO.
 *       Draw destinations are selected by gfxHelperSetDrawTarget.
 *
 * ARGS :
 *      x:              Screen X position (0-SCREEN_WIDTH).
 *      y:              Screen Y position (0-SCREEN_HEIGHT).
 *      scale:          Text scale.
 *      color:          color of box.
 *      str:            String to draw. Max 64 characters for DZO.
 *      length:         Number of characters to draw. -1 to draw full length of string.
 *      alignment:      Text alignment.
 *
 * RETURN :
 *      Total height of wrapped text.
 *
 * AUTHOR :      Daniel "Dnawrkshp" Gerendasy
 */
float gfxHelperDrawTextWindow(float x, float y, float offsetX, float offsetY, float width, float height, float textOffsetX, float textOffsetY, float scale, u32 color, char *str, int length, enum TextAlign alignment, enum FontWindowFlags flags);

/*
 * NAME :    gfxHelperDrawSprite
 *
 * DESCRIPTION :
 *       Draws a sprite on the screen.
 *
 * NOTES :
 *       Draw destinations are selected by gfxHelperSetDrawTarget.
 *
 * ARGS :
 *      x:              Screen X position (0-SCREEN_WIDTH).
 *      y:              Screen Y position (0-SCREEN_HEIGHT).
 *      w:              Width (0-SCREEN_WIDTH).
 *      h:              Height (0-SCREEN_HEIGHT).
 *      texWidth:       Pixel width of the given texture.
 *      texHeight:      Pixel height of the given texture.
 *      color:          Color of box.
 *      alignment:      Image alignment.
 *      dzoStretched:   Nonzero stretches the DZO sprite to the screen aspect ratio.
 *
 * RETURN :
 *
 * AUTHOR :      Daniel "Dnawrkshp" Gerendasy
 */
void gfxHelperDrawSprite(float x, float y, float offsetX, float offsetY, float w, float h, int texWidth, int texHeight, int texId, u32 color, enum TextAlign alignment, int dzoStretched);
void gfxHelperDrawSprite_WS(VECTOR worldPosition, float w, float h, int texWidth, int texHeight, int texId, u32 color, enum TextAlign alignment);

/*
 * NAME :    gfxHelperDrawQuad_WS
 *
 * DESCRIPTION :
 *       Draws a transformed rectangle natively and on the DZO client.
 *
 * NOTES :
 *       Call every frame from the codegen draw hook or an FX draw callback.
 *       Native textured draws require caller-owned gfxSetupGifPaging(0)/gfxDoGifPaging.
 *       Reuse the engine's active paging block inside FX draw callbacks.
 *       Draw destinations are selected by gfxHelperSetDrawTarget.
 *       The unit rectangle is centered at the local origin in the X/Z plane.
 *       Build worldMatrix with libdl matrix functions to position, rotate, and size it.
 *       The supplied matrix is read without modification.
 *       Uses the full texture UVs and a uniform color.
 *       Quads are double-sided, unlit, and independent of the HUD overlay.
 *       Depth testing is always enabled. ZWRITE enables depth writes for any blend mode.
 *       UVs repeat by default; CLAMP clamps them to the texture bounds.
 *
 * ARGS :
 *      worldMatrix:    Affine local-to-world transform, including width/height scaling.
 *      color:          PS2 RGBA. 0x80 alpha is opaque; 0x80 RGB is neutral modulation.
 *      texId:          Texture id. -1 draws an untextured quad.
 *      textureSource:  Effect or frame texture source.
 *      blend:          Opaque, semitransparent, or additive blending.
 *      flags:          Bitwise COMMON_QUAD_FLAGS. 0 disables depth writes and repeats UVs.
 *
 * RETURN :
 *
 */
void gfxHelperDrawQuad_WS(MATRIX worldMatrix, u32 color, int texId, enum COMMON_QUAD_TEXTURE textureSource, enum COMMON_QUAD_BLEND blend, enum COMMON_QUAD_FLAGS flags);

/*
 * NAME :    gfxHelperDrawQuadFromEuler_WS
 *
 * DESCRIPTION :
 *       Builds a transform and draws a rectangle natively and on the DZO client.
 *
 * NOTES :
 *       Uses matrix_trs to scale, rotate around X/Y/Z, then translate.
 *       Position and rotation use XYZ only and are read without modification.
 *       Draw destinations, texture, depth, and blend behavior match gfxHelperDrawQuad_WS.
 *       Uses the same caller-owned GIF paging as gfxHelperDrawQuad_WS.
 *
 * ARGS :
 *      position:       Rectangle center in PS2 world XYZ.
 *      width:          Width along local +X, before rotation.
 *      height:         Height along local +Z, before rotation.
 *      rotation:       XYZ Euler angles in radians.
 *      color:          PS2 RGBA. 0x80 alpha is opaque; 0x80 RGB is neutral modulation.
 *      texId:          Texture id. -1 draws an untextured quad.
 *      textureSource:  Effect or frame texture source.
 *      blend:          Opaque, semitransparent, or additive blending.
 *      flags:          Bitwise COMMON_QUAD_FLAGS. 0 disables depth writes and repeats UVs.
 *
 * RETURN :
 *
 */
void gfxHelperDrawQuadFromEuler_WS(VECTOR position, float width, float height, VECTOR rotation, u32 color, int texId, enum COMMON_QUAD_TEXTURE textureSource, enum COMMON_QUAD_BLEND blend, enum COMMON_QUAD_FLAGS flags);

/*
 * NAME :    gfxHelperDrawBillboard_WS
 *
 * DESCRIPTION :
 *       Draws a centered billboard natively and on the DZO client.
 *
 * NOTES :
 *       Call every frame from the codegen draw hook or an FX draw callback.
 *       Draw destinations are selected by gfxHelperSetDrawTarget.
 *       Uses the full texture UVs and a uniform color, with the same depth and blend
 *       behavior as gfxHelperDrawQuad_WS.
 *       Uses the same caller-owned GIF paging as gfxHelperDrawQuad_WS.
 *
 * ARGS :
 *      position:       Billboard anchor in PS2 world XYZ.
 *      offsetX:        Camera-right offset in world units, independent of rotation.
 *      offsetY:        Camera-up offset in world units, independent of rotation.
 *      width:          Width in world units.
 *      height:         Height in world units.
 *      rotation:       In-plane rotation in radians. Positive rotates +X toward +Z.
 *      color:          PS2 RGBA. 0x80 alpha is opaque; 0x80 RGB is neutral modulation.
 *      texId:          Texture id. -1 draws an untextured billboard.
 *      textureSource:  Effect or frame texture source.
 *      blend:          Opaque, semitransparent, or additive blending.
 *      flags:          Bitwise COMMON_QUAD_FLAGS. 0 disables depth writes and repeats UVs.
 *
 * RETURN :
 *
 */
void gfxHelperDrawBillboard_WS(const VECTOR position, float offsetX, float offsetY, float width, float height, float rotation, u32 color, int texId, enum COMMON_QUAD_TEXTURE textureSource, enum COMMON_QUAD_BLEND blend, enum COMMON_QUAD_FLAGS flags);

//------------------------------------------------------------------------------
int intArrayContains(int *list, int count, int value);
int charArrayContains(char *list, int count, char value);
int libcRand(void);

#endif // _PATCH_COMMON_
