// gLib2D by Geecko - A simple, fast, light-weight 2D graphics library.
//
// This work is licensed under the Creative Commons BY-SA 3.0 Unported License.
// See LICENSE for more details.
//
// Please report bugs at : geecko.dev@free.fr

#include "glib2d.h"

#include <pspkernel.h>
#include <pspdisplay.h>
#include <pspgu.h>
#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <math.h>

#ifdef USE_PNG
#include <png.h>
#endif

#ifdef USE_JPEG
#include <jpeglib.h>
#include <jerror.h>
#endif

#define PSP_LINE_SIZE       (512)
#define PIXEL_SIZE          (4)
#define FRAMEBUFFER_SIZE    (PSP_LINE_SIZE*G_SCR_H*PIXEL_SIZE)
#define MALLOC_STEP         (128)
#define TRANSFORM_STACK_MAX (64)
#define SLICE_WIDTH         (64)

#define DEFAULT_SIDE       (10)
#define DEFAULT_COORD_MODE (G_UP_LEFT)
#define DEFAULT_X          (0.f)
#define DEFAULT_Y          (0.f)
#define DEFAULT_Z          (0.f)
#define DEFAULT_COLOR      (WHITE)
#define DEFAULT_ALPHA      (0xFF)

#define CURRENT_OBJ obj_list[obj_list_size-1]
#define CURRENT_TRANSFORM transform_stack[transform_stack_size-1]
#define I_OBJ obj_list[i]

enum Obj_Types { RECTS, LINES, QUADS, POINTS };

typedef struct
{
  float x, y, z;
  float rot, rot_sin, rot_cos;
  float scale_w, scale_h;
} Transform;

typedef struct
{
  float x, y, z;
  float rot_x, rot_y; // Rotation center
  float rot_sin, rot_cos;
  int crop_x, crop_y;
  int crop_w, crop_h;
  int scale_w, scale_h;
  gColor color;
} Obj_Properties;


// * Main vars *
static unsigned int __attribute__((aligned(16))) list[262144];
static bool init = G_FALSE, start = G_FALSE, zclear = G_TRUE, scissor = G_FALSE;
static Transform transform_stack[TRANSFORM_STACK_MAX];
static int transform_stack_size;
// * Object vars *
static Obj_Properties* obj_list = NULL;
static gEnum obj_type;
static int obj_list_size, obj_list_size_malloc; // Real & malloc'ed size
static bool obj_begin = G_FALSE, obj_line_strip;
static bool obj_use_z, obj_use_vert_color, obj_use_blend, obj_use_rot,
            obj_use_tex, obj_use_tex_linear, obj_use_tex_repeat;
// * Coord vars *
static gEnum obj_coord_mode;
static float obj_x, obj_y, obj_z;
// * Crop vars *
static int obj_crop_x, obj_crop_y;
static int obj_crop_w, obj_crop_h;
// * Scale vars *
static float obj_scale_w, obj_scale_h;
// * Color & alpha vars *
static gColor obj_color;
static gAlpha obj_alpha;
static bool obj_colors_count;
// * Rotation vars *
static float obj_rot, obj_rot_sin, obj_rot_cos;
// * Texture vars *
static gImage* obj_tex;

// * Internal functions *

void _gInit()
{
  // Init & setup GU
  sceGuInit();
  sceGuStart(GU_DIRECT,list);

  sceGuDrawBuffer(GU_PSM_8888,(void*)FRAMEBUFFER_SIZE,PSP_LINE_SIZE);
  sceGuDispBuffer(G_SCR_W,G_SCR_H,(void*)0,PSP_LINE_SIZE);
  sceGuDepthBuffer((void*)(FRAMEBUFFER_SIZE*2),PSP_LINE_SIZE);
  sceGuOffset(2048-(G_SCR_W/2),2048-(G_SCR_H/2));
  sceGuViewport(2048,2048,G_SCR_W,G_SCR_H);

  gResetScissor();
  sceGuDepthRange(65535,0);
  sceGuClearDepth(65535);
  sceGuAlphaFunc(GU_GREATER,0,0xff);
  sceGuDepthFunc(GU_LEQUAL);
  sceGuBlendFunc(GU_ADD,GU_SRC_ALPHA,GU_ONE_MINUS_SRC_ALPHA,0,0);
  sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGBA);
  sceGuTexFilter(GU_LINEAR,GU_LINEAR);
  sceGuShadeModel(GU_SMOOTH);

  sceGuDisable(GU_CULL_FACE);
  sceGuDisable(GU_CLIP_PLANES);
  sceGuDisable(GU_DITHER);
  sceGuEnable(GU_ALPHA_TEST);
  sceGuEnable(GU_SCISSOR_TEST);

  sceGuFinish();
  sceGuSync(0,0);
  sceDisplayWaitVblankStart();
  sceGuDisplay(GU_TRUE);

  init = G_TRUE;
}


