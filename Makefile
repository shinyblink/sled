# Makefile for sled.
PROJECT = sled

CC ?= cc
CFLAGS := -std=gnu99 -O2 -Wall $(CFLAGS)
CPPFLAGS += -Isrc
LIBS =

ifeq ($(OS),Linux)
	LIBS += -ldl
endif

# Defaults
PLATFORM ?= DEBUG
MATRIX_X ?= 16
MATRIX_Y ?= 16
MATRIX_ORDER ?= GRB

DEFINES = -DPLATFORM_$(PLATFORM) -DMATRIX_X=$(MATRIX_X) -DMATRIX_Y=$(MATRIX_Y) -DMATRIX_ORDER_$(MATRIX_ORDER)

OBJECTS = src/matrix.o

all: DEBUG

# Target specific rules

DEBUG: PLATFORM = DEBUG
DEBUG: LIBS += -lSDL2
DEBUG: CFLAGS += -Og -ggdb
DEBUG: $(PROJECT)

RPI: PLATFORM = RPI
RPI: LIBS += -lws281x
RPI: $(PROJECT)

# Common rules
$(PROJECT): $(OBJECTS) src/main.o
	$(CC) $(CFLAGS) $(LDFLAGS) $(LIBS) -o $@	$^

%.o: %.c
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $(LDFLAGS)	$(DEFINES)	$^	-o $@

clean:
	rm -f src/main.o $(OBJECTS)
