/*
 * This source code file contains all the methods and 
 * definitions required to interface with a SHT7x sensor.
 */

#include "`$INSTANCE_NAME`.h"
#include "`$INSTANCE_NAME`_CR_Data_DRV.h"
#include "`$INSTANCE_NAME`_CR_Data_SEL.h"
#include "`$INSTANCE_NAME`_CR_Clock.h"
#include "`$INSTANCE_NAME`_SR_Data.h"
#include "math.h"

// Useful defines
#define `$INSTANCE_NAME`_HIGH 1
#define `$INSTANCE_NAME`_LOW  0

// Status register bit definitions
#define `$INSTANCE_NAME`_LOW_RES    0x01  // 12-bit Temp / 8-bit RH (vs. 14 / 12)
#define `$INSTANCE_NAME`_NORELOAD   0x02  // No reload of calibrarion data
#define `$INSTANCE_NAME`_HEAT_ON    0x04  // Built-in heater on
#define `$INSTANCE_NAME`_BATT_LOW   0x40  // VDD < 2.47V

// Delay times
#define `$INSTANCE_NAME`_PULSE_SHORT CyDelayUs(1)
#define `$INSTANCE_NAME`_PULSE_LONG CyDelayUs(3)

// Sensirion command definitions:           adr command r/w
#define `$INSTANCE_NAME`_MEAS_TEMP   0x03    // 000  0001    1
#define `$INSTANCE_NAME`_MEAS_HUMI   0x05    // 000  0010    1
#define `$INSTANCE_NAME`_STAT_REG_W  0x06    // 000  0011    0
#define `$INSTANCE_NAME`_STAT_REG_R  0x07    // 000  0011    1
#define `$INSTANCE_NAME`_SOFT_RESET  0x1e    // 000  1111    0

// Useful macros
#define `$INSTANCE_NAME`_MeasTemp(result)  `$INSTANCE_NAME`_Meas(`$INSTANCE_NAME`_READ_TEMP, result, `$INSTANCE_NAME`_BLOCK)
#define `$INSTANCE_NAME`_MeasHum(result)   `$INSTANCE_NAME`_Meas(`$INSTANCE_NAME`_READ_HUMI, result, `$INSTANCE_NAME`_BLOCK)

// Acknowledgment flags
#define `$INSTANCE_NAME`_NoAck 0
#define `$INSTANCE_NAME`_Ack   1

// Status register writable bits
#define `$INSTANCE_NAME`_SR_MASK  0x07

// Temperature & humidity equation constants
const float `$INSTANCE_NAME`_D1[]  = {           // for deg C 
            -40.1,    // @ 5   V
            -39.8,    // @ 4   V  
            -39.7,    // @ 3.5 V
            -39.6,    // @ 3   V
            -39.4     // @ 2.5 V
            };         
const float `$INSTANCE_NAME`_D2h =   0.01;         // for deg C, 14-bit precision
const float `$INSTANCE_NAME`_D2l =   0.04;         // for deg C, 12-bit precision

const float `$INSTANCE_NAME`_C1  = -2.0468;        // for V4 sensors
const float `$INSTANCE_NAME`_C2h =  0.0367;        // for V4 sensors, 12-bit precision
const float `$INSTANCE_NAME`_C3h = -1.5955E-6;     // for V4 sensors, 12-bit precision
const float `$INSTANCE_NAME`_C2l =  0.5872;        // for V4 sensors, 8-bit precision
const float `$INSTANCE_NAME`_C3l = -4.0845E-4;     // for V4 sensors, 8-bit precision

const float `$INSTANCE_NAME`_T1  =  0.01;          // for V3 and V4 sensors
const float `$INSTANCE_NAME`_T2h =  0.00008;       // for V3 and V4 sensors, 12-bit precision
const float `$INSTANCE_NAME`_T2l =  0.00128;       // for V3 and V4 sensors, 8-bit precision

// Variable to store the value of the status register
uint8_t `$INSTANCE_NAME`_StatusReg = 0x00;
// Variable to compute crc value
uint8_t `$INSTANCE_NAME`_Crc;


