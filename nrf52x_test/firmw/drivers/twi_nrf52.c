/*
 * twi_nrf52.c
 *
 *  Created on: Jul 29, 2020
 *      Author: thinhvu
 */

#include "twi_nrf52.h"

#include <stdbool.h>
#include <stdint.h>
#include "nrf.h"
#include "nrf_delay.h"
#include "app_error.h"
#include "nrf_drv_twi.h"


/* TWI instance. */
static const nrf_drv_twi_t m_twi = NRF_DRV_TWI_INSTANCE(TWI_INSTANCE_ID);


void twi_master_init(void)
{
    ret_code_t err_code;

    const nrf_drv_twi_config_t twi_config = {
       .scl                = TWI_MASTER_CLOCK_PIN_NUMBER,
       .sda                = TWI_MASTER_DATA_PIN_NUMBER,
       .frequency          = NRF_DRV_TWI_FREQ_100K,
       .interrupt_priority = APP_IRQ_PRIORITY_HIGH,
       .clear_bus_init     = false
    };

    err_code = nrf_drv_twi_init(&m_twi, &twi_config, NULL, NULL);
    APP_ERROR_CHECK(err_code);

    nrf_drv_twi_enable(&m_twi);
}

/**
 * @brief Function for reading data from temperature sensor.
 */
void twi_master_read(uint8_t address, uint8_t * data, uint16_t data_length)
{
    ret_code_t err_code = nrf_drv_twi_rx(&m_twi, address, data, data_length);
    APP_ERROR_CHECK(err_code);
}

void twi_master_write(uint8_t address, uint8_t * data, uint16_t data_length, bool no_stop)
{
    ret_code_t err_code = nrf_drv_twi_tx(&m_twi, address, data, data_length, no_stop);
    APP_ERROR_CHECK(err_code);
}
