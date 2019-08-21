
#ifndef __INCLUDED_MODPLUGIN__
#define __INCLUDED_MODPLUGIN__

// Header defining what plugins should implement.
// This is the first list of function declarations.
// It must be in the order given in mod.h,
//  and it must be kept in sync with k2link, and mod_dl.c
// Also note that this is automatically 'parsed' by k2link to get function signatures.

// [FUNCTION_DECLARATION_WEBRING]
// See: plugin.h, mod.h, k2link, mod_dl.c

#include "types.h"
// Needed for module* and mod_get, also includes asl.h; including asl.h manually causes weirdness BTW
#include "mod.h"
// For PGCTX things
#include <stdlib.h>
#include <assert.h>

// Function that initializes the plugin.
// Things like buffers, file loading, etc..
// If the plugin wants to get drawn at specific times,
// it must schedule a timer to allocate draw times.
// Otherwise, it'll just be drawn rather randomly.
// Keep the module number, it's needed to schedule a timer.
//
// Returning 0 indicates success. Anything else indicates initialization failure.
// SLED will continue regardless.
// If you just want to load under certain conditions, this is helpful.
//
// NOTE: For "flt" type filter plugins, moduleno is the next filter or
// the output module. In other cases, it is the moduleno of oneself.
//
// argstr is always null for plugins that are not out or flt.
// When it's NOT null, you have to free it, even if you error out!
//
// Also note! "flt" TYPE PLUGINS MAY HAVE MULTIPLE INSTANCES INSTANTIATED.
int init(int moduleno, char* argstr);

// FOR "gfx" TYPE PLUGINS:
// This gets called before draw if this was not the last module drawn.
// This is particularly useful because modules can be unexpectedly turned off,
//  and various timers (among other things) need to be reset in this case.
void reset(int moduleno);

// FOR "gfx" TYPE PLUGINS:
// Draw function, gets called as scheduled.
// If the image should be retained for a certain time
// and not interrupted, sleep here.
// Note, that this shouldn't be used for anything non-critical.
//
// For animations, you should draw a frame and schedule a next time.
// This will define your frame rate. Keep a number which indicates
// which frame is currently being displayed. Simple, but works.
// Other things can interrupt your redraw times, which is rather
// important  because a priority notification might happen.
//
// Anything other than 0 is considered a failure, but handled differently.
// Returning 1 is a "soft" failure, sled will just draw another random module instead.
//  Return this incase you don't wanna draw right now.
//  Also return this at the end of an animation, because it will instantly* repick.
// Returning anything else is a "hard" failure, which sled will act upon and abort.
//
// Sometimes, plugins get called with arguments. It is up to the plugin to interpret them.
// A generic automated call will not contain any arguments (argc = 0).
// argv's substrings and argv itself are freed after the call.
int draw(int moduleno, int argc, char* argv[]);

// FOR "out" and "flt" TYPE PLUGINS:
// Function that sets a pixel, buffered changes.
// Only update the displayed info after calling render.
int set(int moduleno, int x, int y, RGB color);
RGB get(int moduleno, int x, int y);

// FOR "out" and "flt" TYPE PLUGINS:
// Clears the buffer.
int clear(int moduleno);

// FOR "out" and "flt" TYPE PLUGINS:
// Render the updates, starts displaying the buffer.
int render(int moduleno);

// FOR "out" and "flt" TYPE PLUGINS:
// Get dimensions and other stuff.
int getx(int moduleno);
int gety(int moduleno);

// FOR "out" and "flt" TYPE PLUGINS:
// Wait until the desired usec hit.
// If you don't need to do anything special,
// you can just `return wait_until_core(desired_usec);`.
oscore_time wait_until(int moduleno, oscore_time desired_usec);

// FOR "out" and "flt" TYPE PLUGINS:
// Interrupts any ongoing wait_until. Use *after* the timer operation to ensure this works correctly.
void wait_until_break(int moduleno);

// FOR "mod" TYPE PLUGINS:
// This sets the base directory setting.
void setdir(int moduleno, const char* dir);

// FOR "mod" TYPE PLUGINS:
// This loads a module by full name ("bgm_oopsie") into a module structure.
// Returning non-zero means an error occurred.
int load(int moduleno, module* mod, const char * name);

// FOR "mod" TYPE PLUGINS:
// Given the modloader_user field of a module, clean that up.
void unload(int moduleno, void* modloader_user);

// FOR "mod" TYPE PLUGINS:
// This scans the directory for modules this plugin will accept, as module names.
// Error and no-modules are basically equivalent; do NOT assert that modules are present, though!
void findmods(int moduleno, asl_av_t* result);

// Deinit the plugin.
// Free your shit, we need to go.
// It's quite sad, but it's alright, though.
// Our time has come, we'd rather stay,
// but we need to run, core said "Begone!".
// Also also note: flt_ modules DO NOT deinit their targets anymore! chain_link is responsible for this.
void deinit(int moduleno);

#define PGCTX_BEGIN typedef struct {
#define PGCTX_END } pgctx_t;
#define PGCTX_INIT pgctx_t * ctx = (pgctx_t *) (mod_get(_modno)->user = calloc(sizeof(pgctx_t), 1)); assert(ctx);
#define PGCTX_GET pgctx_t * ctx = (pgctx_t *) mod_get(_modno)->user; assert(ctx);
#define PGCTX_DEINIT free(mod_get(_modno)->user);

#define PGCTX_BEGIN_FILTER PGCTX_BEGIN int nextid; module* next;
#define PGCTX_INIT_FILTER PGCTX_INIT ctx->nextid = mod_get(_modno)->chain_link; ctx->next = mod_get(ctx->nextid);

#endif