/* Functions declarations */
/**
* Compute the CRC value
*/
void    `$INSTANCE_NAME`_CalcCRC(uint8_t value, uint8_t *crc);
/**
* Reverse a byte
*/
uint8_t `$INSTANCE_NAME`_Bitrev(uint8_t value);

/* Functions declarations */

/******************************************************************************
 * Start function
 ******************************************************************************/
/**
* @brief Start the component.
*
* This function should always be called before starting any measurements, since
* it sets up the sensor based on the parameters specified in the top design.
* @retval `$INSTANCE_NAME`_ERR_NO_ACK if no acknowledgment was received
* @retval 0 if everything ok
*/
uint8_t `$INSTANCE_NAME`_Start(void) {
    uint8_t error;
    // Reset the sensor
    error = `$INSTANCE_NAME`_Reset();
    if (error)
        return error;
    
    // Set up resolution based on parameter
    if (`$LOW_RES`){
        error = `$INSTANCE_NAME`_SetLowResolution();     
    }
    else {
        error = `$INSTANCE_NAME`_SetHighResolution(); 
    }
    if (error)
        return error;
    
    // Set up OTP reload based on paramter
    if (`$OTP_RELOAD`){
        error = `$INSTANCE_NAME`_ActivateOTPReload();     
    }
    else {
        error = `$INSTANCE_NAME`_DeactivateOTPReload(); 
    }
    if (error)
        return error;
    
    return 0;
}

/******************************************************************************
 * User functions
 ******************************************************************************/

/**
* @brief    Complete blocking measurement.

* This functions performs a blocking measurement for both temperature and
* humidity values.
* @param    temp  Variable where the temperature value will be stored
* @param    humi  Variable where the humidity value will be stored
* @param    dew   Variable where the dewpoint value will be stored
* @retval   `$INSTANCE_NAME`_ERR_NO_ACK  If the sensor did not acknowledge
* @retval   `$INSTANCE_NAME`_ERR_CRC     If CRC check failed
* @retval   `$INSTANCE_NAME`_ERR_TO      If there was a timeout in the measurement
* @retval   0                            Otherwise
*/
uint8_t `$INSTANCE_NAME`_Measure(float *temp, float *humi, float *dew) {
  uint16_t rawData;
  uint8_t error;
  error =  `$INSTANCE_NAME`_MeasTemp(&rawData);
  if (error) {
    return error;
  }
  *temp = `$INSTANCE_NAME`_CalcTemp(rawData);
  error = `$INSTANCE_NAME`_MeasHum(&rawData);
  if (error)
    return error;
  *humi = `$INSTANCE_NAME`_CalcHum(rawData, *temp);
  *dew  = `$INSTANCE_NAME`_CalcDewpoint(*humi, *temp);
  return 0 ;
}

/**
* @brief    Temperature blocking measurement.

* This functions performs a blocking measurement for temperature.
* @param    temp  Variable where the temperature value will be stored
* @retval   `$INSTANCE_NAME`_ERR_NO_ACK  If the sensor did not acknowledge
* @retval   `$INSTANCE_NAME`_ERR_CRC    If CRC check failed
* @retval   `$INSTANCE_NAME`_ERR_TO     If there was a timeout in the measurement
* @retval   0                           Otherwise
*/
uint8_t  `$INSTANCE_NAME`_MeasureTemp(float *temp) {
    uint16_t rawData;
    uint8_t error;
    error =  `$INSTANCE_NAME`_MeasTemp(&rawData);
    if (error) {
        return error;
    } 
    *temp = `$INSTANCE_NAME`_CalcTemp(rawData);
    return 0;
}

/**
* @brief    Humidity blocking measurement.

* This functions performs a blocking measurement for humidity.
* @param    humi  Variable where the humidity value will be stored
* @param    temp  Value of temperature
* @retval   `$INSTANCE_NAME`_ERR_NO_ACK  If the sensor did not acknowledge
* @retval   `$INSTANCE_NAME`_ERR_CRC    If CRC check failed
* @retval   `$INSTANCE_NAME`_ERR_TO     If there was a timeout in the measurement
* @retval   0                           Otherwise
*/
uint8_t `$INSTANCE_NAME`_MeasureHumi(float *humi, float temp) {
    uint16_t rawData;
    uint8_t error;
    error =  `$INSTANCE_NAME`_MeasHum(&rawData);
    if (error) {
        return error;
    } 
    *humi =  `$INSTANCE_NAME`_CalcHum(rawData, temp);
    return 0;
}

