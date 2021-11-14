#ifndef BLE_APP_SAADC_H_
#define BLE_APP_SAADC_H_

#include <stdbool.h>
#include <stdint.h>
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

#include "nrf_drv_saadc.h"

#define BATTERY_LEVEL_MEAS_INTERVAL         APP_TIMER_TICKS(60000)      /** Battery level measurement interval (ticks). This value corresponds to 1 seconds. */
#define ADC_REF_VOLTAGE_IN_MILLIVOLTS       600                         /** Reference voltage (in milli volts) used by ADC while doing conversion. */
#define ADC_PRE_SCALING_COMPENSATION        6                           /** The ADC is configured to use VDD with 1/3 prescaling as input. And hence the result of conversion is to be multiplied by 3 to get the actual value of the battery voltage.*/
#define DIODE_FWD_VOLT_DROP_MILLIVOLTS      270                         /** Typical forward voltage drop of the diode . */
#define ADC_RES_10BIT                       1024                        /** Maximum digital value for 10-bit ADC conversion. */

#define BATT_HIG_THRESHOLD                  3300                    /* high threshold of battery level in which device can work (mV) */
#define BATT_LOW_THRESHOLD                  2000                    /* low threshold of battery level in which device can work (mV) */
#define BATTERY_LOW_LEVEL_PERCENT           20
#define BATTERY_LOW_LEVEL_MILLIVOLTS        0x08D4  //2260mV

/**@brief Macro to convert the result of ADC conversion in millivolts.
 *
 * @param[in]  ADC_VALUE   ADC result.
 *
 * @retval     Result converted to millivolts.
 */
#define ADC_RESULT_IN_MILLI_VOLTS(ADC_VALUE)\
        ((((ADC_VALUE) * ADC_REF_VOLTAGE_IN_MILLIVOLTS) / ADC_RES_10BIT) * ADC_PRE_SCALING_COMPENSATION)

extern void battery_init(void);

#endif  /*BLE_APP_SAADC_H_*/
