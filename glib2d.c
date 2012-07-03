/*
 * gLib2D - A simple, fast, light-weight 2D graphics library.
 *
 * Copyright 2012 Clément Guérin <geecko.dev@free.fr>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "glib2d.h"

#include <pspkernel.h>
#include <pspdisplay.h>
#include <pspgu.h>
#include <vram.h>
#include <malloc.h>
#include <math.h>

#ifdef USE_PNG
#include <png.h>
#endif

#ifdef USE_JPEG
#include <jpeglib.h>
#endif

#define LIST_SIZE               (524288)
#define LINE_SIZE               (512)
#define PIXEL_SIZE              (4)
#define FRAMEBUFFER_SIZE        (LINE_SIZE*G2D_SCR_H*PIXEL_SIZE)
#define MALLOC_STEP             (128)
#define TRANSFORM_STACK_MAX     (64)
#define SLICE_WIDTH             (64.f)
#define M_180_PI                (57.29578f)
#define M_PI_180                (0.017453292f)

#define DEFAULT_SIZE            (10)
#define DEFAULT_COORD_MODE      (G2D_UP_LEFT)
#define DEFAULT_X               (0.f)
#define DEFAULT_Y               (0.f)
#define DEFAULT_Z               (0.f)
#define DEFAULT_COLOR           (WHITE)
#define DEFAULT_ALPHA           (0xFF)

#define CURRENT_OBJ             obj_list[obj_list_size-1]
#define CURRENT_TRANSFORM       transform_stack[transform_stack_size-1]
#define I_OBJ                   obj_list[i]

typedef enum
{
    RECTS, LINES, QUADS, POINTS
} Obj_Type;

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
    float rot, rot_sin, rot_cos;
    int crop_x, crop_y;
    int crop_w, crop_h;
    float scale_w, scale_h;
    g2dColor color;
    g2dAlpha alpha;
} Object;


/* Main vars */
static int *list;
static bool init = false, start = false, zclear = true, scissor = false;
static Transform transform_stack[TRANSFORM_STACK_MAX];
static unsigned int transform_stack_size;
static float global_scale = 1.f;

/* Object vars */
static Object *obj_list = NULL, obj;
static Obj_Type obj_type;
static unsigned int obj_list_size;
static bool obj_begin = false, obj_line_strip;
static bool obj_use_z, obj_use_vert_color, obj_use_rot;
static bool obj_use_tex_linear, obj_use_tex_repeat, obj_use_int;
static g2dCoord_Mode obj_coord_mode;
static unsigned int obj_colors_count;
static g2dTexture *obj_tex;

g2dTexture g2d_draw_buffer =
{
    512, 512,
    G2D_SCR_W, G2D_SCR_H, 
    (float)G2D_SCR_W/G2D_SCR_H,
    false, 
    (g2dColor*)FRAMEBUFFER_SIZE
};

g2dTexture g2d_disp_buffer =
{
    512, 512,
    G2D_SCR_W, G2D_SCR_H, 
    (float)G2D_SCR_W/G2D_SCR_H, false, 
    (g2dColor*)0
};

/* Internal functions */

void _g2dStart()
{
    if (!init)
        g2dInit();

    sceKernelDcacheWritebackRange(list, LIST_SIZE);
    sceGuStart(GU_DIRECT, list);
    start = true;
}


void* _g2dSetVertex(void *vp, int i, float vx, float vy)
{
    // Vertex order: [texture uv] [color] [coord]
    short *vp_short;
    g2dColor *vp_color;
    float *vp_float;

    // Texture coordinates
    vp_short = (short*)vp;
    
    if (obj_tex != NULL)
    {
        *(vp_short++) = I_OBJ.crop_x + vx * I_OBJ.crop_w;
        *(vp_short++) = I_OBJ.crop_y + vy * I_OBJ.crop_h;
    }

    // Color
    vp_color = (g2dColor*)vp_short;

    if (obj_use_vert_color)
    {
        *(vp_color++) = I_OBJ.color;
    }

    // Coordinates
    vp_float = (float*)vp_color;

    vp_float[0] = I_OBJ.x;
    vp_float[1] = I_OBJ.y;

    if (obj_type == RECTS)
    {
        vp_float[0] += vx * I_OBJ.scale_w;
        vp_float[1] += vy * I_OBJ.scale_h;
        
        if (obj_use_rot) // Apply a rotation
        {
            float tx = vp_float[0] - I_OBJ.rot_x;
            float ty = vp_float[1] - I_OBJ.rot_y;

            vp_float[0] = I_OBJ.rot_x - I_OBJ.rot_sin*ty + I_OBJ.rot_cos*tx, 
            vp_float[1] = I_OBJ.rot_y + I_OBJ.rot_cos*ty + I_OBJ.rot_sin*tx;
        }
    }

    if (obj_use_int) // Pixel perfect
    {
        vp_float[0] = floorf(vp_float[0]);
        vp_float[1] = floorf(vp_float[1]);
    }
    vp_float[2] = I_OBJ.z;

    return (void*)(vp_float + 3);
}


