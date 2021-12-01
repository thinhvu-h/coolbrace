#ifndef GPIO_APP_H_
#define GPIO_APP_H_

#include <string.h>
#include "app_timer.h"
#include "app_button.h"
#include "nrf_gpio.h"
#include "nrf_drv_gpiote.h"
#include "nrf_drv_ppi.h"
#include "nrfx_ppi.h"
#include "app_pwm.h"

#include "nrf_log.h"
#include "ble_app.h"

// LED DEFINE
#define CFG_LED_ACTIVE_LOW              1   //active low, setting = 1
#define LED_PIN_1                       0
#define LED_PIN_2                       1
#define LED_PAIR_TIMEOUT_MS             500
#define LED_SYNC_TICK                   6   // tick remain to timeout point, clear LED to blink synchronously

#define LED_COOLING_LEVEL_SETTING_MS    500


// BUTTON DEFINE
#define INPUT_PIN_MAX_NUMBER             1
#define BUTTON_PIN_1                    21
#define BUTTON_PUSH_TIMEOUT_MS          2000    /**< The time to hold for a long push (in milliseconds). */
#define BUTTON_DEBOUNCE_DELAY           50      /**<  Delay from a GPIOTE event until a button is reported as pushed. */

#define PAIRABLE_ADV_TIMEOUT            12000u  /** Time for which the device must be advertising in pairing mode (in seconds). */

#define APP_BUTTON_SHORT_PUSH    2                      /**< Indicates that a button is short push. */
#define APP_BUTTON_LONG_PUSH     3                      /**< Indicates that a button is long push. */

#define FAN_PIN                         20
#define HBRIGE_NSLEEP_PIN               14
#define HBRIGE_COOL_PIN                 18
#define HBRIGE_WARM_PIN                 4

#define COOLER_MODE_SUPPORTED    3
#define HEATER_MODE_SUPPORTED    2

typedef enum {
    RELEASED_BUTTON,
    CLICKED_BUTTON,
    SHORT_PUSHED_BUTTON,
} push_state_t;

typedef enum {
  BLINK_TURN_ON,
  BLINK_CHANGE_MODE,
  BLINK_CHANGE_POWER_LEVEL,
  BLINK_TURN_OFF
} blink_state_t;

extern void gpio_configure(void);
extern void app_led_blink(void);
extern void app_led_on(void);
extern void app_led_off(void);

extern void pwm_app_init(void);
extern void cooling_handler(uint32_t cfg_data);

#endif /*GPIO_APP_H_*/
