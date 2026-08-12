// Dummy output.

#include <types.h>
#include <timers.h>
#include <assert.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>

#include <gccore.h>
#include <wiiuse/wpad.h>

#define SCREEN_SCALE 4

#define GPU_ALIGN 32
#define GPU_ALLOC(size) memalign(GPU_ALIGN, size)
#define DEFAULT_FIFO_SIZE       (256*1024)

static void *xfb = NULL;
static GXRModeObj *rmode = NULL;

static void *gp_fifo = NULL;
static u32 *tex_fb = NULL;
static u32 tex_fb_sz;
static GXTexObj tex_obj;
static u32 tex_width, tex_height;
static u32 fb_width, fb_height;

int init(int moduleno, char* argstr) {
	VIDEO_Init();
    WPAD_Init();

    rmode = VIDEO_GetPreferredMode(NULL);
    xfb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));

    VIDEO_Configure(rmode);
    VIDEO_SetNextFramebuffer(xfb);
    VIDEO_SetBlack(FALSE);
    VIDEO_Flush();
    VIDEO_WaitVSync();
    if (rmode->viTVMode & VI_NON_INTERLACE) VIDEO_WaitVSync();

    gp_fifo = GPU_ALLOC(DEFAULT_FIFO_SIZE);
    memset(gp_fifo, 0, DEFAULT_FIFO_SIZE);

    GX_Init(gp_fifo, DEFAULT_FIFO_SIZE);
    GX_SetCopyClear((GXColor){ 0, 0, 0, 255 }, 0x00ffffff);

    GX_SetViewport(0, 0, rmode->fbWidth, rmode->efbHeight, 0, 1);
    f32 yscale = GX_GetYScaleFactor(rmode->efbHeight,rmode->xfbHeight);
    u32 xfbHeight = GX_SetDispCopyYScale(yscale);
    GX_SetScissor(0,0,rmode->fbWidth,rmode->efbHeight);
    GX_SetDispCopySrc(0,0,rmode->fbWidth,rmode->efbHeight);
    GX_SetDispCopyDst(rmode->fbWidth,xfbHeight);
    GX_SetCopyFilter(rmode->aa,rmode->sample_pattern,GX_TRUE,rmode->vfilter);
    GX_SetFieldMode(rmode->field_rendering,((rmode->viHeight==2*rmode->xfbHeight)?GX_ENABLE:GX_DISABLE));

    if (rmode->aa)
        GX_SetPixelFmt(GX_PF_RGB565_Z16, GX_ZC_LINEAR);
    else
        GX_SetPixelFmt(GX_PF_RGB8_Z24, GX_ZC_LINEAR);

    GX_SetCullMode(GX_CULL_NONE);
    GX_CopyDisp(xfb, GX_TRUE);
    GX_SetDispCopyGamma(GX_GM_1_0);

    GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XY, GX_F32, 0);
    GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);

    GX_SetNumChans(1);
    GX_SetNumTexGens(1);
    GX_SetTevOp(GX_TEVSTAGE0, GX_REPLACE);
    GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
    GX_SetTexCoordGen(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY);

    GX_InvalidateTexAll();

    tex_width = 1024 / SCREEN_SCALE;
    tex_height = 1024 / SCREEN_SCALE;
    fb_width = rmode->fbWidth / SCREEN_SCALE;
    fb_height = rmode->efbHeight / SCREEN_SCALE;

    tex_fb = GPU_ALLOC(tex_fb_sz = GX_GetTexBufferSize(tex_width, tex_height, GX_TF_RGBA8, 0, 0));
    memset(tex_fb, 0, tex_fb_sz);
    GX_InitTexObj(&tex_obj, tex_fb, tex_width, tex_height, GX_TF_RGBA8, GX_MIRROR, GX_MIRROR, GX_FALSE);
    GX_LoadTexObj(&tex_obj, GX_TEXMAP0);

    /*Mtx44 projection = {
        {1, 0, 0, 0},
        {0, 1, 0, 0},
        {0, 0, 1, 0},
        {0, 0, 0, 1},
    };*/
    Mtx44 projection;
    guOrtho(&projection, -1, 1, -1, 1, 0, 1);
    GX_LoadProjectionMtx(projection, GX_ORTHOGRAPHIC);

	return 0;
}

int getx(void) {
	return fb_width;
}
int gety(void) {
	return fb_height;
}

// Shamelessly stolen from GRRLIB!
// RGBA8 textures on the Wii are no joke!

#define RGBA(r,g,b,a) ( (u32)( ( ((u32)(r))        <<24) |  \
                               ((((u32)(g)) &0xFF) <<16) |  \
                               ((((u32)(b)) &0xFF) << 8) |  \
                               ( ((u32)(a)) &0xFF      ) ) )

int set(int x, int y, RGB *color) {
    register u32  offs;
    register u8*  bp = (u8*)tex_fb;

    offs = (((y&(~3))<<2) * tex_width) + ((x&(~3))<<4) + ((((y&3)<<2) + (x&3)) <<1);

    u32 col = RGBA(color->red, color->green, color->blue, 255);

    *((u16*)(bp+offs   )) = (u16)((col <<8) | (col >>24));
    *((u16*)(bp+offs+32)) = (u16) (col >>8);

	return 0;
}

int clear(void) {
	memset(tex_fb, 0, tex_fb_sz);
	return 0;
}

int render(void) {
	WPAD_ScanPads();

    u32 pressed = WPAD_ButtonsDown(0);

    if (pressed & WPAD_BUTTON_HOME) {
        timers_doquit();
        return 0;
    }

    GX_SetViewport(0, 0, rmode->fbWidth, rmode->efbHeight, 0, 1);
    GX_InvVtxCache();
    GX_InvalidateTexAll();

    DCFlushRange(tex_fb, tex_fb_sz);

    GX_ClearVtxDesc();
    GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
    GX_SetVtxDesc(GX_VA_TEX0, GX_DIRECT);

    f32 tx = (f32)fb_width/(f32)tex_width;
    f32 ty = (f32)fb_height/(f32)tex_height;

    GX_Begin(GX_QUADS, GX_VTXFMT0, 4);
        GX_Position2f32(-1.0f, 1.0f);
        GX_TexCoord2f32(0.0f, 0.0f);
        GX_Position2f32(1.0f, 1.0f);
        GX_TexCoord2f32(tx, 0.0f);
        GX_Position2f32(1.0f, -1.0f);
        GX_TexCoord2f32(tx, ty);
        GX_Position2f32(-1.0f, -1.0f);
        GX_TexCoord2f32(0.0f, ty);
    GX_End();

    GX_DrawDone();

    GX_SetZMode(GX_TRUE, GX_LEQUAL, GX_TRUE);
    GX_SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_CLEAR);
    GX_SetAlphaUpdate(GX_TRUE);
    GX_SetColorUpdate(GX_TRUE);
    GX_CopyDisp(xfb, GX_TRUE);

    VIDEO_SetNextFramebuffer(xfb);
    VIDEO_Flush();
    VIDEO_WaitVSync();

	return 0;
}

ulong wait_until(ulong desired_usec) {
	// Hey, we can just delegate work to someone else. Yay!
	return wait_until_core(desired_usec);
}

void wait_until_break(void) {
	wait_until_break_core();
}

int deinit(void) {
	WPAD_Shutdown();

	return 0;
}