#ifdef USE_VFPU
void vfpu_sincosf(float x, float *s, float *c)
{
    __asm__ volatile (
        "mtv    %2, s000\n"           // s000 = x
        "vcst.s s001, VFPU_2_PI\n"    // s001 = 2/pi
        "vmul.s s000, s000, s001\n"   // s000 = s000*s001
        "vrot.p c010, s000, [s, c]\n" // s010 = sinf(s000), s011 = cosf(s000)
        "mfv    %0, s010\n"           // *s = s010
        "mfv    %1, S011\n"           // *c = s011
        : "=r"(*s), "=r"(*c) : "r"(x)
    );
}
#endif

/* Main functions */

void g2dInit()
{
    if (init)
        return;

    // Display list allocation
    list = malloc(LIST_SIZE);

    // Setup GU
    sceGuInit();
    sceGuStart(GU_DIRECT, list);

    sceGuDrawBuffer(GU_PSM_8888, g2d_draw_buffer.data, LINE_SIZE);
    sceGuDispBuffer(G2D_SCR_W, G2D_SCR_H, g2d_disp_buffer.data, LINE_SIZE);
    sceGuDepthBuffer((void*)(FRAMEBUFFER_SIZE*2), LINE_SIZE);
    sceGuOffset(2048-G2D_SCR_W/2, 2048-G2D_SCR_H/2);
    sceGuViewport(2048, 2048, G2D_SCR_W, G2D_SCR_H);

    g2d_draw_buffer.data = vabsptr(g2d_draw_buffer.data);
    g2d_disp_buffer.data = vabsptr(g2d_disp_buffer.data);

    g2dResetScissor();
    sceGuDepthRange(65535, 0);
    sceGuClearDepth(65535);
    sceGuAlphaFunc(GU_GREATER, 0, 255);
    sceGuDepthFunc(GU_LEQUAL);
    sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);
    sceGuTexFunc(GU_TFX_MODULATE, GU_TCC_RGBA);
    sceGuTexFilter(GU_LINEAR, GU_LINEAR);
    sceGuShadeModel(GU_SMOOTH);

    sceGuDisable(GU_CULL_FACE);
    sceGuDisable(GU_CLIP_PLANES);
    sceGuDisable(GU_DITHER);
    sceGuEnable(GU_ALPHA_TEST);
    sceGuEnable(GU_SCISSOR_TEST);
    sceGuEnable(GU_BLEND);

    sceGuFinish();
    sceGuSync(0, 0);
    sceDisplayWaitVblankStart();
    sceGuDisplay(GU_TRUE);

    init = true;
}


void g2dTerm()
{
    if (!init)
        return;
 
    sceGuTerm();

    free(list);
    
    init = false;
}

void g2dClear(g2dColor color)
{
    if (!start)
        _g2dStart();

    sceGuClearColor(color);
    sceGuClear(GU_COLOR_BUFFER_BIT |
               GU_FAST_CLEAR_BIT |
               (zclear ? GU_DEPTH_BUFFER_BIT : 0));

    zclear = false;
}


void g2dClearZ()
{
    if (!start)
        _g2dStart();

    sceGuClear(GU_DEPTH_BUFFER_BIT | GU_FAST_CLEAR_BIT);
    zclear = true;
}


void _g2dBeginCommon()
{
    if (!start)
        _g2dStart();

    obj_list_size = 0;
    obj_list = realloc(obj_list, MALLOC_STEP * sizeof(Object));

    obj_use_z = false;
    obj_use_vert_color = false;
    obj_use_rot = false;
    obj_use_int = false;
    obj_colors_count = 0;
    g2dReset();

    obj_begin = true;
}


void g2dBeginRects(g2dTexture *tex)
{
    if (obj_begin)
        return;

    obj_type = RECTS;
    obj_tex = tex;

    _g2dBeginCommon();
}


