# sled: The satanic LED controller.

Modular LED Matrix controller.

# Manual
Read the Satanic Bible by Anton Szandor LaVey.

# Hardware

Matrix consisting of ws2812b pixels.
Minimum recommended size is 8x8, anything less and you won't see much of the effects.

Can handle a ~~maximum 256x256~~ pretty ridiculous matrix (switched to ints) in either plain or snake tiling.
Plain means every row starts with the left pixel, while snake means it changes starting position every tile.
Both start upper left, as that is (0, 0) for this code.

Connected to the ports of the specific board you're using.

* Raspberry Pi (Zero): See https://github.com/jgarff/rpi_ws281x
 - I can recommend using PCM, little chance it is used. It is used by default. Pin 40 on the Zero.
 - You might need a level shifter to shift the 3.3V logic level to 5V the strips want.

* ESP8266 DevBoard with UDP Sketch
 - Uses output D2 by default, but you can use almost anything the Adafruit NeoPixel library supports.
 - You might need a level shifter to shift the 3.3V logic level to 5V the strips want.

# Software

Common:
* Some C99 compiler
* Some libc
* GNU Make

Platforms might need additional dependencies, check below.

Compile with `make PLATFORM=<PLATFORM> MATRIX_X=<X size> MATRIX_Y=<Y size>`

# Platforms

1) `SDL2`
 - SDL2-based virtual matrix for development.

2) `DEBUG`
 - Alias to SDL2 + some debugging-friendly compiler options.

3) `RPI`
 - Uses https://github.com/jgarff/rpi_ws281x to drive the strips.
 - Uses PCM, DMA channel 10 and SoC pin 21/RPI header pin 40 by default.

4) `UDP`
 - UDP output following the protocol of CalcProgrammer1/KeyboardVisualizer's LED strip output.
 - An ESP8266 Arduino sketch will be uploaded here soon. In the meantime, CalcProgrammer1's repository has a compatible sketch, I believe.

# Modules

By default, only modules with zero dependencies are built. Apart from the output modules, of course.
They are the following:

## Effects/Graphic Modules
1) `gfx_random_static`: Randomized static color.

2) `gfx_random_rects`: Random colored rectangle animation. Note, your matrix axies have to be dividable by 4.

3) `gfx_twinkle`: Twinkle, twinkle, little star? Made by @20kdc.

4) `gfx_text`: Displays text using custom proportional font. Satanic greeting by default, obviously. 8x8 or bigger required. Made by @20kdc.

5) `bgm_fish`: FIfo Shell. A small FIFO-based queue manipulator. Uses a little background CPU usage. Creates `sled.fish` FIFO in the sled tree. Made by 20kdc.

6) `gfx_gol`: A Conway's Game of Life clone.

7) `gfx_rainbow`: A simple rainbow animation.

8) `gfx_math_sinpi`: A sinus curve of Pi.

9) `gfx_plasma`: A plasma animation, ported from borgware.

---

If you want to only build a specific set of modules, pass a `MODULES="myfavouritemodule bestmodule"` to the `make` line.

If you want to build a specific module later on, you can do the same but with just the
module you wanna build. It'll get added to the same directory where the others are.
Or, if you are particularly lazy, just add the module name to the make invocation.
