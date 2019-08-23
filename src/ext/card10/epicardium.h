#ifndef _EPICARDIUM_H
#define _EPICARDIUM_H

#include <stdint.h>
#include <errno.h>

#ifndef __SPHINX_DOC
/* Some headers are not recognized by hawkmoth for some odd reason */
#include <stddef.h>
#include <stdbool.h>
#else
typedef unsigned int size_t;
typedef _Bool bool;
#endif /* __SPHINX_DOC */

/*
 * These definitions are required for the code-generator.  Please don't touch!
 */
#ifndef API
#define API(id, def) def
#endif
#ifndef API_ISR
#define API_ISR(id, isr) void isr(void);
#endif

/*
 * IDs for all defined API calls.  These IDs should not be needed in application
 * code on any side.
 */

/* clang-format off */
#define API_SYSTEM_EXIT             0x1
#define API_SYSTEM_EXEC             0x2
#define API_SYSTEM_RESET            0x3
#define API_BATTERY_VOLTAGE         0x4

#define API_INTERRUPT_ENABLE        0xA
#define API_INTERRUPT_DISABLE       0xB

#define API_UART_WRITE_STR         0x10
#define API_UART_READ_CHAR         0x11
#define API_UART_READ_STR          0x12

#define API_STREAM_READ            0x1F

#define API_DISP_OPEN              0x20
#define API_DISP_CLOSE             0x21
#define API_DISP_PRINT             0x22
#define API_DISP_CLEAR             0x23
#define API_DISP_UPDATE            0x24
#define API_DISP_LINE              0x25
#define API_DISP_RECT              0x26
#define API_DISP_CIRC              0x27
#define API_DISP_PIXEL             0x28
#define API_DISP_FRAMEBUFFER       0x29

#define API_FILE_OPEN              0x40
#define API_FILE_CLOSE             0x41
#define API_FILE_READ              0x42
#define API_FILE_WRITE             0x44
#define API_FILE_FLUSH             0x45
#define API_FILE_SEEK              0x46
#define API_FILE_TELL              0x47
#define API_FILE_STAT              0x48
#define API_FILE_OPENDIR           0x49
#define API_FILE_READDIR           0x4a
#define API_FILE_UNLINK            0x4b
#define API_FILE_RENAME            0x4c
#define API_FILE_MKDIR             0x4d

#define API_RTC_GET_SECONDS        0x50
#define API_RTC_SCHEDULE_ALARM     0x51
#define API_RTC_SET_MILLISECONDS   0x52

#define API_LEDS_SET               0x60
#define API_LEDS_SET_HSV           0x61
#define API_LEDS_PREP              0x62
#define API_LEDS_PREP_HSV          0x63
#define API_LEDS_UPDATE            0x64
#define API_LEDS_SET_POWERSAVE     0x65
#define API_LEDS_SET_ROCKET        0x66
#define API_LEDS_SET_FLASHLIGHT    0x67
#define API_LEDS_DIM_TOP           0x68
#define API_LEDS_DIM_BOTTOM        0x69
#define API_LEDS_SET_ALL           0x6a
#define API_LEDS_SET_ALL_HSV       0x6b
#define API_LEDS_SET_GAMMA_TABLE   0x6c
#define API_LEDS_CLEAR_ALL         0x6d

#define API_VIBRA_SET              0x70
#define API_VIBRA_VIBRATE          0x71

#define API_LIGHT_SENSOR_RUN       0x80
#define API_LIGHT_SENSOR_GET       0x81
#define API_LIGHT_SENSOR_STOP      0x82

#define API_BUTTONS_READ           0x90

#define API_GPIO_SET_PIN_MODE      0xA0
#define API_GPIO_GET_PIN_MODE      0xA1
#define API_GPIO_WRITE_PIN         0xA2
#define API_GPIO_READ_PIN          0xA3

#define API_TRNG_READ              0xB0

#define API_PERSONAL_STATE_SET     0xc0
#define API_PERSONAL_STATE_GET     0xc1
#define API_PERSONAL_STATE_IS_PERSISTENT 0xc2

/* clang-format on */

typedef uint32_t api_int_id_t;

/**
 * Interrupts
 * ==========
 * Next to API calls, Epicardium API also has an interrupt mechanism to serve
 * the other direction.  These interrupts can be enabled/disabled
 * (masked/unmasked) using :c:func:`epic_interrupt_enable` and
 * :c:func:`epic_interrupt_disable`.
 */

/**
 * Enable/unmask an API interrupt.
 *
 * :param int_id: The interrupt to be enabled
 */
API(API_INTERRUPT_ENABLE, int epic_interrupt_enable(api_int_id_t int_id));

/**
 * Disable/mask an API interrupt.
 *
 * :param int_id: The interrupt to be disabled
 */
API(API_INTERRUPT_DISABLE, int epic_interrupt_disable(api_int_id_t int_id));

/**
 * The following interrupts are defined:
 */

/* clang-format off */
/** Reset Handler */
#define EPIC_INT_RESET                  0
/** ``^C`` interrupt. See :c:func:`epic_isr_ctrl_c` for details.  */
#define EPIC_INT_CTRL_C                 1
/** UART Receive interrupt.  See :c:func:`epic_isr_uart_rx`. */
#define EPIC_INT_UART_RX                2
/** RTC Alarm interrupt.  See :c:func:`epic_isr_rtc_alarm` */
#define EPIC_INT_RTC_ALARM              3

/* Number of defined interrupts. */
#define EPIC_INT_NUM                    4
/* clang-format on */

/*
 * "Reset Handler*.  This isr is implemented by the API caller and is used to
 * reset the core for loading a new payload.
 *
 * Just listed here for completeness.  You don't need to implement this yourself.
 */
API_ISR(EPIC_INT_RESET, __epic_isr_reset);