/**
* @brief Initiate non blocking temperature measurement.

* This functions starts a non blocking measurement for temperature.
*
* @retval   `$INSTANCE_NAME`_ERR_NO_ACK  If the sensor did not acknowledge
* @retval   0                           Otherwise
*/
uint8_t `$INSTANCE_NAME`_StartMeasureTemp() {
    return `$INSTANCE_NAME`_StartMeasure(`$INSTANCE_NAME`_READ_TEMP);     
}

/**
* @brief Initiate non blocking humidity measurement.

* This functions starts a non blocking measurement for humidity.
*
* @retval   `$INSTANCE_NAME`_ERR_NO_ACK  If the sensor did not acknowledge
* @retval   0                           Otherwise
*/
uint8_t `$INSTANCE_NAME`_StartMeasureHumi() {
    return `$INSTANCE_NAME`_StartMeasure(`$INSTANCE_NAME`_READ_HUMI);
}

/**
* @brief Initiate non blocking measurement.

* This functions starts a measurement for temperature or humidity based
* on the passed parameter.
*
* @param    cmd     `$INSTANCE_NAME`_TEMP for temperature, `$INSTANCE_NAME`_HUMI for humidity
* @retval   `$INSTANCE_NAME`_ERR_NO_ACK  If the sensor did not acknowledge
* @retval   0                           Otherwise
*/
uint8_t `$INSTANCE_NAME`_StartMeasure(uint8_t cmd) {
    uint8_t error;
    if(`$CRC_ENABLE`) {
        // Initialize CRC calculation
        `$INSTANCE_NAME`_Crc = `$INSTANCE_NAME`_Bitrev(`$INSTANCE_NAME`_StatusReg & `$INSTANCE_NAME`_SR_MASK);  
    }
    `$INSTANCE_NAME`_StartTransmission();
    if (cmd == `$INSTANCE_NAME`_READ_TEMP)
        cmd = `$INSTANCE_NAME`_MEAS_TEMP;
    else 
        cmd = `$INSTANCE_NAME`_MEAS_HUMI;
    error = `$INSTANCE_NAME`_PutByte(cmd);
    if (error != 0) {
        return error;
    }
    // Include command byte in CRC calculation
    if (`$CRC_ENABLE`)
        `$INSTANCE_NAME`_CalcCRC(cmd, &`$INSTANCE_NAME`_Crc);  
    return 0;
}

/**
* @brief Initiate blocking/non blocking measurement.

* This functions starts a measurement, and if this is blocking 
* it waits until the measurement is complete before returning.
*
* @param    cmd     `$INSTANCE_NAME`_TEMP for temperature, `$INSTANCE_NAME`_HUMI for humidity
* @param    result  Variable where to store the result
* @param    block   True if blocking
* @retval   `$INSTANCE_NAME`_ERR_NO_ACK  If the sensor did not acknowledge
* @retval   `$INSTANCE_NAME`_ERR_CRC    If CRC check failed
* @retval   `$INSTANCE_NAME`_ERR_TO     If there was a timeout in the measurement
* @retval                         0     Otherwise
*/
uint8_t `$INSTANCE_NAME`_Meas(uint8_t cmd, uint16_t *result, int block) {
    uint8_t error;
    if(`$CRC_ENABLE`) {
        // Initialize CRC calculation
        `$INSTANCE_NAME`_Crc = `$INSTANCE_NAME`_Bitrev(`$INSTANCE_NAME`_StatusReg & `$INSTANCE_NAME`_SR_MASK);  
    }
    `$INSTANCE_NAME`_StartTransmission();
    if (cmd == `$INSTANCE_NAME`_READ_TEMP)
        cmd = `$INSTANCE_NAME`_MEAS_TEMP;
    else 
        cmd = `$INSTANCE_NAME`_MEAS_HUMI;
    error = `$INSTANCE_NAME`_PutByte(cmd);
    if (error != 0) {
        return error;
    }
    // Include command byte in CRC calculation
    if (`$CRC_ENABLE`)
        `$INSTANCE_NAME`_CalcCRC(cmd, &`$INSTANCE_NAME`_Crc);   

  // If non-blocking return
  if (!block) {
    return 0;
  }
  // Otherwise, wait for measurement to complete with specified timeout
  int del = `$MEAS_TO`;
  while (`$INSTANCE_NAME`_SR_Data_Read()) {
    del--;
    if (del == 0)
      return `$INSTANCE_NAME`_ERR_TO;              // Error: Timeout
    CyDelay(1);
  }
  error = `$INSTANCE_NAME`_GetResult(result);
  return error;
}

