#include <types.h>
#include <oscore.h>
#include <timers.h>
#include <assert.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdatomic.h>
#include <Processing.NDI.Lib.h>  // NDI SDK headers

// Matrix size
#ifndef MATRIX_X
#error Define MATRIX_X as the matrix's X size.
#endif

#ifndef MATRIX_Y
#error Define MATRIX_Y as the matrix's Y size.
#endif

#define FPS 60

static RGB *primary_buffer;
static RGB *front_buffer;
static RGB *back_buffer;
static atomic_bool keep_running;
static atomic_int front_or_back;

static NDIlib_send_instance_t ndi_send = NULL;
static oscore_task ndi_task;

static void* send_task(void *arg);
	
int init(void) {
	assert(sizeof(RGB) == 4);

	// Allocate memory for the buffers
	primary_buffer = (RGB*)malloc(MATRIX_X * MATRIX_Y * sizeof(RGB));
	front_buffer = (RGB*)malloc(MATRIX_X * MATRIX_Y * sizeof(RGB));
	back_buffer = (RGB*)malloc(MATRIX_X * MATRIX_Y * sizeof(RGB));

	atomic_init(&keep_running, true);
	atomic_init(&front_or_back, true);

	// Start the NDI sending task
	ndi_task = oscore_task_create("NDI Sender", send_task, NULL);
	return 0;
}

int getx(int _modno) {
	return MATRIX_X;
}

int gety(int _modno) {
	return MATRIX_Y;
}

int set(int _modno, int x, int y, RGB color) {
	primary_buffer[(y * MATRIX_X + x)] = color;
	return 0;
}

RGB get(int _modno, int x, int y) {
	return primary_buffer[(y * MATRIX_X + x)];
}

int clear(int _modno) {
	// Clear the primary buffer
	memset(primary_buffer, 0, MATRIX_X * MATRIX_Y * sizeof(RGB));
	return 0;
}

int render(void) {
	// Get current buffer
	int current_fob = atomic_load(&front_or_back);
	RGB *current_buffer = current_fob ? front_buffer : back_buffer;

	// Copy primary buffer to current buffer
	memcpy(current_buffer, primary_buffer, MATRIX_X * MATRIX_Y * sizeof(RGB));

	// Flip buffers
	atomic_store(&front_or_back, !current_fob);

	return 0;
}

oscore_time wait_until(int _modno, oscore_time desired_usec) {
#ifdef CIMODE
	return desired_usec;
#else
	return timers_wait_until_core(desired_usec);
#endif
}

void wait_until_break(int _modno) {
#ifndef CIMODE
	timers_wait_until_break_core();
#endif
}

void deinit(int _modno) {
	// Stop the NDI Sender task
	atomic_store(&keep_running, false);
	oscore_task_join(&ndi_task);

	// Clean up resources
	NDIlib_send_destroy(ndi_send);
	NDIlib_destroy();

	free(primary_buffer);
	free(front_buffer);
	free(back_buffer);
}

// NDI sending function (runs in a separate task)
static void* send_task(void *arg) {
	// Create the NDI sender
	NDIlib_initialize();
	NDIlib_send_create_t send_create_desc = {0};
	send_create_desc.p_ndi_name = "SLED";
	send_create_desc.clock_video = false;
	send_create_desc.clock_audio = false;
	ndi_send = NDIlib_send_create(&send_create_desc);

	// Prepopulate frame with static info.
	NDIlib_video_frame_v2_t video_frame;
	video_frame.xres = MATRIX_X;
	video_frame.yres = MATRIX_Y;
	video_frame.line_stride_in_bytes = MATRIX_X * sizeof(RGB);
	video_frame.picture_aspect_ratio = (float)MATRIX_X / (float)MATRIX_Y;
	video_frame.FourCC = NDIlib_FourCC_type_RGBX;
	video_frame.frame_format_type = NDIlib_frame_format_type_progressive;
	video_frame.frame_rate_N = FPS * 1000;
	video_frame.frame_rate_D = 1000;
	
	while (atomic_load(&keep_running)) {
		// Get current buffer
		int current_fob = atomic_load(&front_or_back);

		// Send a video frame with the actual data.
		video_frame.p_data = (uint8_t*) (current_fob ? front_buffer : back_buffer);
		NDIlib_send_send_video_async_v2(ndi_send, &video_frame);

		// Wait for the next frame
		usleep(1000000 / FPS);
	}

	// Force sync for last frame.
	NDIlib_send_send_video_async_v2(ndi_send, NULL);
	return NULL;
}

