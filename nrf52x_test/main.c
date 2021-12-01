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
#include "twi_nrf52.h"

#include "ble_app.h"
#include "ble_cus.h"

#include "gpio_app.h"
#include "battery_app.h"
#include "temperature_app.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

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
    if (NRF_LOG_PROCESS() == false)
    {
        nrf_pwr_mgmt_run();
    }
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

static void power_management_init(void)
{
    uint32_t err_code = nrf_pwr_mgmt_init();
    APP_ERROR_CHECK(err_code);
}

static void log_init(void)
{
    ret_code_t err_code = NRF_LOG_INIT(NULL);
    APP_ERROR_CHECK(err_code);

    NRF_LOG_DEFAULT_BACKENDS_INIT();
}

/**
 * @brief Function for application main entry.
 */

int main(void)
{
    NRF_LOG_INFO("### START COOLBRACE ###");
    // ret_code_t err_code;

    log_init();
    twi_master_init();
    battery_init();

    // Initialize the async SVCI interface to bootloader before any interrupts are enabled.
    // err_code = ble_dfu_buttonless_async_svci_init();
    // APP_ERROR_CHECK(err_code);

    timers_init();
    gpio_configure();
    pwm_app_init();
    power_management_init();
    ble_stack_init();
    gap_params_init();
    gatt_init();
    advertising_init();
    services_init();
    conn_params_init();
    peer_manager_init();

    application_timers_start();
    advertising_start();
    NRF_LOG_INFO("Start bluetooth...");

	while(1)
	{
		idle_state_handle();
	}
}
