/** \mainpage gLib2D Documentation
 *
 * \section intro Introduction
 *
 * gLib2D by Geecko - A simple, fast, light-weight 2D graphics library. \n\n
 * This library has been designed to replace the old graphics.c library
 * and to simplify the use of the pspgu.\n
 * The goals : keep it simple, keep it small, keep it fast.
 *
 * \section limits Known limitations
 *
 * - Only rectangles can be used. Other primitives,
 *   like triangles or lines are just skipped.
 * - When some 512*512 rotated, colorized and scaled textures are rendered
 *   at a time, the framerate *could* go under 60 fps.
 *
 * \section install Installation
 *
 * - Simply put glib2d.c and glib2d.h in your source directory.\n
 * - Then add glib2d.o and link "-lpng -ljpeg -lz -lpspgu -lm" in your Makefile.
 * - You're done !
 *
 * \section copyright License
 *
 * This work is licensed under the Creative Commons BY-SA 3.0 License. \n
 * See http://creativecommons.org/licenses/by-sa/3.0/ or the LICENSE file
 * for more details. \n
 * You can support the library by marking your homebrew with
 * "Using gLib2D by Geecko".
 *
 * \section contact Contact
 *
 * Please report bugs or submit ideas at : \n geecko.dev@free.fr \n
 * Stay tuned on Twitter or Tumblr : \n
 * http://twitter.com/GeeckoDev \n http://geeckodev.tumblr.com
 */


/**
 * \file glib2d.h
 * \brief gLib2D Header
 * \version Beta 3
 */

#ifndef GLIB2D_H
#define GLIB2D_H

/**
 * \def USE_PNG
 * \brief Choose if the PNG support is enabled.
 *
 * Otherwise, this part will be not compiled to gain some space.
 * Enable this to get PNG support, disable to avoid compilation errors
 * when libpng is not linked in the Makefile.
 */ 
/**
 * \def USE_JPEG
 * \brief Choose if the JPEG support is enabled.
 *
 * Otherwise, this part will be not compiled to gain some space.
 * Enable this to get JPEG support, disable to avoid compilation errors
 * when libjpeg is not linked in the Makefile.
 */ 
 
#define USE_PNG
#define USE_JPEG

/**
 * \def G_SCR_W
 * \brief Screen width constant, in pixels.
 */ 
/**
 * \def G_SCR_H
 * \brief Screen height constant, in pixels.
 */ 
/**
 * \def G_FALSE
 * \brief Boolean constant, false or 0
 */ 
/**
 * \def G_TRUE
 * \brief Boolean constant, true or 1
 */ 
 
#define G_SCR_W (480)
#define G_SCR_H (272)
#define G_FALSE 0
#define G_TRUE !0

/**
 * \def G_RGBA(r,g,b,a)
 * \brief Create a gColor.
 *
 * This macro creates a gColor from 4 values, red, green, blue and alpha.
 * Input range is from 0 to 255.
 */ 
#define G_RGBA(r,g,b,a) \
((r)|((g)<<8)|((b)<<16)|((a)<<24))

/**
 * \def G_GET_R(color)
 * \brief Get red channel value from a gColor.
 */ 
/**
 * \def G_GET_G(color)
 * \brief Get green channel value from a gColor.
 */ 
/**
 * \def G_GET_B(color)
 * \brief Get blue channel value from a gColor.
 */ 
/**
 * \def G_GET_A(color)
 * \brief Get alpha channel value from a gColor.
 */ 

#define G_GET_R(color) (((color)    ) & 0xFF)
#define G_GET_G(color) (((color)>>8 ) & 0xFF)
#define G_GET_B(color) (((color)>>16) & 0xFF)
#define G_GET_A(color) (((color)>>24) & 0xFF)

/**
 * \def G_MODULATE(color,luminance,alpha)
 * \brief gColor modulation.
 *
 * This macro modulates the luminance & alpha of a gColor.
 * Input range is from 0 to 255. 
 */ 
 
#define G_MODULATE(color,luminance,alpha) \
G_RGBA((int)(luminance)*G_GET_R(color)/255, \
       (int)(luminance)*G_GET_G(color)/255, \
       (int)(luminance)*G_GET_B(color)/255, \
       (int)(alpha    )*G_GET_A(color)/255)