/**
 * Core API
 * ========
 * The following functions control execution of code on core 1.
 */

/**
 * Stop execution of the current payload and return to the menu.
 *
 * :param int ret:  Return code.
 * :return: :c:func:`epic_exit` will never return.
 */
void epic_exit(int ret) __attribute__((noreturn));

/*
 * The actual epic_exit() function is not an API call because it needs special
 * behavior.  The underlying call is __epic_exit() which returns.  After calling
 * this API function, epic_exit() will enter the reset handler.
 */
API(API_SYSTEM_EXIT, void __epic_exit(int ret));

/**
 * Stop execution of the current payload and immediately start another payload.
 *
 * :param char* name: Name (path) of the new payload to start.  This can either
 *    be:
 *
 *    - A path to an ``.elf`` file (l0dable).
 *    - A path to a ``.py`` file (will be loaded using Pycardium).
 *    - A path to a directory (assumed to be a Python module, execution starts
 *      with ``__init__.py`` in this folder).
 *
 * :return: :c:func:`epic_exec` will only return in case loading went wrong.
 *    The following error codes can be returned:
 *
 *    - ``-ENOENT``: File not found.
 *    - ``-ENOEXEC``: File not a loadable format.
 */
int epic_exec(char *name);

/*
 * Underlying API call for epic_exec().  The function is not an API call itself
 * because it needs special behavior when loading a new payload.
 */
API(API_SYSTEM_EXEC, int __epic_exec(char *name));

/**
 * Reset/Restart card10
 */
API(API_SYSTEM_RESET, void epic_system_reset(void));

/**
 * Battery Voltage
 * ===============
 */

/**
 * Read the current battery voltage.
 */
API(API_BATTERY_VOLTAGE, int epic_read_battery_voltage(float *result));

/**
 * UART/Serial Interface
 * =====================
 */

/**
 * Write a string to all connected serial devices.  This includes:
 *
 * - Real UART, whose pins are mapped onto USB-C pins.  Accessible via the HW-debugger.
 * - A CDC-ACM device available via USB.
 * - Maybe, in the future, bluetooth serial?
 *
 * :param str:  String to write.  Does not necessarily have to be NULL-terminated.
 * :param length:  Amount of bytes to print.
 */
API(API_UART_WRITE_STR, void epic_uart_write_str(
	const char *str,
	intptr_t length
));

/**
 * Try reading a single character from any connected serial device.
 *
 * If nothing is available, :c:func:`epic_uart_read_char` returns ``(-1)``.
 *
 * :return:  The byte or ``(-1)`` if no byte was available.
 */
API(API_UART_READ_CHAR, int epic_uart_read_char(void));

/**
 * Read as many characters as possible from the UART queue.
 *
 * :c:func:`epic_uart_read_str` will not block if no new data is available.  For
 * an example, see :c:func:`epic_isr_uart_rx`.
 *
 * :param char* buf: Buffer to be filled with incoming data.
 * :param size_t cnt: Size of ``buf``.
 * :returns: Number of bytes read.  Can be ``0`` if no data was available.
 *    Might be a negative value if an error occured.
 */
API(API_UART_READ_STR, int epic_uart_read_str(char *buf, size_t cnt));

/**
 * **Interrupt Service Routine**
 *
 * UART receive interrupt.  This interrupt is triggered whenever a new character
 * becomes available on any connected UART device.  This function is weakly
 * aliased to :c:func:`epic_isr_default` by default.
 *
 * **Example**:
 *
 * .. code-block:: cpp
 *
 *    void epic_isr_uart_rx(void)
 *    {
 *            char buffer[33];
 *            int n = epic_uart_read_str(&buffer, sizeof(buffer) - 1);
 *            buffer[n] = '\0';
 *            printf("Got: %s\n", buffer);
 *    }
 *
 *    int main(void)
 *    {
 *            epic_interrupt_enable(EPIC_INT_UART_RX);
 *
 *            while (1) {
 *                    __WFI();
 *            }
 *    }
 */
API_ISR(EPIC_INT_UART_RX, epic_isr_uart_rx);

/**
 * **Interrupt Service Routine**
 *
 * A user-defineable ISR which is triggered when a ``^C`` (``0x04``) is received
 * on any serial input device.  This function is weakly aliased to
 * :c:func:`epic_isr_default` by default.
 *
 * To enable this interrupt, you need to enable :c:data:`EPIC_INT_CTRL_C`:
 *
 * .. code-block:: cpp
 *
 *    epic_interrupt_enable(EPIC_INT_CTRL_C);
 */
API_ISR(EPIC_INT_CTRL_C, epic_isr_ctrl_c);

/**
 * Buttons
 * =======
 *
 */

/** Button IDs */
enum epic_button {
	/** ``1``, Bottom left button (bit 0). */
	BUTTON_LEFT_BOTTOM   = 1,
	/** ``2``, Bottom right button (bit 1). */
	BUTTON_RIGHT_BOTTOM  = 2,
	/** ``4``, Top right button (bit 2). */
	BUTTON_RIGHT_TOP     = 4,
	/** ``8``, Top left (power) button (bit 3). */
	BUTTON_LEFT_TOP      = 8,
	/** ``8``, Top left (power) button (bit 3). */
	BUTTON_RESET         = 8,
};

