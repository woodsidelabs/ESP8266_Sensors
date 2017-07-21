# ESP8266_Sensors
This is an example project built on ESP8266 for home sensing.
It does the following:

- Detects temperature and humidity with I2C-based Si7021 breakout
- Detects lighting conditions with I2C-based 
- Detects motion with GPIO-based PIR
- Supports OTA code update

I am managing this sensor kit with Home Assistant or "HA" (http://www.home-assistant.io/)
Temperature, humidity, lighting are polled periodically by HA.  Motion is pushed to HA over HTTPS connection; HA is configured for HTTPS for secure communicaton from the Internet as described at https://home-assistant.io/docs/ecosystem/certificates/lets_encrypt/

Code with comments is posted here along with a picture and wiring diagram.