void g2dBeginLines(g2dLine_Mode mode)
{
    if (obj_begin)
        return;

    obj_type = LINES;
    obj_tex = NULL;
    obj_line_strip = (mode & G2D_STRIP);

    _g2dBeginCommon();
}


void g2dBeginQuads(g2dTexture *tex)
{
    if (obj_begin)
        return;

    obj_type = QUADS;
    obj_tex = tex;

    _g2dBeginCommon();
}


void g2dBeginPoints()
{
    if (obj_begin)
        return;

    obj_type = POINTS;
    obj_tex = NULL;

    _g2dBeginCommon();
}


void _g2dEndRects()
{
    // Define vertices properties
    int v_prim = (obj_use_rot ? GU_TRIANGLES : GU_SPRITES);
    int v_obj_nbr = (obj_use_rot ? 6 : 2);
    int v_nbr;
    int v_coord_size = 3;
    int v_tex_size = (obj_tex != NULL ? 2 : 0);
    int v_color_size = (obj_use_vert_color ? 1 : 0);
    int v_size = v_tex_size * sizeof(short) +
                 v_color_size * sizeof(g2dColor) +
                 v_coord_size * sizeof(float);
    int v_type = GU_VERTEX_32BITF | GU_TRANSFORM_2D;
    int i;

    if (obj_tex != NULL)    v_type |= GU_TEXTURE_16BIT;
    if (obj_use_vert_color) v_type |= GU_COLOR_8888;

    // Count how many vertices to allocate.
    if (obj_tex == NULL || obj_use_rot) // No slicing
    {
        v_nbr = v_obj_nbr * obj_list_size;
    }
    else // Can use texture slicing for tremendous performance :)
    {
        v_nbr = 0;

        for (i=0; i<obj_list_size; i++)
        {
            v_nbr += v_obj_nbr * ceilf(I_OBJ.crop_w/SLICE_WIDTH);
        }
    }

    // Allocate vertex list memory
    void *v = sceGuGetMemory(v_nbr * v_size);
    void *vi = v;

    // Build the vertex list
    for (i=0; i<obj_list_size; i+=1)
    {
        if (obj_use_rot) // Two triangles per object
        {
            vi = _g2dSetVertex(vi, i, 0.f, 0.f);
            vi = _g2dSetVertex(vi, i, 1.f, 0.f);
            vi = _g2dSetVertex(vi, i, 0.f, 1.f);
            vi = _g2dSetVertex(vi, i, 0.f, 1.f);
            vi = _g2dSetVertex(vi, i, 1.f, 0.f);
            vi = _g2dSetVertex(vi, i, 1.f, 1.f);
        }
        else if (obj_tex == NULL) // One sprite per object
        {
            vi = _g2dSetVertex(vi, i, 0.f, 0.f);
            vi = _g2dSetVertex(vi, i, 1.f, 1.f);
        }
        else // Several sprites per object for a better texture cache use
        {
            float step = SLICE_WIDTH/I_OBJ.crop_w;
            float u;

            for (u=0.f; u<1.f; u+=step)
            {
                vi = _g2dSetVertex(vi, i, u, 0.f);
                vi = _g2dSetVertex(vi, i, (u+step>1.f ? 1.f : u+step), 1.f);
            }
        }
    }

    // Then put it in the display list.
    sceGuDrawArray(v_prim, v_type, v_nbr, NULL, v);
}


void _g2dEndLines()
{
    // Define vertices properties
    int v_prim = (obj_line_strip ? GU_LINE_STRIP : GU_LINES);
    int v_obj_nbr = (obj_line_strip ? 1 : 2);
    int v_nbr = v_obj_nbr * (obj_line_strip ? obj_list_size : obj_list_size/2);
    int v_coord_size = 3;
    int v_color_size = (obj_use_vert_color ? 1 : 0);
    int v_size = v_color_size * sizeof(g2dColor) +
                 v_coord_size * sizeof(float);
    int v_type = GU_VERTEX_32BITF | GU_TRANSFORM_2D;
    int i;

    if (obj_use_vert_color) v_type |= GU_COLOR_8888;

    // Allocate vertex list memory
    void *v = sceGuGetMemory(v_nbr * v_size);
    void *vi = v;

    // Build the vertex list
    if (obj_line_strip)
    {
        vi = _g2dSetVertex(vi, 0, 0.f, 0.f);

        for (i=1; i<obj_list_size; i+=1)
        {
            vi = _g2dSetVertex(vi, i, 0.f, 0.f);
        }
    }
    else
    {
        for (i=0; i+1<obj_list_size; i+=2)
        {
            vi = _g2dSetVertex(vi, i  , 0.f, 0.f);
            vi = _g2dSetVertex(vi, i+1, 0.f, 0.f);
        }
    }

    // Then put it in the display list.
    sceGuDrawArray(v_prim, v_type, v_nbr, NULL, v);
}


