// Dummy output.
//
// Copyright (c) 2021, fridtjof <fridtjof@das-labor.org>
//
// Permission to use, copy, modify, and/or distribute this software for any
// purpose with or without fee is hereby granted, provided that the above
// copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
// WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
// ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
// WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
// ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
// OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

#include "../os/sh_tty_sled_JniSled.h"

#include <types.h>
#include <timers.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include "stdio.h"

#define PPOS(x, y) (x + (y * matx))

static size_t JNI_BUFFER_SIZE;
static jintArray buffers[2];
static jint* working_buffer;
static jint* currentBufferPtr;
static int currentBuffer = -1;
#define NEXTBUFFER ((currentBuffer + 1) % 2)

extern JNIEnv *pJniEnv;
#define JniEnv (*pJniEnv)
extern jobject jo_jni_sled;

static jclass jc_jni_sled;
static jmethodID jm_setCurrentBuffer;
static jfieldID jf_matx;
static jfieldID jf_maty;

static int matx;
static int maty;

void print_any_exceptions() {
    if (JniEnv->ExceptionCheck(pJniEnv)) {
        // this doesn't actually output anything??
        JniEnv->ExceptionDescribe(pJniEnv);
        JniEnv->ExceptionClear(pJniEnv);
    }
}

void swapBuffers() {
    currentBuffer = NEXTBUFFER;
    JniEnv->CallVoidMethodA(pJniEnv, jo_jni_sled, jm_setCurrentBuffer, (jvalue *) &buffers[currentBuffer]);

    print_any_exceptions();
}

inline unsigned int rgb2uint(RGB color) {
    return (color.blue << 0) | (color.green << 8) | (color.red << 16) | (color.alpha << 24);
}

inline RGB uint2rgb(unsigned int color) {
    RGB col;
    col.blue  = (color <<  0) & 0xFF;
    col.green = (color <<  8) & 0xFF;
    col.red   = (color << 16) & 0xFF;
    col.alpha = (color << 24) & 0xFF;

    return col;
}

int init(void) {
    jc_jni_sled = JniEnv->GetObjectClass(pJniEnv, jo_jni_sled);

    // determine sizes
    jf_matx = JniEnv->GetFieldID(pJniEnv, jc_jni_sled, "matrixX", "I");
    jf_maty = JniEnv->GetFieldID(pJniEnv, jc_jni_sled, "matrixY", "I");
    matx = JniEnv->GetIntField(pJniEnv, jo_jni_sled, jf_matx);
    maty = JniEnv->GetIntField(pJniEnv, jo_jni_sled, jf_maty);
    JNI_BUFFER_SIZE = matx * maty * sizeof(jint);

    // allocate output buffers
    working_buffer = malloc(JNI_BUFFER_SIZE);
    buffers[0] = JniEnv->NewIntArray(pJniEnv, matx * maty);
    buffers[1] = JniEnv->NewIntArray(pJniEnv, matx * maty);

    // get ready for rendering
    jm_setCurrentBuffer = JniEnv->GetMethodID(pJniEnv, jc_jni_sled, "setCurrentBuffer", "([I)V");
    swapBuffers();

    return 0;
}

int getx(int _modno) {
    return matx;
}
int gety(int _modno) {
    return maty;
}

inline void bounds_check(int x, int y) {
    assert(x >= 0);
    assert(y >= 0);
    assert(x < matx);
    assert(y < maty);
}

int set(int _modno, int x, int y, RGB color) {
    bounds_check(x, y);
    working_buffer[PPOS(x, y)] = rgb2uint(color);
    return 0;
}

RGB get(int _modno, int x, int y) {
    bounds_check(x, y);
    return uint2rgb((unsigned int) working_buffer[PPOS(x, y)]);
}

int clear(int _modno) {
    memset(working_buffer, 0, JNI_BUFFER_SIZE);
    return 0;
}

int render(void) {
    // prepare for working on the next frame
    currentBufferPtr = JniEnv->GetIntArrayElements(pJniEnv, buffers[NEXTBUFFER], 0 /*ptr bool*/);
    memcpy(currentBufferPtr, working_buffer, JNI_BUFFER_SIZE);
    JniEnv->ReleaseIntArrayElements(pJniEnv, buffers[NEXTBUFFER], currentBufferPtr, 0);
    swapBuffers();
    // notify? prob not needed i think?
    return 0;
}

oscore_time wait_until(int _modno, oscore_time desired_usec) {
    // Hey, we can just delegate work to someone else. Yay!
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
    // TODO actually do stuff here. Sled and/or jvm won't exit properly.
    //  maybe this needs some more fiddling in the os_ module too?
}