/**
 * Read buttons.
 *
 * :c:func:`epic_buttons_read` will read all buttons specified in ``mask`` and
 * return set bits for each button which was reported as pressed.
 *
 * .. note::
 *
 *    The reset button cannot be unmapped from reset functionality.  So, while
 *    you can read it, it cannot be used for app control.
 *
 * **Example**:
 *
 * .. code-block:: cpp
 *
 *    #include "epicardium.h"
 *
 *    uint8_t pressed = epic_buttons_read(BUTTON_LEFT_BOTTOM | BUTTON_RIGHT_BOTTOM);
 *
 *    if (pressed & BUTTON_LEFT_BOTTOM) {
 *            // Bottom left button is pressed
 *    }
 *
 *    if (pressed & BUTTON_RIGHT_BOTTOM) {
 *            // Bottom right button is pressed
 *    }
 *
 * :param uint8_t mask: Mask of buttons to read.  The 4 LSBs correspond to the 4
 *     buttons:
 *
 *     ===== ========= ============ ===========
 *     ``3`` ``2``     ``1``        ``0``
 *     ----- --------- ------------ -----------
 *     Reset Right Top Right Bottom Left Bottom
 *     ===== ========= ============ ===========
 *
 *     Use the values defined in :c:type:`epic_button` for masking, as shown in
 *     the example above.
 * :return: Returns nonzero value if unmasked buttons are pushed.
 */
API(API_BUTTONS_READ, uint8_t epic_buttons_read(uint8_t mask));

/**
 * Wristband GPIO
 * ==============
 */

/** GPIO pins IDs */
enum gpio_pin {
    /** ``1``, Wristband connector 1 */
    GPIO_WRISTBAND_1 = 1,
    /** ``2``, Wristband connector 2 */
    GPIO_WRISTBAND_2 = 2,
    /** ``3``, Wristband connector 3 */
    GPIO_WRISTBAND_3 = 3,
    /** ``4``, Wristband connector 4 */
    GPIO_WRISTBAND_4 = 4,
};

/** GPIO pin modes */
enum gpio_mode {
    /** Configure the pin as input */
    GPIO_MODE_IN = (1<<0),
    /** Configure the pin as output */
    GPIO_MODE_OUT = (1<<1),

    /** Enable the internal pull-up resistor */
    GPIO_PULL_UP = (1<<6),
    /** Enable the internal pull-down resistor */
    GPIO_PULL_DOWN = (1<<7),
};

/**
 * Set the mode of a card10 GPIO pin.
 *
 * :c:func:`epic_gpio_set_pin_mode` will set the pin specified by ``pin`` to the mode ``mode``.
 * If the specified pin ID is not valid this function will do nothing.
 *
 * **Example:**
 *
 * .. code-block:: cpp
 *
 *    #include "epicardium.h"
 *
 *    // Configure wristband pin 1 as output.
 *    if (epic_gpio_set_pin_mode(GPIO_WRISTBAND_1, GPIO_MODE_OUT)) {
 *        // Do your error handling here...
 *    }
 *
 * :param uint8_t pin: ID of the pin to configure. Use on of the IDs defined in :c:type:`gpio_pin`.
 * :param uint8_t mode: Mode to be configured. Use a combination of the :c:type:`gpio_mode` flags.
 * :returns: ``0`` if the mode was set, ``-EINVAL`` if ``pin`` is not valid or the mode could not be set.
 */
API(API_GPIO_SET_PIN_MODE, int epic_gpio_set_pin_mode(uint8_t pin, uint8_t mode));

/**
 * Get the mode of a card10 GPIO pin.
 *
 * :c:func:`epic_gpio_get_pin_mode` will get the current mode of the GPIO pin specified by ``pin``.
 *
 * **Example:**
 *
 * .. code-block:: cpp
 *
 *    #include "epicardium.h"
 *
 *    // Get the mode of wristband pin 1.
 *    int mode = epic_gpio_get_pin_mode(GPIO_WRISTBAND_1);
 *    if (mode < 0) {
 *        // Do your error handling here...
 *    } else {
 *        // Do something with the queried mode information
 *    }
 *
 * :param uint8_t pin: ID of the pin to get the configuration of. Use on of the IDs defined in :c:type:`gpio_pin`.
 * :returns: Configuration byte for the specified pin or ``-EINVAL`` if the pin is not valid.
 */
API(API_GPIO_GET_PIN_MODE, int epic_gpio_get_pin_mode(uint8_t pin));

/**
 * Write value to a card10 GPIO pin,
 *
 * :c:func:`epic_gpio_write_pin` will set the value of the GPIO pin described by ``pin`` to either on or off depending on ``on``.
 *
 * **Example:**
 *
 * .. code-block:: cpp
 *
 *    #include "epicardium.h"
 *
 *    // Get the mode of wristband pin 1.
 *    int mode = epic_gpio_get_pin_mode(GPIO_WRISTBAND_1);
 *    if (mode < 0) {
 *        // Do your error handling here...
 *    } else {
 *        // Do something with the queried mode information
 *    }
 *
 * :param uint8_t pin: ID of the pin to get the configuration of. Use on of the IDs defined in :c:type:`gpio_pin`.
 * :param bool on: Sets the pin to either true (on/high) or false (off/low).
 * :returns: ``0`` on succcess, ``-EINVAL`` if ``pin`` is not valid or is not configured as an output.
 */
API(API_GPIO_WRITE_PIN, int epic_gpio_write_pin(uint8_t pin, bool on));

/**
 * Read value of a card10 GPIO pin.
 *
 * :c:func:`epic_gpio_read_pin` will get the value of the GPIO pin described by ``pin``.
 *
 * **Example:**
 *
 * .. code-block:: cpp
 *
 *    #include "epicardium.h"
 *
 *    // Get the current value of wristband pin 1.
 *    uint32_t value = epic_gpio_read_pin(GPIO_WRISTBAND_1);
 *    if (mode == -EINVAL) {
 *        // Do your error handling here...
 *    } else {
 *        // Do something with the current value
 *    }
 *
 * :param uint8_t pin: ID of the pin to get the configuration of. Use on of the IDs defined in :c:type:`gpio_pin`.
 * :returns: ``-EINVAL`` if ``pin`` is not valid, an integer value otherwise.
 */
