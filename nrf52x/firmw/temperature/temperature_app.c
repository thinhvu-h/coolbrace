#include "temperature_app.h"

static void AS6212_readTemperature(float* human_temp, float* env_temp);

static void AS6212_readTemperature(float* human_temp, float* env_temp)
{
    uint8_t twi_cmd=0x00;
    uint8_t twi_data[2]={0};

    twi_master_write(AS6212_HUMAN_SENSOR_ADDR, &twi_cmd, sizeof(twi_cmd), true);
    twi_master_read(AS6212_HUMAN_SENSOR_ADDR, twi_data, sizeof(twi_data));
    *human_temp = 0.0078125*(twi_data[0] << 8 | twi_data[1]);
    NRF_LOG_INFO("twi_data[0]:%d, twi_data[1]:%d, human_temp: %d\r\n", twi_data[0], twi_data[1], *human_temp);

    // memset(twi_data, 0, sizeof(twi_data));
    // twi_master_write(AS6212_HUMAN_SENSOR_ADDR, &twi_cmd, sizeof(twi_cmd), true);
    // twi_master_read(AS6212_HUMAN_SENSOR_ADDR, twi_data, sizeof(twi_data));
    // *env_temp = 0.0078125*(twi_data[0] << 8 | twi_data[1]);
    *env_temp = 30;
    // NRF_LOG_INFO("twi_data[0]:%d, twi_data[1]:%d, env_temp: %d\r\n", twi_data[0], twi_data[1], *env_temp);
}


void temperature_measurement(ble_hts_meas_t * p_meas)
{
    static ble_date_time_t time_stamp = { 2020, 30, 11, 10, 50, 0 };
    float human_temp;
    float env_temp;

    p_meas->temp_in_fahr_units = false;
    p_meas->time_stamp_present = false;
    p_meas->temp_type_present  = (TEMP_TYPE_AS_CHARACTERISTIC ? false : true);

    AS6212_readTemperature(&human_temp, &env_temp);

    p_meas->temp_in_celcius.exponent = 0;
    p_meas->temp_in_celcius.mantissa = human_temp;
    p_meas->temp_in_fahr.exponent    = 0;
    p_meas->temp_in_fahr.mantissa    = env_temp;
    p_meas->time_stamp               = time_stamp;
    p_meas->temp_type                = BLE_HTS_TEMP_TYPE_BODY;

    // update simulated time stamp
    time_stamp.seconds += 27;
    if (time_stamp.seconds > 59)
    {
        time_stamp.seconds -= 60;
        time_stamp.minutes++;
        if (time_stamp.minutes > 59)
        {
            time_stamp.minutes = 0;
        }
    }
}
