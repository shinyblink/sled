ifneq (,$(findstring card10,$(DEFAULT_OUTMOD)))
ifeq (,${FIRMWARE_HOME})
  $(error Please set FIRMWARE_HOME)
endif

PLATFORM := freertos

CC := arm-none-eabi-gcc
AR := arm-none-eabi-ar
LD := arm-none-eabi-ld
STRIP := arm-none-eabi-strip

# CFLAGS
# various
#CFLAGS += -mthumb
#CFLAGS += -mcpu=cortex-m4
#CFLAGS += -mfloat-abi=softfp
#CFLAGS += -mfpu=fpv4-sp-d16
#CFLAGS += -Wa,-mimplicit-it=thumb
#CFLAGS += -ffunction-sections
#CFLAGS += -fdata-sections
#CFLAGS += -fsingle-precision-constant
#CFLAGS += -fno-isolate-erroneous-paths-dereference
#CFLAGS += -DMATRIX_ORDER_SNAKE
# includes
CFLAGS += -Isrc
CFLAGS += -I${FIRMWARE_HOME}/lib/FreeRTOS/Source/include
CFLAGS += -I${FIRMWARE_HOME}/epicardium
CFLAGS += -I${FIRMWARE_HOME}/lib/sdk/Libraries/MAX32665PeriphDriver/Include
CFLAGS += -I${FIRMWARE_HOME}/lib/sdk/Libraries/CMSIS/Device/Maxim/MAX32665/Include
CFLAGS += -I${FIRMWARE_HOME}/lib/sdk/Libraries/CMSIS/Include/
CFLAGS += -I${FIRMWARE_HOME}/lib/FreeRTOS/Source/portable/GCC/ARM_CM4F/

# LINK flags
LDFLAGS += -mthumb
LDFLAGS += -mcpu=cortex-m4
LDFLAGS += -mfloat-abi=softfp
LDFLAGS += -mfpu=fpv4-sp-d16
LDFLAGS += -Wl,--start-group
LDFLAGS += -lc
LDFLAGS += -lnosys
LDFLAGS += -Wl,--end-group
LDFLAGS += --specs=nano.specs
endif