void _g2dEndQuads()
{
    // Define vertices properties
    int v_prim = GU_TRIANGLES;
    int v_obj_nbr = 6;
    int v_nbr = v_obj_nbr * (obj_list_size / 4);
    int v_coord_size = 3;
    int v_tex_size = (obj_tex != NULL ? 2 : 0);
    int v_color_size = (obj_use_vert_color ? 1 : 0);
    int v_size = v_tex_size * sizeof(short) +
                 v_color_size * sizeof(g2dColor) +
                 v_coord_size * sizeof(float);
    int v_type = GU_VERTEX_32BITF | GU_TRANSFORM_2D;
    int i;

    if (obj_tex != NULL)    v_type |= GU_TEXTURE_16BIT;
    if (obj_use_vert_color) v_type |= GU_COLOR_8888;

    // Allocate vertex list memory
    void *v = sceGuGetMemory(v_nbr * v_size);
    void *vi = v;

    // Build the vertex list
    for (i=0; i+3<obj_list_size; i+=4)
    {
        vi = _g2dSetVertex(vi, i  , 0.f, 0.f);
        vi = _g2dSetVertex(vi, i+1, 1.f, 0.f);
        vi = _g2dSetVertex(vi, i+3, 0.f, 1.f);
        vi = _g2dSetVertex(vi, i+3, 0.f, 1.f);
        vi = _g2dSetVertex(vi, i+1, 1.f, 0.f);
        vi = _g2dSetVertex(vi, i+2, 1.f, 1.f);
    }

    // Then put it in the display list.
    sceGuDrawArray(v_prim, v_type, v_nbr, NULL, v);
}


void _g2dEndPoints()
{
    // Define vertices properties
    int v_prim = GU_POINTS;
    int v_obj_nbr = 1;
    int v_nbr = v_obj_nbr * obj_list_size;
    int v_coord_size = 3;
    int v_color_size = (obj_use_vert_color ? 1 : 0);
    int v_size = v_color_size * sizeof(g2dColor) +
                 v_coord_size * sizeof(float);
    int v_type = GU_VERTEX_32BITF | GU_TRANSFORM_2D;
    int i;

    if (obj_use_vert_color) v_type |= GU_COLOR_8888;

    // Allocate vertex list memory
    void *v = sceGuGetMemory(v_nbr * v_size);
    void *vi = v;

    // Build the vertex list
    for (i=0; i<obj_list_size; i+=1)
    {
        vi = _g2dSetVertex(vi, i, 0.f, 0.f);
    }

    // Then put it in the display list.
    sceGuDrawArray(v_prim, v_type, v_nbr, NULL, v);
}


void g2dEnd()
{
    if (!obj_begin || obj_list_size == 0)
    {
        obj_begin = false;
        return;
    }

    // Manage pspgu extensions
    if (obj_use_z)          sceGuEnable(GU_DEPTH_TEST);
    else                    sceGuDisable(GU_DEPTH_TEST);

    if (obj_use_vert_color) sceGuColor(WHITE);
    else                    sceGuColor(obj_list[0].color);

    if (obj_tex == NULL)    sceGuDisable(GU_TEXTURE_2D);
    else
    {
        sceGuEnable(GU_TEXTURE_2D);

        if (obj_use_tex_linear) sceGuTexFilter(GU_LINEAR, GU_LINEAR);
        else                    sceGuTexFilter(GU_NEAREST, GU_NEAREST);

        if (obj_use_tex_repeat) sceGuTexWrap(GU_REPEAT, GU_REPEAT);
        else                    sceGuTexWrap(GU_CLAMP, GU_CLAMP);

        // Load texture
        sceGuTexMode(GU_PSM_8888, 0, 0, obj_tex->swizzled);
        sceGuTexImage(0, obj_tex->tw, obj_tex->th, obj_tex->tw, obj_tex->data);
    }

    switch (obj_type)
    {
        case RECTS:
            _g2dEndRects();
            break;

        case LINES:
            _g2dEndLines();
            break;

        case QUADS:
            _g2dEndQuads();
            break;

        case POINTS:
            _g2dEndPoints();
            break;
    }

    sceGuColor(WHITE);

    if (obj_use_z)
        zclear = true;

    obj_begin = false;
}


