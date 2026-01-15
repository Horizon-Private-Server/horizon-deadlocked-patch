#ifndef _PATCH_COMMON_
#define _PATCH_COMMON_

#include <libdl/graphics.h>
#include <libdl/math3d.h>

enum COMMON_DZO_DRAW_TYPE
{
  COMMON_DZO_DRAW_NONE = 0,
  COMMON_DZO_DRAW_NORMAL,
  COMMON_DZO_DRAW_STRETCHED,
  COMMON_DZO_DRAW_ONLY,
};

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
void helperAlign(float* pX, float* pY, float w, float h, enum TextAlign alignment);

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
void helperRealign(float* pX, float* pY, float w, float h, enum TextAlign fromAlignment, enum TextAlign toAlignment);

//------------------------------------------------------------------------------
//------------------------------------ DRAW ------------------------------------
//------------------------------------------------------------------------------


/*
 * NAME :    gfxHelperDrawBox
 * 
 * DESCRIPTION :
 *       Draws a box on the screen.
 * 
 * NOTES :
 * 
 * ARGS : 
 *      x:              Screen X position (0-SCREEN_WIDTH).
 *      y:              Screen Y position (0-SCREEN_HEIGHT).
 *      w:              Width (0-SCREEN_WIDTH).
 *      h:              Height (0-SCREEN_HEIGHT).
 *      color:          Color of box.
 *      alignment:      
 *      dzoDrawType:    DZO draw type.
 * 
 * RETURN :
 * 
 * AUTHOR :      Daniel "Dnawrkshp" Gerendasy
 */
void gfxHelperDrawBox(float x, float y, float offsetX, float offsetY, float w, float h, u32 color, enum TextAlign alignment, enum COMMON_DZO_DRAW_TYPE dzoDrawType);
void gfxHelperDrawBox_WS(VECTOR worldPosition, float w, float h, u32 color, enum TextAlign alignment, enum COMMON_DZO_DRAW_TYPE dzoDrawType);

/*
 * NAME :    gfxHelperDrawText
 * 
 * DESCRIPTION :
 *       Draws text on the screen.
 * 
 * NOTES :
 * 
 * ARGS : 
 *      x:              Screen X position (0-SCREEN_WIDTH).
 *      y:              Screen Y position (0-SCREEN_HEIGHT).
 *      scale:          Text scale.
 *      color:          color of box.
 *      str:            String to draw. Max 64 characters for DZO.
 *      length:         Number of characters to draw. -1 to draw full length of string.
 *      alignment:      Text alignment.
 *      dzoDraw:        Whether to draw on the DZO client.
 * 
 * RETURN :
 * 
 * AUTHOR :      Daniel "Dnawrkshp" Gerendasy
 */
void gfxHelperDrawText(float x, float y, float offsetX, float offsetY, float scale, u32 color, char* str, int length, enum TextAlign alignment, enum COMMON_DZO_DRAW_TYPE dzoDrawType);
void gfxHelperDrawText_WS(VECTOR worldPosition, float scale, u32 color, char* str, int length, enum TextAlign alignment, enum COMMON_DZO_DRAW_TYPE dzoDrawType);

/*
 * NAME :    gfxHelperDrawTextWindow
 * 
 * DESCRIPTION :
 *       Draws text on the screen constrained to a window.
 * 
 * NOTES :
 * 
 * ARGS : 
 *      x:              Screen X position (0-SCREEN_WIDTH).
 *      y:              Screen Y position (0-SCREEN_HEIGHT).
 *      scale:          Text scale.
 *      color:          color of box.
 *      str:            String to draw. Max 64 characters for DZO.
 *      length:         Number of characters to draw. -1 to draw full length of string.
 *      alignment:      Text alignment.
 *      dzoDraw:        Whether to draw on the DZO client.
 * 
 * RETURN :
 * 
 * AUTHOR :      Daniel "Dnawrkshp" Gerendasy
 */
void gfxHelperDrawTextWindow(float x, float y, float offsetX, float offsetY, float width, float height, float textOffsetX, float textOffsetY, float scale, u32 color, char* str, int length, enum TextAlign alignment, enum FontWindowFlags flags, enum COMMON_DZO_DRAW_TYPE dzoDrawType);

/*
 * NAME :    gfxHelperDrawSprite
 * 
 * DESCRIPTION :
 *       Draws a sprite on the screen.
 * 
 * NOTES :
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
 *      dzoDrawType:    DZO draw type.
 * 
 * RETURN :
 * 
 * AUTHOR :      Daniel "Dnawrkshp" Gerendasy
 */
void gfxHelperDrawSprite(float x, float y, float offsetX, float offsetY, float w, float h, int texWidth, int texHeight, int texId, u32 color, enum TextAlign alignment, enum COMMON_DZO_DRAW_TYPE dzoDrawType);
void gfxHelperDrawSprite_WS(VECTOR worldPosition, float w, float h, int texWidth, int texHeight, int texId, u32 color, enum TextAlign alignment, enum COMMON_DZO_DRAW_TYPE dzoDrawType);

//------------------------------------------------------------------------------
int intArrayContains(int* list, int count, int value);
int charArrayContains(char* list, int count, char value);
int libcRand(void);

#endif // _PATCH_COMMON_
