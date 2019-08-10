# sled: The satanic LED controller. [![Build Status](https://travis-ci.com/shinyblink/sled.svg?branch=master)](https://travis-ci.com/shinyblink/sled) [![Build Status](https://dev.azure.com/shinyblink/sled/_apis/build/status/shinyblink.sled?branchName=master)](https://dev.azure.com/shinyblink/sled/_build/latest?definitionId=1&branchName=master) [![builds.sr.ht status](https://builds.sr.ht/~vifino/sled.svg)](https://builds.sr.ht/~vifino/sled?) [![Join the chat at https://gitter.im/shinyblink/sled](https://badges.gitter.im/shinyblink/sled.svg)](https://gitter.im/shinyblink/sled?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)

Modular LED Matrix controller.

## Manual
Read the Satanic Bible by Anton Szandor LaVey.

## Getting started (SDL2 Tutorial on debian)

```bash
sudo apt-get install gcc make git libsdl2-dev
git clone https://github.com/shinyblink/sled.git
cd sled
make
./sled
```

You can overwrite the default build by using `sledconf` locally. Examples are given in the `Makefiles/` folder. You might just copy one e.g. `cp Makefiles/sledconf.sdl2 sledconf` to get a sensible initial configuration.

## Getting started on developing modules

This assumes you already got something to run (see Getting Started)

### How to select modules
Modules are compiled into `*.so` files into `modules/`. You can delete any module you don't want to run from the `modules/` folder.

Additionally the `GNUmakefile` includes the `sledconf` file which selects which modules are build with the `MODULES` variable. It should contain at least one outputmodule and one additional module to run. Please copy a file from `Makefiles/` to `sledconf` for changing your build locally and don't change the `GNUmakefile`.

```make
MODULES := $(MODULES_DEFAULT) out_$(DEFAULT_OUTMOD)
```
This will build all modules that are listed in the GNUmakefile.

```make
MODULES := out_$(DEFAULT_OUTMOD)
MODULES += gfx_newshinyeffect
MODULES += gfx_another_effect gfx_effect3
```
Only the listed modules will be build. This is good for testing. The third line can be commented out fast (by putting a `#` in front of the line) so only `gfx_newshinyeffect` will be build.

The old modules will still be in the `modules/` folder and thus will still be loaded by sled. You can remove them with `rm modules/gfx_*` or `make clean` before building with `make`.

### Build a new module

The sources for the modules are located in `src/modules/`. Looking inside a graphic effect module (e.g. `gfx_rainbow.c`) you see that gfx modules provide an interface via four functions.
```c
int init(int moduleno, char* argstr); // Called once at program start
int deinit(void); // Called once on program exit
void reset(void); // Called on module change to this module
int draw(int argc, char* argv[]); // Called once per frame
```
All other functions and variables should be declared `static` and used internally only.

The `draw(...)` function should returns 0 while the module is running and 1 if it's done.
Also each module has it's own timer that is controlled with `timer_add(...)` from `timers.h` that controls the next time `draw(...)` is called. This mechanism allows modules to control their own framerates.
You can use `matrix_set(...)` and `matrix_render()` to output images platform independently.

To get an idea of how `gfx_*` modules work just look (and copy/modify) some modules.

### Testing

If you wrote a module add it to `MODULES` in `sledconf` to test it locally or add it to `MODULES_DEFAULT` in the `GNUmakefile` if it's done.

Also try it with different output sizes. `MATRIX_X` and `MATRIX_Y` can be used to control the output size for the sdl2 output.

### Before you commit/merge into master

Check that:

* [ ] It compiles after a `make clean`

* [ ] It works with other modules and quits after a reasonable time

* [ ] It works with different display sizes
  * [ ] not 2^n
  * [ ] Landscape
  * [ ] Portrait
  * [ ] small sizes (<=16x16)
  * [ ] big sizes (>=256x256)

* [ ] It has a short description

* [ ] It's well formatted (no mixed tabs/spaces) (have you considered `astyle`)

## Hardware

Matrix consisting of ws2812b pixels, or HUB75/HUB75E LED matrix panels.
Minimum recommended size of a matrix is 8x8, anything less and you won't see much of the effects.

