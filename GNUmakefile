# SLED makefile, second edition

# The following lists are for reference.

MODULES_AVAILABLE := gfx_random_static gfx_random_rects gfx_twinkle gfx_gol
MODULES_AVAILABLE += gfx_rainbow gfx_math_sinpi gfx_text gfx_plasma gfx_checkerboard
MODULES_AVAILABLE += gfx_balls gfx_clock gfx_sinematrix gfx_error gfx_partirush
MODULES_AVAILABLE += gfx_matrix gfx_cube gfx_mandelbrot gfx_golc gfx_sinefield gfx_affinematrix
MODULES_AVAILABLE += gfx_ip gfx_candyflow gfx_bttrblls gfx_sort1 gfx_xorrid
MODULES_AVAILABLE += gfx_starfield gfx_reddot gfx_sparkburn gfx_sort1D

MODULES_AVAILABLE += bgm_fish bgm_opc bgm_xyscope bgm_pixelflut
MODULES_AVAILABLE += flt_debug flt_gamma_correct flt_flip_x flt_flip_y flt_scale
MODULES_AVAILABLE += flt_rot_90 flt_smapper

OUTMODS_AVAILABLE := out_dummy out_sdl2 out_rpi_ws2812b out_udp out_fb out_rpi_hub75
OUTMODS_AVAILABLE += out_sf75_bi_spidev

# List of modules to compile.
MODULES_DEFAULT := gfx_twinkle gfx_gol gfx_rainbow gfx_math_sinpi gfx_plasma
MODULES_DEFAULT += gfx_balls gfx_clock gfx_sinematrix gfx_error gfx_partirush
MODULES_DEFAULT += gfx_matrix gfx_cube gfx_mandelbrot gfx_golc gfx_sinefield
MODULES_DEFAULT += gfx_affinematrix gfx_ip gfx_candyflow gfx_bttrblls gfx_sort1
MODULES_DEFAULT += gfx_xorrid gfx_starfield gfx_reddot gfx_sparkburn gfx_sort1D

MODULES_DEFAULT += bgm_fish bgm_pixelflut
MODULES_DEFAULT += flt_gamma_correct flt_flip_x flt_flip_y flt_scale flt_rot_90 flt_smapper

# Include local configuration.
ifneq (,$(wildcard sledconf))
include sledconf
else
COPY_SLEDCONF ?= default_sledconf
endif


# Default configuration starts here. This all uses ?=, so SLEDconfig makes the final decision.

# The binary is called 'sled' by default.
PROJECT ?= sled

# By default, debugging is NOT enabled.
DEBUG ?= 0

# By default, the kslink fake-dynamic-module system is NOT enabled.
STATIC ?= 0

# By default, we are not building for CI
CIMODE ?= 0

# By default, we're compiling for a generic Unix.
# Available: 'unix', possibly '3ds'
PLATFORM ?= unix

DEFAULT_OUTMOD ?= sdl2
DEFAULT_MODULEDIR ?= "./modules"

# The list of modules that will be compiled in this build.
# By default, all modules will be compiled, along with the currently selected default output module.

MODULES ?= $(MODULES_DEFAULT) out_$(DEFAULT_OUTMOD)

# Those backends that emulate a matrix should use a matrix of this size.

MATRIX_X ?= 64
MATRIX_Y ?= 64
SDL_SCALE_FACTOR ?= 4

# Basic compiler information, and we add the debug here.
CC ?= cc
ifeq ($(DEBUG),0)
 # Not going to use tabs here, it's a Makefile...
 CFLAGS ?= -O2 -march=native
else
 # Optimizations become non-default if debugging is on, but can still be changed
 CFLAGS ?= -march=native
 CFLAGS += -Og -ggdb
 CPPFLAGS += -DDEBUG
endif
CPPFLAGS += -Wall

# NOTE: This is overridable because a nonposix user might also not be able to rely on -lm.
# In this case, it's their problem as to how to get the maths routines into the system...
LIBS ?= -lm

ifeq ($(STATIC),0)
 OS := $(shell uname)
 ifeq ($(OS),Linux)
  LIBS += -ldl
 endif
 ifeq ($(OS),Darwin)
  CFLAGS += -undefined dynamic_lookup
 endif
 CFLAGS += -fPIC
endif

ifeq ($(CIMODE),1)
 CPPFLAGS += -DCIMODE
endif

