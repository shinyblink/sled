# Rules for the 3ds, out_ctru and such.
ifeq ($(PLATFORM),emscripten)

CC := emcc

## ASYNCIFY is needed to allow for yielding in wasm,
## or else we would block the entire browser thread while running
CFLAGS += -s ASYNCIFY=1
## to minimize performance losses by ASYNCIFY, optimize a lot
CFLAGS += -flto -O3

STATIC := 1

DEFAULT_OUTMOD := emscripten_canvas2d

## emcc determines what to output based on the file extension.
## In this case, we will get a sled.wasm (the compiled module) and a sled.js (runtime code to handle the module easily)
PROJECT := $(EM_PROJECT).js

endif