API(API_GPIO_READ_PIN, uint32_t epic_gpio_read_pin(uint8_t pin));

/**
 * LEDs
 * ====
 */

/**
 * Set one of card10's RGB LEDs to a certain color in RGB format.
 *
 * This function is rather slow when setting multiple LEDs, use
 * :c:func:`leds_set_all` or :c:func:`leds_prep` + :c:func:`leds_update`
 * instead.
 *
 * :param int led:  Which LED to set.  0-10 are the LEDs on the top and 11-14
 *    are the 4 "ambient" LEDs.
 * :param uint8_t r:  Red component of the color.
 * :param uint8_t g:  Green component of the color.
 * :param uint8_t b:  Blue component of the color.
 */
API(API_LEDS_SET, void epic_leds_set(int led, uint8_t r, uint8_t g, uint8_t b));

/**
 * Set one of card10's RGB LEDs to a certain color in HSV format.
 *
 * This function is rather slow when setting multiple LEDs, use
 * :c:func:`leds_set_all_hsv` or :c:func:`leds_prep_hsv` + :c:func:`leds_update`
 * instead.
 *
 * :param int led:  Which LED to set.  0-10 are the LEDs on the top and 11-14 are the 4 "ambient" LEDs.
 * :param float h:  Hue component of the color. (0 <= h < 360)
 * :param float s:  Saturation component of the color. (0 <= s <= 1)
 * :param float v:  Value/Brightness component of the color. (0 <= v <= 0)
 */
API(API_LEDS_SET_HSV, void epic_leds_set_hsv(int led, float h, float s, float v));

/**
 * Set multiple of card10's RGB LEDs to a certain color in RGB format.
 *
 * The first ``len`` leds are set, the remaining ones are not modified.
 *
 * :param uint8_t[len][r,g,b] pattern:  Array with RGB Values with 0 <= len <=
 *    15. 0-10 are the LEDs on the top and 11-14 are the 4 "ambient" LEDs.
 * :param uint8_t len: Length of 1st dimension of ``pattern``, see above.
 */
API(API_LEDS_SET_ALL, void epic_leds_set_all(uint8_t *pattern, uint8_t len));

/**
 * Set multiple of card10's RGB LEDs to a certain color in HSV format.
 *
 * The first ``len`` led are set, the remaining ones are not modified.
 *
 * :param uint8_t[len][h,s,v] pattern:  Array of format with HSV Values with 0
 *    <= len <= 15.  0-10 are the LEDs on the top and 11-14 are the 4 "ambient"
 *    LEDs. (0 <= h < 360, 0 <= s <= 1, 0 <= v <= 1)
 * :param uint8_t len: Length of 1st dimension of ``pattern``, see above.
 */
API(API_LEDS_SET_ALL_HSV, void epic_leds_set_all_hsv(float *pattern, uint8_t len));

/**
 * Prepare one of card10's RGB LEDs to be set to a certain color in RGB format.
 *
 * Use :c:func:`leds_update` to apply changes.
 *
 * :param int led:  Which LED to set.  0-10 are the LEDs on the top and 11-14
 *    are the 4 "ambient" LEDs.
 * :param uint8_t r:  Red component of the color.
 * :param uint8_t g:  Green component of the color.
 * :param uint8_t b:  Blue component of the color.
 */
API(API_LEDS_PREP, void epic_leds_prep(int led, uint8_t r, uint8_t g, uint8_t b));

/**
 * Prepare one of card10's RGB LEDs to be set to a certain color in HSV format.
 *
 * Use :c:func:`leds_update` to apply changes.
 *
 * :param int led:  Which LED to set.  0-10 are the LEDs on the top and 11-14
 *    are the 4 "ambient" LEDs.
 * :param uint8_t h:  Hue component of the color. (float, 0 <= h < 360)
 * :param uint8_t s:  Saturation component of the color. (float, 0 <= s <= 1)
 * :param uint8_t v:  Value/Brightness component of the color. (float, 0 <= v <= 0)
 */
API(API_LEDS_PREP_HSV, void epic_leds_prep_hsv(int led, float h, float s, float v));

/**
 * Set global brightness for top RGB LEDs.
 *
 * Aside from PWM, the RGB LEDs' overall brightness can be controlled with a
 * current limiter independently to achieve a higher resolution at low
 * brightness which can be set with this function.
 *
 * :param uint8_t value:  Global brightness of top LEDs. (1 <= value <= 8, default = 1)
 */
API(API_LEDS_DIM_BOTTOM, void epic_leds_dim_bottom(uint8_t value));

/**
 * Set global brightness for bottom RGB LEDs.
 *
 * Aside from PWM, the RGB LEDs' overall brightness can be controlled with a
 * current limiter independently to achieve a higher resolution at low
 * brightness which can be set with this function.
 *
 * :param uint8_t value:  Global brightness of bottom LEDs. (1 <= value <= 8, default = 8)
 */
API(API_LEDS_DIM_TOP, void epic_leds_dim_top(uint8_t value));

/**
 * Enables or disables powersave mode.
 *
 * Even when set to zero, the RGB LEDs still individually consume ~1mA.
 * Powersave intelligently switches the supply power in groups. This introduces
 * delays in the magnitude of ~10µs, so it can be disabled for high speed
 * applications such as POV.
 *
 * :param bool eco:  Activates powersave if true, disables it when false. (default = True)
 */
API(API_LEDS_SET_POWERSAVE, void epic_leds_set_powersave(bool eco));

/**
 * Updates the RGB LEDs with changes that have been set with :c:func:`leds_prep`
 * or :c:func:`leds_prep_hsv`.
 *
 * The LEDs can be only updated in bulk, so using this approach instead of
 * :c:func:`leds_set` or :c:func:`leds_set_hsv` significantly reduces the load
 * on the corresponding hardware bus.
 */
