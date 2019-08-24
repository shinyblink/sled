// Actually now a libctru output

#include <types.h>
#include <timers.h>
#include <stdlib.h>
#include <3ds.h>
#include <string.h>

#include <citro3d.h>

#include "out_ctru_shbin.h"

#define SCREEN_ID GFX_TOP
#define CONSOLE_ID GFX_BOTTOM
#define SCREEN_SCALE 4

static u16 fb_w, fb_h;
static u16 tex_w, tex_h;
static u32* fb = NULL;

static DVLB_s* vshader_dvlb;
static shaderProgram_s program;
static int uLoc_texscale;

static C3D_Tex screen_fb_tex;
static C3D_RenderTarget *screen_target = NULL;
static float *screen_vtx_buf = NULL;
static float *screen_ind_buf = NULL;

// Set up flags for scaled transfer
#define DISPLAY_TRANSFER_FLAGS \
    (GX_TRANSFER_FLIP_VERT(0) | GX_TRANSFER_OUT_TILED(0) | GX_TRANSFER_RAW_COPY(0) | \
    GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGB8) | GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGB8) | \
    GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO))

static u16 npo2(u16 v) {
	v--;
	v |= v >> 1;
	v |= v >> 2;
	v |= v >> 4;
	v |= v >> 8;
	v++;
	return v;
}

int init(int moduleno, char* argstr) {
	gfxInitDefault();
	consoleInit(CONSOLE_ID, NULL);

	C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);

	// Grab the dimensions of the screen we're rendering to.
	u16 lcd_w, lcd_h;
	gfxGetFramebuffer(SCREEN_ID, GFX_LEFT, &lcd_w, &lcd_h);

	// Create a render target for this screen.
	// RGB8 with no depth/stencil buffer.
	screen_target = C3D_RenderTargetCreate(lcd_w, lcd_h, GPU_RB_RGB8, -1);
	C3D_RenderTargetSetClear(screen_target, C3D_CLEAR_ALL, 0, 0);
	C3D_RenderTargetSetOutput(screen_target, SCREEN_ID, GFX_LEFT, DISPLAY_TRANSFER_FLAGS);

	// Scale the framebuffer height down
	fb_w = lcd_w / SCREEN_SCALE;
	fb_h = lcd_h / SCREEN_SCALE;
	// Textures need a power of 2 dimension
	tex_w = npo2(fb_w);
	tex_h = npo2(fb_h);

	C3D_TexInitVRAM(&screen_fb_tex, tex_w, tex_h, GPU_RGBA8);
	C3D_TexSetWrap(&screen_fb_tex, GPU_REPEAT, GPU_REPEAT);
	C3D_TexSetFilter(&screen_fb_tex, GPU_NEAREST, GPU_NEAREST);
	C3D_TexBind(0, &screen_fb_tex);

	// linearAlloc allocates contiguous memory.
	// The GPU works on physical memory, so we use it instead of malloc.
	fb = linearAlloc(tex_w * tex_h * 4); // RGBA8
	memset(fb, 0, tex_w * tex_h * 4);

	// Vertex Layout: x/u, y/v
	// We have 2 floats per vertex, where each element describes the position and texture coordinate.
	screen_vtx_buf = linearAlloc(sizeof(float) * 4 * 2);

	// Vertex Order:
	// 0 2
	// 6 4
	screen_vtx_buf[0] = 0.0f;
	screen_vtx_buf[1] = 0.0f;
	screen_vtx_buf[2] = 1.0f;
	screen_vtx_buf[3] = 0.0f;

	screen_vtx_buf[4] = 1.0f;
	screen_vtx_buf[5] = 1.0f;
	screen_vtx_buf[6] = 0.0f;
	screen_vtx_buf[7] = 1.0f;

	static u8 indices[] = {
		0, 1, 2,
		0, 2, 3
	};

	// GPU reads the index buffer too, so allocate in linear memory.
	screen_ind_buf = linearAlloc(sizeof(indices));
	memcpy(screen_ind_buf, indices, sizeof(indices));

	// Describe our vertex format to the GPU:
	C3D_AttrInfo *attr = C3D_GetAttrInfo();
	AttrInfo_Init(attr);
	// Two float elements as inputs to shader input register 0.
	AttrInfo_AddLoader(attr, 0, GPU_FLOAT, 2);

	// Describe our buffer layout to the GPU:
	C3D_BufInfo *buf = C3D_GetBufInfo();
	BufInfo_Init(buf);
	// One attribute (attribute 0) in the buffer with a stride of two float elements.
	BufInfo_Add(buf, screen_vtx_buf, sizeof(float) * 2, 1, 0x0);

	// Load up the shader with magic incantations!
	vshader_dvlb = DVLB_ParseFile((u32*)out_ctru_shbin, out_ctru_shbin_size);
	shaderProgramInit(&program);
	shaderProgramSetVsh(&program, &vshader_dvlb->DVLE[0]);
	C3D_BindProgram(&program);
	uLoc_texscale = shaderInstanceGetUniformLocation(program.vertexShader, "texscale");

	// Tell the GPU that we just want to replace the framebuffer's color with the sampled texture color.
	// Below is equivalent to vec4(tex.rgb, 1.0) in GLSL.
	C3D_TexEnv* env = C3D_GetTexEnv(0);
	C3D_TexEnvSrc(env, C3D_RGB, GPU_TEXTURE0, 0, 0);
	C3D_TexEnvSrc(env, C3D_Alpha, GPU_CONSTANT, 0, 0);
	C3D_TexEnvOp(env, C3D_Both, 0, 0, 0);
	C3D_TexEnvFunc(env, C3D_Both, GPU_REPLACE);
	C3D_TexEnvColor(env, 0xFFFFFFFF);

	// Disable depth test:
	C3D_DepthTest(false, GPU_GEQUAL, GPU_WRITE_ALL);

	return 0;
}

