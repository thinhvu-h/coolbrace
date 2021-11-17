#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "nordic_common.h"
#include "bsp.h"
#include "nrf_soc.h"
#include "nrf_sdh.h"
#include "nrf_sdh_soc.h"
#include "nrf_sdh_ble.h"

#include "app_timer.h"
#include "nrf_gpio.h"
#include "nrf_drv_gpiote.h"
#include "nrf_drv_ppi.h"

#include "nrf_pwr_mgmt.h"
#include "ble_app.h"
#include "twi_nrf52.h"
#include "SEGGER_RTT.h"

APP_TIMER_DEF(m_battery_timer_id);                                                  /**< Battery timer. */
APP_TIMER_DEF(m_temperature_timer_id);                                              /**< Temperature timer. */
APP_TIMER_DEF(m_notification_timer_id);

/**@brief Function for starting application timers.
 */
static void application_timers_start(void)
{
    ret_code_t err_code;

    // Start battery timers.
    err_code = app_timer_start(m_battery_timer_id, BATTERY_LEVEL_MEAS_INTERVAL, NULL);
    APP_ERROR_CHECK(err_code);

    // Start temperature timers.
    err_code = app_timer_start(m_temperature_timer_id, TEMPERATURE_MEAS_INTERVAL, NULL);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for doing power management. */
static void idle_state_handle(void)
{
    nrf_pwr_mgmt_run();
}

/**@brief Function for initializing the power management module. */
static void pwr_mgmt_init(void)
{
    ret_code_t err_code = nrf_pwr_mgmt_init();
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for the Timer initialization.
 *
 * @details Initializes the timer module. This creates and starts application timers.
 */
static void timers_init(void)
{
    ret_code_t err_code;

    // Initialize timer module.
    err_code = app_timer_init();
    APP_ERROR_CHECK(err_code);

    // Create timers.
    // err_code = app_timer_create(&m_notification_timer_id, 
    //                             APP_TIMER_MODE_REPEATED, 
    //                             notification_timeout_handler);
    APP_ERROR_CHECK(err_code);

    // Create battery timers.
    err_code = app_timer_create(&m_battery_timer_id, 
                                APP_TIMER_MODE_REPEATED, 
                                battery_timeout_handler);
    APP_ERROR_CHECK(err_code);

    // Create temperature timers.
    err_code = app_timer_create(&m_temperature_timer_id, 
                                APP_TIMER_MODE_REPEATED, 
                                temperature_timeout_handler);
    APP_ERROR_CHECK(err_code);
}

/**
 * @brief Function for application main entry.
 */

int main(void)
{
    bool erase_bonds;

    pwr_mgmt_init();
    twi_master_init();
    battery_init();

    timers_init();
    buttons_leds_init(&erase_bonds);
    ble_stack_init();
    gap_params_init();
    gatt_init();
    advertising_init();
    services_init();
    sensor_simulator_init();
    conn_params_init();
    peer_manager_init();

    application_timers_start();
    advertising_start(erase_bonds);
    SEGGER_RTT_printf(0, "\nStart bluetooth...\n");

	while(1)
	{
		idle_state_handle();
	}
}
