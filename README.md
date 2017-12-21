# sled: The satanic LED controller.

Modular LED Matrix controller.

# Manual
Read the Satanic Bible by Anton Szandor LaVey.

# Hardware

Matrix consisting of ws2812b pixels.

Connected to the ports of the specific board you're using.

* Raspberry Pi: See https://github.com/jgarff/rpi_ws281x
 - I can recommend using PCM, little chance it is used.

# Software

Common: 
* Some C99 compiler
* Some libc
* GNU Make

Platform specific:

* `DEBUG`
 - SDL.

* `RPI`
 - https://github.com/jgarff/rpi_ws281x


Compile with `make <PLATFORM> MATRIX_X=<X size> MATRIX_Y=<Y size>`
