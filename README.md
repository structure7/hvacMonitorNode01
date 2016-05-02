# hvacMonitorNode01
The project idea that got me started in Arduino... a way to get a notification on my phone if my security alarm went off. It morphed into also a handful of temperature sensors.
<p align="center"><img src="http://i.imgur.com/WPQASBv.jpg"/></p>
<p align="center"><img src="http://i.imgur.com/AhJXoua.jpg"/></p>
Bonus points if you can spot the DS18B20!<br>
## Hardware
* ESP-8266 (ESP-01... the tiny one)
* Two DS18B20 temperature sensors (one in attic, one in the house thermostat)<br>
## Features
* Monitors a Broadview security alarm panel (details below) and notifies my mobile device when an alarm is triggered.
* Tweets the house's daily high and low temperature, and the attic's daily high temperature.<br>
## Security Alarm Panel Monitoring
Almost every new home security system includes the ability to receive notifications on your mobile device, as well as remotely arm/disarm the system and more. I just wanted to receive a text if the alarm went off. My security company quoted me about $700 to replace the panels and keypads to support this. I did it for $3.<br>
My home's main alarm panel is a mess of wires connected to an uninviting, large circuit board. After doing some research, I found my particular model of alarm was not "open source," friendly to being hacked, or compatible with modules and devices intended to add remote control and notifications.
## Libraries and Resources

Title | Include | Link | w/ IDE?
------|---------|------|----------
OneWire | OneWire.h | https://github.com/PaulStoffregen/OneWire | No
Arduino-Temperature-Control-Library | DallasTemperature.h | https://github.com/milesburton/Arduino-Temperature-Control-Library | No
Time | Timelib.h | https://github.com/PaulStoffregen/Time
SimpleTimer | SimpleTimer.h | https://github.com/jfturcot/SimpleTimer
ESP8266/Arduino | ESP8266WiFi.h | https://github.com/esp8266/Arduino | No
blynk-library | BlynkSimpleEsp8266.h | https://github.com/blynkkk/blynk-library | No
ESP8266 board mgr | N/A | [json](http://arduino.esp8266.com/stable/package_esp8266com_index.json) & [instructions](https://github.com/esp8266/Arduino#installing-with-boards-manager) | No

Many thanks to all of the people above. [How to edit this.](https://guides.github.com/features/mastering-markdown/)
