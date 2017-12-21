// Main loader.

#include <types.h>
#include <matrix.h>

int main(int argc, char* argv[]) {
	// TODO: parse args.

	// Initialize Matrix.
	if (matrix_init() != 0) {
		// Fail.
		// Should probably mention how, or at least what.
		return 2;
	}

	// TODO: Load modules.
	// TODO: Call modules' init function.
	// TODO: Run all timers, if none is available, load random page or something.
	return 0;
}
