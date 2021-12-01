#ifndef TEMP_APP_H_
#define TEMP_APP_H_

#include <string.h>
#include "ble_hts.h"
#include "twi_nrf52.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

#define AS6212_HUMAN_SENSOR_ADDR     (0x92>>1)
#define AS6212_ENV_SENSOR_ADDR     (0x92>>1)
#define TEMPERATURE_MEAS_INTERVAL   APP_TIMER_TICKS(4000)      /** temperature measurement interval (ticks). */

#define TEMP_TYPE_AS_CHARACTERISTIC     0  /**< Determines if temperature type is given as characteristic (1) or as a field of measurement (0). */

extern void temperature_measurement(ble_hts_meas_t * p_meas);

#endif /*TEMP_APP_H_*/
