# Makefile for sled.
PROJECT = sled

CC ?= cc
CFLAGS := -O2 -Wall -Werror $(CFLAGS)
CPPFLAGS += -Isrc

# Defaults
PLATFORM ?= RPI
MATRIX_X ?= 16
MATRIX_Y ?= 16
MATRIX_ORDER ?= GRB

DEFINES = -DPLATFORM_$(PLATFORM) -DMATRIX_X=$(MATRIX_X) -DMATRIX_Y=$(MATRIX_Y) -DMATRIX_ORDER_$(MATRIX_ORDER)

OBJECTS = src/matrix.o

all: $(PROJECT)

# Rules
$(PROJECT): $(OBJECTS) src/main.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@	$^

%.o: %.c
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $(LDFLAGS)	$(DEFINES)	$^	-o $@

clean:
	rm -f src/main.o $(OBJECTS)