/**
* @brief Check if non-blocking measurement has completed
*
* Non-zero return indicates complete.
* @return `$INSTANCE_NAME`_MEAS_READY if measurement is complete
* 
*/
uint8_t `$INSTANCE_NAME`_MeasReady(void) {
  if (`$INSTANCE_NAME`_SR_Data_Read() != 0) 
    return 0;                   
  return `$INSTANCE_NAME`_MEAS_READY;
}

/** 
* @brief Get measurement result from sensor (plus CRC, if enabled)
* 
* @param result Variable where the result will be stored
* @retval S_Err_CRC if CRC check failed
* @retval 0 if everything ok
*/
uint8_t `$INSTANCE_NAME`_GetResult(uint16_t *result) {
    if (`$CRC_ENABLE`) {
        uint8_t val;
        val = `$INSTANCE_NAME`_GetByte(`$INSTANCE_NAME`_Ack);
        `$INSTANCE_NAME`_CalcCRC(val, &`$INSTANCE_NAME`_Crc);
        *result = val;
        val = `$INSTANCE_NAME`_GetByte(`$INSTANCE_NAME`_Ack);
        `$INSTANCE_NAME`_CalcCRC(val, &`$INSTANCE_NAME`_Crc);
        *result = (*result << 8) | val;
        val = `$INSTANCE_NAME`_GetByte(`$INSTANCE_NAME`_NoAck);
        val = `$INSTANCE_NAME`_Bitrev(val);
        if (val != `$INSTANCE_NAME`_Crc) {
            *result = 0xFFFF;
            return `$INSTANCE_NAME`_ERR_CRC;
        }
    }
    else {
        *result = `$INSTANCE_NAME`_GetByte(`$INSTANCE_NAME`_Ack);
        *result = (*result << 8) | `$INSTANCE_NAME`_GetByte(`$INSTANCE_NAME`_NoAck);
    }
    return 0;
}

/**
* @brief Write to status register
*
* @param value the byte to write to status register
* @retval S_Err_NoAck if no acknowledgement was received
* @retval 0 otherwise
*/
uint8_t `$INSTANCE_NAME`_WriteSR(uint8_t value) {
  uint8_t error;
  value &= `$INSTANCE_NAME`_SR_MASK;   // Mask off unwritable bits
  `$INSTANCE_NAME`_StatusReg = value; 
  `$INSTANCE_NAME`_StartTransmission();
  error = `$INSTANCE_NAME`_PutByte(`$INSTANCE_NAME`_STAT_REG_W);
  if (error)
    return error;
  return `$INSTANCE_NAME`_PutByte(value);
}

/**
* @brief Read to status register
*
* @param result Variable where the SR byte will be stored
* @retval S_Err_NoAck if no acknowledgement was received
* @retval 0 otherwise
*/
uint8_t `$INSTANCE_NAME`_ReadSR(uint8_t *result) {
    uint8_t error = 0;
    // Initialize CRC calculation
    if (`$CRC_ENABLE`)
        `$INSTANCE_NAME`_Crc = `$INSTANCE_NAME`_Bitrev(`$INSTANCE_NAME`_StatusReg & `$INSTANCE_NAME`_SR_MASK);  
    
    `$INSTANCE_NAME`_StartTransmission();
    error = `$INSTANCE_NAME`_PutByte(`$INSTANCE_NAME`_STAT_REG_R);
    if (error) {
        *result = 0xFF;
        return error;
    }
    
    if (`$CRC_ENABLE`){
        // Include command byte in CRC calculation
        uint8_t val;
        `$INSTANCE_NAME`_CalcCRC(`$INSTANCE_NAME`_STAT_REG_R, &`$INSTANCE_NAME`_Crc);       
        *result = `$INSTANCE_NAME`_GetByte(`$INSTANCE_NAME`_Ack);
        `$INSTANCE_NAME`_CalcCRC(*result, &`$INSTANCE_NAME`_Crc);
        val = `$INSTANCE_NAME`_GetByte(`$INSTANCE_NAME`_NoAck);
        val = `$INSTANCE_NAME`_Bitrev(val);
        if (val != `$INSTANCE_NAME`_Crc) {
            *result = 0xFF;
            error = `$INSTANCE_NAME`_ERR_CRC;
        }
    }
    else
        *result = `$INSTANCE_NAME`_GetByte(`$INSTANCE_NAME`_NoAck);
    
    return error;
}