void g2dReset()
{
    g2dResetCoord();
    g2dResetScale();
    g2dResetColor();
    g2dResetAlpha();
    g2dResetRotation();
    g2dResetCrop();
    g2dResetTex();
}


void g2dFlip(g2dFlip_Mode mode)
{
    if (scissor)
        g2dResetScissor();

    sceGuFinish();
    sceGuSync(0, 0);

    if (mode & G2D_VSYNC)
        sceDisplayWaitVblankStart();

    g2d_disp_buffer.data = g2d_draw_buffer.data;
    g2d_draw_buffer.data = vabsptr(sceGuSwapBuffers());

    start = false;
}


void g2dAdd()
{
    if (!obj_begin || obj.scale_w == 0.f || obj.scale_h == 0.f)
        return;

    if (obj_list_size % MALLOC_STEP == 0)
    {
        obj_list = realloc(obj_list,
                           (obj_list_size+MALLOC_STEP) * sizeof(Object));
    }

    obj_list_size++;
    obj.rot_x = obj.x;
    obj.rot_y = obj.y;
    CURRENT_OBJ = obj;

    // Coordinate mode stuff
    switch (obj_coord_mode)
    {
        case G2D_UP_RIGHT:
            CURRENT_OBJ.x -= CURRENT_OBJ.scale_w;
            break;

        case G2D_DOWN_RIGHT:
            CURRENT_OBJ.x -= CURRENT_OBJ.scale_w;
            CURRENT_OBJ.y -= CURRENT_OBJ.scale_h;
            break;

        case G2D_DOWN_LEFT:
            CURRENT_OBJ.y -= CURRENT_OBJ.scale_h;
            break;

        case G2D_CENTER:
            CURRENT_OBJ.x -= CURRENT_OBJ.scale_w / 2.f;
            CURRENT_OBJ.y -= CURRENT_OBJ.scale_h / 2.f;
            break;
            
        case G2D_UP_LEFT:
        default:
            break;
    };

    // Alpha stuff
    CURRENT_OBJ.color = G2D_MODULATE(CURRENT_OBJ.color, 255, obj.alpha);
}


void g2dPush()
{
    if (transform_stack_size >= TRANSFORM_STACK_MAX)
        return;

    transform_stack_size++;

    CURRENT_TRANSFORM.x = obj.x;
    CURRENT_TRANSFORM.y = obj.y;
    CURRENT_TRANSFORM.z = obj.z;
    CURRENT_TRANSFORM.rot = obj.rot;
    CURRENT_TRANSFORM.rot_sin = obj.rot_sin;
    CURRENT_TRANSFORM.rot_cos = obj.rot_cos;
    CURRENT_TRANSFORM.scale_w = obj.scale_w;
    CURRENT_TRANSFORM.scale_h = obj.scale_h;
}


void g2dPop()
{
    if (transform_stack_size <= 0)
        return;

    obj.x = CURRENT_TRANSFORM.x;
    obj.y = CURRENT_TRANSFORM.y;
    obj.z = CURRENT_TRANSFORM.z;
    obj.rot = CURRENT_TRANSFORM.rot;
    obj.rot_sin = CURRENT_TRANSFORM.rot_sin;
    obj.rot_cos = CURRENT_TRANSFORM.rot_cos;
    obj.scale_w = CURRENT_TRANSFORM.scale_w;
    obj.scale_h = CURRENT_TRANSFORM.scale_h;

    transform_stack_size--;
    
    if (obj.rot != 0.f) obj_use_rot = true;
    if (obj.z != 0.f)   obj_use_z = true;
}

/* Coord functions */

void g2dResetCoord()
{
    obj_coord_mode = DEFAULT_COORD_MODE;
    obj.x = DEFAULT_X;
    obj.y = DEFAULT_Y;
    obj.z = DEFAULT_Z;
}


void g2dSetCoordMode(g2dCoord_Mode mode)
{
    if (mode > G2D_CENTER)
        return;

    obj_coord_mode = mode;
}


void g2dGetCoordXYZ(float *x, float *y, float *z)
{
    if (x != NULL) *x = obj.x;
    if (y != NULL) *y = obj.y;
    if (z != NULL) *z = obj.z;
}


void g2dSetCoordXY(float x, float y)
{
    obj.x = x * global_scale;
    obj.y = y * global_scale;
    obj.z = 0.f;
}


