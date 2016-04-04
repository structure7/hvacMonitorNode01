#include <SimpleTimer.h>
#define BLYNK_PRINT Serial    // Comment this out to disable prints and save space
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <TimeLib.h>
#define ONE_WIRE_BUS 2

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

DeviceAddress ds18b20house = { 0x28, 0xFF, 0x35, 0x11, 0x01, 0x16, 0x04, 0x25 }; // House temperature probe

char auth[] = "fromBlynkApp(04)";

int buzzerPin = 0;

SimpleTimer timer;

void setup()
{
  Serial.begin(9600);
  Blynk.begin(auth, "ssid", "pw");

  sensors.begin();
  sensors.setResolution(ds18b20house, 10);

  timer.setInterval(2000L, sendTemps); // Temperature sensor polling interval
  timer.setInterval(2000L, sendStatus); // Maybe alter setInterval to something else so it doesn't hammer the notification app... look at http://playground.arduino.cc/Code/SimpleTimer
}

void sendTemps()
{
  sensors.requestTemperatures(); // Polls the sensors
  float tempHouse = sensors.getTempF(ds18b20house);

  Blynk.virtualWrite(3, tempHouse);
}

void sendStatus()
{
  if (digitalRead(buzzerPin) == HIGH) // Runs when alarm buzzer is OFF
  {
    // Blank (since HIGH == no buzzer with BJT)
  }
  else
  {
    // SEND NOTIFICATION TO APP
  }
}

void loop()
{
  Blynk.run();
  timer.run();
}
