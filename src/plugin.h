// Header defining what plugins should implement.

// Function that initializes the plugin.
// Things like buffers, file loading, etc..
// If the plugin wants to get drawn at specific times,
// it must schedule a timer to allocate draw times.
// Otherwise, it'll just be drawn rather randomly.
int plugin_init();

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
int plugin_draw();

// Deinit the plugin.
// Free your shit, we need to go.
// It's quite sad, but it's alright, though.
// Our time has come, we'd rather stay,
// but we need to run, core said "Begone!".
int plugin_deinit();
