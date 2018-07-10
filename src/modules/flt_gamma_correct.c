// Gamma correction filter.
// Corrects colors according to a generated LUT.
// Not the best, but it's better than nothing, I suppose.

#include <types.h>
#include <mod.h>
#include <math.h>

static module* next;

#define GAMMA 2.8f
#define WHITEPOINT {0.98f, 1.0f, 1.0f} // R, G, B, respectively.

#define MAX_VAL 255
static byte LUT_R[MAX_VAL + 1];
static byte LUT_G[MAX_VAL + 1];
static byte LUT_B[MAX_VAL + 1];

#define CORRECTION ((powf((float)i / MAX_VAL, GAMMA) * MAX_VAL) + 0.5f)

int init(int nextno, char* argstr) {
	// get next ptr.
	next = modules_get(nextno);
	float whitepoint[3] = WHITEPOINT;

	int i;
	for (i = 0; i <= 255; ++i)
		LUT_R[i] = whitepoint[0] * CORRECTION;
	for (i = 0; i <= 255; ++i)
		LUT_G[i] = whitepoint[1] * CORRECTION;
	for (i = 0; i <= 255; ++i)
		LUT_B[i] = whitepoint[2] * CORRECTION;
	return 0;
}

int getx(void) {
	return next->getx();
}
int gety(void) {
	return next->gety();
}

int set(int x, int y, RGB color) {
	RGB corrected = RGB(LUT_R[color.red], LUT_G[color.green], LUT_B[color.blue]);
	return next->set(x, y, corrected);
}

// TODO: reverse LUT to get back semi-original values
// if we pass the corrected values to the set function,
// it doesn't have the same color it had before.
// every time that happens, it'll get visibly darker.
RGB get(int x, int y) {
	return next->get(x, y);
}

int clear(void) {
	return next->clear();
}

int render(void) {
	return next->render();
}

ulong wait_until(ulong desired_usec) {
	return next->wait_until(desired_usec);
}

void wait_until_break(void) {
	if (next && next->wait_until_break)
		return next->wait_until_break();
}

int deinit(void) {
	return next->deinit();
}
