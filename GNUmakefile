# SLED makefile, second edition

# The following lists are for reference.

GFXMODS_AVAILABLE := gfx_random_static gfx_random_rects gfx_twinkle gfx_gol
GFXMODS_AVAILABLE += gfx_rainbow gfx_math_sinpi gfx_text gfx_plasma
GFXMODS_AVAILABLE += gfx_checkerboard gfx_balls gfx_clock gfx_sinematrix
GFXMODS_AVAILABLE += gfx_error gfx_partirush gfx_matrix gfx_cube gfx_mandelbrot
GFXMODS_AVAILABLE += gfx_golc gfx_sinefield gfx_affinematrix gfx_ip
GFXMODS_AVAILABLE += gfx_candyflow gfx_bttrblls gfx_sort2D gfx_xorrid
GFXMODS_AVAILABLE += gfx_starfield gfx_reddot gfx_sparkburn gfx_sort1D
GFXMODS_AVAILABLE += gfx_rgbmatrix gfx_mandelbrot2 gfx_disturbedcandy
GFXMODS_AVAILABLE += gfx_ghostery gfx_ursuppe gfx_afterglow gfx_fire
GFXMODS_AVAILABLE += gfx_no13 gfx_candyswarm gfx_ursuppe2 gfx_rule90
GFXMODS_AVAILABLE += gfx_maze gfx_invfourier gfx_colorwheel

BGMMODS_AVAILABLE += bgm_fish bgm_opc bgm_xyscope bgm_pixelflut

FLTMODS_AVAILABLE += flt_debug flt_gamma_correct flt_flip_x flt_flip_y flt_scale
FLTMODS_AVAILABLE += flt_rot_90 flt_smapper flt_channel_reorder

OUTMODS_AVAILABLE := out_dummy out_sdl2 out_rpi_ws2812b out_udp out_fb out_rpi_hub75
OUTMODS_AVAILABLE += out_sf75_bi_spidev out_ansi

# List of modules to compile.
GFXMODS_DEFAULT := gfx_twinkle gfx_gol gfx_rainbow gfx_math_sinpi gfx_plasma
GFXMODS_DEFAULT += gfx_balls gfx_clock gfx_sinematrix gfx_error gfx_partirush
GFXMODS_DEFAULT += gfx_matrix gfx_cube gfx_mandelbrot gfx_golc gfx_sinefield
GFXMODS_DEFAULT += gfx_affinematrix gfx_ip gfx_candyflow gfx_bttrblls
GFXMODS_DEFAULT += gfx_sort2D gfx_xorrid gfx_starfield gfx_reddot gfx_sparkburn
GFXMODS_DEFAULT += gfx_sort1D gfx_rgbmatrix gfx_mandelbrot2 gfx_disturbedcandy
GFXMODS_DEFAULT += gfx_ghostery gfx_ursuppe gfx_afterglow gfx_fire
GFXMODS_DEFAULT += gfx_candyswarm gfx_ursuppe2 gfx_invfourier gfx_colorwheel

BGMMODS_DEFAULT += bgm_fish bgm_pixelflut
FLTMODS_DEFAULT += flt_gamma_correct flt_flip_x flt_flip_y flt_scale flt_rot_90
FLTMODS_DEFAULT += flt_smapper flt_channel_reorder

MODULES_DEFAULT += $(BGMMODS_DEFAULT) $(FLTMODS_DEFAULT) $(GFXMODS_DEFAULT) mod_farbherd

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

# By default, we don't use k2link for everything (but it's needed for bootstrap)
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
# Explicitly dynamic modules
MODULES_DYNAMIC ?= 
# Explicitly static modules
MODULES_STATIC ?= 

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

SOURCES := src/asl.c      src/main.c        src/mod.c
SOURCES += src/matrix.c   src/random.c      src/timers.c  src/util.c
SOURCES += src/color.c    src/graphics.c    src/mathey.c
SOURCES += src/taskpool.c src/os/os_$(PLATFORM).c         src/modloader.c

HEADERS := src/graphics.h src/main.h        src/mod.h
HEADERS += src/matrix.h   src/plugin.h      src/timers.h  src/util.h
HEADERS += src/asl.h      src/mathey.h      src/modloader.h
HEADERS += src/random.h   src/types.h       src/oscore.h  src/perf.h
HEADERS += src/taskpool.h src/ext/farbherd.h

