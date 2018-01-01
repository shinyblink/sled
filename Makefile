# Makefile for sled.
PROJECT = sled
MODULES_AVAILABLE = gfx_random_static gfx_random_rects gfx_twinkle gfx_gol gfx_rainbow gfx_math_sinpi gfx_text bgm_fish out_dummy out_sdl2 out_rpi_ws2812b
MODULES = gfx_random_static gfx_random_rects gfx_twinkle gfx_gol gfx_rainbow gfx_math_sinpi gfx_text bgm_fish

CC ?= cc
CFLAGS := -std=gnu99 -O2 -Wall -Wno-unused-command-line-argument $(CFLAGS)
CPPFLAGS += -Isrc
LIBS = -lm

OS := $(shell uname)
ifeq ($(OS),Linux)
	LIBS += -ldl
endif

# Defaults
PLATFORM ?= SDL2
MATRIX_X ?= 16
MATRIX_Y ?= 8
MATRIX_ORDER ?= SNAKE

DEFINES = -DPLATFORM_$(PLATFORM) -DMATRIX_X=$(MATRIX_X) -DMATRIX_Y=$(MATRIX_Y) -DMATRIX_ORDER_$(MATRIX_ORDER)

OBJECTS = src/modloader.o src/matrix.o src/timers.o src/random.o src/mathey.o src/graphics.o

all: DEBUG modules

# Target specific rules
SDL2: PLATFORM = SDL2
SDL2: LIBS += -lSDL2
SDL2: $(PROJECT) out_sdl2

DEBUG: CFLAGS += -Og -ggdb
DEBUG: SDL2

RPI: PLATFORM = RPI
RPI: $(PROJECT) out_rpi_ws2812b

# Common rules
$(PROJECT): $(OBJECTS) src/main.o
	$(CC) $(CFLAGS) -rdynamic $(LDFLAGS) $(LIBS) -o $@	$^ $(EXTRA_OBJECTS)

src/%.o: src/%.c
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $(LDFLAGS)	$(DEFINES)	$^	-o $@

# Module rules.
modules: $(MODULES)

$(MODULES_AVAILABLE): src/modules
	$(MAKE) -C src/modules $@.so DEFINES="$(DEFINES)" CC="$(CC)" CFLAGS="$(CFLAGS)"
	mkdir -p modules
	cp src/modules/$@.so modules/

$(OUTMOD): src/modules/out_$(OUTMOD).c
	$(MAKE) -C src/modules out_$@.so DEFINES="$(DEFINES)" CC="$(CC)" CFLAGS="$(CFLAGS)"
	mkdir -p modules

clean:
	rm -f $(PROJECT) src/main.o $(OBJECTS) modules/* src/modules/*.o src/modules/*.so