void _gStart()
{
  if (!init) _gInit();

  sceKernelDcacheWritebackAll();
  sceGuStart(GU_DIRECT,list);
  start = G_TRUE;
}


void _gCoordInit()
{
  gResetCoord();
  obj_use_z = G_FALSE;
}


void _gScaleInit()
{
  gResetScale();
}


void _gColorInit()
{
  gResetColor();
  obj_colors_count = 0;
  obj_use_vert_color = G_FALSE;
}


void _gAlphaInit()
{
  gResetAlpha();
  obj_use_blend = G_FALSE;
}


void _gRotationInit()
{
  gResetRotation();
  obj_use_rot = G_FALSE;
}


void _gCropInit()
{
  if (!obj_use_tex) return;
  gResetCrop();
}


void _gTexInit()
{
  if (obj_tex == NULL)
  {
    obj_use_tex = G_FALSE;
    return;
  }
  obj_use_tex = G_TRUE;
  gResetTex();
}


// Vertex order: [texture uv] [color] [vertex]
void* _gSetVertex(void* vp, int i, float vx, float vy)
{
  short* v_p_short = vp;
  gColor* v_p_color;
  float* v_p_float;

  // Texture
  if (obj_use_tex)
  {
    *(v_p_short++) = I_OBJ.crop_x + vx * I_OBJ.crop_w;
    *(v_p_short++) = I_OBJ.crop_y + vy * I_OBJ.crop_h;
  }

  // Color
  v_p_color = (gColor*)v_p_short;

  if (obj_use_vert_color)
  {
    *(v_p_color++) = I_OBJ.color;
  }

  // Coord
  v_p_float = (float*)v_p_color;

  v_p_float[0] = I_OBJ.x + (obj_type == RECTS ? vx * I_OBJ.scale_w : 0.f);
  v_p_float[1] = I_OBJ.y + (obj_type == RECTS ? vy * I_OBJ.scale_h : 0.f);

  // Then apply the rotation
  if (obj_use_rot && !(I_OBJ.rot_x == I_OBJ.x && I_OBJ.rot_y == I_OBJ.y))
  {
    float tx = v_p_float[0]-I_OBJ.rot_x, ty = v_p_float[1]-I_OBJ.rot_y;
    v_p_float[0] = I_OBJ.rot_x - I_OBJ.rot_sin*ty + I_OBJ.rot_cos*tx,
    v_p_float[1] = I_OBJ.rot_y + I_OBJ.rot_cos*ty + I_OBJ.rot_sin*tx;
  }
  v_p_float[2] = I_OBJ.z;

  v_p_float += 3;

  return (void*)v_p_float;
}


// Insertion sort, because it is a fast and _stable_ sort.
void _gVertexSort()
{
  int i, j;
  Obj_Properties obj_tmp;
  for (i=1; i<obj_list_size; i++)
  {
    j = i;
    memcpy(&obj_tmp,obj_list+j,sizeof(Obj_Properties));
    while (j>0 && obj_list[j-1].z < obj_tmp.z)
    {
      memcpy(obj_list+j,obj_list+j-1,sizeof(Obj_Properties));
      j--;
    }
    memcpy(obj_list+j,&obj_tmp,sizeof(Obj_Properties));
  }
}

// * Main functions *

void gClear(gColor color)
{
  if (!start) _gStart();

  sceGuClearColor(color);
  sceGuClear(GU_COLOR_BUFFER_BIT | GU_FAST_CLEAR_BIT
             | (zclear ? GU_DEPTH_BUFFER_BIT : 0));
  zclear = G_FALSE;
}


void gClearZ()
{
  sceGuClear(GU_DEPTH_BUFFER_BIT | GU_FAST_CLEAR_BIT);
  zclear = G_TRUE;
}


void _gBeginCommon()
{
  if (!start) _gStart();

  obj_list_size = 0;
  obj_list_size_malloc = MALLOC_STEP;
  obj_list = realloc(obj_list,obj_list_size_malloc * sizeof(Obj_Properties));

  _gCoordInit();
  _gColorInit();
  _gAlphaInit();
  _gRotationInit();
  _gTexInit();
  _gCropInit();
  _gScaleInit();

  obj_begin = G_TRUE;
}


