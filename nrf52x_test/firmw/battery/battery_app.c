#include "battery_app.h"

#include "nrf_log.h"

// STATIC VARIABLE
static nrf_saadc_value_t adc_buf;
static uint16_t batt_lvl_in_milli_volts = 0;

// STATIC FUNCTION
static void saadc_batt_event_handler(nrf_drv_saadc_evt_t const * p_event);

static uint8_t battery_level_avr[5] = {0};
uint8_t battery_level_percent(uint16_t mvolts) {
    
    static uint8_t battery_index = 0;
    uint16_t arr_size = sizeof(battery_level_avr) / sizeof(battery_level_avr[0]);
    uint16_t battery_level = 0;
    
    battery_level_avr[battery_index] = ((mvolts - BATT_LOWER_THRESHOLD)*100/(BATT_UPPER_THRESHOLD - BATT_LOWER_THRESHOLD));
    
    // NRF_LOG_INFO("battery_level_avr: ");
    // for (int i = 0 ; i <= battery_index; i++) {
    //     NRF_LOG_INFO(" %d", battery_level_avr[i]);
    // }

    for (int i = 0 ; i <= battery_index; i++) {
        battery_level += battery_level_avr[i];
    }
    // NRF_LOG_INFO("sum of battery_level %d, battery index: %d", battery_level, battery_index);
    battery_level = battery_level/(battery_index+1);

    battery_index++;
    if(battery_index > arr_size-1) {
        memset(battery_level_avr, 0, sizeof(battery_level_avr));
        battery_index = 0;
    }
    
    if (battery_level>100) battery_level = 100;

    return battery_level;
}

void battery_voltage_get(uint16_t * p_vbatt) {
    *p_vbatt = batt_lvl_in_milli_volts;
    if (!nrf_drv_saadc_is_busy())
    {
        ret_code_t err_code = nrf_drv_saadc_buffer_convert(&adc_buf, 1);
        APP_ERROR_CHECK(err_code);

        err_code = nrf_drv_saadc_sample();
        APP_ERROR_CHECK(err_code);
    }
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
        nrf_saadc_value_t adc_result;

        adc_result = p_event->data.done.p_buffer[0];

        batt_lvl_in_milli_volts = ADC_RESULT_IN_MILLI_VOLTS(adc_result) + DIODE_FWD_VOLT_DROP_MILLIVOLTS;
        // NRF_LOG_INFO("ADC reading - ADC:%d,  In Millivolts: %d", adc_result, batt_lvl_in_milli_volts);
    }
}

/**@brief Function for configuring ADC to do battery level conversion.
 */
void battery_init(void)
{
    ret_code_t err_code;

    // VDD as input.
    nrf_saadc_channel_config_t config =
        NRF_DRV_SAADC_DEFAULT_CHANNEL_CONFIG_SE(NRF_SAADC_INPUT_AIN3);

    err_code = nrf_drv_saadc_init(NULL, saadc_batt_event_handler);
    APP_ERROR_CHECK(err_code);

    err_code = nrf_drv_saadc_channel_init(0, &config);
    APP_ERROR_CHECK(err_code);

    err_code = nrf_drv_saadc_buffer_convert(&adc_buf, 1);
    APP_ERROR_CHECK(err_code);

    err_code = nrf_drv_saadc_sample();
    APP_ERROR_CHECK(err_code);
}
