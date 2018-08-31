# PSoC SHT7x Custom Component
This repository contains a PSoC Creator Library Project featuring a 
[Sensirion SHT7x](https://www.sensirion.com/en/environmental-sensors/humidity-sensors/pintype-digital-humidity-sensors/)
Custom Component. As stated in the header file of the library, this work is based on the on the
[Sensirion SHT7x Arduino library](https://github.com/spease/Sensirion) written by Carl Jackson. 

## Sensirion SHT7x
The Sensirion SHT7x sensors is a temperature and humidity sensor. According to Sensirion description, the SHTx sensor is
*a digital pin-type relative humidity sensor for easy replaceability in a wide range of applications. 
The series consists of a standard version, SHT71, which convinces with its attractive price-performance ratio, 
and a high-end version, SHT75.*

## PSoC Custom Component
![alt text](https://i.imgur.com/1ti2PgW.png "SHT7x Component Symbol")

The PSoC Custom component features all the useful methods to interface a PSoC board with an SHT7x sensor. To date, 
the component was tested with a [PSoC 5LP Development Kit](http://www.cypress.com/documentation/development-kitsboards/cy8ckit-059-psoc-5lp-prototyping-kit-onboard-programmer-and). 
The component requires the connection to two pins:

- Clock PIN: this pin must be connected to the clock pin of the SHT7x sensor
- Data PIN: this pin must be connected to the data pin of the SHT7x sensor with a 10k pull up resistor

A Component macro is provided, where two bidirectional pins are already connected to the component.

### Details
The API of the component allow to easily perform temperature, humidity, and dewpoint measurements. 
Coefficients for temperature conversion, together with formulas for temperature, humidity,nd dewpoint 
computation, are provided. Both high-level methods and low-level methods are accessible to the 
developer.

At the moment, five parameters are available:

1. **Enable CRC check**: to enable CRC checking when communicating with the sensor (by default it is enabled)
2. **Measurement Timeout**: to specify the amount of time to wait for measurement complete when using one of the blocking methods
3. **OTP Reload**: to determine if calibration coefficients should be uploaded to registers every time there is a measurement or not
4. **Resolution**: to specifiy the resolution of the sensors. The sensors provide two possible choices:

   1. 12 bit humidity/14 bit temperature (default)
   2. 8 bit humidity/12 bit temperature
5. **Sensor Supply Voltage**: to specify the supply voltage of the sensors. This information is used to used the correct 
calibration coefficients, since they vary according to the supply voltage.

### Sample Code
Performing a temperature, humidity, and dewpoint measurement requires very few lines of code:

```
float temperature, humidity, dewpoint;
SHT7x_Start();
SHT7x_Measure(&temperature, &humidity, &dewpoint);
```

### Requirements
This custom component requires the use of the `math` library. In the header file of the custom component this library is already included, but if you do not follow the procedure reported [here](https://community.cypress.com/docs/DOC-12152), you would get the following error

```
Build error: undefined reference to `log'
```

This is due to the fact that the dewpoint computation requires the use of a logarithmic function.