void gBeginRects(gImage* tex)
{
  if (obj_begin) return;

  obj_type = RECTS;
  obj_tex = tex;
  _gBeginCommon();
}


void gBeginLines(bool use_line_strip)
{
  if (obj_begin) return;

  obj_type = LINES;
  obj_tex = NULL;
  obj_line_strip = use_line_strip;
  _gBeginCommon();
}


void gBeginQuads(gImage* tex)
{
  if (obj_begin) return;

  obj_type = QUADS;
  obj_tex = tex;
  _gBeginCommon();
}


void gBeginPoints()
{
  if (obj_begin) return;

  obj_type = POINTS;
  obj_tex = NULL;
  _gBeginCommon();
}


void _gEndRects()
{
  // Horror : we need to sort the vertices.
  if (obj_use_z && obj_use_blend) _gVertexSort();

  // Define vertices properties
  int prim = (obj_use_rot ? GU_TRIANGLES : GU_SPRITES),
      v_obj_nbr = (obj_use_rot ? 6 : 2),
      v_nbr,
      v_coord_size = 3,
      v_tex_size = (obj_use_tex ? 2 : 0),
      v_color_size = (obj_use_vert_color ? 1 : 0),
      v_size = v_tex_size * sizeof(short) +
               v_color_size * sizeof(gColor) +
               v_coord_size * sizeof(float),
      v_type = GU_VERTEX_32BITF | GU_TRANSFORM_2D,
      n_slices = -1, i;

  if (obj_use_tex)        v_type |= GU_TEXTURE_16BIT;
  if (obj_use_vert_color) v_type |= GU_COLOR_8888;

  // Count how many vertices to allocate.
  if (!obj_use_tex || obj_use_rot) // No slicing
  {
    v_nbr = v_obj_nbr * obj_list_size;
  }
  else // Can use texture slicing for tremendous performance :)
  {
    for (n_slices=0, i=0; i!=obj_list_size; i++)
    {
      n_slices += (int)(I_OBJ.crop_w/SLICE_WIDTH)+1;
    }
    v_nbr = v_obj_nbr * n_slices;
  }

  // Allocate vertex list memory
  void *v = sceGuGetMemory(v_nbr * v_size), *vi = v;

  // Build the vertex list
  for (i=0; i<obj_list_size; i+=1)
  {
    if (!obj_use_rot)
    {
      if (!obj_use_tex)
      {
        vi = _gSetVertex(vi,i,0.f,0.f);
        vi = _gSetVertex(vi,i,1.f,1.f);
      }
      else // Use texture slicing
      {
        float u, step = (float)SLICE_WIDTH/I_OBJ.crop_w;
        for (u=0.f; u<1.f; u+=step)
        {
          vi = _gSetVertex(vi,i,u,0.f);
          vi = _gSetVertex(vi,i,((u+step) > 1.f ? 1.f : u+step),1.f);
        }
      }
    }
    else // Rotation : draw 2 triangles per obj
    {
      vi = _gSetVertex(vi,i,0.f,0.f);
      vi = _gSetVertex(vi,i,1.f,0.f);
      vi = _gSetVertex(vi,i,0.f,1.f);
      vi = _gSetVertex(vi,i,0.f,1.f);
      vi = _gSetVertex(vi,i,1.f,0.f);
      vi = _gSetVertex(vi,i,1.f,1.f);
    }
  }

  // Then put it in the display list.
  sceGuDrawArray(prim,v_type,v_nbr,NULL,v);
}


void _gEndLines()
{
  // Define vertices properties
  int prim = (obj_line_strip ? GU_LINE_STRIP : GU_LINES),
      v_obj_nbr = (obj_line_strip ? 1 : 2),
      v_nbr = v_obj_nbr * (obj_line_strip ? obj_list_size
                                          : obj_list_size / 2),
      v_coord_size = 3,
      v_color_size = (obj_use_vert_color ? 1 : 0),
      v_size = v_color_size * sizeof(gColor) +
               v_coord_size * sizeof(float),
      v_type = GU_VERTEX_32BITF | GU_TRANSFORM_2D,
      i;

  if (obj_use_vert_color) v_type |= GU_COLOR_8888;

  // Allocate vertex list memory
  void *v = sceGuGetMemory(v_nbr * v_size), *vi = v;

  // Build the vertex list
  if (obj_line_strip)
  {
    vi = _gSetVertex(vi,0,0.f,0.f); 
    for (i=1; i<obj_list_size; i+=1)
    {
      vi = _gSetVertex(vi,i,0.f,0.f);
    }
  }
  else
  {
    for (i=0; i+1<obj_list_size; i+=2)
    {
      vi = _gSetVertex(vi,i  ,0.f,0.f);
      vi = _gSetVertex(vi,i+1,0.f,0.f);
    }
  }

  // Then put it in the display list.
  sceGuDrawArray(prim,v_type,v_nbr,NULL,v);
}