/**
 * \enum gColors
 * \brief Colors enumeration.
 *
 * Primary, secondary, tertiary and grayscale colors are defined.
 */

enum gColors
{
  // Primary colors
  RED          = 0xFF0000FF,
  GREEN        = 0xFF00FF00,
  BLUE         = 0xFFFF0000,
  // Secondary colors
  CYAN         = 0xFFFFFF00,
  MAGENTA      = 0xFFFF00FF,
  YELLOW       = 0xFF00FFFF,
  // Tertiary colors
  AZURE        = 0xFFFF7F00,
  VIOLET       = 0xFFFF007F,
  ROSE         = 0xFF7F00FF,
  ORANGE       = 0xFF007FFF,
  CHARTREUSE   = 0xFF00FF7F,
  SPRING_GREEN = 0xFF7FFF00,
  // Grayscale
  WHITE        = 0xFFFFFFFF,
  LITEGRAY     = 0xFFBFBFBF,
  GRAY         = 0xFF7F7F7F,
  DARKGRAY     = 0xFF3F3F3F,  
  BLACK        = 0xFF000000
};

/**
 * \enum gCoord_Mode
 * \brief Coord modes enumeration.
 *
 * Choose where the coordinates correspond in the object.
 * This can be a corner or the center.
 */ 
 
enum gCoord_Mode { G_UP_LEFT, G_UP_RIGHT, 
                    G_DOWN_RIGHT, G_DOWN_LEFT,
                    G_CENTER };

/**
 * \var bool
 * \brief Boolean type.
 */
/**
 * \var gAlpha
 * \brief Alpha type.
 */
/**
 * \var gColor
 * \brief Color type.
 */
/**
 * \var gEnum
 * \brief Enumeration type.
 */

typedef char bool;
typedef int gAlpha;
typedef unsigned int gColor;
typedef int gEnum;

/**
 * \struct gImage
 * \brief Image structure.
 */

typedef struct
{
  int tw;         /**< Real texture width. A power of two. */
  int th;         /**< Real texture height. A power of two. */
  int w;          /**< Texture width, as seen when draw. */
  int h;          /**< Texture height, as seen when draw. */
  float ratio;    /**< Width/Height ratio. */
  bool swizzled;  /**< Is the texture swizzled ? */
  bool can_blend; /**< Can the texture blend ? */
  gColor* data;   /**< Pointer to the texture raw data. */
} gImage;

/**
 * \brief Clears screen & depth buffer, starts rendering.
 * @param color Screen clear color
 *
 * This function clears the screen, and calls gClearZ() if depth coordinate
 * is used in the loop. You MUST call this function at the beginning of the
 * loop to start the render process. Will automatically init the GU.
 */
 
void gClear(gColor color);

/**
 * \brief Clears depth buffer.
 *
 * This function clears the zbuffer to zero (z range 0-65535).
 */
 
void gClearZ();

/**
 * \brief Begins rectangles rendering.
 * @param tex Pointer to a texture, pass NULL to get a colored rectangle.
 *
 * This function begins object rendering. Calls gReset().
 * One gAdd() call per object.
 * Only one texture can be used, but multiple objects can be rendered at a time.
 * gBegin*() / gEnd() couple can be called multiple times in the loop,
 * to render multiple textures.
 */

void gBeginRects(gImage* tex);

/**
 * \brief Begins lines rendering.
 *
 * This function begins object rendering. Calls gReset().
 * Two gAdd() calls per object.
 */

void gBeginLines();

/**
 * \brief Begins quads rendering.
 * @param tex Pointer to a texture, pass NULL to get a colored quad.
 *
 * This function begins object rendering. Calls gReset().
 * Four gAdd() calls per object, first is for up left corner, then clockwise.
 * Only one texture can be used, but multiple objects can be rendered at a time.
 * gBegin*() / gEnd() couple can be called multiple times in the loop,
 * to render multiple textures.
 */

void gBeginQuads(gImage* tex);

