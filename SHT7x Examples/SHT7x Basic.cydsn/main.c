/* ========================================
 *
 * This project contains an example for the
 * usage of the SHT7x custom component.
 *
 * @author Davide Marzorati
 * 
*/
#include "project.h"
#include "stdio.h"

/* Functions declarations */
void startComponents(void);
void logError(uint8_t errorCode);
void logData(int temperature, int humidity, int dewpoint);

int main(void)
{
    CyGlobalIntEnable; /* Enable global interrupts. */
    
    // Local variables
    float temperature = 0,
        humidity = 0, 
        dewpoint = 0;
    uint8_t measurementStarted = 0;
    uint16_t rawData;
    uint8_t measType;
    uint8_t error;
    
    // Start the UART and the heater
    startComponents();
    
    // Perform blocking measuremet
    error = SHT7x_1_Measure(&temperature, &humidity, &dewpoint);
    if (error)
        logError(error);
    else
        logData(temperature*1000, humidity*1000, dewpoint*1000);
   
    for(;;)
    {
        // Perform non blocking measurements -- 1 s delay when meas is complete
        if (measurementStarted == 0){
            SHT7x_1_StartMeasureTemp();
            measurementStarted = 1;
            measType = SHT7x_1_READ_TEMP;
        }
        
        if (SHT7x_1_MeasReady()) 
        {
            if (measType == SHT7x_1_READ_TEMP) {
                SHT7x_1_GetResult(&rawData);
                temperature = SHT7x_1_CalcTemp(rawData);
                measType = SHT7x_1_READ_HUMI;
                SHT7x_1_StartMeasureHumi();
            }
            else {
                measurementStarted = 0;
                SHT7x_1_GetResult(&rawData);
                humidity = SHT7x_1_CalcHumi(rawData, temperature);
                dewpoint = SHT7x_1_CalcDewpoint(humidity, temperature);
                logData(temperature*1000, humidity*1000, dewpoint*1000);
                CyDelay(1000);
            }
        }
    }
}

/* Functions definitions */

// Start the components
void startComponents(void){
    UART_1_Start();
    SHT7x_1_Start();
    UART_1_PutString("--------------SHT7x BASIC USAGE--------------\r\n");
}

// Log explanation of error on the uart
void logError(uint8_t errorCode) {
    switch(errorCode){
        case SHT7x_1_ERR_CRC: 
            UART_1_PutString("CRC Check failed\r\n");
        break;
        case SHT7x_1_ERR_NO_ACK: 
            UART_1_PutString("Sensor did not acknowledge\r\n");
        break;
        case SHT7x_1_ERR_TO: 
            UART_1_PutString("Measurement timeout\r\n");
        break;
        default:
            UART_1_PutString("Unknown error\r\n");
        break;
    }
}


// Log data over uart -- use int instead of float so that we 
// don't have to increase heap size
void logData(int temperature, int humidity, int dewpoint) {
    char message[100];
    sprintf(message,"Temp: %d Hum: %d Dewpoint: %d\r\n",
        temperature, humidity, dewpoint);
    UART_1_PutString(message);
}

/* [] END OF FILE */