void _gEndQuads()
{
  // Define vertices properties
  int prim = GU_TRIANGLES,
      v_obj_nbr = 6,
      v_nbr = v_obj_nbr * (obj_list_size / 4),
      v_coord_size = 3,
      v_tex_size = (obj_use_tex ? 2 : 0),
      v_color_size = (obj_use_vert_color ? 1 : 0),
      v_size = v_tex_size * sizeof(short) +
               v_color_size * sizeof(gColor) +
               v_coord_size * sizeof(float),
      v_type = GU_VERTEX_32BITF | GU_TRANSFORM_2D,
      i;

  if (obj_use_tex)        v_type |= GU_TEXTURE_16BIT;
  if (obj_use_vert_color) v_type |= GU_COLOR_8888;

  // Allocate vertex list memory
  void *v = sceGuGetMemory(v_nbr * v_size), *vi = v;

  // Build the vertex list
  for (i=0; i+3<obj_list_size; i+=4)
  {
    vi = _gSetVertex(vi,i  ,0.f,0.f);
    vi = _gSetVertex(vi,i+1,1.f,0.f);
    vi = _gSetVertex(vi,i+3,0.f,1.f);
    vi = _gSetVertex(vi,i+3,0.f,1.f);
    vi = _gSetVertex(vi,i+1,1.f,0.f);
    vi = _gSetVertex(vi,i+2,1.f,1.f);
  }

  // Then put it in the display list.
  sceGuDrawArray(prim,v_type,v_nbr,NULL,v);
}


void _gEndPoints()
{
  // Define vertices properties
  int prim = GU_POINTS,
      v_obj_nbr = 1,
      v_nbr = v_obj_nbr * obj_list_size,
      v_coord_size = 3,
      v_color_size = (obj_use_vert_color ? 1 : 0),
      v_size = v_color_size * sizeof(gColor) +
               v_coord_size * sizeof(float),
      v_type = GU_VERTEX_32BITF | GU_TRANSFORM_2D,
      i;

  if (obj_use_vert_color) v_type |= GU_COLOR_8888;

  // Allocate vertex list memory
  void *v = sceGuGetMemory(v_nbr * v_size), *vi = v;

  // Build the vertex list
  for (i=0; i<obj_list_size; i+=1)
  {
    vi = _gSetVertex(vi,i,0.f,0.f);
  }

  // Then put it in the display list.
  sceGuDrawArray(prim,v_type,v_nbr,NULL,v);
}


void gEnd()
{
  if (!obj_begin || obj_list_size <= 0)
  {
    obj_begin = G_FALSE;
    return;
  }

  // Manage pspgu extensions
  if (obj_use_z)          sceGuEnable(GU_DEPTH_TEST);
  else                    sceGuDisable(GU_DEPTH_TEST);
  if (obj_use_blend)      sceGuEnable(GU_BLEND);
  else                    sceGuDisable(GU_BLEND);
  if (obj_use_vert_color) sceGuColor(WHITE);
  else                    sceGuColor(obj_list[0].color);
  if (!obj_use_tex)       sceGuDisable(GU_TEXTURE_2D);
  else
  {
    sceGuEnable(GU_TEXTURE_2D);
    if (obj_use_tex_linear) sceGuTexFilter(GU_LINEAR,GU_LINEAR);
    else                    sceGuTexFilter(GU_NEAREST,GU_NEAREST);
    if (obj_use_tex_repeat) sceGuTexWrap(GU_REPEAT,GU_REPEAT);
    else                    sceGuTexWrap(GU_CLAMP,GU_CLAMP);
    // Load texture
    sceGuTexMode(GU_PSM_8888,0,0,obj_tex->swizzled);
    sceGuTexImage(0,obj_tex->tw,obj_tex->th,obj_tex->tw,obj_tex->data);
  }

  switch (obj_type)
  {
    case RECTS:  _gEndRects();  break;
    case LINES:  _gEndLines();  break;
    case QUADS:  _gEndQuads();  break;
    case POINTS: _gEndPoints(); break;
  }

  sceGuColor(WHITE);
  sceGuEnable(GU_BLEND);

  obj_begin = G_FALSE;
  if (obj_use_z) zclear = G_TRUE;
}


void gReset()
{
  gResetCoord();
  gResetScale();
  gResetColor();
  gResetAlpha();
  gResetRotation();
  gResetCrop();
  gResetScissor();
}


