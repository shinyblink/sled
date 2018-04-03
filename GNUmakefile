# Makefile for sled.
PROJECT = sled

MODULES_AVAILABLE := gfx_random_static gfx_random_rects gfx_twinkle gfx_gol
MODULES_AVAILABLE += gfx_rainbow gfx_math_sinpi gfx_text gfx_plasma gfx_checkerboard
MODULES_AVAILABLE += gfx_balls gfx_clock gfx_sinematrix gfx_error gfx_partirush
MODULES_AVAILABLE += gfx_matrix gfx_cube

MODULES_AVAILABLE += out_dummy out_sdl2 out_rpi_ws2812b out_udp out_fb out_rpi_hub75
MODULES_AVAILABLE += bgm_fish bgm_opc
MODULES_AVAILABLE += flt_debug flt_gamma_correct flt_flip_x flt_flip_y

MODULES := gfx_random_static gfx_random_rects gfx_twinkle gfx_gol
MODULES += gfx_rainbow gfx_math_sinpi gfx_plasma gfx_checkerboard
MODULES += gfx_balls gfx_clock gfx_sinematrix gfx_error gfx_partirush
MODULES += gfx_matrix gfx_cube

MODULES += bgm_fish bgm_opc
MODULES += flt_gamma_correct flt_flip_x flt_flip_y

CC ?= cc
CFLAGS ?= -O2 -march=native
CFLAGS += -std=gnu99 -Wall -Wno-unused-command-line-argument
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
# For those who want to emulate layouts/wall.json from OPC, use 25x50 here.
MATRIX_X ?= 64
MATRIX_Y ?= 64
MATRIX_ORDER ?= SNAKE

DEFINES = -DPLATFORM_$(PLATFORM) -DMATRIX_X=$(MATRIX_X) -DMATRIX_Y=$(MATRIX_Y) -DMATRIX_ORDER_$(MATRIX_ORDER)

SOURCES := src/asl.c src/modloader.c src/color.c src/matrix.c src/timers.c src/random.c src/mathey.c src/graphics.c src/util.c
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
RPI_WS2812B: PLATFORM := RPI_WS2812B
RPI_WS2812B: DEFINES += -DRPI_DMA=$(RPI_DMA) -DRPI_PIN=$(RPI_PIN)
RPI_WS2812B: $(PROJECT) out_rpi_ws2812b

RPI_HUB75: PLATFORM := RPI_HUB75
RPI_HUB75: $(PROJECT) out_rpi_hub75

UDP: $(PROJECT) out_udp

FB: $(PROJECT) out_fb

# Common rules
$(PROJECT): $(OBJECTS) src/main.o
	$(CC) $(CFLAGS) -rdynamic $(LDFLAGS)	-o $@	$^ $(EXTRA_OBJECTS) $(LIBS)

src/%.o: src/%.c
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $(LDFLAGS)	$(DEFINES)	-o $@	$^

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
