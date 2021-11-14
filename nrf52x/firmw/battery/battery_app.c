#include "battery_app.h"

// STATIC VARIABLE
static nrf_saadc_value_t adc_buf[1] = {0};
static uint16_t batt_lvl_mV = 0;
static uint8_t batt_lvl_percent = 0;

// STATIC FUNCTION
static void saadc_batt_event_handler(nrf_drv_saadc_evt_t const * p_event);

void battery_level_update(void) {

}

/**@brief Function for handling the ADC interrupt.
 *
 * @details  This function will fetch the conversion result from the ADC, convert the value into
 *           percentage and send it to peer.
 */
static void saadc_batt_event_handler(nrf_drv_saadc_evt_t const * p_event)
{
    if (p_event->type == NRF_DRV_SAADC_EVT_DONE)
    {
    	uint32_t          err_code;
        nrf_saadc_value_t adc_result;

        adc_result = p_event->data.done.p_buffer[0];

        err_code = nrf_drv_saadc_buffer_convert(p_event->data.done.p_buffer, 1);
        APP_ERROR_CHECK(err_code);

        batt_lvl_mV = ADC_RESULT_IN_MILLI_VOLTS(adc_result);

        batt_lvl_percent = (batt_lvl_mV - BATT_LOW_THRESHOLD)*100/(BATT_HIG_THRESHOLD - BATT_LOW_THRESHOLD);

        battery_level_update();
    }
}

/**@brief Function for configuring ADC to do battery level conversion.
 */
void battery_init(void)
{
    ret_code_t err_code;

    // VDD as input.
    nrf_saadc_channel_config_t config =
        NRF_DRV_SAADC_DEFAULT_CHANNEL_CONFIG_SE(NRF_SAADC_INPUT_VDD);

    err_code = nrf_drv_saadc_init(NULL, saadc_batt_event_handler);
    APP_ERROR_CHECK(err_code);

    err_code = nrf_drv_saadc_channel_init(0, &config);
    APP_ERROR_CHECK(err_code);

    err_code = nrf_drv_saadc_buffer_convert(&adc_buf[0], 1);
    APP_ERROR_CHECK(err_code);

    err_code = nrf_drv_saadc_sample();
    APP_ERROR_CHECK(err_code);
}
