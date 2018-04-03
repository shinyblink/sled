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

`MATRIX_X` and `MATRIX_Y` are optional, defaults are there. However, you probably want to change them.

# Platforms

* `SDL2`
	- SDL2-based virtual matrix for development.

* `DEBUG`
	- Alias to SDL2 + some debugging-friendly compiler options.

* `RPI_WS2812B`
	- Uses https://github.com/jgarff/rpi_ws281x to drive the strips.
	- Uses PCM, DMA channel 10 and SoC pin 21/RPI header pin 40 by default.

* `UDP`
	- UDP output following the protocol of CalcProgrammer1/KeyboardVisualizer's LED strip output.
	- An ESP8266 Arduino sketch will be uploaded here soon. In the meantime, CalcProgrammer1's repository has a compatible sketch, I believe.

* `RPI_HUB75`
	- A backend that drives HUB75-style matrices using https://github.com/hzeller/rpi-rgb-led-matrix
	- Does *not* use `MATRIX_X`/`MATRIX_Y`, as that's a bit more complicated.
	- Instead, use `./sled -o "rpi_hub75:--led-rows=32 --led-cols=64 --led-multiplexing=1 --led-chain=2 --led-pixel-mapper=U-mapper"`, for example. Arguments are explained at the library's project page.

# Modules

By default, only modules with zero dependencies are built. Apart from the output modules, of course.
They are the following:

## Effects/Graphic Modules
* `gfx_random_static`: Randomized static color.

* `gfx_random_rects`: Random colored rectangle animation.
	- Note, your matrix axies have to be dividable by 4.

* `gfx_twinkle`: Twinkle, twinkle, little star? Made by @20kdc.

* `gfx_text`: Displays text using custom proportional font. Made by @20kdc.
	- "Hack me" greeting by default, obviously. 8x8 or bigger required.

* `bgm_fish`: FIfo Shell. A small FIFO-based queue manipulator. Made by @20kdc.
	- Uses a little background CPU usage. Creates `sled.fish` FIFO in the sled tree.

* `gfx_gol`: A Conway's Game of Life clone.

* `gfx_rainbow`: A simple rainbow animation.

* `gfx_math_sinpi`: A sinus curve of Pi.

* `gfx_plasma`: A plasma animation, ported from borgware.

* `gfx_balls`: Small ball animation.

* `gfx_checkerboard`: A checkerboard animation.

* `gfx_clock`: A digital clock.

* `gfx_rainbow`: A rainbow!

* `gfx_sinematrix`: A psychadelic matrix manipulating effect, made by @orithena.

* `bgm_opc`: An OpenPixelControl server, displays things when it is written to.

---

If you want to only build a specific set of modules, pass a `MODULES="myfavouritemodule bestmodule"` to the `make` line.

If you want to build a specific module later on, you can do the same but with just the
module you wanna build. It'll get added to the same directory where the others are.
Or, if you are particularly lazy, just add the module name to the make invocation.

# Support
Support for sled is done on a if-i-can-find-the-time basis. Mostly, this project is for myself.
I'll try to help everyone, but I might take some time.



If you'd like to support my work or me in general, I have a [donation page](https://i0i0.me/donateme.html).
Anything is highly appreciated. If you are donating via PayPal, please set a message so I can thank you!

Most of the donations tracable to anything sled related will be spend buying different LED matrices and other hardware.

Plus, buying cola and booze. ;)
