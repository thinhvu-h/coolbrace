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
#include "nrf_drv_pwm.h"

// LED DEFINE
#define CFG_LED_ACTIVE_LOW               1   //active low, setting = 1
#define LED_PIN_1               		17
#define LED_PIN_2               		18
#define LED_PIN_3               		19
#define LED_PIN_4               		20
#define LED_PAIR_TIMEOUT_MS     		500
#define LED_SYNC_TICK                   6   // tick remain to timeout point, clear LED to blink synchronously

// BUTTON DEFINE
#define INPUT_PIN_MAX_NUMBER             1
#define BUTTON_PIN_1                    13
#define BUTTON_PIN_2                    14
#define BUTTON_PIN_3                    15
#define BUTTON_PIN_4                    16
#define BUTTON_PUSH_TIMEOUT_MS          2000    /**< The time to hold for a long push (in milliseconds). */
#define BUTTON_DEBOUNCE_DELAY           50      /**<  Delay from a GPIOTE event until a button is reported as pushed. */

#define PAIRABLE_ADV_TIMEOUT            12000u  /** Time for which the device must be advertising in pairing mode (in seconds). */

typedef enum {
    RELEASED_BUTTON,
    CLICKED_BUTTON,
    SHORT_PUSHED_BUTTON,
} push_state_t;

extern void gpio_configure(void);
extern void pwm_init(void);
extern void cooling_user_config(uint32_t temperature);

#endif /*GPIO_APP_H_*/
