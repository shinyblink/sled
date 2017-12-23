# sled: The satanic LED controller.

Modular LED Matrix controller.

# Manual
Read the Satanic Bible by Anton Szandor LaVey.

# Hardware

Matrix consisting of ws2812b pixels.
Can handle a maximum 256x256 matrix in either plain or snake tiling.
Plain means every row starts with the left pixel, while snake means it changes starting position every tile.
Both start upper left, as that is (0, 0) for this code. 

Connected to the ports of the specific board you're using.

* Raspberry Pi: See https://github.com/jgarff/rpi_ws281x
 - I can recommend using PCM, little chance it is used.

# Software

Common: 
* Some C99 compiler
* Some libc
* GNU Make

Platform specific:

* `SDL2`
 - ... SDL2.

* `DEBUG`
 - Alias to SDL2 + some debugging-friendly compiler options.

* `RPI`
 - https://github.com/jgarff/rpi_ws281x


Compile with `make <PLATFORM> MATRIX_X=<X size> MATRIX_Y=<Y size>`

# Modules

By default, only modules with zero dependencies are built.
They are the following:

1) `random_static`: Randomized static color.
2) `random_rects`: Random colored rectangle animation. Note, your matrix axies have to be dividable by 4.
3) `twinkle`: Twinkle, twinkle, little star? Made by @20kdc.

If you want to only build a specific set of modules, pass a `MODULES="myfavouritemodule bestmodule"` to the `make` line.

If you want to build a specific module later on, you can do the same but with just the 
module you wanna build. It'll get added to the same directory where the others are.