/** 
 * @brief Public reset function
 * 
 * Reset communication and reset status register to default
 */
uint8_t `$INSTANCE_NAME`_Reset(void) {
    // Reset communication link with sensor
    `$INSTANCE_NAME`_ResetConnection();                
    `$INSTANCE_NAME`_StatusReg = 0x00;
    // Send soft reset command & return status
    return `$INSTANCE_NAME`_PutByte(`$INSTANCE_NAME`_SOFT_RESET);       
}

/******************************************************************************
 * Sensirion data communication
 ******************************************************************************/

/**
* @brief Write a byte to the sensor and check acknowledgement.
*
* This function sends a byte to the sensor and check if the sensor
* acknowledged it. The sensor returns a value greater than 0 if 
* no acknowledgement was received.
* @param d the byte to write
* @retval S_Err_NoAck if no ack
* @retval 0 if everything ok
*/
uint8_t `$INSTANCE_NAME`_PutByte(char d)
{
    auto int i;
    // Set up data pin since we need to write to it
    `$INSTANCE_NAME`_CR_Data_SEL_Write(`$INSTANCE_NAME`_HIGH);
    for (i = 0; i < 8; i++) {
        if (d & 0x80) {
            // data high
            `$INSTANCE_NAME`_CR_Data_DRV_Write(`$INSTANCE_NAME`_HIGH);         
        }
        else {
            // data low
            `$INSTANCE_NAME`_CR_Data_DRV_Write(`$INSTANCE_NAME`_LOW);          
        }
        // shift the data
        d <<= 1;
        `$INSTANCE_NAME`_PULSE_SHORT;
        // clock high
        `$INSTANCE_NAME`_CR_Clock_Write(`$INSTANCE_NAME`_HIGH);            
        `$INSTANCE_NAME`_PULSE_SHORT;
        // clock low
        `$INSTANCE_NAME`_CR_Clock_Write(`$INSTANCE_NAME`_LOW);             
    }
    `$INSTANCE_NAME`_CR_Data_DRV_Write(`$INSTANCE_NAME`_HIGH);
    // We need to read the data pin, so change its state
    `$INSTANCE_NAME`_CR_Data_SEL_Write(`$INSTANCE_NAME`_LOW);
    `$INSTANCE_NAME`_PULSE_LONG;
    // Clock high
    `$INSTANCE_NAME`_CR_Clock_Write(`$INSTANCE_NAME`_HIGH);                
    `$INSTANCE_NAME`_PULSE_LONG;
    i = `$INSTANCE_NAME`_SR_Data_Read();
    // Clock low
    `$INSTANCE_NAME`_CR_Clock_Write(`$INSTANCE_NAME`_LOW);                 
    if ( i > 0)
        return `$INSTANCE_NAME`_ERR_NO_ACK;
    return i;                              
}

