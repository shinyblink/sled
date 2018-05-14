# Makefile for sled (statically linked)
PROJECT = sled

MODULES := gfx_twinkle gfx_gol gfx_golc out_sdl2

# C compiler

CC ?= cc
CFLAGS ?= -O2 -march=native
CFLAGS += -std=gnu99 -Wall -Wno-unused-command-line-argument
CPPFLAGS += -I../src
LIBS = -lm -lSDL2

MATRIX_X ?= 64
MATRIX_Y ?= 64
SDL_SCALE_FACTOR ?= 8

DEFINES := -DPLATFORM_SDL -DMATRIX_X=$(MATRIX_X) -DMATRIX_Y=$(MATRIX_Y) -DSDL_SCALE_FACTOR=$(SDL_SCALE_FACTOR)

EMODULES := $(addsuffix .c,$(addprefix ../src/modules/,$(MODULES)))
XMODULES := $(addsuffix .c,$(addprefix modwraps/,$(MODULES)))

SOURCES := ../src/asl.c ../src/modloader.c ../src/color.c ../src/matrix.c
SOURCES += ../src/timers.c ../src/random.c ../src/mathey.c ../src/graphics.c ../src/util.c
SOURCES += ../src/slloadcore.gen.c $(XMODULES)
SOURCES += ../src/modules/text.c


OBJECTS := $(SOURCES:.c=.o)
HEADERS := ../src/asl.h ../src/graphics.h ../src/loadcore.h ../src/main.h ../src/mathey.h
HEADERS += ../src/matrix.h ../src/modloader.h ../src/plugin.h ../src/random.h ../src/timers.h ../src/types.h ../src/util.h
HEADERS += ../src/modules/font.h ../src/modules/text.h

all: $(PROJECT)

debug: all
debug: CFLAGS += -Og -ggdb

# Common rules

$(PROJECT): $(OBJECTS) ../src/main.o
	$(CC) $(CFLAGS) -rdynamic $(LDFLAGS)	-o $@	$^ $(EXTRA_OBJECTS) $(LIBS)

../src/slloadcore.gen.c $(XMODULES): kslink $(EMODULES)
	./kslink $(EMODULES) > ../src/slloadcore.gen.c

%.o: %.c
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $(LDFLAGS)	$(DEFINES)	-o $@	$^

# Cleanup
clean:
	rm -f $(PROJECT) ../src/main.o $(OBJECTS) ../modules/* ../src/modules/*.o ../src/modules/*.so modwraps/*.c modwraps/*.o