int getx(int _modno) {
	return fb_h;
}
int gety(int _modno) {
	return fb_w;
}

int set(int _modno, int x, int y, RGB color) {
	// Ideally we'd want this to be here, however, since we want decent performance even on the old 3DS,
	// this check just isn't in the budget.
	/*
		if (x < 0 || y < 0)
		return 1;
		if (x >= matrix_w || y >= matrix_h)
		return 2; */

	y = fb_w - y - 1;
	x = fb_h - x - 1;
	fb[y + (x * tex_w)] = (color.red << 24) | (color.green << 16) | (color.blue << 8);
	return 0;
}

RGB get(int _modno, int x, int y) {
	y = fb_w - y - 1;
	x = fb_h - x - 1;
	u32 v = fb[y + (x * tex_w)];
	return RGB((v >> 24) & 0xFF, (v >> 16) & 0xFF, (v >> 8) & 0xFF);
}

int clear(int _modno) {
	memset(fb, 0, tex_w * tex_h * 4);
	return 0;
}

int render(void) {
	GSPGPU_FlushDataCache(fb, tex_w * (size_t) tex_h * (size_t) 4);

	void* out = C3D_Tex2DGetImagePtr(&screen_fb_tex, 0, NULL);

	// Transfer to VRAM (linear layout -> morton tiled layout, flip vertically)
	GX_DisplayTransfer(
		(u32*)fb,
		GX_BUFFER_DIM((u32)tex_w, (u32)tex_h),
		(u32*)out,
		GX_BUFFER_DIM((u32)tex_w, (u32)tex_h),
		GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8) | GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGBA8) |
		GX_TRANSFER_OUT_TILED(1) | GX_TRANSFER_FLIP_VERT(1)
	);
	gspWaitForPPF();

	// Draw the texture to the screen
	C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
		C3D_FrameDrawOn(screen_target);
		// Send the texture coordinate scale factor to the shader
		float xscale = ((float)fb_w)/(float)tex_w;
		float yscale = ((float)fb_h)/(float)tex_h;
		C3D_FVUnifSet(GPU_VERTEX_SHADER, uLoc_texscale, xscale, yscale, 0.0f, 0.0f);
		C3D_DrawElements(GPU_TRIANGLES, 6, C3D_UNSIGNED_BYTE, screen_ind_buf);
	C3D_FrameEnd(0);

	// Check if start is pressed, if it is, exit.
	hidScanInput();
	// Raise the shutdown alarm.
	if ((!aptMainLoop()) || (hidKeysDown() & KEY_START))
		timers_doquit();
	return 0;
}

oscore_time wait_until(int _modno, oscore_time desired_usec) {
	// Hey, we can just delegate work to someone else. Yay!
	return timers_wait_until_core(desired_usec);
}

void wait_until_break(int _modno) {
	timers_wait_until_break_core();
}

void deinit(int _modno) {
	// Can we just.. chill for a moment, please?
	shaderProgramFree(&program);
	linearFree(screen_ind_buf);
	linearFree(screen_vtx_buf);
	linearFree(fb);
	C3D_TexDelete(&screen_fb_tex);
	C3D_RenderTargetDelete(screen_target);
	C3D_Fini();
	gfxExit();
}