/**
* @brief Get a byte from the sensor.
*
* This function gets a byte from the sensor and allows to choose if
* doing or skipping the acknowledgement.
* @param ack 0 if no ack, 1 if ack
* @return the byte read 
*/
uint8_t `$INSTANCE_NAME`_GetByte(int ack)
{
    auto int i,s;
    auto char c;
    s = 0;
    for (i=0; i<8; i++) {
        s <<= 1;
        `$INSTANCE_NAME`_PULSE_SHORT;
        `$INSTANCE_NAME`_CR_Clock_Write(`$INSTANCE_NAME`_HIGH); // clock high
        `$INSTANCE_NAME`_PULSE_SHORT;
        c = `$INSTANCE_NAME`_SR_Data_Read();    // get the data bit
        `$INSTANCE_NAME`_CR_Clock_Write(`$INSTANCE_NAME`_LOW);  // clock low
        if ( c )
            s |= 1;
    }
    // We need to write to the data pin, so set it up correctly
    `$INSTANCE_NAME`_CR_Data_SEL_Write(`$INSTANCE_NAME`_HIGH);
    if (ack == 1)
        `$INSTANCE_NAME`_CR_Data_DRV_Write(`$INSTANCE_NAME`_LOW);  // data low
    else
        `$INSTANCE_NAME`_CR_Data_DRV_Write(`$INSTANCE_NAME`_HIGH); // data hi
    `$INSTANCE_NAME`_CR_Clock_Write(`$INSTANCE_NAME`_HIGH);        // clock low
    `$INSTANCE_NAME`_PULSE_SHORT;
    `$INSTANCE_NAME`_CR_Clock_Write(`$INSTANCE_NAME`_LOW);         // clock low
    `$INSTANCE_NAME`_CR_Data_DRV_Write(`$INSTANCE_NAME`_HIGH);     // data hi
    // Return data line to input
    `$INSTANCE_NAME`_CR_Data_SEL_Write(`$INSTANCE_NAME`_LOW);
    return s;
}

/******************************************************************************
 * Sensirion signaling
 ******************************************************************************/

/** 
* @brief Generate Sensirion-specific transmission start sequence
*
* This is where Sensirion does not conform to the I2C standard.
*/

/*      _____         ________
* DATA:      |_______|
*           ___     ___
* SCK : ___|   |___|   |______*/
void `$INSTANCE_NAME`_StartTransmission(void) {
    // We need to write to the data pin, so set it up correctly
    `$INSTANCE_NAME`_CR_Data_SEL_Write(`$INSTANCE_NAME`_HIGH);
    `$INSTANCE_NAME`_CR_Data_DRV_Write(`$INSTANCE_NAME`_HIGH);
    `$INSTANCE_NAME`_PULSE_SHORT;
    `$INSTANCE_NAME`_CR_Clock_Write(`$INSTANCE_NAME`_HIGH);
    `$INSTANCE_NAME`_PULSE_SHORT;
    `$INSTANCE_NAME`_CR_Data_DRV_Write(`$INSTANCE_NAME`_LOW);
    `$INSTANCE_NAME`_PULSE_SHORT;
    `$INSTANCE_NAME`_CR_Clock_Write(`$INSTANCE_NAME`_LOW);
    `$INSTANCE_NAME`_PULSE_SHORT;
    `$INSTANCE_NAME`_CR_Clock_Write(`$INSTANCE_NAME`_HIGH);
    `$INSTANCE_NAME`_PULSE_SHORT;
    `$INSTANCE_NAME`_CR_Data_DRV_Write(`$INSTANCE_NAME`_HIGH);
    `$INSTANCE_NAME`_PULSE_SHORT;
    `$INSTANCE_NAME`_CR_Clock_Write(`$INSTANCE_NAME`_LOW);
    `$INSTANCE_NAME`_PULSE_SHORT;
}

/**
* @brief    Reset connection with sensor

* Set up a predefined sequence of signals on clock and data line to 
* restore a connection with the sensor.
*/
/*
*      ______________________________________________________         ________
* DATA:                                                      |_______|
*          _    _    _    _    _    _    _    _    _        ___     ___
* SCK : __| |__| |__| |__| |__| |__| |__| |__| |__| |______|   |___|   |______
*/
void `$INSTANCE_NAME`_ResetConnection(void) {
    uint8_t i;
    // We need to write to the data pin, so set it up correctly
    `$INSTANCE_NAME`_CR_Data_SEL_Write(`$INSTANCE_NAME`_HIGH);
    `$INSTANCE_NAME`_CR_Data_DRV_Write(`$INSTANCE_NAME`_HIGH);
    `$INSTANCE_NAME`_PULSE_LONG;
    for (i = 0; i < 9; i++) {
        `$INSTANCE_NAME`_CR_Clock_Write(`$INSTANCE_NAME`_HIGH);
        `$INSTANCE_NAME`_PULSE_LONG;
        `$INSTANCE_NAME`_CR_Clock_Write(`$INSTANCE_NAME`_LOW);
        `$INSTANCE_NAME`_PULSE_LONG;
    }
    `$INSTANCE_NAME`_StartTransmission();
}

