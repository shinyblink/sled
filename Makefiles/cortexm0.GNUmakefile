# Rules for bare-metal ARM Cortex-M0 platforms (see: sledconf.microbit)
ifeq ($(PLATFORM),nrf51)

CC := arm-none-eabi-gcc
AR := arm-none-eabi-ar
LD := arm-none-eabi-ld
STRIP := arm-none-eabi-strip
OBJCOPY := arm-none-eabi-objcopy

# first and foremost
# it is INCREDIBLY IMPORTANT TO HEED THE FOLLOWING.
# make sure your newlib is fully Cortex-M0 compliant
# the Cortex-M0 will RESET IF THIS IS NOT FOLLOWED.
# the reason _prestart exists is to help debug this condition.

CFLAGS += -mthumb -march=armv6-m -mcpu=cortex-m0 -mfloat-abi=soft
CFLAGS += -ffreestanding
CFLAGS += -Wl,--start-group
# Note: don't use nosys.
# This is because nosys adds stubs over functions that we can actually implement,
#  and should implement to improve debugging
#  vitally important since DAPLink isn't exactly the friendliest thing
CFLAGS += -lc
CFLAGS += -static-libgcc
CFLAGS += -Wl,--end-group
CFLAGS += --specs=thumb/v6-m/nano.specs

endif