/**
 * \brief Ends object rendering.
 *
 * This function ends object rendering. Must be called after gBegin*() to add
 * objects to the display list. Automatically adapts pspgu functionnalities
 * to get the best performance possible.
 */

void gEnd();

/**
 * \brief Resets current transformation and attribution.
 *
 * This function must be called during object rendering.
 * Calls gResetCoord(), gResetRotation(), gResetScale(),
 * gResetColor(), gResetAlpha() and gResetCrop().
 */

void gReset();

/**
 * \brief Flips the screen.
 * @param use_vsync Limit FPS to 60 ?
 *
 * This function must be called at the end of the loop.
 * Inverts screen buffers to display the whole thing.
 */

void gFlip(bool use_vsync);

/**
 * \brief Pushes the current transformation & attribution to a new object.
 *
 * This function must be called during object rendering.
 * This is a *basic* function.
 */

void gAdd();

/**
 * \brief Saves the current transformation to stack.
 *
 * This function must be called during object rendering.
 * The stack is 64 saves high.
 * Use it like the OpenGL one.
 */
 
void gPush();

/**
 * \brief Restore the current transformation from stack.
 *
 * This function must be called during object rendering.
 * The stack is 64 saves high.
 * Use it like the OpenGL one.
 */

void gPop();

/**
 * \brief Frees an image & set the pointer to NULL.
 * @param tex Pointer to the variable which contains the pointer.
 *
 * This function is used to gain memory when an image is useless.
 * Must pass the pointer to the variable which contains the pointer,
 * to set it to NULL. This is a more secure approach.
 */

void gTexFree(gImage** tex);

/**
 * \brief Loads an image.
 * @param path Path to the file.
 * @param use_swizzle Pass G_TRUE to get *more* speed (recommended).
 *                    Pass G_FALSE to avoid artifacts.
 * @returns Pointer to the image.
 *
 * This function loads an image file. There is support for PNG & JPEG files
 * (if USE_PNG and USE_JPEG are defined). Swizzling is enabled only for 16*16+
 * textures (useless on small textures). Image support up to 512*512 only.
 */

gImage* gTexLoad(char path[], bool use_swizzle);

/**
 * \brief Resets the current coordinates.
 *
 * This function must be called during object rendering.
 * Sets gSetCoordMode() to G_UP_LEFT and gSetCoordXYZ() to (0,0,0).
 */

void gResetCoord();

/**
 * \brief Set coordinate mode.
 * @param mode A gCoord_Mode.
 *
 * This function must be called during object rendering.
 * Defines where the coordinates correspond in the object.
 * Works even if the texture is inverted.
 */

void gSetCoordMode(gEnum mode);

/**
 * \brief Gets the current position.
 * @param x Pointer to save the current x (in pixels).
 * @param y Pointer to save the current y (in pixels).
 * @param z Pointer to save the current z (in pixels).
 *
 * This function must be called during object rendering.
 * Parameters are pointers to float, not int !
 * Pass NULL if not needed.
 */

void gGetCoordXYZ(float* x, float* y, float* z);

/**
 * \brief Sets the new position.
 * @param x New x, in pixels.
 * @param y New y, in pixels.
 *
 * This function must be called during object rendering.
 */

void gSetCoordXY(float x, float y);

/**
 * \brief Sets the new position, with depth support.
 * @param x New x, in pixels.
 * @param y New y, in pixels.
 * @param z New z, in pixels. (0-65535)
 *
 * This function must be called during object rendering.
 */

void gSetCoordXYZ(float x, float y, float z);

/**
 * \brief Sets the new position, relative to the current.
 * @param x New x increment, in pixels.
 * @param y New y increment, in pixels.
 * @param use_rot Take account of the rotation ?
 *
 * This function must be called during object rendering.
 */

void gSetCoordXYRelative(float x, float y, bool use_rot);

/**
 * \brief Sets the new position, with depth support, relative to the current.
 * @param x New x increment, in pixels.
 * @param y New y increment, in pixels.
 * @param z New z increment, in pixels.
 * @param use_rot Take account of the rotation ?
 *
 * This function must be called during object rendering.
 */

