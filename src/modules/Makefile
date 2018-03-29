# Makefile for sled modules.
CC ?= cc
CFLAGS := -std=gnu99 -O2 -Wall -Wno-unused-command-line-argument
CPPFLAGS += -I..
EXTRA_OBJECTS :=

OS := $(shell uname)
ifeq ($(OS),Linux)
	LIBS += -ldl
endif

DEFINES := -DISMODULE $(DEFINES)

# Common rules
%.so: %.o
	$(CC) -fPIC -shared $(CFLAGS) $(LDFLAGS)	-o $@	$^ $(EXTRA_OBJECTS) $(LIBS)

%.o: %.c
	$(CC) -c -fPIC $(CFLAGS) $(CPPFLAGS) $(LDFLAGS)	$(DEFINES)	-o $@	$^

# Include files go here
%.c: font.h ../main.h ../asl.h ../matrix.h ../modloader.h ../plugin.h ../random.h ../timers.h ../types.h ../graphics.h ../mathey.h ../util.h

# Module specific stuffs
bgm_fish.so: LDFLAGS += -pthread
gfx_math_sinpi.so: LIBS += -lm
gfx_plasma.so: LIBS += -lm
flt_gamma_correct.so: LIBS += -lm
gfx_text.so: text.o
gfx_clock.so: text.o
gfx_error.so: text.o
gfx_sinematrix.so: LIBS += -lm

out_sdl2.so: LIBS += -lSDL2

RPI_WS2812B_PATH ?= ../../../rpi_ws281x
out_rpi_ws2812b.so: EXTRA_OBJECTS = $(RPI_WS2812B_PATH)/libws2811.a
out_rpi_ws2812b.so: CPPFLAGS += -I$(RPI_WS2812B_PATH)/

RGB_LIB_PATH ?= ../../../rpi-rgb-led-matrix
out_rpi_hub75.so: EXTRA_OBJECTS = $(RGB_LIB_PATH)/lib/librgbmatrix.a
out_rpi_hub75.so: CPPFLAGS += -I$(RGB_LIB_PATH)/include
out_rpi_hub75.so: LIBS += -lrt -lm -lpthread -lstdc++