void gFlip(bool use_vsync)
{
  if (scissor) gResetScissor();

  sceGuFinish();
  sceGuSync(0,0);
  if (use_vsync) sceDisplayWaitVblankStart();
  sceGuSwapBuffers();

  start = G_FALSE;
}


void gAdd()
{
  if (!obj_begin) return;
  if (obj_scale_w == 0 || obj_scale_h == 0) return;

  if (obj_list_size >= obj_list_size_malloc)
  {
    obj_list_size_malloc += MALLOC_STEP;
    obj_list = realloc(obj_list,obj_list_size_malloc * sizeof(Obj_Properties));
  }

  obj_list_size++;

  CURRENT_OBJ.x = obj_x;
  CURRENT_OBJ.y = obj_y;
  CURRENT_OBJ.z = obj_z;
  CURRENT_OBJ.crop_x = obj_crop_x;
  CURRENT_OBJ.crop_y = obj_crop_y;
  CURRENT_OBJ.crop_w = obj_crop_w;
  CURRENT_OBJ.crop_h = obj_crop_h;
  CURRENT_OBJ.scale_w = obj_scale_w;
  CURRENT_OBJ.scale_h = obj_scale_h;
  CURRENT_OBJ.color = obj_color;
  CURRENT_OBJ.rot_x = obj_x;
  CURRENT_OBJ.rot_y = obj_y;
  CURRENT_OBJ.rot_sin = obj_rot_sin;
  CURRENT_OBJ.rot_cos = obj_rot_cos;

  // Coord mode stuff
  CURRENT_OBJ.x -= (obj_coord_mode == G_UP_RIGHT ||
                    obj_coord_mode == G_DOWN_RIGHT ?
                    CURRENT_OBJ.scale_w :
                   (obj_coord_mode == G_CENTER ?
                    CURRENT_OBJ.scale_w/2 : 0));
  CURRENT_OBJ.y -= (obj_coord_mode == G_DOWN_LEFT ||
                    obj_coord_mode == G_DOWN_RIGHT ?
                    CURRENT_OBJ.scale_h :
                   (obj_coord_mode == G_CENTER ?
                    CURRENT_OBJ.scale_h/2 : 0));
  // Image inverted
  if (CURRENT_OBJ.scale_w < 0)
  {
    CURRENT_OBJ.x += (obj_coord_mode == G_UP_LEFT ||
                      obj_coord_mode == G_DOWN_LEFT ?
                      -CURRENT_OBJ.scale_w :
                     (obj_coord_mode == G_UP_RIGHT ||
                      obj_coord_mode == G_DOWN_RIGHT ?
                       CURRENT_OBJ.scale_w : 0));
  }
  if (CURRENT_OBJ.scale_h < 0)
  {
    CURRENT_OBJ.y += (obj_coord_mode == G_UP_LEFT ||
                      obj_coord_mode == G_UP_RIGHT ?
                      -CURRENT_OBJ.scale_h :
                     (obj_coord_mode == G_DOWN_LEFT ||
                      obj_coord_mode == G_DOWN_RIGHT ?
                       CURRENT_OBJ.scale_h : 0));
  }
  // Alpha stuff
  CURRENT_OBJ.color = G_MODULATE(CURRENT_OBJ.color,255,obj_alpha);
}


void gPush()
{
  if (transform_stack_size >= TRANSFORM_STACK_MAX) return;
  transform_stack_size++;
  CURRENT_TRANSFORM.x = obj_x;
  CURRENT_TRANSFORM.y = obj_y;
  CURRENT_TRANSFORM.z = obj_z;
  CURRENT_TRANSFORM.rot = obj_rot;
  CURRENT_TRANSFORM.rot_sin = obj_rot_sin;
  CURRENT_TRANSFORM.rot_cos = obj_rot_cos;
  CURRENT_TRANSFORM.scale_w = obj_scale_w;
  CURRENT_TRANSFORM.scale_h = obj_scale_h;
}


void gPop()
{
  if (transform_stack_size <= 0) return;
  obj_x = CURRENT_TRANSFORM.x;
  obj_y = CURRENT_TRANSFORM.y;
  obj_z = CURRENT_TRANSFORM.z;
  obj_rot = CURRENT_TRANSFORM.rot;
  obj_rot_sin = CURRENT_TRANSFORM.rot_sin;
  obj_rot_cos = CURRENT_TRANSFORM.rot_cos;
  obj_scale_w = CURRENT_TRANSFORM.scale_w;
  obj_scale_h = CURRENT_TRANSFORM.scale_h;
  if (obj_rot != 0.f) obj_use_rot = G_TRUE;
  if (obj_z != 0.f) obj_use_z = G_TRUE;
  transform_stack_size--;
}