API(API_LEDS_UPDATE, void epic_leds_update(void));

/**
 * Set the brightness of one of the rocket LEDs.
 *
 * :param int led:  Which LED to set.
 *
 *    +-------+--------+----------+
 *    |   ID  | Color  | Location |
 *    +=======+========+==========+
 *    | ``0`` | Blue   | Left     |
 *    +-------+--------+----------+
 *    | ``1`` | Yellow | Top      |
 *    +-------+--------+----------+
 *    | ``2`` | Green  | Right    |
 *    +-------+--------+----------+
 * :param uint8_t value:  Brightness of LED (value between 0 and 31).
 */
API(API_LEDS_SET_ROCKET, void epic_leds_set_rocket(int led, uint8_t value));

/**
 * Turn on the bright side LED which can serve as a flashlight if worn on the left wrist or as a rad tattoo illuminator if worn on the right wrist.
 *
 *:param bool power:  Side LED on if true.
 */
API(API_LEDS_SET_FLASHLIGHT, void epic_set_flashlight(bool power));

/**
 * Set gamma lookup table for individual rgb channels.
 *
 * Since the RGB LEDs' subcolor LEDs have different peak brightness and the
 * linear scaling introduced by PWM is not desireable for color accurate work,
 * custom lookup tables for each individual color channel can be loaded into the
 * Epicardium's memory with this function.
 *
 * :param uint8_t rgb_channel:  Color whose gamma table is to be updated, 0->Red, 1->Green, 2->Blue.
 * :param uint8_t[256] gamma_table: Gamma lookup table. (default = 4th order power function rounded up)
 */
API(API_LEDS_SET_GAMMA_TABLE, void epic_leds_set_gamma_table(
	uint8_t rgb_channel,
	uint8_t *gamma_table
));

/**
 * Set all LEDs to a certain RGB color.
 *
 * :param uint8_t r: Value for the red color channel.
 * :param uint8_t g: Value for the green color channel.
 * :param uint8_t b: Value for the blue color channel.
 */
API(API_LEDS_CLEAR_ALL, void epic_leds_clear_all(uint8_t r, uint8_t g, uint8_t b));

/**
 * Personal State
 * ==============
 * Card10 can display your personal state.
 *
 * If a personal state is set the top-left LED on the bottom side of the
 * harmonics board is directly controlled by epicardium and it can't be
 * controlled by pycardium.
 *
 * To re-enable pycardium control the personal state has to be cleared. To do
 * that simply set it to ``STATE_NONE``.
 *
 * The personal state can be set to be persistent which means it won't get reset
 * on pycardium application change/restart.
 */

/** Possible personal states. */
enum personal_state {
    /** ``0``, No personal state - LED is under regular application control. */
    STATE_NONE = 0,
    /** ``1``, "no contact, please!" - I am overloaded. Please leave me be - red led, continuously on. */
    STATE_NO_CONTACT = 1,
    /** ``2``, "chaos" - Adventure time - blue led, short blink, long blink. */
    STATE_CHAOS = 2,
    /** ``3``, "communication" - want to learn something or have a nice conversation - green led, long blinks. */
    STATE_COMMUNICATION = 3,
    /** ``4``, "camp" - I am focussed on self-, camp-, or community maintenance - yellow led, fade on and off. */
    STATE_CAMP = 4,
};

/**
 * Set the users personal state.
 *
 * Using :c:func:`epic_personal_state_set` an application can set the users personal state.
 *
 * :param uint8_t state: The users personal state. Must be one of :c:type:`personal_state`.
 * :param bool persistent: Indicates whether the configured personal state will remain set and active on pycardium application restart/change.
 * :returns: ``0`` on success, ``-EINVAL`` if an invalid state was requested.
 */
API(API_PERSONAL_STATE_SET, int epic_personal_state_set(uint8_t state,
                                                        bool persistent));

/**
 * Get the users personal state.
 *
 * Using :c:func:`epic_personal_state_get` an application can get the currently set personal state of the user.
 *
 * :returns: A value with exactly one value of :c:type:`personal_state` set.
 */
API(API_PERSONAL_STATE_GET, int epic_personal_state_get());

/**
 * Get whether the users personal state is persistent.
 *
 * Using :c:func:`epic_personal_state_is_persistent` an app can find out whether the users personal state is persistent or transient.
 *
 * :returns: ``1`` if the state is persistent, ``0`` otherwise.
 */
API(API_PERSONAL_STATE_IS_PERSISTENT, int epic_personal_state_is_persistent());

/**
 * Sensor Data Streams
 * ===================
 * A few of card10's sensors can do continuous measurements.  To allow
 * performant access to their data, the following function is made for generic
 * access to streams.
 */

/**
 * Read sensor data into a buffer.  ``epic_stream_read()`` will read as many
 * sensor samples into the provided buffer as possible and return the number of
 * samples written.  If no samples are available, ``epic_stream_read()`` will
 * return ``0`` immediately.
 *
 * ``epic_stream_read()`` expects the provided buffer to have a size which is a
 * multiple of the sample size for the given stream.  For the sample-format and
 * size, please consult the sensors documentation.
 *
 * Before reading the internal sensor sample queue, ``epic_stream_read()`` will
 * call a sensor specific *poll* function to allow the sensor driver to fetch
 * new samples from its hardware.  This should, however, never take a long
 * amount of time.
 *
 * :param int sd: Sensor Descriptor.  You get sensor descriptors as return
 *    values when activating the respective sensors.
 * :param void* buf: Buffer where sensor data should be read into.
 * :param size_t count: How many bytes to read at max.  Note that fewer bytes
 *    might be read.  In most cases, this should be ``sizeof(buf)``.
 * :return: Number of data packets read (**not** number of bytes) or a negative
 *    error value.  Possible errors:
 *
 *    - ``-ENODEV``: Sensor is not currently available.
 *    - ``-EBADF``: The given sensor descriptor is unknown.
 *    - ``-EINVAL``:  ``count`` is not a multiple of the sensor's sample size.
 *    - ``-EBUSY``: The descriptor table lock could not be acquired.
 *
 * **Example**:
 *
 * .. code-block:: cpp
 *
 *    #include "epicardium.h"
 *
 *    struct foo_measurement sensor_data[16];
 *    int foo_sd, n;
 *
 *    foo_sd = epic_foo_sensor_enable(9001);
 *
 *    while (1) {
 *            n = epic_stream_read(
 *                    foo_sd,
 *                    &sensor_data,
 *                    sizeof(sensor_data)
 *            );
 *
 *            // Print out the measured sensor samples
 *            for (int i = 0; i < n; i++) {
 *                    printf("Measured: %?\n", sensor_data[i]);
 *            }
 *    }
 */
