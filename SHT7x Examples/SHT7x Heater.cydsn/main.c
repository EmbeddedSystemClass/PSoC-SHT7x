/* ========================================
 *
 * Copyright YOUR COMPANY, THE YEAR
 * All Rights Reserved
 * UNPUBLISHED, LICENSED SOFTWARE.
 *
 * CONFIDENTIAL AND PROPRIETARY INFORMATION
 * WHICH IS THE PROPERTY OF your company.
 *
 * ========================================
*/
#include "project.h"
#include "stdio.h"

/* Functions declaration */
void startComponents(void);
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
    uint8_t counter = 0;
    
    // Start our components
    startComponents();
    
    UART_1_PutString("***Starting heater***\r\n");
    SHT7x_1_ActivateHeater();
    for(;;)
    {
        // Perform non blocking measurements -- 1 s delay when meas is complete
        if (measurementStarted == 0){
            SHT7x_1_StartMeasureTemp();
            measurementStarted = 1;
            measType = SHT7x_1_READ_TEMP;
        }
        
        if (SHT7x_1_MeasReady()) {
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
                counter++;
                if (counter == 10) {
                    UART_1_PutString("***Stopping heater***\r\n");
                    SHT7x_1_DeactivateHeater();
                }
                if (counter == 30) {
                    UART_1_PutString("***Starting heater***\r\n");
                    SHT7x_1_ActivateHeater();
                    counter = 0;
                }
                
                CyDelay(1000);
            }
        }
    }
}

void startComponents(void) {
    UART_1_Start();
    SHT7x_1_Start();
    UART_1_PutString("--------------SHT7x HEATER TEST--------------\r\n\r\n");
    UART_1_PutString("You should see temperature rising and RH decreasing\r\n");
    UART_1_PutString("while dewpoint should stay the same.\r\n");
    
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
