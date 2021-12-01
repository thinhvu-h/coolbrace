/*
 * twi_types.h
 *
 *  Created on: Jul 29, 2020
 *      Author: thinhvu
 */

#ifndef DRIVERS_TWI_TYPES_H_
#define DRIVERS_TWI_TYPES_H_

typedef enum {
	I2C_E_OK, // The message was sent.
	I2C_BUSY,  // Message was NOT sent, bus was busy.
	I2C_FAIL   // Message was NOT sent, bus failure
	           // If you are interested in the failure reason,
	           // Sit on the event call-backs.
} i2c_error_t;

typedef uint8_t i2c_address_t;


#endif /* DRIVERS_TWI_TYPES_H_ */