API(API_STREAM_READ, int epic_stream_read(int sd, void *buf, size_t count));

/**
 * Vibration Motor
 * ===============
 */

/**
 * Turn vibration motor on or off
 *
 * :param status: 1 to turn on, 0 to turn off.
 */
API(API_VIBRA_SET, void epic_vibra_set(int status));

/**
 * Turn vibration motor on for a given time
 *
 * :param millis: number of milliseconds to run the vibration motor.
 */
API(API_VIBRA_VIBRATE, void epic_vibra_vibrate(int millis));

/**
 * Display
 * =======
 * The card10 has an LCD screen that can be accessed from user code.
 *
 * There are two ways to access the display:
 *
 *  - *immediate mode*, where you ask Epicardium to draw shapes and text for
 *    you.  Most functions in this subsection are related to *immediate mode*.
 *  - *framebuffer mode*, where you provide Epicardium with a memory range where
 *    you already drew graphics whichever way you wanted and Epicardium will
 *    copy them to the display.  To use *framebuffer mode*, use the
 *    :c:func:`epic_disp_framebuffer` function.
 */

/** Line-Style */
enum disp_linestyle {
  /** */
  LINESTYLE_FULL = 0,
  /** */
  LINESTYLE_DOTTED = 1
};

/** Fill-Style */
enum disp_fillstyle {
  /** */
  FILLSTYLE_EMPTY = 0,
  /** */
  FILLSTYLE_FILLED = 1
};

/** Width of display in pixels */
#define DISP_WIDTH 160

/** Height of display in pixels */
#define DISP_HEIGHT 80

/**
 * Framebuffer
 *
 * The frambuffer stores pixels as RGB565, but byte swapped.  That is, for every ``(x, y)`` coordinate, there are two ``uint8_t``\ s storing 16 bits of pixel data.
 *
 * .. todo::
 *
 *    Document (x, y) in relation to chirality.
 *
 * **Example**: Fill framebuffer with red
 *
 * .. code-block:: cpp
 *
 * 	union disp_framebuffer fb;
 * 	uint16_t red = 0b1111100000000000;
 * 	for (int y = 0; y < DISP_HEIGHT; y++) {
 * 		for (int x = 0; x < DISP_WIDTH; x++) {
 * 			fb.fb[y][x][0] = red >> 8;
 * 			fb.fb[y][x][1] = red & 0xFF;
 * 		}
 * 	}
 * 	epic_disp_framebuffer(&fb);
 */
union disp_framebuffer {
  /** Coordinate based access (as shown in the example above). */
  uint8_t fb[DISP_HEIGHT][DISP_WIDTH][2];
  /** Raw byte-indexed access. */
  uint8_t raw[DISP_HEIGHT*DISP_WIDTH*2];
};

/**
 * Locks the display.
 *
 * :return: ``0`` on success or a negative value in case of an error:
 *
 *    - ``-EBUSY``: Display was already locked from another task.
 */
API(API_DISP_OPEN, int epic_disp_open());

/**
 * Unlocks the display again.
 *
 * :return: ``0`` on success or a negative value in case of an error:
 *
 *    - ``-EBUSY``: Display was already locked from another task.
 */
API(API_DISP_CLOSE, int epic_disp_close());

/**
 * Causes the changes that have been written to the framebuffer
 * to be shown on the display
 * :return: ``0`` on success or a negative value in case of an error:
 *
 *    - ``-EBUSY``: Display was already locked from another task.
 */
API(API_DISP_UPDATE, int epic_disp_update());

/**
 * Prints a string into the display framebuffer
 *
 * :param posx: x position to print to. 0 <= x <= 160
 * :param posy: y position to print to. 0 <= y <= 80
 * :param pString: string to print
 * :param fg: foreground color in rgb565
 * :param bg: background color in rgb565
 * :return: ``0`` on success or a negative value in case of an error:
 *
 *    - ``-EBUSY``: Display was already locked from another task.
 */
API(API_DISP_PRINT,
    int epic_disp_print(
	    uint16_t posx,
	    uint16_t posy,
	    const char *pString,
	    uint16_t fg,
	    uint16_t bg)
    );

/**
 * Fills the whole screen with one color
 *
 * :param color: fill color in rgb565
 * :return: ``0`` on success or a negative value in case of an error:
 *
 *    - ``-EBUSY``: Display was already locked from another task.
 */
API(API_DISP_CLEAR, int epic_disp_clear(uint16_t color));

/**
 * Draws a pixel on the display
 *
 * :param x: x position; 0 <= x <= 160
 * :param y: y position; 0 <= y <= 80
 * :param color: pixel color in rgb565
 * :return: ``0`` on success or a negative value in case of an error:
 *
 *    - ``-EBUSY``: Display was already locked from another task.
 */
