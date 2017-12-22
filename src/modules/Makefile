# Makefile for sled modules.
MODULES = random_static.so

CC ?= cc
CFLAGS := -std=gnu99 -O2 -Wall -Wno-unused-command-line-argument
CPPFLAGS += -I..

ifeq ($(OS),Linux)
	LIBS += -ldl
endif

DEFINES := -DISMODULE $(DEFINES)

all: $(MODULES)

# Common rules
%.so: %.o
	$(CC) -fPIC -shared $(CFLAGS) $(LDFLAGS) $(LIBS) -o $@	$^

%.o: %.c
	$(CC) -c -fPIC $(CFLAGS) $(CPPFLAGS) $(LDFLAGS)	$(DEFINES)	$^	-o $@
