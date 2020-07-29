// output module for the nSpire CX with ndless
// does not support the flipped LCD on Rev W and later yet

// -- std
#include <stdlib.h>
#include <unistd.h>
// -- sled
#include <colors.h>
#include <timers.h>
#include <types.h>
// -- ndls
#include <libndls.h>

#define WORLD_X 320
#define WORLD_Y 240

static byte * frameBuffer;

int init(void) {
    frameBuffer = malloc(WORLD_X * WORLD_Y * sizeof(uint16_t));
    if (!frameBuffer) {
        printf("out_nspire ran out of memory allocating buffer\n");
        free(frameBuffer);
        return 1;
    }
    return 0;
}

int getx(int _modno) {
    return WORLD_X;
}
int gety(int _modno) {
    return WORLD_Y;
}

int set(int _modno, int x, int y, RGB color) {
    // no bounds check for performance, yolo

    uint16_t rgb565 = RGB2RGB565(color);

    byte high, low;
    high = rgb565 >> 8;
    low = rgb565 & 0xFF;
    frameBuffer[((x + (y * WORLD_X)) * sizeof(uint16_t)) + 0] = high;
    frameBuffer[((x + (y * WORLD_X)) * sizeof(uint16_t)) + 1] = low;

    return 0;
}

RGB get(int _modno, int x, int y) {
    // no bounds check, yolo

    byte high, low;
    high = frameBuffer[((x + (y * WORLD_X)) * sizeof(uint16_t)) + 0];
    low = frameBuffer[((x + (y * WORLD_X)) * sizeof(uint16_t)) + 1];

    uint16_t rgb565 = (high << 8) | (low & 0xFF);

    RGB color = RGB5652RGB(rgb565);

    return color;
}

int clear(int _modno) {
    memset(frameBuffer, 0, WORLD_X * WORLD_Y * sizeof(uint16_t));
    return 0;
};

int render(void) {
    lcd_blit(frameBuffer, SCR_320x240_565);

    if (any_key_pressed()) {
        // check for {"esc", "menu", "home"} keys to exit
        if (isKeyPressed(KEY_NSPIRE_ESC) ||
            isKeyPressed(KEY_NSPIRE_MENU) ||
            isKeyPressed(KEY_NSPIRE_HOME))
            timers_doquit();
    }

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
    free(frameBuffer);
}
