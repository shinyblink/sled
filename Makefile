# Makefile for sled.
PROJECT = sled
MODULES = random_static random_rects twinkle text

CC ?= cc
CFLAGS := -std=gnu99 -O2 -Wall -Wno-unused-command-line-argument $(CFLAGS)
CPPFLAGS += -Isrc
LIBS =

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

OBJECTS = src/modloader.o src/matrix.o src/timers.o src/random.o

# Target specific settings

# RPI
RPI_WS281X_PATH ?= ../rpi_ws281x

all: DEBUG $(MODULES)

# Target specific rules

DEBUG: PLATFORM = SDL2
DEBUG: LIBS += -lSDL2
DEBUG: CFLAGS += -Og -ggdb
DEBUG: $(PROJECT)

RPI: PLATFORM = RPI
RPI: LIBS += $(RPI_WS281X_PATH)/libws2811.a
RPI: CFLAGS += -I$(RPI_WS281X_PATH)
RPI: $(PROJECT)

# Common rules
$(PROJECT): $(OBJECTS) src/main.o
	$(CC) $(CFLAGS) -rdynamic $(LDFLAGS) $(LIBS) -o $@	$^

src/%.o: src/%.c
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $(LDFLAGS)	$(DEFINES)	$^	-o $@

# Module rules.
$(MODULES): src/modules
	$(MAKE) -C src/modules $@.so DEFINES="$(DEFINES)" CC="$(CC)" CFLAGS="$(CFLAGS)"
	mkdir -p modules
	cp src/modules/$@.so modules/

clean:
	rm -f src/main.o $(OBJECTS) modules/* src/modules/*.o src/modules/*.so
