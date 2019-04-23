# ESP8266 433MHz-Receiver sending data via MQTT (and DS18B20 temperature value)
Arduino project to send messages to predefined topics based on reveiced 433MHz data from door/window sensors.

Version: 1.0.0 (2019-04-20)
Author: Link-Technologies - [Dipl.-Inform. (FH) Andreas Link](http://www.AndreasLink.de)


## Arduino ESP8266 433MHz-Receiver

This project allows to receive "any" 433MHz signal from mainly door/window sensors (e.g. from Stabo) as well as home power plugs remote controls and sending the values immediately via MQTT. It uses an **ESP8266** as uController and **RXB6 433Mhz Superheterodyne Wireless Receiver Module**. Software is also prepared for using a **DS18B20** to send measured temperature values on a dedicated MQTT topic (I use it, so measure temp in the same room, where I have installed the receiver :-)).

By using the [Link-Tech Sensor MQTT library](https://github.com/andreaslink-de/LinkTech_SensorMQTTlib) Stabo door/window sensors ([Stabo Funk-Tür-/Fensterkontakt 51126](https://stabo.de/hs-zubehoer/)) are easily integrated and topics are automatically formed according to the received signal. So corresponding MQTT messages are easily sent on predefind topics for radio detected events. All setup is done via MQTT and based on button hold ID and then all other events are automtically handled.

### Usage
- Install/Load missings libs
- Adjust WiFi Settings
- Setup is prepared for fixed IP, but currently running in DHCP (can be changed in wifi.ino)
- Adjust pins according to setup for DS18B20 and Wireless Receiver Module
- Adjust MQTT broker IP
- Adjust MQTT base topic as well as temperature topic if used

###Used libs:
- ESP8266WiFi.h
- RCSwitch.h
- PubSubClient.h
- OneWire.h
- DallasTemperature.h
- LinkTech_SensorMQTTlib.h

### Memory consumption
- Sketch uses 294988 Bytes
- Global variables use 33704 Bytes (using preset of 100 possible sensors) of dynamic memory

**More documentation to come**