void g2dSetCoordXYZ(float x, float y, float z)
{
    obj.x = x * global_scale;
    obj.y = y * global_scale;
    obj.z = z * global_scale;

    if (z != 0.f)
        obj_use_z = true;
}


void g2dSetCoordXYRelative(float x, float y)
{
    float inc_x = x;
    float inc_y = y;

    if (obj.rot_cos != 1.f)
    {
        inc_x = -obj.rot_sin*y + obj.rot_cos*x;
        inc_y =  obj.rot_cos*y + obj.rot_sin*x;
    }

    obj.x += inc_x * global_scale;
    obj.y += inc_y * global_scale;
}


void g2dSetCoordXYZRelative(float x, float y, float z)
{
    g2dSetCoordXYRelative(x, y);

    obj.z += z * global_scale;

    if (z != 0.f)
        obj_use_z = true;
}


void g2dSetCoordInteger(bool use)
{
    obj_use_int = use;
}

/* Scale functions */

void g2dResetGlobalScale()
{
    global_scale = 1.f;
}


void g2dResetScale()
{
    if (obj_tex == NULL)
    {
        obj.scale_w = DEFAULT_SIZE;
        obj.scale_h = DEFAULT_SIZE;
    }
    else
    {
        obj.scale_w = obj_tex->w;
        obj.scale_h = obj_tex->h;
    }

    obj.scale_w *= global_scale;
    obj.scale_h *= global_scale;
}


void g2dGetGlobalScale(float *scale)
{
    if (scale != NULL)
        *scale = global_scale;
}


void g2dGetScaleWH(float *w, float *h)
{
    if (w != NULL) *w = obj.scale_w;
    if (h != NULL) *h = obj.scale_h;
}


void g2dSetGlobalScale(float scale)
{
    global_scale = scale;
}


void g2dSetScale(float w, float h)
{
    g2dResetScale();

    g2dSetScaleRelative(w, h);
}


void g2dSetScaleWH(float w, float h)
{
    obj.scale_w = w * global_scale;
    obj.scale_h = h * global_scale;

    // A trick to prevent an unexpected behavior when mirroring with GU_SPRITES.
    if (obj.scale_w < 0 || obj.scale_h < 0)
        obj_use_rot = true;
}


void g2dSetScaleRelative(float w, float h)
{
    obj.scale_w *= w;
    obj.scale_h *= h;

    if (obj.scale_w < 0 || obj.scale_h < 0)
        obj_use_rot = true;
}


void g2dSetScaleWHRelative(float w, float h)
{
    obj.scale_w += w * global_scale;
    obj.scale_h += h * global_scale;

    if (obj.scale_w < 0 || obj.scale_h < 0)
        obj_use_rot = true;
}

/* Color functions */

void g2dResetColor()
{
    obj.color = DEFAULT_COLOR;
}


void g2dResetAlpha()
{
    obj.alpha = DEFAULT_ALPHA;
}


void g2dGetAlpha(g2dAlpha *alpha)
{
    if (alpha != NULL)
        *alpha = obj.alpha;
}


void g2dSetColor(g2dColor color)
{
    obj.color = color;

    if (++obj_colors_count > 1)
        obj_use_vert_color = true;
}


void g2dSetAlpha(g2dAlpha alpha)
{
    if (alpha < 0)   alpha = 0;
    if (alpha > 255) alpha = 255;

    obj.alpha = alpha;

    if (++obj_colors_count > 1)
        obj_use_vert_color = true;
}


void g2dSetAlphaRelative(int alpha)
{
    g2dSetAlpha(obj.alpha + alpha);
}

/* Rotation functions */

void g2dResetRotation()
{
    obj.rot = 0.f;
    obj.rot_sin = 0.f;
    obj.rot_cos = 1.f;
}


void g2dGetRotationRad(float *radians)
{
    if (radians != NULL)
        *radians = obj.rot;
}


void g2dGetRotation(float *degrees)
{
    if (degrees != NULL)
        *degrees = obj.rot * M_180_PI;
}


void g2dSetRotationRad(float radians)
{
    if (radians == obj.rot)
        return;

    obj.rot = radians;

#ifdef USE_VFPU
    vfpu_sincosf(radians, &obj.rot_sin, &obj.rot_cos);
#else
    sincosf(radians, &obj.rot_sin, &obj.rot_cos);
#endif

    if (radians != 0.f)
        obj_use_rot = true;
}


void g2dSetRotation(float degrees)
{
    g2dSetRotationRad(degrees * M_PI_180);
}


void g2dSetRotationRadRelative(float radians)
{
    g2dSetRotationRad(obj.rot + radians);
}


