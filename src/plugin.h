// Header defining what plugins should implement.

#include <types.h>

// Function that initializes the plugin.
// Things like buffers, file loading, etc..
// If the plugin wants to get drawn at specific times,
// it must schedule a timer to allocate draw times.
// Otherwise, it'll just be drawn rather randomly.
// Keep the module number, it's needed to schedule a timer.
//
// Returning 0 indicates success, 1 indicates the module should be ignored.
// Anything else indicates initialization failure, and this sled will exit.
// If you just want to load under certain conditions, this is helpful.
int init(int moduleno);

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
int draw(int argc, char* argv[]);

// FOR "out" TYPE PLUGINS:
// Function that sets a pixel, buffered changes.
// Only update the displayed info after calling render.
int set(byte x, byte y, RGB *color);

// FOR "out" TYPE PLUGINS:
// Clears the buffer.
int clear(void);

// FOR "out" TYPE PLUGINS:
// Render the updates, starts displaying the buffer.
int render(void);

// Deinit the plugin.
// Free your shit, we need to go.
// It's quite sad, but it's alright, though.
// Our time has come, we'd rather stay,
// but we need to run, core said "Begone!".
int deinit(void);