void gSetCoordXYZRelative(float x, float y, float z, bool use_rot);

/**
 * \brief Resets the current scale.
 *
 * This function must be called during object rendering.
 * Sets the scale to the current image size or (10,10).
 */

void gResetScale();

/**
 * \brief Gets the current scale.
 * @param w Pointer to save the current width (in pixels).
 * @param h Pointer to save the current height (in pixels).
 *
 * This function must be called during object rendering.
 * Parameters are pointers to float, not int !
 * Pass NULL if not needed.
 */

void gGetScaleWH(float* w, float* h);

/**
 * \brief Sets the new scale.
 * @param w Width scale factor.
 * @param h Height scale factor.
 *
 * This function must be called during object rendering.
 * gResetScale() is called, then width & height scale are
 * multiplied by these values.
 * Note: negative values can be passed to invert the image.
 */

void gSetScale(float w, float h);

/**
 * \brief Sets the new scale, in pixels.
 * @param w New width, in pixels.
 * @param h New height, in pixels.
 *
 * This function must be called during object rendering.
 * Note: negative values can be passed to invert the image.
 */

void gSetScaleWH(int w, int h);

/**
 * \brief Sets the new scale, relative to the current.
 * @param w Width scale factor.
 * @param h Height scale factor.
 *
 * This function must be called during object rendering.
 * Current width & height scale are multiplied by these values.
 * Note: negative values can be passed to invert the image.
 */

void gSetScaleRelative(float w, float h);

/**
 * \brief Sets the new scale, in pixels, relative to the current.
 * @param w New width to increment, in pixels.
 * @param h New height to increment, in pixels.
 *
 * This function must be called during object rendering.
 * Note: negative values can be passed to invert the image.
 */

void gSetScaleWHRelative(int w, int h);

/**
 * \brief Resets the current color.
 *
 * This function must be called during object rendering.
 * Sets gSetColor() to WHITE.
 */

void gResetColor();

/**
 * \brief Resets the current alpha.
 *
 * This function must be called during object rendering.
 * Sets gSetAlpha() to 255.
 */

void gResetAlpha();

/**
 * \brief Gets the current alpha.
 * @param alpha Pointer to save the current alpha (0-255).
 *
 * This function must be called during object rendering.
 * Pass NULL if not needed.
 */

void gGetAlpha(gAlpha* alpha);

/**
 * \brief Sets the new color.
 * @param color The new color.
 *
 * This function must be called during object rendering.
 * Can be used to colorize a non-textured objet.
 * Can also be used as a color mask for a texture.
 */

void gSetColor(gColor color);

/**
 * \brief Sets the new alpha.
 * @param alpha The new alpha (0-255).
 *
 * This function must be called during object rendering.
 * Can be used for both textured and non-textured objects.
 */

void gSetAlpha(gAlpha alpha);

/**
 * \brief Sets the new alpha, relative to the current alpha.
 * @param alpha The new alpha increment.
 *
 * This function must be called during object rendering.
 * Can be used for both textured and non-textured objects.
 */

void gSetAlphaRelative(int alpha);

/**
 * \brief Resets the current rotation.
 *
 * This function must be called during object rendering.
 * Sets gSetRotation() to 0°.
 */

void gResetRotation();

/**
 * \brief Gets the current rotation, in radians.
 * @param radians Pointer to save the current rotation.
 *
 * This function must be called during object rendering.
 * Pass NULL if not needed.
 */

void gGetRotationRad(float* radians);

/**
 * \brief Gets the current rotation, in degrees.
 * @param degrees Pointer to save the current rotation.
 *
 * This function must be called during object rendering.
 * Pass NULL if not needed.
 */

void gGetRotation(float* degrees);

/**
 * \brief Sets the new rotation, in radians.
 * @param radians The new angle.
 *
 * This function must be called during object rendering.
 * The rotation center is the actual coordinates.
 */

void gSetRotationRad(float radians);

/**
 * \brief Sets the new rotation, in degrees.
 * @param degrees The new angle.
 *
 * This function must be called during object rendering.
 * The rotation center is the actual coordinates.
 */

void gSetRotation(float degrees);

