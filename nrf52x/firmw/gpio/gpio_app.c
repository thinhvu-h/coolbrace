#include <string.h>
#include "gpio_app.h"

/*
 * Handler to be called when button is pushed.
 * param[in]   pin_no 			The pin number where the event is genereated
 * param[in]   button_action 	Is the button pushed or released
 */
void button_handler(uint8_t pin_no, uint8_t button_action)
{
    /**< Pin number of a currently pushed button, that could become a long push if held long enough. */
    static uint8_t current_push_pin_no;

    switch (button_action)
    {
        case APP_BUTTON_PUSH:
            current_push_pin_no = pin_no;
            (void)app_timer_start(m_button_timer_id, APP_TIMER_TICKS(BUTTON_PUSH_TIMEOUT_MS), (void*)&current_push_pin_no);
            if(button_pushed_state == SHORT_PUSHED_BUTTON) {
                num_clicked++;
            }
            break;

        case APP_BUTTON_RELEASE:
            (void)app_timer_stop(m_button_timer_id);
            //if previous state is shorted pushed
            if((button_pushed_state == SHORT_PUSHED_BUTTON) & (!fds_write_enable))
            {
                switch (num_clicked) {
                    case 1:
                        app_gpio_pin_clear(LED_RED_PIN);
                        app_gpio_pin_set(LED_BLUE_PIN);
                        ble_cfg_param.ble_tx_power = CONFIG_SHORT_RANGE;
                        break;
                    case 2:
                        app_gpio_pin_set(LED_RED_PIN);
                        app_gpio_pin_clear(LED_BLUE_PIN);
                        ble_cfg_param.ble_tx_power = CONFIG_MEDIUM_RANGE;
                        break;
                    case 3:
                        //active high
                        app_gpio_pin_set(LED_RED_PIN);
                        app_gpio_pin_set(LED_BLUE_PIN);
                        ble_cfg_param.ble_tx_power = CONFIG_LONG_RANGE;
                        break;
                    default:
                        app_gpio_pin_clear(LED_RED_PIN);
                        app_gpio_pin_clear(LED_BLUE_PIN);
                        num_clicked = 0;
                        break;
                }
            }
            else {
                button_pushed_state = CLICKED_BUTTON;
                num_clicked = 0;
                app_gpio_pin_clear(LED_RED_PIN);
                app_gpio_pin_clear(LED_BLUE_PIN);
            }
            break;
        case APP_BUTTON_ACTION_SHORT_PUSH:
            /*stop adv to config*/
            advertising_stop();
            button_pushed_state = SHORT_PUSHED_BUTTON;
            (void)app_timer_start(m_leds_cfg_timer_id, APP_TIMER_TICKS(LED_PAIR_TIMEOUT_MS), NULL);
            fds_write_enable = false;
            break;
    }
}

/**@brief Function for initializing BUTTONS & LEDs. */
void button_led_init(void)
{
	ret_code_t err_code;

	/*init output*/
    nrf_gpio_cfg_output(LED_RED_PIN);
	app_gpio_pin_clear(LED_RED_PIN);
    nrf_gpio_cfg_output(LED_BLUE_PIN);
	app_gpio_pin_clear(LED_BLUE_PIN);

    // Button configuration structure.
    static app_button_cfg_t p_button[INPUT_PIN_MAX_NUMBER] =  {
                                                            {BUTTON_PIN_1, APP_BUTTON_ACTIVE_LOW, NRF_GPIO_PIN_PULLUP, button_handler},
                                                            {MAGNETIC_SENSOR_PIN, MAGNETIC_SENSOR_OPENED_ACTIVE, NRF_GPIO_PIN_PULLUP, door_sensor_handler}
                                                            };

    // Initializing the buttons.
    err_code = app_button_init(p_button, sizeof(p_button) / sizeof(p_button[0]), APP_TIMER_TICKS(BUTTON_DEBOUNCE_DELAY));
    APP_ERROR_CHECK(err_code);

    // Enabling the buttons.
    err_code = app_button_enable();
    APP_ERROR_CHECK(err_code);
}
