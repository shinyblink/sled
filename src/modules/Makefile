# Makefile for sled modules.
MODULES = test.so

CC ?= cc
CFLAGS := -std=gnu99 -O2 -Wall -Wno-unused-command-line-argument
CPPFLAGS += -I..

ifeq ($(OS),Linux)
	LIBS += -ldl
endif

DEFINES := -DISMODULE $(DEFINES)

all: $(MPATHS)

# Common rules
%.so: %.o
	$(CC) -fPIC -shared $(CFLAGS) $(LDFLAGS) $(LIBS) -o $@	$^

%.o: %.c
	$(CC) -c -fPIC $(CFLAGS) $(CPPFLAGS) $(LDFLAGS)	$(DEFINES)	$^	-o $@