/******************************************************************************
 * Helper Functions
 ******************************************************************************/

/**
* @brief Calcuate temperature in degrees C
*
* Starting from the raw sensor data, returns the temperature
* in degrees (C).
* @param rawData   raw sensor data
* @return temperature in degrees C
*/
float `$INSTANCE_NAME`_CalcTemp(uint16_t rawData) {
    if (`$INSTANCE_NAME`_StatusReg & `$INSTANCE_NAME`_LOW_RES)
        return `$INSTANCE_NAME`_D1[`$VDD`] + `$INSTANCE_NAME`_D2l * (float) rawData;
    else
        return `$INSTANCE_NAME`_D1[`$VDD`] + `$INSTANCE_NAME`_D2h * (float) rawData;
}

/**
* @brief Calcuate relative humidity with temperature compensation
*
* Starting from the raw sensor data, returns the value of relative
* humidity with temperature compensation.
* @param rawData   raw sensor data
* @param temp      value of temperature
* @return relative humidity (%)
*/
float `$INSTANCE_NAME`_CalcHum(uint16_t rawData, float temp) {
    float humi;
    // Check resolution
    if (`$INSTANCE_NAME`_StatusReg & `$INSTANCE_NAME`_LOW_RES) {
        humi = `$INSTANCE_NAME`_C1 + `$INSTANCE_NAME`_C2l * rawData
            + `$INSTANCE_NAME`_C3l * rawData * rawData;
        humi = (temp - 25.0) * (`$INSTANCE_NAME`_T1 + `$INSTANCE_NAME`_T2l * rawData) + humi;
    }
    else {
        humi = `$INSTANCE_NAME`_C1 + `$INSTANCE_NAME`_C2h * rawData 
            + `$INSTANCE_NAME`_C3h * rawData * rawData;
        humi = (temp - 25.0) * (`$INSTANCE_NAME`_T1 + `$INSTANCE_NAME`_T2h * rawData) + humi;
    }
    // Check if we are above and below limits
    if (humi > 100.0) { 
        humi = 100.0;
    }
    else if (humi < 0.1) {
        humi = 0.1;
    }
    return humi;
}

/**
* @brief Calcuate dewpoint.
*
* Starting from the value of humidity and temperature, it computes
* the dewpoint value
* @param humi      humidity
* @param temp      temperature
* @return dewpoint value
*/
float `$INSTANCE_NAME`_CalcDewpoint(float humi, float temp) {
    float k = 0;
    float Tn,m;
    // Set up coefficients based on temperature range
    if (temp >= 0 && temp <= 50) {
        m = 17.62;
        Tn = 243.12; 
    }
    else if ( temp >= -40 && temp <0) {
        m = 22.46;
        Tn = 272.62;    
    } else {
        return 0;
    }
    // Apply datasheet formula
    k = log(humi/100) + (m * temp) / (Tn + temp); 
    return Tn * k / (m - k);
}

/******************************************************************************
 * Resolution functions
 ******************************************************************************/
/**
* @brief Set resolution to high.
*
* Resolution is set to 12 bits for humidity, and to 14 bits for temperature
*/
uint8_t `$INSTANCE_NAME`_SetHighResolution(void) {
    uint8_t error = `$INSTANCE_NAME`_WriteSR( `$INSTANCE_NAME`_StatusReg & ~`$INSTANCE_NAME`_LOW_RES);     
    if (error)
        return error;
    return 0; 
}
/**
* @brief Set resolution to low.
*
* Resolution is set to 8 bits for humidity, and to 12 bits for temperature
*/
uint8_t `$INSTANCE_NAME`_SetLowResolution(void) {
    uint8_t error = `$INSTANCE_NAME`_WriteSR( `$INSTANCE_NAME`_StatusReg | `$INSTANCE_NAME`_LOW_RES);     
    if (error)
        return error;
    return 0;
}