// * Coord functions *

void gResetCoord()
{
  obj_coord_mode = DEFAULT_COORD_MODE;
  obj_x = DEFAULT_X;
  obj_y = DEFAULT_Y;
  obj_z = DEFAULT_Z;
}


void gSetCoordMode(gEnum mode)
{
  if (mode < G_UP_LEFT || mode > G_CENTER) return;
  obj_coord_mode = mode;
}


void gGetCoordXYZ(float* x, float* y, float* z)
{
  if (x != NULL) *x = obj_x;
  if (y != NULL) *y = obj_y;
  if (z != NULL) *z = obj_z;
}


void gSetCoordXY(float x, float y)
{
  obj_x = x;
  obj_y = y;
  obj_z = 0.f;
}


void gSetCoordXYZ(float x, float y, float z)
{
  obj_x = x;
  obj_y = y;
  obj_z = z;
  if (z != 0.f) obj_use_z = G_TRUE;
}


void gSetCoordXYRelative(float x, float y, bool use_rot)
{
  if (!use_rot) gSetCoordXY(obj_x + x, obj_y + y);
  else          gSetCoordXY(obj_x - obj_rot_sin*y + obj_rot_cos*x,
                            obj_y + obj_rot_cos*y + obj_rot_sin*x);
}


void gSetCoordXYZRelative(float x, float y, float z, bool use_rot)
{
  if (!use_rot) gSetCoordXYZ(obj_x + x, obj_y + y,obj_z + z);
  else          gSetCoordXYZ(obj_x - obj_rot_sin*y + obj_rot_cos*x,
                             obj_y + obj_rot_cos*y + obj_rot_sin*x,
                             obj_z + z);
}

// * Scale functions *

void gResetScale()
{
  if (!obj_use_tex)
  {
    obj_scale_w = DEFAULT_SIDE;
    obj_scale_h = DEFAULT_SIDE;
  }
  else
  {
    obj_scale_w = obj_tex->w;
    obj_scale_h = obj_tex->h;
  }
}


void gGetScaleWH(float* w, float* h)
{
  if (w != NULL) *w = obj_scale_w;
  if (h != NULL) *h = obj_scale_h;
}


void gSetScale(float w, float h)
{
  gResetScale();
  gSetScaleRelative(w,h);
}


void gSetScaleWH(int w, int h)
{
  obj_scale_w = w;
  obj_scale_h = h;
  // A trick to prevent an unexpected behavior when mirroring with GU_SPRITES.
  if (obj_scale_w < 0 || obj_scale_h < 0) obj_use_rot = G_TRUE;  
}


void gSetScaleRelative(float w, float h)
{
  gSetScaleWH(obj_scale_w * w, obj_scale_h * h);
}


void gSetScaleWHRelative(int w, int h)
{
  gSetScaleWH(obj_scale_w + w, obj_scale_h + h);
}

// * Color functions *

void gResetColor()
{
  obj_color = DEFAULT_COLOR;
}


void gResetAlpha()
{
  obj_alpha = DEFAULT_ALPHA;
}


void gGetAlpha(gAlpha* alpha)
{
  if (alpha != NULL) *alpha = obj_alpha;
}


void gSetColor(gColor color)
{
  obj_color = color;
  if (++obj_colors_count > 1) obj_use_vert_color = G_TRUE;
  if (G_GET_A(obj_color) < 255) obj_use_blend = G_TRUE;
}


void gSetAlpha(gAlpha alpha)
{
  if (alpha < 0) alpha = 0;
  if (alpha > 255) alpha = 255;
  obj_alpha = alpha;
  if (++obj_colors_count > 1) obj_use_vert_color = G_TRUE;
  if (obj_alpha < 255) obj_use_blend = G_TRUE;
}


void gSetAlphaRelative(int alpha)
{
  gSetAlpha(obj_alpha + alpha);
}

// * Rotations functions *

void gResetRotation()
{
  obj_rot = 0.f;
  obj_rot_sin = 0.f;
  obj_rot_cos = 1.f;
}


void gGetRotationRad(float* radians)
{
  if (radians != NULL) *radians = obj_rot;
}


void gGetRotation(float* degrees)
{
  if (degrees != NULL) *degrees = obj_rot * 180.f / GU_PI;
}


