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
	$(CC) -fPIC -shared $(CFLAGS) $(LDFLAGS) $(LIBS) -o $@	$^ $(EXTRA_OBJECTS)

%.o: %.c
	$(CC) -c -fPIC $(CFLAGS) $(CPPFLAGS) $(LDFLAGS)	$(DEFINES)	$^	-o $@

# Include files go here
%.c: font.h ../main.h ../matrix.h ../modloader.h ../plugin.h ../random.h ../timers.h ../types.h

# Module specific stuffs
bgm_fish.so: LDFLAGS += -pthread
gfx_math_sinpi.so: LDFLAGS += -lm
gfx_plasma.so: LDFLAGS += -lm
flt_gamma_correct.so: LDFLAGS += -lm

out_sdl2.so: LDFLAGS += -lSDL2

RPI_WS2812B_PATH ?= ../../../rpi_ws281x
out_rpi_ws2812b.so: EXTRA_OBJECTS = $(RPI_WS2812B_PATH)/libws2811.a
out_rpi_ws2812b.so: CPPFLAGS += -I$(RPI_WS2812B_PATH)/
