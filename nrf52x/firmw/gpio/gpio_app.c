#include "gpio_app.h"
#include "SEGGER_RTT.h"

static void app_gpio_pin_set(uint8_t m_pin);
static void app_gpio_pin_clear(uint8_t m_pin);
static void button_handler(uint8_t pin_no, uint8_t button_action);
static void led_handler(void);
static void button_timeout_handler(void * p_context);
static void led_timeout_handler(void * p_context);

static push_state_t button_pushed_state = RELEASED_BUTTON;
static uint8_t num_clicked = 0;

APP_TIMER_DEF(m_button_timer_id);           /**< Button timer. */
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
            SEGGER_RTT_printf(0, "\nPush Button...\n");
            current_push_pin_no = pin_no;
            (void)app_timer_start(m_button_timer_id, APP_TIMER_TICKS(BUTTON_PUSH_TIMEOUT_MS), (void*)&current_push_pin_no);
            if(button_pushed_state == SHORT_PUSHED_BUTTON) {
                num_clicked++;
            }
            break;

        case APP_BUTTON_RELEASE:
            SEGGER_RTT_printf(0, "\nRelease Button...\n");
            (void)app_timer_stop(m_button_timer_id);
            //if previous state is shorted pushed
            if(button_pushed_state == SHORT_PUSHED_BUTTON)
            {
                switch (num_clicked) {
                    case 1:
                        app_gpio_pin_clear(LED_PIN_3);
                        app_gpio_pin_set(LED_PIN_4);
                        break;
                    case 2:
                        app_gpio_pin_set(LED_PIN_3);
                        app_gpio_pin_clear(LED_PIN_4);
                        break;
                    case 3:
                        //active high
                        app_gpio_pin_set(LED_PIN_3);
                        app_gpio_pin_set(LED_PIN_4);
                        break;
                    default:
                        app_gpio_pin_clear(LED_PIN_3);
                        app_gpio_pin_clear(LED_PIN_4);
                        num_clicked = 0;
                        break;
                }
            }
            else {
                button_pushed_state = CLICKED_BUTTON;
                num_clicked = 0;
                app_gpio_pin_clear(LED_PIN_3);
                app_gpio_pin_clear(LED_PIN_4);
            }
            break;
        case APP_BUTTON_ACTION_SHORT_PUSH:
            button_pushed_state = SHORT_PUSHED_BUTTON;
            (void)app_timer_start(m_leds_timer_id, APP_TIMER_TICKS(LED_PAIR_TIMEOUT_MS), NULL);
            break;
    }
}

/**@brief Handle events from led configuration timer.
 *
 * @param[in]   p_context   parameter registered in timer start function, unused
 */
static void led_handler()
{
    static uint8_t LED_sync_status = 0;
    static uint16_t num_tick = 0;
    num_tick++;

    // number of tick in amount of time
    if(num_tick >= (PAIRABLE_ADV_TIMEOUT/LED_PAIR_TIMEOUT_MS)) {
    	num_tick = 0;
        (void)app_timer_stop(m_leds_timer_id);
        // turn off LED
        app_gpio_pin_clear(LED_PIN_3);
        app_gpio_pin_clear(LED_PIN_4);
        button_pushed_state = RELEASED_BUTTON;
        //reset clicked state & indicator
        num_clicked = 0;
        LED_sync_status = 0;
        return;
    }
    else if((num_tick >= (PAIRABLE_ADV_TIMEOUT/LED_PAIR_TIMEOUT_MS - LED_SYNC_TICK)) &
            (num_tick < PAIRABLE_ADV_TIMEOUT/LED_PAIR_TIMEOUT_MS))
    {
        //enable flash, start to write to flash, prevent changes at last minute
        LED_sync_status = !LED_sync_status;
        nrf_gpio_pin_write(LED_PIN_3,LED_sync_status);
        nrf_gpio_pin_write(LED_PIN_4,LED_sync_status);
    }
    else {
        //if not clicked
        if(num_clicked == 0){
        	nrf_gpio_pin_toggle(LED_PIN_3);
            nrf_gpio_pin_toggle(LED_PIN_4);
        }
    }
}

/**@brief Handle events from button timeout.
 *
 * @param[in]   p_context   parameter registered in timer start function.
 */
void button_timeout_handler(void * p_context)
{
    UNUSED_PARAMETER(p_context);
	button_handler(*(uint8_t *)p_context, APP_BUTTON_ACTION_SHORT_PUSH);
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

/**@brief Function for initializing LEDs. */
void gpio_configure(void)
{
	ret_code_t err_code;

	/*init output*/
    nrf_gpio_cfg_output(LED_PIN_1);
	app_gpio_pin_clear(LED_PIN_1);
    nrf_gpio_cfg_output(LED_PIN_2);
	app_gpio_pin_clear(LED_PIN_2);
    nrf_gpio_cfg_output(LED_PIN_3);
	app_gpio_pin_clear(LED_PIN_3);
    nrf_gpio_cfg_output(LED_PIN_4);
	app_gpio_pin_clear(LED_PIN_4);

    // Button configuration structure.
    static app_button_cfg_t p_button[INPUT_PIN_MAX_NUMBER] =  {
                                                            {BUTTON_PIN_2, APP_BUTTON_ACTIVE_LOW, NRF_GPIO_PIN_PULLUP, button_handler},
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