void gSetRotationRad(float radians)
{
  if (radians == obj_rot) return;
  obj_rot = radians;
  obj_rot_sin = sinf(radians);
  obj_rot_cos = cosf(radians);
  if (radians != 0.f) obj_use_rot = G_TRUE;
}


void gSetRotation(float degrees)
{
  gSetRotationRad(degrees * GU_PI / 180.f);
}


void gSetRotationRadRelative(float radians)
{
  gSetRotationRad(obj_rot + radians);
}


void gSetRotationRelative(float degrees)
{
  gSetRotationRadRelative(degrees * GU_PI / 180.f);
}

// * Crop functions *

void gResetCrop()
{
  if (!obj_use_tex) return;
  obj_crop_x = 0;
  obj_crop_y = 0;
  obj_crop_w = obj_tex->w;
  obj_crop_h = obj_tex->h;
}


void gGetCropXY(int* x, int* y)
{
  if (!obj_use_tex) return;
  if (x != NULL) *x = obj_crop_x;
  if (y != NULL) *y = obj_crop_y;
}


void gGetCropWH(int* w, int* h)
{
  if (!obj_use_tex) return;
  if (w != NULL) *w = obj_crop_w;
  if (h != NULL) *h = obj_crop_h;
}


void gSetCropXY(int x, int y)
{
  if (!obj_use_tex) return;
  obj_crop_x = x;
  obj_crop_y = y;
}


void gSetCropWH(int w, int h)
{
  if (!obj_use_tex) return;
  obj_crop_w = w;
  obj_crop_h = h;
}


void gSetCropXYRelative(int x, int y)
{
  if (!obj_use_tex) return;
  gSetCropXY(obj_crop_x + x, obj_crop_y + y);
}


void gSetCropWHRelative(int w, int h)
{
  if (!obj_use_tex) return;
  gSetCropWH(obj_crop_w + w, obj_crop_h + h);
}

// * Texture functions *

void gResetTex()
{
  if (!obj_use_tex) return;
  obj_use_tex_repeat = G_FALSE;
  obj_use_tex_linear = G_TRUE;
  if (obj_tex->can_blend) obj_use_blend = G_TRUE;
}


void gSetTexRepeat(bool use)
{
  if (!obj_use_tex) return;
  obj_use_tex_repeat = use;
}


void gSetTexBlend(bool use)
{
  if (!obj_use_tex) return;
  if (!obj_tex->can_blend) return;
  obj_use_blend = use;
}


void gSetTexLinear(bool use)
{
  if (!obj_use_tex) return;
  obj_use_tex_linear = use;
}

// * Texture management *

int _getNextPower2(int n)
{
  int p=1;
  while ((p <<= 1) < n);
  return p;
}


void _swizzle(unsigned char *dest, unsigned char *source, int width, int height)
{
  int i,j;
  int rowblocks = (width / 16);
  int rowblocks_add = (rowblocks-1)*128;
  unsigned int block_address = 0;
  unsigned int *img = (unsigned int*)source;
  for (j = 0; j < height; j++)
  {
    unsigned int *block = (unsigned int*)((unsigned int)&dest[block_address] | 0x40000000);
    for (i = 0; i < rowblocks; i++)
    {
      *block++ = *img++;
      *block++ = *img++;
      *block++ = *img++;
      *block++ = *img++;
      block += 28;
    }
    if ((j&0x7)==0x7)
    {
      block_address += rowblocks_add;
    }
    block_address+=16;
  }
}


#ifdef USE_PNG
void _gTexLoadPNG(FILE* fp, gImage* tex)
{
  png_structp png_ptr;
  png_infop info_ptr;
  unsigned int sig_read = 0;
  png_uint_32 width, height;
  int bit_depth, color_type, interlace_type, x, y;
  u32* line;
  png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING,NULL,NULL,NULL);
  png_set_error_fn(png_ptr,NULL,NULL,NULL);
  info_ptr = png_create_info_struct(png_ptr);
  png_init_io(png_ptr,fp);
  png_set_sig_bytes(png_ptr,sig_read);
  png_read_info(png_ptr,info_ptr);
  png_get_IHDR(png_ptr,info_ptr,&width,&height,&bit_depth,&color_type,
                                &interlace_type,NULL,NULL);
  png_set_strip_16(png_ptr);
  png_set_packing(png_ptr);
  if (color_type == PNG_COLOR_TYPE_PALETTE)
    png_set_palette_to_rgb(png_ptr);
  if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
    png_set_gray_1_2_4_to_8(png_ptr);
  if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
    png_set_tRNS_to_alpha(png_ptr);
  png_set_filler(png_ptr, 0xff, PNG_FILLER_AFTER);
  tex->w = width;
  tex->h = height;
  tex->tw = _getNextPower2(width);
  tex->th = _getNextPower2(height);
  tex->ratio = (float)width / height;
  tex->data = memalign(16, tex->tw * tex->th * sizeof(gColor));
  line = malloc(width * 4);
  for (y = 0; y < height; y++) {
    png_read_row(png_ptr, (u8*) line, png_bytep_NULL);
    for (x = 0; x < width; x++) {
      u32 color = line[x];
      tex->data[x + y * tex->tw] =  color;
    }
  }
  free(line);
  png_read_end(png_ptr, info_ptr);
  png_destroy_read_struct(&png_ptr, &info_ptr, png_infopp_NULL);
}
#endif


