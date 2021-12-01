#include "gpio_app.h"

static void button_handler(uint8_t pin_no, uint8_t button_action);
static void led_handler(void);
static void button_timeout_handler(void * p_context);
static void led_timeout_handler(void * p_context);

static void app_gpio_pin_set(uint8_t m_pin);
static void app_gpio_pin_clear(uint8_t m_pin);

static push_state_t pushed_state = RELEASED_BUTTON;
static uint8_t pre_cooling_level = 0;
static uint8_t cooling_level = 0;

uint8_t cooling_level_supported = COOLER_MODE_SUPPORTED;

APP_TIMER_DEF(m_button_timer_id);       /**< Button timer. */
APP_TIMER_DEF(m_leds_timer_id);         /**< LEDs config timer. */

/*
 * Handler to be called when button is pushed.
 * param[in]   pin_no 			The pin number where the event is genereated
 * param[in]   button_action 	Is the button pushed or released
 */
static void button_handler(uint8_t pin_no, uint8_t button_action)
{
    /**< Pin number of a currently pushed button, that could become a long push if held long enough. */
    static uint8_t current_push_pin_no;
    switch (button_action)
    {
        case APP_BUTTON_PUSH:
            NRF_LOG_INFO("Push Button");
            current_push_pin_no = pin_no;
            (void)app_timer_start(m_button_timer_id, APP_TIMER_TICKS(BUTTON_PUSH_TIMEOUT_MS), (void*)&current_push_pin_no);
            if(pushed_state == SHORT_PUSHED_BUTTON) {
                
            }
            break;

        case APP_BUTTON_RELEASE:
            NRF_LOG_INFO("Release Button");
            (void)app_timer_stop(m_button_timer_id);
            if(cooling_level < cooling_level_supported) {
                cooling_level++;
            } else {  // roll back mode 1
                cooling_level = 1;
            }
            break;
        case APP_BUTTON_SHORT_PUSH:
            NRF_LOG_INFO("Push Short");
            pushed_state = SHORT_PUSHED_BUTTON;
            advertising_start();
            (void)app_timer_start(m_leds_timer_id, APP_TIMER_TICKS(LED_COOLING_LEVEL_SETTING_MS), NULL);
            break;
    }
}

/**@brief Handle events from led configuration timer.
 *
 * @param[in]   p_context   parameter registered in timer start function, unused
 */
static void led_handler()
{
    // ble advertise indicate 
    nrf_gpio_pin_toggle(LED_PIN_2);
}

/**@brief Handle events from button timeout.
 *
 * @param[in]   p_context   parameter registered in timer start function.
 */
void button_timeout_handler(void * p_context)
{
    UNUSED_PARAMETER(p_context);
	button_handler(*(uint8_t *)p_context, APP_BUTTON_SHORT_PUSH);
}

/**@brief Handle events from led timeout.
 *
 * @param[in]   p_context   parameter registered in timer start function.
 */
void led_timeout_handler(void * p_context)
{
    UNUSED_PARAMETER(p_context);
	led_handler();
}

static void app_gpio_pin_set(uint8_t m_pin)
{
    //low-level active
    #if(CFG_LED_ACTIVE_LOW)
	nrf_gpio_pin_clear(m_pin);
    #else
    nrf_gpio_pin_set(m_pin);
    #endif
}

static void app_gpio_pin_clear(uint8_t m_pin)
{
	//low-level active
    #if(CFG_LED_ACTIVE_LOW)
	nrf_gpio_pin_set(m_pin);
    #else
    nrf_gpio_pin_clear(m_pin);
    #endif
}

void app_led_blink()
{
    (void)app_timer_start(m_leds_timer_id, APP_TIMER_TICKS(LED_PAIR_TIMEOUT_MS), NULL);
}

void app_led_on()
{
    (void)app_timer_stop(m_leds_timer_id);
    app_gpio_pin_set(LED_PIN_2);
}

void app_led_off()
{
    (void)app_timer_stop(m_leds_timer_id);
    app_gpio_pin_clear(LED_PIN_2);
}


/**@brief Function for initializing LEDs. */
void gpio_configure(void)
{
	ret_code_t err_code;

	/*init LED*/
    // nrf_gpio_cfg_output(LED_PIN_1);
    // app_gpio_pin_clear(LED_PIN_1);
    nrf_gpio_cfg_output(LED_PIN_2);
    app_gpio_pin_clear(LED_PIN_2);

    nrf_gpio_cfg_output(HBRIGE_NSLEEP_PIN);
    nrf_gpio_pin_clear(HBRIGE_NSLEEP_PIN);

    // Button configuration structure.
    static app_button_cfg_t p_button[INPUT_PIN_MAX_NUMBER] =  {
                                                            {BUTTON_PIN_1, APP_BUTTON_ACTIVE_LOW, NRF_GPIO_PIN_PULLUP, button_handler},
                                                        	};

    // Initializing the buttons.
    err_code = app_button_init(p_button, sizeof(p_button) / sizeof(p_button[0]), APP_TIMER_TICKS(BUTTON_DEBOUNCE_DELAY));
    APP_ERROR_CHECK(err_code);

    // Enabling the buttons.
    err_code = app_button_enable();
    APP_ERROR_CHECK(err_code);

    // Create configuration led timer.
    err_code = app_timer_create(&m_leds_timer_id,
                                APP_TIMER_MODE_REPEATED,
                                led_timeout_handler);

    // Create button timer.
    err_code = app_timer_create(&m_button_timer_id,
    							APP_TIMER_MODE_SINGLE_SHOT,
                                button_timeout_handler);
}
