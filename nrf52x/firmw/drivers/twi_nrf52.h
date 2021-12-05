/*
 * twi_nrf52.h
 *
 *  Created on: Apr 24, 2019
 *      Author: mylap
 */

#ifndef DRIVERS_TWI_NRF52_H_
#define DRIVERS_TWI_NRF52_H_

#include <stdio.h>
#include "boards.h"
#include "app_util_platform.h"
#include "app_error.h"
#include "nrf_drv_twi.h"
#include "nrf_delay.h"
#include "twi_types.h"

/** @file
* @brief Software controlled TWI Master driver.
*
*
* @defgroup lib_driver_twi_master Software controlled TWI Master driver
* @{
* @ingroup nrf_twi
* @brief Software controlled TWI Master driver (deprecated).
*
* @warning This module is deprecated.
*
* Supported features:
* - Repeated start
* - No multi-master
* - Only 7-bit addressing
* - Supports clock stretching (with optional SMBus style slave timeout)
* - Tries to handle slaves stuck in the middle of transfer
*/

#define TWI_MASTER_CLOCK_PIN_NUMBER     12
#define TWI_MASTER_DATA_PIN_NUMBER 	    16

/* TWI instance ID */
#define TWI_INSTANCE_ID                 0

#define TWI_READ_BIT                 (0x01)        //!< If this bit is set in the address field, transfer direction is from slave to master.
#define TWI_WRITE_BIT                (0x00)        //!< If this bit is set in the address field, transfer direction is from slave to master.

/**
 * @brief Function for initializing TWI bus IO pins and checks if the bus is operational.
 *
 * Both pins are configured as Standard-0, No-drive-1 (open drain).
 *
 * @return
 * @retval true TWI bus is clear for transfers.
 * @retval false TWI bus is stuck.
 */
void twi_master_init(void);
ret_code_t twi_master_read(uint8_t address, uint8_t * data, uint16_t data_length);
ret_code_t twi_master_write(uint8_t address, uint8_t * data, uint16_t data_length, bool no_stop);

#endif /* DRIVERS_TWI_NRF52_H_ */
