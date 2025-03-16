# arduino_GPS_synchronized_clock

Highly accurate digital clock with time synchronization using GPS signals. Achieved time deviation less than 100ms in relation to atomic clocks found on the internet.

## Features
- 8x32 LED matrix display
- adjustable display brightness
- two fonts to choose from
- rotary encoder for menu navigation
- time and temperature display, alarm, timer and stopwatch
- speaker to sound the alarm
- RTC backup in case of weak GPS signal
- settings are storred in case of power failure

## Hardware requirements
- Arduino-based board (Dasduino CORE)
- NEO-6M GPS module
- DS3231 RTC module
- MAX7219 8x32 matrix display
- rotary encoder (brand: Soldered)
- temperature sensor (SHTC3, brand: Soldered)
- active GPS antenna
- SMA to uFL/u.FL/IPX/IPEX adapter
- speaker
- perfboard
- 100uF capacitor
- 8 10uF capacitors
- 8 100nF capacitors
- miscellaneous

## Software requirements
- Arduino IDE or Visual Studio Code

### Required libraries
- I2C communication: https://github.com/arduino/ArduinoCore-avr/tree/master/libraries/Wire
- EEPROM: https://github.com/arduino/ArduinoCore-avr/tree/master/libraries/EEPROM
- RTC: https://github.com/adafruit/RTClib
- Software serial: https://github.com/arduino/ArduinoCore-avr/tree/master/libraries/SoftwareSerial
- TinyGPSPlus: https://github.com/mikalhart/TinyGPSPlus
- rotary encoder: https://github.com/SolderedElectronics/Soldered-Rotary-Encoder-With-easyC-Arduino-Library
- temperature sensor: https://github.com/SolderedElectronics/Soldered-SHTC3-Temperature-Humidity-Sensor-Arduino-Library
- MAX7219: my own library, included in the main code

## Wiring and schematics
### Board schematic
<img alt="schematic" src="https://github.com/user-attachments/assets/3614a174-8044-440d-be30-566845f54209" /> <br><br>

## Troubleshooting
### Components connected on the breadboard
![troubleshooting](https://github.com/user-attachments/assets/9f6a17fc-5e2e-4b28-9ca7-78611e482847) <br><br>

**Problem:** GPS receiver doesn't communicate with the Dasduino board, can't read data <br>
**Solution:** switch TX and RX pins or change baud rate <br>

**Problem:** PPS LED on GPS receiver doesn't blink <br>
**Solution:** check GPS receiver antenna connecion, change location of the antenna <br>

**Problem:** GPS time is offset few seconds in the future <br>
**Solution:** wait for the GPS receiver to receive ephemeris data <br>

**Problem:** GPS receiver doesn't receive data <br>
**Solution:** if the antenna has a clear view of the sky, connect an antenna with a higher gain <br>

**Problem:** GPS receiver works unreliably <br>
**Solution:** check power supply, connect 10uF and 100nF capacitors close to it's power pins <br>

**Problem:** RTC works unreliably, looses second every few minutes <br>
**Solution:** check power supply, connect 10uF and 100nF capacitors close to it's power pins <br>

**Problem:** LED Matrix displays random characters <br>
**Solution:** SPI communication problem, check wiring and connections <br>

**Problem:** LED Matrix causes interference <br>
**Solution:** connect 10uF and 100nF capacitors to power pins of each module on LED Matrix display <br>

**Problem:** LED Matrix constantly glows, doesn't exit TEST mode <br>
**Solution:** check it's configuration in program code, check electrical connections <br>

**Problem:** Dasduino Core gives error while uploading code <br>
**Solution:** change board to Arduino Nano and bootloader to ATMega328P (old bootloader) <br>

## Usage
### Front side
![front_side](https://github.com/user-attachments/assets/91879d17-8f91-4144-a56c-23b9d698211f) <br><br>
### Back side
![back_side](https://github.com/user-attachments/assets/bf403fba-7a47-45a9-83ee-5640c246faef) <br><br>
### Inside
![inside](https://github.com/user-attachments/assets/cb6fb834-1479-4de9-9cea-b969d91f94cc) <br><br>

- rotate the encoder to change from time display to alarm, timer, stopwatch or settings menu
- press the encoder to confirm the selected option or setting
- in the alarm, timer and stopwatch menu, short press of the encoder cycles the digits you are adjusting (hours, minutes, seconds) while the long press confirms the adjusted digits
- int the settings menu, short press enters the menu, long press confirms the settings and double press takes you back
- in the settings menu you can change the font (display hours+minutes or hours+minutes+seconds), change the display brightness in 15 steps, turn off all alarms, turn on or off daylight saving time and reset all settings


## Tool for creating fonts

https://pjrp.github.io/MDParolaFontEditor <br>

This is the best approach for creating fonts because it allows you to have characters with different widths. <br> 
For a more detailed info visit: https://arduinoplusplus.wordpress.com/2016/11/08/parola-fonts-a-to-z-defining-fonts/