void g2dSetRotationRelative(float degrees)
{
    g2dSetRotationRadRelative(degrees * M_PI_180);
}

/* Crop functions */

void g2dResetCrop()
{
    if (obj_tex == NULL)
        return;

    obj.crop_x = 0;
    obj.crop_y = 0;
    obj.crop_w = obj_tex->w;
    obj.crop_h = obj_tex->h;
}


void g2dGetCropXY(int *x, int *y)
{
    if (obj_tex == NULL)
        return;

    if (x != NULL) *x = obj.crop_x;
    if (y != NULL) *y = obj.crop_y;
}


void g2dGetCropWH(int *w, int *h)
{
    if (obj_tex == NULL)
        return;

    if (w != NULL) *w = obj.crop_w;
    if (h != NULL) *h = obj.crop_h;
}


void g2dSetCropXY(int x, int y)
{
    if (obj_tex == NULL)
        return;

    obj.crop_x = x;
    obj.crop_y = y;
}


void g2dSetCropWH(int w, int h)
{
    if (obj_tex == NULL)
        return;

    obj.crop_w = w;
    obj.crop_h = h;
}


void g2dSetCropXYRelative(int x, int y)
{
    if (obj_tex == NULL)
        return;

    g2dSetCropXY(obj.crop_x + x, obj.crop_y + y);
}


void g2dSetCropWHRelative(int w, int h)
{
    if (obj_tex == NULL)
        return;

    g2dSetCropWH(obj.crop_w + w, obj.crop_h + h);
}

/* Texture functions */

void g2dResetTex()
{
    if (obj_tex == NULL)
        return;

    obj_use_tex_repeat = false;
    obj_use_tex_linear = true;
}


void g2dSetTexRepeat(bool use)
{
    if (obj_tex == NULL)
        return;

    obj_use_tex_repeat = use;
}


void g2dSetTexLinear(bool use)
{
    if (obj_tex == NULL)
        return;

    obj_use_tex_linear = use;
}

/* Texture management */

unsigned int _getNextPower2(unsigned int n)
{
    n--;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;

    return n+1;
}


void _swizzle(unsigned char *dest, unsigned char *source, int width, int height)
{
    int i, j;
    int rowblocks = (width / 16);
    int rowblocks_add = (rowblocks-1) * 128;
    unsigned int block_address = 0;
    unsigned int *img = (unsigned int*)source;

    for (j=0; j<height; j++)
    {
        unsigned int *block = (unsigned int*)(dest + block_address);

        for (i = 0; i < rowblocks; i++)
        {
            *block++ = *img++;
            *block++ = *img++;
            *block++ = *img++;
            *block++ = *img++;

            block += 28;
        }

        if ((j & 0x7) == 0x7)
            block_address += rowblocks_add;

        block_address += 16;
    }
}


g2dTexture*g2dTexCreate(int w, int h)
{
    g2dTexture *tex = malloc(sizeof(g2dTexture));
    if (tex == NULL)
        return NULL;

    tex->tw = _getNextPower2(w);
    tex->th = _getNextPower2(h);
    tex->w = w;
    tex->h = h;
    tex->ratio = (float)w / h;
    tex->swizzled = false;

    tex->data = malloc(tex->tw * tex->th * sizeof(g2dColor));
    if (tex->data == NULL)
    {
        free(tex);
        return NULL;
    }

    memset(tex->data, 0, tex->tw * tex->th * sizeof(g2dColor));

    return tex;
}


void g2dTexFree(g2dTexture **tex)
{
    if (tex == NULL)
        return;
    if (*tex == NULL)
        return;

    free((*tex)->data);
    free((*tex));

    *tex = NULL;
}


#ifdef USE_PNG
g2dTexture* _g2dTexLoadPNG(FILE *fp)
{
    png_structp png_ptr;
    png_infop info_ptr;
    unsigned int sig_read = 0;
    png_uint_32 width, height;
    int bit_depth, color_type, interlace_type;
    u32 x, y;
    g2dColor *line;
    g2dTexture *tex;

    png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    png_set_error_fn(png_ptr, NULL, NULL, NULL);
    info_ptr = png_create_info_struct(png_ptr);
    png_init_io(png_ptr, fp);
    png_set_sig_bytes(png_ptr, sig_read);
    png_read_info(png_ptr, info_ptr);
    png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type, 
                 &interlace_type, NULL, NULL);
    png_set_strip_16(png_ptr);
    png_set_packing(png_ptr);

    if (color_type == PNG_COLOR_TYPE_PALETTE)
        png_set_palette_to_rgb(png_ptr);
    if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
        png_set_tRNS_to_alpha(png_ptr);

    png_set_filler(png_ptr, 0xff, PNG_FILLER_AFTER);
    
    tex = g2dTexCreate(width, height);
    line = malloc(width * 4);

    for (y = 0; y < height; y++)
    {
        png_read_row(png_ptr, (u8*) line, NULL);
        
        for (x = 0; x < width; x++)
            tex->data[x + y*tex->tw] = line[x];
    }

    free(line);
    png_read_end(png_ptr, info_ptr);
    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);

    return tex;
}
#endif