API(API_DISP_PIXEL,
    int epic_disp_pixel(
	    uint16_t x,
	    uint16_t y,
	    uint16_t color)
    );

/**
 * Draws a line on the display
 *
 * :param xstart: x starting position; 0 <= x <= 160
 * :param ystart: y starting position; 0 <= y <= 80
 * :param xend: x ending position; 0 <= x <= 160
 * :param yend: y ending position; 0 <= y <= 80
 * :param color: line color in rgb565
 * :param linestyle: 0 for solid, 1 for dottet (almost no visual difference)
 * :param pixelsize: thickness of the line; 1 <= pixelsize <= 8
 * :return: ``0`` on success or a negative value in case of an error:
 *
 *    - ``-EBUSY``: Display was already locked from another task.
 */
API(API_DISP_LINE,
    int epic_disp_line(
	    uint16_t xstart,
	    uint16_t ystart,
	    uint16_t xend,
	    uint16_t yend,
	    uint16_t color,
	    enum disp_linestyle linestyle,
	    uint16_t pixelsize)
    );

/**
 * Draws a rectangle on the display
 *
 * :param xstart: x coordinate of top left corner; 0 <= x <= 160
 * :param ystart: y coordinate of top left corner; 0 <= y <= 80
 * :param xend: x coordinate of bottom right corner; 0 <= x <= 160
 * :param yend: y coordinate of bottom right corner; 0 <= y <= 80
 * :param color: line color in rgb565
 * :param fillstyle: 0 for empty, 1 for filled
 * :param pixelsize: thickness of the rectangle outline; 1 <= pixelsize <= 8
 * :return: ``0`` on success or a negative value in case of an error:
 *
 *    - ``-EBUSY``: Display was already locked from another task.
 */
API(API_DISP_RECT,
    int epic_disp_rect(
	    uint16_t xstart,
	    uint16_t ystart,
	    uint16_t xend,
	    uint16_t yend,
	    uint16_t color,
	    enum disp_fillstyle fillstyle,
	    uint16_t pixelsize)
    );

/**
 * Draws a circle on the display
 *
 * :param x: x coordinate of the center; 0 <= x <= 160
 * :param y: y coordinate of the center; 0 <= y <= 80
 * :param rad: radius of the circle
 * :param color: fill and outline color of the circle (rgb565)
 * :param fillstyle: 0 for empty, 1 for filled
 * :param pixelsize: thickness of the circle outline; 1 <= pixelsize <= 8
 * :return: ``0`` on success or a negative value in case of an error:
 *
 *    - ``-EBUSY``: Display was already locked from another task.
 */
API(API_DISP_CIRC,
    int epic_disp_circ(
	    uint16_t x,
	    uint16_t y,
	    uint16_t rad,
	    uint16_t color,
	    enum disp_fillstyle fillstyle,
	    uint16_t pixelsize)
    );

/**
 * Immediately send the contents of a framebuffer to the display. This overrides
 * anything drawn by immediate mode graphics and displayed using ``epic_disp_update``.
 *
 * :param fb: framebuffer to display
 * :return: ``0`` on success or negative value in case of an error:
 *
 *    - ``-EBUSY``: Display was already locked from another task.
 */
API(API_DISP_FRAMEBUFFER, int epic_disp_framebuffer(union disp_framebuffer *fb));


/**
 * Start continuous readout of the light sensor. Will read light level
 * at preconfigured interval and make it available via `epic_light_sensor_get()`.
 *
 * If the continuous readout was already running, this function will silently pass.
 *
 *
 * :return: `0` if the start was successful or a negative error value
 *      if an error occured. Possible errors:
 *
 *      - ``-EBUSY``: The timer could not be scheduled.
 */
API(API_LIGHT_SENSOR_RUN, int epic_light_sensor_run());

/**
 * Get the last light level measured by the continuous readout.
 *
 * :param uint16_t* value: where the last light level should be written.
 * :return: `0` if the readout was successful or a negative error
 *      value. Possible errors:
 *
 *      - ``-ENODATA``: Continuous readout not currently running.
 */
API(API_LIGHT_SENSOR_GET, int epic_light_sensor_get(uint16_t* value));


/**
 * Stop continuous readout of the light sensor.
 *
 * If the continuous readout wasn't running, this function will silently pass.
 *
 * :return: `0` if the stop was sucessful or a negative error value
 *      if an error occured. Possible errors:
 *
 *      - ``-EBUSY``: The timer stop could not be scheduled.
 */
API(API_LIGHT_SENSOR_STOP, int epic_light_sensor_stop());

/**
 * File
 * ====
 * Except for :c:func:`epic_file_open`, which models C stdio's ``fopen``
 * function, ``close``, ``read`` and ``write`` model `close(2)`_, `read(2)`_ and
 * `write(2)`_.  All file-related functions return >= ``0`` on success and
 * ``-Exyz`` on failure, with error codes from errno.h (``EIO``, ``EINVAL``
 * etc.)
 *
 * .. _close(2): http://man7.org/linux/man-pages/man2/close.2.html
 * .. _read(2): http://man7.org/linux/man-pages/man2/read.2.html
 * .. _write(2): http://man7.org/linux/man-pages/man2/write.2.html
 */

/** */
API(API_FILE_OPEN, int epic_file_open(
	const char* filename, const char* modeString
));

/** */
API(API_FILE_CLOSE, int epic_file_close(int fd));

/** */
API(API_FILE_READ, int epic_file_read(int fd, void* buf, size_t nbytes));

/**
 * Write bytes to a file.
 *
 * :param int fd: Descriptor returned by :c:func:`epic_file_open`.
 * :param void* buf: Data to write.
 * :param size_t nbytes: Number of bytes to write.
 *
 * :return: ``< 0`` on error, ``nbytes`` on success. (Partial writes don't occur on success!)
 *
*/
API(
	API_FILE_WRITE,
	int epic_file_write(int fd, const void* buf, size_t nbytes)
);

