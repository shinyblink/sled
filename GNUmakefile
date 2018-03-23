# Makefile for sled.
PROJECT = sled
MODULES_AVAILABLE := gfx_random_static gfx_random_rects gfx_twinkle gfx_gol gfx_rainbow gfx_math_sinpi gfx_text bgm_fish gfx_plasma gfx_checkerboard gfx_balls gfx_clock
MODULES_AVAILABLE += out_dummy out_sdl2 out_rpi_ws2812b out_udp out_fb
MODULES_AVAILABLE += flt_debug flt_gamma_correct flt_flip_x flt_flip_y
MODULES := gfx_random_static gfx_random_rects gfx_twinkle gfx_gol gfx_rainbow gfx_math_sinpi gfx_text bgm_fish gfx_plasma gfx_checkerboard gfx_balls gfx_clock
MODULES += flt_debug flt_gamma_correct

CC ?= cc
CFLAGS := -std=gnu99 -O2 -Wall -Wno-unused-command-line-argument $(CFLAGS)
CPPFLAGS += -Isrc
LIBS = -lm

OS := $(shell uname)
ifeq ($(OS),Linux)
	LIBS += -ldl
endif
ifeq ($(OS),Darwin)
	CFLAGS += -undefined dynamic_lookup
endif

# Defaults
PLATFORM ?= DEBUG
MATRIX_X ?= 16
MATRIX_Y ?= 8
MATRIX_ORDER ?= SNAKE

DEFINES = -DPLATFORM_$(PLATFORM) -DMATRIX_X=$(MATRIX_X) -DMATRIX_Y=$(MATRIX_Y) -DMATRIX_ORDER_$(MATRIX_ORDER)

SOURCES := src/asl.c src/modloader.c src/matrix.c src/timers.c src/random.c src/mathey.c src/graphics.c src/util.c
OBJECTS := $(SOURCES:.c=.o)
HEADERS := $(SOURCES:.c=.h)

all: $(PLATFORM) modules

# Target specific rules
SDL_SCALE_FACTOR ?= 8
SDL2: PLATFORM := SDL2
SDL2: DEFINES += -DSDL_SCALE_FACTOR=$(SDL_SCALE_FACTOR)
SDL2: $(PROJECT) out_sdl2

DEBUG: CFLAGS += -Og -ggdb
DEBUG: SDL2

RPI_DMA ?= 10
RPI_PIN ?= 21
RPI: PLATFORM := RPI
RPI: DEFINES += -DRPI_DMA=$(RPI_DMA) -DRPI_PIN=$(RPI_PIN)
RPI: $(PROJECT) out_rpi_ws2812b

UDP: $(PROJECT) out_udp

FB: $(PROJECT) out_fb

# Common rules
$(PROJECT): $(OBJECTS) src/main.o
	$(CC) $(CFLAGS) -rdynamic $(LDFLAGS) $(LIBS) -o $@	$^ $(EXTRA_OBJECTS)

src/%.o: src/%.c
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $(LDFLAGS)	$(DEFINES)	$^	-o $@

# Generate tags
tags: $(SOURCES) $(HEADERS)
	exctags $^

# Module rules.
modules: $(MODULES)

$(MODULES_AVAILABLE): src/modules
	$(MAKE) -C src/modules $@.so DEFINES="$(DEFINES)" CC="$(CC)" CFLAGS="$(CFLAGS)"
	mkdir -p modules
	cp src/modules/$@.so modules/

$(OUTMOD): src/modules/out_$(OUTMOD).c
	$(MAKE) -C src/modules out_$@.so DEFINES="$(DEFINES)" CC="$(CC)" CFLAGS="$(CFLAGS)"
	mkdir -p modules

# Cleanup
clean:
	rm -f $(PROJECT) src/main.o $(OBJECTS) modules/* src/modules/*.o src/modules/*.so