#ifdef USE_JPEG
g2dTexture* _g2dTexLoadJPEG(FILE *fp)
{
    struct jpeg_decompress_struct dinfo;
    struct jpeg_error_mgr jerr;
    int width, height;
    g2dTexture *tex;
    u8 *line;

    dinfo.err = jpeg_std_error(&jerr);
    jpeg_create_decompress(&dinfo);
    jpeg_stdio_src(&dinfo, fp);
    jpeg_read_header(&dinfo, TRUE);

    width = dinfo.image_width;
    height = dinfo.image_height;
    tex = g2dTexCreate(width, height);
    line = malloc(width * 3);
    
    jpeg_start_decompress(&dinfo);
    
    if (dinfo.jpeg_color_space == JCS_GRAYSCALE)
    {
        while (dinfo.output_scanline < dinfo.output_height)
        {
            int y = dinfo.output_scanline;
            int x;

            jpeg_read_scanlines(&dinfo, &line, 1);

            for (x=0; x<width; x++)
            {
                g2dColor gray = line[x];
                g2dColor c = gray | (gray << 8) | (gray << 16) | 0xff000000;

                tex->data[x + tex->tw * y] = c;
            }
        }
    }
    else
    {
        while (dinfo.output_scanline < dinfo.output_height)
        {
            int y = dinfo.output_scanline;
            int x;
            u8 *pline;

            jpeg_read_scanlines(&dinfo, &line, 1);
            pline = line;

            for (x=0; x<width; x++)
            {
                g2dColor c;

                c  = (*(pline++));
                c |= (*(pline++)) << 8;
                c |= (*(pline++)) << 16;

                tex->data[x + tex->tw * y] = c | 0xff000000;
            }
        }
    }

    jpeg_finish_decompress(&dinfo);
    jpeg_destroy_decompress(&dinfo);
    free(line);

    return tex;
}
#endif


g2dTexture* g2dTexLoad(char path[], g2dTex_Mode mode)
{
    g2dTexture *tex = NULL;
    FILE *fp = NULL;

    if (path == NULL)
        return NULL;
    if ((fp = fopen(path, "rb")) == NULL)
        return NULL;

#ifdef USE_PNG
    if (strstr(path, ".png"))
    {
        tex = _g2dTexLoadPNG(fp);
    }
#endif

#ifdef USE_JPEG
    if (strstr(path, ".jpg") || strstr(path, ".jpeg"))
    {
        tex = _g2dTexLoadJPEG(fp);
    }
#endif

    if (tex == NULL)
        goto error;

    fclose(fp);
    fp = NULL;

    // The PSP can't draw 512*512+ textures.
    if (tex->w > 512 || tex->h > 512)
        goto error;

    // Swizzling is useless with small textures.
    if ((mode & G2D_SWIZZLE) && (tex->w >= 16 || tex->h >= 16))
    {
        u8 *tmp = malloc(tex->tw*tex->th*PIXEL_SIZE);
        _swizzle(tmp, (u8*)tex->data, tex->tw*PIXEL_SIZE, tex->th);
        free(tex->data);
        tex->data = (g2dColor*)tmp;
        tex->swizzled = true;
    }
    else
        tex->swizzled = false;

    sceKernelDcacheWritebackRange(tex->data, tex->tw*tex->th*PIXEL_SIZE);

    return tex;

    // Load failure... abort
error:
    if (fp != NULL)
        fclose(fp);

    g2dTexFree(&tex);

    return NULL;
}

/* Scissor functions */

void g2dResetScissor()
{
    g2dSetScissor(0, 0, G2D_SCR_W, G2D_SCR_H);

    scissor = false;
}


void g2dSetScissor(int x, int y, int w, int h)
{
    sceGuScissor(x, y, x+w, y+h);

    scissor = true;
}

// EOF
