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
	$(CC) -fPIC -shared $(CFLAGS) $(LDFLAGS) $(LIBS) -o $@	$^

%.o: %.c
	$(CC) -c -fPIC $(CFLAGS) $(CPPFLAGS) $(LDFLAGS)	$(DEFINES)	$^	-o $@ $(EXTRA_OBJECTS)

# Include files go here
%.c: font.h ../main.h ../matrix.h ../modloader.h ../plugin.h ../random.h ../timers.h ../types.h

# Module specific stuffs
bgm_fish.so: LDFLAGS += -pthread
out_sdl2.so: LDFLAGS += -lSDL2
out_rpi_ws2812b.so: EXTRA_OBJECTS = ../rpi_ws281x/libws2811.a
out_rpi_ws2812b.so: CFLAGS += -I../rpi_ws281x/libws2811.a
