// os_unix_jni
// you're going to hate this
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

#include "os_unix.c"

#include "sh_tty_sled_JniSled.h"

JNIEnv *pJniEnv;
jobject jo_jni_sled;

JNIEXPORT void JNICALL Java_sh_tty_sled_JniSled_main(JNIEnv * env, jobject obj) {
    pJniEnv = env;
    jo_jni_sled = obj;

    char * args[] = {"sled"};
    sled_main(1,args);
}