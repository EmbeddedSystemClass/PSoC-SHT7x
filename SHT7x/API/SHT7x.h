/*! @file `$INSTANCE_NAME`.h
    @brief  Header file for interfacing with a SHT7x sensor.

    This header contains macros and function prototypes to interface
            the PSoC with a Sensirion SHT7x temperature and humidity sensor.
    This work is based on the Sensirion SHT7x Arduino library written by
            Carl Jackson: https://github.com/spease/Sensirion
    @author Davide Marzorati
*/

#ifndef `$INSTANCE_NAME`_H

#define `$INSTANCE_NAME`_H



/**
 * Error code for acknowledgment not received.
 */
#define  `$INSTANCE_NAME`_ERR_NO_ACK   1  

/**
 * Error code for CRC failure.
 */
#define  `$INSTANCE_NAME`_ERR_CRC     2 

/**
 * Error code for measurement timeout.
 */
#define  `$INSTANCE_NAME`_ERR_TO      3 

/**
 * 
 * Value returned when measurement is ready
 */
#define  `$INSTANCE_NAME`_MEAS_READY    4  

/**
 * Helper value to use as a parameter to read temperature.
 */
#define `$INSTANCE_NAME`_READ_TEMP        0

/**
 * Value to pass to read humidity.
 */
#define `$INSTANCE_NAME`_READ_HUMI        1

/**
 * Value to pass to functions to start blocking measurements
 */
#define `$INSTANCE_NAME`_BLOCK       1

/**
 * Value to pass to functions to start non-blocking measurements
 */
#define `$INSTANCE_NAME`_NON_BLOCK    0
    
/**
 * Value returned when battery is low
 */
#define `$INSTANCE_NAME`_BATTERY_LOW   1
    
/**
 * Value returned when battery is ok
 */
#define `$INSTANCE_NAME`_BATTERY_OK    0

#include "cytypes.h"
#include "CyLib.h"

/* Function declarations */

// Start function
uint8_t `$INSTANCE_NAME`_Start(void);

// Blocking measurement functions
uint8_t `$INSTANCE_NAME`_Measure(float *temp, float *humi, float *dew);
uint8_t `$INSTANCE_NAME`_MeasureTemp(float *temp);
uint8_t `$INSTANCE_NAME`_MeasureHumi(float *humi, float temp);

// Non blocking measurement function
uint8_t `$INSTANCE_NAME`_StartMeasure(uint8_t cmd);
uint8_t `$INSTANCE_NAME`_StartMeasureTemp();
uint8_t `$INSTANCE_NAME`_StartMeasureHumi();
// Measurement helper function
uint8_t `$INSTANCE_NAME`_Meas(uint8_t cmd, uint16_t *result, int block);
uint8_t `$INSTANCE_NAME`_MeasReady(void);

// Communication function
uint8_t `$INSTANCE_NAME`_PutByte(char command);
uint8_t `$INSTANCE_NAME`_GetByte(int ack);
void    `$INSTANCE_NAME`_StartTransmission(void);
void    `$INSTANCE_NAME`_ResetConnection(void);
uint8_t `$INSTANCE_NAME`_Reset(void);
uint8_t `$INSTANCE_NAME`_GetResult(uint16_t *result);

// Status register functions
uint8_t `$INSTANCE_NAME`_ReadSR(uint8_t *result);
uint8_t `$INSTANCE_NAME`_WriteSR(uint8_t value);

// Resolution functions
uint8_t `$INSTANCE_NAME`_SetHighResolution(void);
uint8_t `$INSTANCE_NAME`_SetLowResolution(void);

// OTP Reload functions
uint8_t `$INSTANCE_NAME`_ActivateOTPReload(void);
uint8_t `$INSTANCE_NAME`_DeactivateOTPReload(void);

// Heater functions
uint8_t `$INSTANCE_NAME`_ActivateHeater(void);
uint8_t `$INSTANCE_NAME`_DeactivateHeater(void);

// End of battery check
uint8_t `$INSTANCE_NAME`_CheckEndOfBattery(void);

// Helper functions 
float `$INSTANCE_NAME`_CalcTemp(uint16_t rawData);
float `$INSTANCE_NAME`_CalcHum(uint16_t rawData, float temp);
float `$INSTANCE_NAME`_CalcDewpoint(float humi, float temp);
    
#endif

/* [] END OF FILE */