/**
 * \brief Sets the new rotation, relative to the current, in radians.
 * @param radians The new angle increment.
 *
 * This function must be called during object rendering.
 * The rotation center is the actual coordinates.
 */

void gSetRotationRadRelative(float radians);

/**
 * \brief Sets the new rotation, relative to the current, in degrees.
 * @param degrees The new angle increment.
 *
 * This function must be called during object rendering.
 * The rotation center is the actual coordinates.
 */

void gSetRotationRelative(float degrees);

/**
 * \brief Resets the current crop.
 *
 * This function must be called during object rendering.
 * Sets gSetCropXY() to (0;0) and gSetCropWH() to (tex->w,tex->h) or (10,10).
 */

void gResetCrop();

/**
 * \brief Gets the current crop position.
 * @param x Pointer to save the current crop x.
 * @param y Pointer to save the current crop y.
 *
 * This function must be called during object rendering.
 * Pass NULL if not needed.
 */

void gGetCropXY(int* x, int* y);

/**
 * \brief Gets the current crop scale.
 * @param w Pointer to save the current crop width.
 * @param h Pointer to save the current crop height.
 *
 * This function must be called during object rendering.
 * Pass NULL if not needed.
 */

void gGetCropWH(int* w, int* h);

/**
 * \brief Sets the new crop position.
 * @param x New x, in pixels.
 * @param y New y, in pixels.
 *
 * This function must be called during object rendering.
 * Defines the rectangle position of the crop.
 * If the rectangle is larger or next to the image, texture will be repeated.
 * Useful for a tileset.
 */

void gSetCropXY(int x, int y);

/**
 * \brief Sets the new crop size.
 * @param w New width, in pixels.
 * @param h New height, in pixels.
 *
 * This function must be called during object rendering.
 * Defines the rectangle size of the crop.
 * If the rectangle is larger or next to the image, texture will be repeated.
 * Useful for a tileset.
 */

void gSetCropWH(int w, int h);

/**
 * \brief Sets the new crop position, relative to the current.
 * @param x New x increment, in pixels.
 * @param y New y increment, in pixels.
 *
 * This function must be called during object rendering.
 * Defines the rectangle position of the crop.
 * If the rectangle is larger or next to the image, texture will be repeated.
 * Useful for a tileset.
 */

void gSetCropXYRelative(int x, int y);

/**
 * \brief Sets the new crop size, relative to the current.
 * @param w New width increment, in pixels.
 * @param h New height increment, in pixels.
 *
 * This function must be called during object rendering.
 * Defines the rectangle size of the crop.
 * If the rectangle is larger or next to the image, texture will be repeated.
 * Useful for a tileset.
 */

void gSetCropWHRelative(int w, int h);

/**
 * \brief Resets texture properties.
 *
 * This function can be called everywhere in the loop.
 */

void gResetTex();

/**
 * \brief Set texture wrap.
 * @param use G_TRUE to repeat.
              G_FALSE to clamp (by default).
 *
 * This function must be called during object rendering.
 */

void gSetTexRepeat(bool use);

/**
 * \brief Use the alpha blending with the texture.
 * @param use G_TRUE to activate (better look, by default).
              G_FALSE to desactivate (better performance).
 *
 * This function must be called during object rendering.
 * Automaticaly disabled when gImage::can_blend is set to G_FALSE.
 */

void gSetTexBlend(bool use);

/**
 * \brief Use the bilinear filter with the texture.
 * @param use G_TRUE to activate (better look, by default).
              G_FALSE to desactivate (better performance).
 *
 * This function must be called during object rendering.
 * Only useful when scaling.
 */

void gSetTexLinear(bool use);

/**
 * \brief Resets the draw zone to the entire screen.
 *
 * This function can be called everywhere in the loop.
 */

void gResetScissor();

/**
 * \brief Sets the draw zone.
 * @param x New x position.
 * @param y New y position.
 * @param w New width.
 * @param h New height.
 *
 * This function can be called everywhere *in* the loop.
 * Pixel draw will be skipped outside this rectangle.
 */

void gSetScissor(int x, int y, int w, int h);

#endif
