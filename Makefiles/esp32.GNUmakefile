ifneq (,$(findstring esp32,$(DEFAULT_OUTMOD)))
ifeq (,${PLATFORMIO_HOME})
	$(error Please set PLATFORMIO_HOME)
endif

PLATFORM := freertos
TARGET := esp32

CC := ${PLATFORMIO_HOME}/packages/toolchain-xtensa32/bin/xtensa-esp32-elf-gcc
AR := ${PLATFORMIO_HOME}/packages/toolchain-xtensa32/bin/xtensa-esp32-elf-ar

CFLAGS += -O3
CFLAGS += -ffunction-sections
CFLAGS += -fdata-sections
CFLAGS += -fstrict-volatile-bitfields
CFLAGS += -DSDLFULLSCREEN
CFLAGS += -mlongcalls
CFLAGS += -I${PLATFORMIO_HOME}/packages/framework-arduinoespressif32/tools/sdk/include/freertos/
CFLAGS += -I${PLATFORMIO_HOME}/packages/framework-arduinoespressif32/tools/sdk/include/config/
CFLAGS += -I${PLATFORMIO_HOME}/packages/framework-arduinoespressif32/tools/sdk/include/esp32/
CFLAGS += -I${PLATFORMIO_HOME}/packages/framework-arduinoespressif32/tools/sdk/include/soc/
CFLAGS += -I${PLATFORMIO_HOME}/packages/framework-arduinoespressif32/tools/sdk/include/heap/
CFLAGS += -I${PLATFORMIO_HOME}/packages/framework-arduinoespressif32/tools/sdk/include/driver/
CFLAGS += -I${PLATFORMIO_HOME}/packages/framework-arduinoespressif32/tools/sdk/include/esp_ringbuf/

LDFLAGS := -nostdlib \
	-u call_user_start_cpu0	\
	$(EXTRA_LDFLAGS) \
	-Wl,--gc-sections	\
	-Wl,-static	\
	-Wl,--start-group	\
	$(COMPONENT_LDFLAGS) \
	-lgcc \
	-lstdc++ \
	-lgcov \
	-Wl,--end-group \
	-Wl,-EL

ifeq ($(DEFAULT_OUTMOD),esp32_digitalledlib)
CFLAGS += -I${PLATFORMIO_HOME}/lib/ESP32\ Digital\ RGB\ LED\ Drivers/src/
endif
endif