/** */
API(API_FILE_FLUSH, int epic_file_flush(int fd));

/** */
API(API_FILE_SEEK, int epic_file_seek(int fd, long offset, int whence));

/** */
API(API_FILE_TELL, int epic_file_tell(int fd));

/** */
enum epic_stat_type {
	/**
	 * Basically ``ENOENT``. Although :c:func:`epic_file_stat` returns an
	 * error for 'none', the type will still be set to none additionally.
	 *
	 * This is also used internally to track open FS objects, where we use
	 * ``EPICSTAT_NONE`` to mark free objects.
	 */
	EPICSTAT_NONE,
	/** normal file */
	EPICSTAT_FILE,
	/** directory */
	EPICSTAT_DIR,
};

/**
 * Maximum length of a path string (=255).
 */
#define EPICSTAT_MAX_PATH        255
/* conveniently the same as FF_MAX_LFN */

/** */
struct epic_stat {
	/** Entity Type: file, directory or none */
	enum epic_stat_type type;

	/*
	 * Note about padding & placement of uint32_t size:
	 *
	 *   To accomodate for future expansion, we want padding at the end of
	 *   this struct. Since sizeof(enum epic_stat_type) can not be assumed
	 *   to be have a certain size, we're placing uint32_t size here so we
	 *   can be sure it will be at offset 4, and therefore the layout of the
	 *   other fields is predictable.
	 */

	/** Size in bytes. */
	uint32_t size;

	/** File Name. */
	char name[EPICSTAT_MAX_PATH + 1];
	uint8_t _reserved[12];
};

/**
 * stat path
 *
 * :param char* filename: path to stat
 * :param epic_stat* stat: pointer to result
 *
 * :return: ``0`` on success, negative on error
 */
API(API_FILE_STAT, int epic_file_stat(
	const char* path, struct epic_stat* stat
));

/**
 * Open a directory, for enumerating its contents.
 *
 * Use :c:func:`epic_file_readdir` to iterate over the directories entries.
 *
 * **Example**:
 *
 * .. code-block:: cpp
 *
 *    #include "epicardium.h"
 *
 *    int fd = epic_file_opendir("/path/to/dir");
 *
 *    struct epic_stat entry;
 *    for (;;) {
 *            epic_file_readdir(fd, &entry);
 *
 *            if (entry.type == EPICSTAT_NONE) {
 *                    // End
 *                    break;
 *            }
 *
 *            printf("%s\n", entry.name);
 *    }
 *
 *    epic_file_close(fd);
 *
 * :param char* path: Directory to open.
 *
 * :return: ``> 0`` on success, negative on error
 */
API(API_FILE_OPENDIR, int epic_file_opendir(const char* path));

/**
 * Read one entry from a directory.
 *
 * Call :c:func:`epic_file_readdir` multiple times to iterate over all entries
 * of a directory.  The end of the entry list is marked by returning
 * :c:data:`EPICSTAT_NONE` as the :c:member:`epic_stat.type`.
 *
 * :param int fd: Descriptor returned by :c:func:`epic_file_opendir`.
 * :param epic_stat* stat: Pointer where to store the result.  Pass NULL to
 *    reset iteration offset of ``fd`` back to the beginning.
 *
 * :return: ``0`` on success, negative on error
 */
API(API_FILE_READDIR, int epic_file_readdir(int fd, struct epic_stat* stat));

/**
 * Unlink (remove) a file.
 *
 * :param char* path: file to delete
 *
 * :return: ``0`` on success, negative on error
 */
API(API_FILE_UNLINK, int epic_file_unlink(const char* path));

/**
 * Rename a file or directory.
 *
 * :param char* oldp: old name
 * :param char* newp: new name
 *
 * :return: ``0`` on success, negative on error
 */
API(API_FILE_RENAME, int epic_file_rename(const char *oldp, const char* newp));

/**
 * Create directory in CWD
 *
 * :param char* dirname: directory name
 *
 * :return: ``0`` on success, negative on error
 */
API(API_FILE_MKDIR, int epic_file_mkdir(const char *dirname));

/**
 * RTC
 * ===
 */

/**
 * Read the current RTC value.
 *
 * :return: Unix time in seconds
 */
API(API_RTC_GET_SECONDS, uint32_t epic_rtc_get_seconds(void));

/**
 * Sets the current RTC time in milliseconds
 */
API(API_RTC_SET_MILLISECONDS, void epic_rtc_set_milliseconds(uint64_t milliseconds));

/**
 * Schedule the RTC alarm for the given timestamp.
 *
 * :param uint32_t timestamp: When to schedule the IRQ
 * :return: `0` on success or a negative value if an error occured. Possible
 *    errors:
 *
 *    - ``-EINVAL``: RTC is in a bad state
 */
API(API_RTC_SCHEDULE_ALARM, int epic_rtc_schedule_alarm(uint32_t timestamp));

/**
 * **Interrupt Service Routine**
 *
 * ``epic_isr_rtc_alarm()`` is called when the RTC alarm triggers.  The RTC alarm
 * can be scheduled using :c:func:`epic_rtc_schedule_alarm`.
 */
API_ISR(EPIC_INT_RTC_ALARM, epic_isr_rtc_alarm);

/**
 * TRNG
 * ====
 */

/**
 * Read random bytes from the TRNG.
 *
 * :param uint8_t * dest: Destination buffer
 * :param size: Number of bytes to read.
 * :return: `0` on success or a negative value if an error occured. Possible
 *    errors:
 *
 *    - ``-EFAULT``: Invalid destination address.
 */
API(API_TRNG_READ, int epic_trng_read(uint8_t *dest, size_t size));

#endif /* _EPICARDIUM_H */