LDSOFLAGS ?= -shared

# --- Non-user-configurable source info begins here ---
CFLAGS += -Isrc -DMATRIX_X=$(MATRIX_X) -DMATRIX_Y=$(MATRIX_Y) -DSDL_SCALE_FACTOR=$(SDL_SCALE_FACTOR)
CFLAGS += -DDEFAULT_OUTMOD=\"$(DEFAULT_OUTMOD)\" -DDEFAULT_MODULEDIR=\"$(DEFAULT_MODULEDIR)\"

SOURCES := src/asl.c      src/main.c        src/mod.c     src/modloaders/native.c
SOURCES += src/matrix.c   src/random.c      src/timers.c  src/util.c
SOURCES += src/color.c    src/graphics.c    src/mathey.c
SOURCES += src/taskpool.c src/os/os_$(PLATFORM).c

HEADERS := src/graphics.h src/main.h        src/mod.h     src/modloaders/native.h
HEADERS += src/matrix.h   src/plugin.h      src/timers.h  src/util.h
HEADERS += src/asl.h      src/loadcore.h    src/mathey.h  src/modloader.h
HEADERS += src/random.h   src/types.h       src/oscore.h  src/perf.h
HEADERS += src/taskpool.h

# Module libraries.
# If we're statically linking, we want these to be around at all times.
# If we're dynamically linking, we want the modules to refer to them if needed.

ML_SOURCES := src/modules/text.c
ML_HEADERS := src/modules/text.h src/modules/font.h

ifeq ($(STATIC),0)
 SOURCES += src/dlloadcore.c
else
 SOURCES += src/slloadcore.gen.c
endif

MODULES_SO := $(addprefix modules/, $(addsuffix .so, $(MODULES)))
MODULES_C := $(addprefix src/modules/, $(addsuffix .c, $(MODULES)))

MODULES_WC := $(addprefix static/modwraps/, $(addsuffix .c, $(MODULES)))
MODULES_WCO := $(addprefix static/modwraps/, $(addsuffix .o, $(MODULES)))
MODULES_LIBS := $(addprefix src/modules/, $(addsuffix .libs, $(MODULES)))

PLATFORM_LIBS := src/os/os_$(PLATFORM).libs

OBJECTS := $(SOURCES:.c=.o)
ML_OBJECTS := $(ML_SOURCES:.c=.o)

# --- Include other makefiles ---
include Makefiles/3ds.GNUmakefile

# --- All/Cleaning begins here ---
ifeq ($(STATIC),0)
 all: $(PROJECT) $(MODULES_SO) $(COPY_SLEDCONF)
else
 all: $(PROJECT) $(COPY_SLEDCONF)
endif

clean: FORCE
	rm -f $(PROJECT) $(OBJECTS) modules/*.so src/modules/*.o static/modwraps/*.c static/modwraps/*.o src/slloadcore.gen.c

default_sledconf: FORCE
	[ -e sledconf ] || cp Makefiles/sledconf.default sledconf

FORCE:

# --- Generic object conversion rule begins here ---
%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ `cat $(@:.o=.incs) 2>/dev/null` $^

# --- Module compile info begins here ---
ifeq ($(STATIC),0)
 # To build modules/X.so, link src/modules/X.o with information in an optional .libs file
 modules/%.so: src/modules/%.o $(ML_OBJECTS)
	mkdir -p modules
	$(CC) $(CFLAGS) $(CPPFLAGS) $(LDFLAGS) $(LDSOFLAGS) -o $@ $^ `cat src/modules/$*.libs 2>/dev/null`
else
 # To build all modwraps, run kslink
 $(MODULES_WC) src/slloadcore.gen.c: $(MODULES_C) static/kslink
	cd static ; ./kslink $(addsuffix .c, $(addprefix ../src/modules/, $(MODULES))) > ../src/slloadcore.gen.c
endif

# --- The actual build begins here ---
ifeq ($(STATIC),0)
 sled: $(OBJECTS)
	$(CC) $(CPPFLAGS) $(CFLAGS) -rdynamic $(LDFLAGS) -o $@ $^ `cat $(PLATFORM_LIBS) 2>/dev/null` $(LIBS)
else
 sled: $(OBJECTS) $(MODULES_WCO) $(ML_OBJECTS)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LIBS) `cat $(PLATFORM_LIBS) $(MODULES_LIBS) 2>/dev/null`
endif