#ifdef USE_JPEG
void _gTexLoadJPEG(FILE* fp, gImage* tex)
{
  struct jpeg_decompress_struct dinfo;
  struct jpeg_error_mgr jerr;
  dinfo.err = jpeg_std_error(&jerr);
  jpeg_create_decompress(&dinfo);
  jpeg_stdio_src(&dinfo, fp);
  jpeg_read_header(&dinfo, TRUE);
  int width = dinfo.image_width;
  int height = dinfo.image_height;
  jpeg_start_decompress(&dinfo);
  tex->w = width;
  tex->h = height;
  tex->tw = _getNextPower2(width);
  tex->th = _getNextPower2(height);
  tex->ratio = (float)width / height;
  tex->data = (gColor*) memalign(16, tex->tw * tex->th * sizeof(gColor));
  u8* line = (u8*) malloc(width * 3);
  if (dinfo.jpeg_color_space == JCS_GRAYSCALE) {
    while (dinfo.output_scanline < dinfo.output_height) {
      int y = dinfo.output_scanline, x;
      jpeg_read_scanlines(&dinfo, &line, 1);
      for (x = 0; x < width; x++) {
        gColor c = line[x];
        tex->data[x + tex->tw * y] = c | (c << 8) | (c << 16) | 0xff000000;;
      }
    }
  } else {
    while (dinfo.output_scanline < dinfo.output_height) {
      int y = dinfo.output_scanline, x;
      jpeg_read_scanlines(&dinfo, &line, 1);
      u8* linePointer = line;
      for (x = 0; x < width; x++) {
        gColor c = *(linePointer++);
        c |= (*(linePointer++)) << 8;
        c |= (*(linePointer++)) << 16;
        tex->data[x + tex->tw * y] = c | 0xff000000;
      }
    }
  }
  jpeg_finish_decompress(&dinfo);
  jpeg_destroy_decompress(&dinfo);
  free(line);
}
#endif


void gTexFree(gImage** tex)
{
  if (tex == NULL) return;
  free((*tex)->data);
  free((*tex));
  *tex = NULL;
}


gImage* gTexLoad(char path[], bool use_swizzle)
{
  if (path == NULL) return NULL;

  gImage* tex = malloc(sizeof(gImage));
  if (tex == NULL) return NULL;
  FILE* fp = fopen(path,"rb");
  if (fp == NULL) return NULL;

  #ifdef USE_PNG
  if (strstr(path,".png") != NULL)
  {
    _gTexLoadPNG(fp,tex);
    tex->can_blend = G_TRUE;
  }
  #endif
  #ifdef USE_JPEG
  if (strstr(path,".jpg")  != NULL ||
      strstr(path,".jpeg") != NULL )
  {
    _gTexLoadJPEG(fp,tex);
    tex->can_blend = G_FALSE;
  }
  #endif

  fclose(fp);
  sceKernelDcacheWritebackAll();

  // The PSP can't draw 512*512+ textures.
  if (tex->w > 512 || tex->h > 512)
  {
    gTexFree(&tex);
    return NULL;
  }

  // Swizzling is useless with small textures.
  if (use_swizzle && (tex->w >= 16 || tex->h >= 16))
  {
    u8* tmp = malloc(tex->tw*tex->th*PIXEL_SIZE);
    _swizzle(tmp,(u8*)tex->data,tex->tw*PIXEL_SIZE,tex->th);
    free(tex->data);
    tex->data = (gColor*)tmp;
    tex->swizzled = G_TRUE;
  }

  return tex;
}

// * Scissor functions *

void gResetScissor()
{
  gSetScissor(0,0,G_SCR_W,G_SCR_H);
  scissor = G_FALSE;
}


void gSetScissor(int x, int y, int w, int h)
{
  sceGuScissor(x,y,x+w,y+h);
  scissor = G_TRUE;
}

// EOF