# Module libraries.
# If we're statically linking, we want these to be around at all times.
# NOTE FROM THE FUTURE: Or do we???
# If we're dynamically linking, we want the modules to refer to them if needed.

ML_SOURCES := src/modules/text.c
ML_HEADERS := src/modules/text.h src/modules/font.h

ifeq ($(STATIC),0)
 # User's selected module set gets compiled dynamically (including outmod),
 #  while static modules is just the minimum to bootstrap dynamic loading
 MODULES_DYNAMIC += $(MODULES)
 MODULES_STATIC += mod_dl
else
 # We'd like to be static, so send user's selected modules to the static linker.
 MODULES_STATIC += $(MODULES)
endif

MODULES_DYNAMIC_SO := $(addprefix modules/, $(addsuffix .so, $(MODULES_DYNAMIC)))
MODULES_DYNAMIC_C := $(addprefix src/modules/, $(addsuffix .c, $(MODULES_DYNAMIC)))
MODULES_DYNAMIC_LIBS := $(addprefix src/modules/, $(addsuffix .libs, $(MODULES_DYNAMIC)))

MODULES_STATIC_O := $(addprefix static/modwraps/, $(addsuffix .o, $(MODULES_STATIC)))
MODULES_STATIC_CW := $(addprefix static/modwraps/, $(addsuffix .c, $(MODULES_STATIC)))
MODULES_STATIC_CWL := $(addprefix static/modwraps/, $(addsuffix .incs, $(MODULES_STATIC)))
MODULES_STATIC_C := $(addprefix src/modules/, $(addsuffix .c, $(MODULES_STATIC)))
MODULES_STATIC_LIBS := $(addprefix src/modules/, $(addsuffix .libs, $(MODULES_STATIC)))

PLATFORM_LIBS := src/os/os_$(PLATFORM).libs

OBJECTS := $(SOURCES:.c=.o) src/slloadcore.gen.o $(MODULES_STATIC_O)
ML_OBJECTS := $(ML_SOURCES:.c=.o)

# --- Include other makefiles ---
include Makefiles/3ds.GNUmakefile

# --- All/Cleaning begins here ---

all: $(PROJECT) $(MODULES_DYNAMIC_SO) $(COPY_SLEDCONF)

clean: FORCE
	rm -f $(PROJECT) $(OBJECTS) modules/*.so src/modules/*.o static/modwraps/*.c static/modwraps/*.o static/modwraps/*.incs src/slloadcore.gen.c
	rm -f src/modules/mod_dl.c.libs

default_sledconf: FORCE
	[ -e sledconf ] || cp Makefiles/sledconf.default sledconf

FORCE:

# --- Generic object conversion rule begins here ---
%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ `cat $(@:.o=.incs) 2>/dev/null || true` $^

# --- Module compile info begins here ---
# To build modules/X.so, link src/modules/X.o with information in an optional .libs file
modules/%.so: src/modules/%.o $(ML_OBJECTS)
	mkdir -p modules
	$(CC) $(CFLAGS) $(CPPFLAGS) $(LDFLAGS) $(LDSOFLAGS) -o $@ $^ `cat src/modules/$*.libs 2>/dev/null || true`

# -- k2wrap/k2link
src/slloadcore.gen.c: src/plugin.h static/k2link
	./static/k2link $(MODULES_STATIC) > src/slloadcore.gen.c
# The wrapper is made dependent on the module .c file not because it really has to be,
#  but because it ensures that the compiled module depends indirectly on the module source.
# A dependency on the %.incs file SHOULD exist but doesn't because it breaks things
static/modwraps/%.c: src/modules/%.c
	./static/k2wrap $*

# --- Platform-specific module library rules begin here ---

ifeq ($(OS),Linux)
src/modules/mod_dl.c.libs:
	echo -ldl > src/modules/mod_dl.c.libs
else
src/modules/mod_dl.c.libs:
	echo "" > src/modules/mod_dl.c.libs
endif

# --- The actual build begins here ---
ifeq ($(STATIC),0)
 sled: $(OBJECTS)
	$(CC) $(CPPFLAGS) $(CFLAGS) -rdynamic $(LDFLAGS) -o $@ $^ `cat $(PLATFORM_LIBS) $(MODULES_STATIC_LIBS) 2>/dev/null || true` $(LIBS)
else
 sled: $(OBJECTS) $(ML_OBJECTS)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LIBS) `cat $(PLATFORM_LIBS) $(MODULES_STATIC_LIBS) 2>/dev/null || true`
endif
