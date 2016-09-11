# hvacMonitorNode01
The project idea that got me started in Arduino... a way to get a notification on my phone if my security alarm went off. It morphed into also a handful of temperature sensors.
<p align="center"><img src="http://i.imgur.com/7SssWXI.png"/></p>
<p align="center"><img src="http://i.imgur.com/oWPaq46.png"/></p>

## Hardware
* ESP-8266 (ESP-01... the tiny one).
* Two DS18B20 temperature sensors (one in attic, one in the house thermostat).
* Adjustable buck converter (4.5-28VDC in - .8-20VDC out - model CN1584). Adjusted to 3.3V. (<a href="http://www.ebay.com/itm/171868430133">eBay source</a>)
* Upgraded the ESP-01's 1MB flash chip to 4MB to support OTA.

## Features
* Monitors a Broadview security alarm panel (details below) and notifies my mobile device when an alarm is triggered.
* Monitors temperature sensor added to house tstat and a temperature sensor located in the attic.
* In-app ability to manually update high/low temperatures using terminal `manual` command.

## Security Alarm Panel Monitoring
Almost every new home security system includes the ability to receive notifications on your mobile device, as well as remotely arm/disarm the system and more. I just wanted to receive a text if the alarm went off. My security company quoted me about $700 to replace the panels and keypads to support this. I did it for $10.<p>
My home's main alarm panel is a mess of wires connected to an uninviting, large circuit board. After doing some research, I found my particular model of alarm was not "open source," friendly to being hacked, or compatible with modules and devices intended to add remote control and notifications.

## Libraries and Resources

Title | Include | Link
------|---------|------
OneWire | OneWire.h | https://github.com/PaulStoffregen/OneWire
Arduino-Temperature-Control-Library | DallasTemperature.h | https://github.com/milesburton/Arduino-Temperature-Control-Library
Time | Timelib.h | https://github.com/PaulStoffregen/Time
SimpleTimer | SimpleTimer.h | https://github.com/jfturcot/SimpleTimer
ESP8266/Arduino | ESP8266WiFi.h | https://github.com/esp8266/Arduino
blynk-library | BlynkSimpleEsp8266.h | https://github.com/blynkkk/blynk-library
WidgetRTC | WidgetRTC.h | https://github.com/blynkkk/blynk-library
ESP8266 board mgr | N/A | [json](http://arduino.esp8266.com/stable/package_esp8266com_index.json) & [instructions](https://github.com/esp8266/Arduino#installing-with-boards-manager)
BasicOTA | ESP8266mDNS.h | Incl in Arduino IDE example for BasicOTA
BasicOTA | WiFiUdp.h | Incl in Arduino IDE example for BasicOTA
BasicOTA | ArduinoOTA.h | Incl in Arduino IDE example for BasicOTA

Many thanks to all of the people above. [How to edit this.](https://guides.github.com/features/mastering-markdown/)