/******************************************************************************
 * OTP Reload functions
 ******************************************************************************/
/**
* @brief Activate OTP Reload.
*
* If this is called, the calibration data are uploaded to the register before each
* measurement. This increases measurement time by about 10 ms.
*/
uint8_t `$INSTANCE_NAME`_ActivateOTPReload(void) {
    uint8_t error = `$INSTANCE_NAME`_WriteSR( `$INSTANCE_NAME`_StatusReg | `$INSTANCE_NAME`_NORELOAD);     
    if (error)
        return error;
    return 0; 
}

/**
* @brief Deactivate OTP Reload.
*
* If this is called, the calibration data are not uploaded to the register before each
* measurement. This reduces measurement time by about 10 ms.
*/
uint8_t `$INSTANCE_NAME`_DeactivateOTPReload(void) {
    uint8_t error = `$INSTANCE_NAME`_WriteSR( `$INSTANCE_NAME`_StatusReg & ~`$INSTANCE_NAME`_NORELOAD);     
    if (error)
        return error;
    return 0;
}

/******************************************************************************
 * Heater functions
 ******************************************************************************/
/**
* @brief Activate on chip heater.
*
* Calling this function activates the on chip heater. The heater may increase the 
* temperature of the sensor by 5-10°C (9-18°F) beyond ambient temperature. The heater
* draw 8ma @ 5 V supply voltage. The heater can be helpful for functionality analysis.
* Humidity and temperature readings before and after applying the heater are
* compared. Temperature shall increase while relative humidity decreases at the same
* time. Dew point shall remain the same. 
*
* Note: the temperature reading will display the temperature of the heated sensor
* element and not the ambient temperature. Furthermore, the sensor is not qualified
* fo continuous application of the heater.
*/
uint8_t `$INSTANCE_NAME`_ActivateHeater(void) {
    uint8_t error = `$INSTANCE_NAME`_WriteSR( `$INSTANCE_NAME`_StatusReg | `$INSTANCE_NAME`_HEAT_ON);     
    if (error)
        return error;
    return 0; 
}

/**
* @brief Deactivate on chip heater
*
*/
uint8_t `$INSTANCE_NAME`_DeactivateHeater(void) {
    uint8_t error = `$INSTANCE_NAME`_WriteSR( `$INSTANCE_NAME`_StatusReg & ~`$INSTANCE_NAME`_HEAT_ON);     
    if (error)
        return error;
    return 0;
}

/******************************************************************************
 * Battery check function
 ******************************************************************************/
/**
* @brief Check if vdd value is below 2.47 V
*/
uint8_t `$INSTANCE_NAME`_CheckEndOfBattery(void) {
    uint8_t sr;
    uint8_t error = `$INSTANCE_NAME`_ReadSR(&sr);     
    if (error)
        return error;
    if (sr & `$INSTANCE_NAME`_BATT_LOW)
        return `$INSTANCE_NAME`_BATTERY_LOW;
    else
        return `$INSTANCE_NAME`_BATTERY_OK; 
}




/******************************************************************************
 * Helper Functions for CRC computation
 ******************************************************************************/
/*
* @brief Calculate CRC for a single byte
* @param value The value to compute the crc
* @param crc   The variable where crc will be stored
*/ 
void `$INSTANCE_NAME`_CalcCRC(uint8_t value, uint8_t *crc) {
  const uint8_t POLY = 0x31;   // Polynomial: x**8 + x**5 + x**4 + 1
  uint8_t i;
  *crc ^= value;
  for (i = 8; i > 0; i--) {
    if (*crc & 0x80)
      *crc = (*crc << 1) ^ POLY;
    else
      *crc = (*crc << 1);
  }
}

/*
* @brief Bit-reverse a byte (for CRC calculations)
* @param value The byte to reverse
* @return the byte reversed
*/ 
uint8_t `$INSTANCE_NAME`_Bitrev(uint8_t value) {
  uint8_t i;
  uint8_t result = 0;
  for (i = 8; i > 0; i--) {
    result = (result << 1) | (value & 0x01);
    value >>= 1;
  }
  return result;
}

/* [] END OF FILE */
