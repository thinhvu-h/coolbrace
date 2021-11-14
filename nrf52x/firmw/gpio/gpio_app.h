#ifndef GPIO_APP_H_
#define GPIO_APP_H_

// LED DEFINE
#define CFG_LED_ACTIVE_LOW               1   //active low, setting = 1
#define LED_RED_PIN               		12
#define LED_BLUE_PIN               		15

// BUTTON DEFINE
#define INPUT_PIN_MAX_NUMBER              1
#define BUTTON_PIN_1                    10
#define BUTTON_PUSH_TIMEOUT_MS          2000    /**< The time to hold for a long push (in milliseconds). */
#define BUTTON_DEBOUNCE_DELAY           50      /**<  Delay from a GPIOTE event until a button is reported as pushed. */

#endif /*GPIO_APP_H_*/