Can handle a ~~maximum 256x256~~ pretty ridiculous matrix (switched to ints) in either plain or snake tiling.
Plain means every row starts with the left pixel, while snake means it changes starting position every tile.
Both start upper left, as that is (0, 0) for this code.

Connected to the ports of the specific board you're using.

* Raspberry Pi (Zero): See [rpi_ws281x](https://github.com/jgarff/rpi_ws281x).
  * I can recommend using PCM, little chance it is used. It is used by default. Pin 40 on the Zero.
  * You might need a level shifter to shift the 3.3V logic level to 5V the strips want.
  
* ESP8266 DevBoard with UDP Sketch
  * Uses output D2 by default, but you can use almost anything the Adafruit NeoPixel library supports.
  * You might need a level shifter to shift the 3.3V logic level to 5V the strips want.
  
* Raspberry Pi 3: Bitbanging HUB75 LED panels.
  * High performance RaspberryPi is needed as bitbanging is quite resource intensive.
  
* iCEBreaker FPGA with LED Panel Driver Pmod
  * You can use the MPSSE SPI sled output module in combination with @smunaut [rgb_led panel design](https://github.com/smunaut/ice40-playground/tree/master/projects/rgb_panel).
  
Note: When buying HUB75 LED panels be very careful what shift registers are used for the panel. When buying from Aliexpress you should explicitly ask for Panels with the ICN2037.
Note: If you have HUB75 LED panels that use for example the FM6126A shift registers you will need a driver that can set up the panels on power up. For more information refer to the [lengthy discussion on hzeller's RPi LED Matrix repo](https://github.com/hzeller/rpi-rgb-led-matrix/issues/746).

## Building Options

Common:
* Some C99 compiler
* Some libc
* GNU Make

Platforms might need additional dependencies, check below.

Your local SLED configuration is in `sledconf`, which is thus .gitignore'd by default.
Examples are given in the Makefiles directory, check `sledconf.*`.
It can override various settings:

* `PROJECT`
  * The name of the final binary.
  * Defaults to 'sled'.

* `PLATFORM`
  * The platform being compiled for.
  * Defaults to 'unix'.
  * See src/os_unix.c and similar.

* `DEBUG`
  * Set to 1 to add debug information to all files and disable -O2.
  * Defaults to 0.

* `CFLAGS`
  * Defaults to `-O2 -march=native` or `-march=native -Og -ggdb` dependent on `DEBUG`.

* `STATIC`
  * Set to 1 to use static linking.
  * Set to 0 to use -ldl based linking.
  * Defaults to 0.

* `DEFAULT_OUTMOD`
  * The default -o parameter.
  * Defaults to "sdl2".

* `DEFAULT_MODULEDIR`
  * The default -m parameter.
  * Defaults to "./modules".

* `MODULES`
  * The modules being compiled.
  * `MODULES_AVAILABLE` contains all available GFX modules.
  * Default is this & DEFAULT_OUTMOD prefixed with `out_`.

Compile with simply `make`.

## Output modules

* `out_sdl2`
  * SDL2-based virtual matrix for development.

* `out_rpi_ws2812b`
  * Uses [rpi_ws281x](https://github.com/jgarff/rpi_ws281x) to drive the strips.
  * Uses PCM, DMA channel 10 and SoC pin 21/RPI header pin 40 by default.

* `out_udp`
  * UDP output following the protocol of CalcProgrammer1/KeyboardVisualizer's LED strip output.
  * An ESP8266 Arduino sketch will be uploaded here soon. In the meantime, CalcProgrammer1's repository has a compatible sketch, I believe.

* `out_rpi_hub75`
  * A backend that drives HUB75-style matrices using https://github.com/hzeller/rpi-rgb-led-matrix
  * Does *not* use `MATRIX_X`/`MATRIX_Y`, as that's a bit more complicated.
  * Instead, use `./sled -o "rpi_hub75:--led-rows=32 --led-cols=64 --led-multiplexing=1 --led-chain=2 --led-pixel-mapper=U-mapper"`, for example. Arguments are explained at the library's project page.

* `out_mpsse_spi`
  * Uses [libmpsse](https://github.com/devttys0/libmpsse/) to speak SPI using USB FTDI chips utilizing the FTDI MPSSE engine.
  * Use this to speak with [@smunaut's RGB matrix driver for iCEBreaker FPGA boards](https://github.com/smunaut/ice40-playground/tree/master/projects/rgb_panel).
  * Also does not use `MATRIX_X`/`MATRIX_Y`.

## Modules

By default, only modules with zero dependencies are built. Apart from the output modules, of course.
They are the following:

### Effects/Graphic Modules
* `gfx_random_static`: Randomized static color.

* `gfx_random_rects`: Random colored rectangle animation.
  - Note, your matrix axies have to be dividable by 4.

* `gfx_twinkle`: Twinkle, twinkle, little star? Made by @20kdc.

* `gfx_text`: Displays text using custom proportional font. Made by @20kdc.
  - "Hack me" greeting by default, obviously. 8x8 or bigger required.

* `bgm_fish`: FIfo Shell. A small FIFO-based queue manipulator. Made by @20kdc.
  - Shouldn't have background CPU usage as of the select abuse. Creates `sled.fish` FIFO in the sled tree.

* `gfx_gol`: A simple black and white Conway's Game of Life clone.

* `gfx_golc`: A Conway's Game of Life clone; with fading, color inheritance and loop detection, made by @orithena.

* `gfx_rainbow`: A simple rainbow animation.

* `gfx_math_sinpi`: A sinus curve of Pi.

* `gfx_plasma`: A plasma animation, ported from borgware.

* `gfx_balls`: Small ball animation.

* `gfx_checkerboard`: A checkerboard animation.

* `gfx_clock`: A digital clock.

* `gfx_sinematrix`: A psychedelic matrix manipulation effect, made by @orithena.

* `gfx_affinematrix`: A billowing matrix manipulation effect, made by @orithena.

* `gfx_candyflow`: A tweak of `gfx_affinematrix` to look like candy colored lava, made by @orithena.

* `gfx_sinefield`: A strange effect that emerged while whacking random math functions into a loop, made by @orithena.

* `bgm_opc`: An OpenPixelControl server, displays things when it is written to.

* `gfx_bttrblls`: Tweak of `gfx_balls` with fractional speeds and less noisy colors. by @cyriax0

* `gfx_sort2D`: 2D partial bubblesort on color ranges, may change direction. by @cyriax0

* `gfx_starfield`: Fly through random stars executing random turns. by @cyriax0

* `gfx_reddot`: A red dot that oscillates in brightness. by @cyriax0

* `gfx_sparkburn`: A short transitve effect. by @cyriax0

* `gfx_sort1D`: Sorting algorithm visualizer. by @cyriax0

* `gfx_ursuppe`: Wierd particle simulation based on bttrblls. by @cyriax0

* `gfx_ursuppe2`: Slightly improved particle simulation. by @cyriax0

* `gfx_candyswarm`: Based on `gfx_candyflow` with additional particle physics and graphical effects. by @cyriax

* `gfx_afterglow`: CRT Tube like glow effect that uses the last image of the previous effect. by @cyriax0

* `gfx_rule90`: [Rule90](https://en.wikipedia.org/wiki/Rule_90) cellular automata. by @mattvenn 

* `gfx_invfourier`: Inverse fourier transformation on a limited spectrum. by @cyriax0

* `gfx_colorwheel`: Plots polynomials on the complex plane. by @cyriax0

---

If you want to only build a specific set of modules, change the "MODULES" array in your `sledconf` file.

If you want to build a specific module later on, you can do the same but with just the
module you wanna build. It'll get added to the same directory where the others are.
Or, if you are particularly lazy, just add the module name to the make invocation.

## License
Most non-trivial files contain an explicit statement regarding the license.

Anything with no statement follows the ISC, with the copyright holder being
Adrian "vifino" Pistol, as per the `COPYING` file in this project.

## Support
Support for sled is done on a if-i-can-find-the-time basis. Mostly, this project is for myself.

I'll try to help everyone, but I might take some time.

If you'd like to support my work or me in general, I have a [donation page](https://i0i0.me/donateme.html).

Anything is highly appreciated. If you are donating via PayPal, please set a message so I can thank you!

Most of the donations tracable to anything sled related will be spend buying different LED matrices and other hardware.

Plus, buying cola and booze. ;)
