#include <SimpleTimer.h>
//#define BLYNK_PRINT Serial    // Comment this out to disable prints and save space
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#define ONE_WIRE_BUS 2  // ESP-01 GPIO 2

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

DeviceAddress ds18b20house = { 0x28, 0xFF, 0x35, 0x11, 0x01, 0x16, 0x04, 0x25 }; // House temperature probe
DeviceAddress ds18b20attic = { 0x28, 0xC6, 0x89, 0x1E, 0x00, 0x00, 0x80, 0xAA }; // Attic temperature probe

char auth[] = "fromBlynkApp";

int buzzerPin = 0;  // ESP-01 GPIO 0
int alarmLatch = 0;
int alarmTimer;

SimpleTimer timer;

WidgetLED led1(V11);

void setup()
{
  Serial.begin(9600);
  Blynk.begin(auth, "ssid", "pw");

  sensors.begin();
  sensors.setResolution(ds18b20house, 10);
  sensors.setResolution(ds18b20attic, 10);

  timer.setInterval(2000L, sendTemps); // Temperature sensor polling interval
  timer.setInterval(1000L, sendAlarmStatus);
  timer.setInterval(5000L, sendHeartbeat); // Blinks Blynk LED to reflect online status
}

void sendTemps()
{
  sensors.requestTemperatures(); // Polls the sensors
  float tempHouse = sensors.getTempF(ds18b20house);
  float tempAttic = sensors.getTempF(ds18b20attic);

  if (tempHouse >= 30 && tempHouse <= 120)
  {
    Blynk.virtualWrite(3, tempHouse); // Original code
  }
  else
  {
    Blynk.virtualWrite(3, "ERR");
  }

  if (tempAttic >= 0) // Different than above due to very high attic temps
  {
    Blynk.virtualWrite(7, tempAttic);
  }
  else
  {
    Blynk.virtualWrite(7, "ERR");
  }
  led1.off();
}

void sendHeartbeat()
{
  led1.on();
}

void sendAlarmStatus()
{
  if (digitalRead(buzzerPin) == HIGH) // Runs when alarm buzzer is OFF (pulled high)
  {
    alarmTimer = 0;
    alarmLatch = 0;
    Serial.println("No alarm");
  }
  else if (digitalRead(buzzerPin) == LOW && alarmLatch == 0)
  {
    Blynk.notify("SECURITY ALARM TRIGGERED.");
    Serial.println("SECURITY ALARM TRIGGERED.");
    alarmLatch++;
  }
  else if (digitalRead(buzzerPin) == LOW && alarmLatch != 0)
  {
    alarmTimer++;
    Serial.println(alarmTimer);
  }

  if (digitalRead(buzzerPin) == LOW && alarmTimer == 60)
  {
    Blynk.notify("SECURITY ALARM STILL GOING: 60 SEC.");
    Serial.println("SECURITY ALARM STILL GOING: 60 SEC.");
  }
  if (digitalRead(buzzerPin) == LOW && alarmTimer == 300)
  {
    Blynk.notify("SECURITY ALARM STILL GOING: 5 MIN.");
    Serial.println("SECURITY ALARM STILL GOING: 5 MIN.");
  }
  if (digitalRead(buzzerPin) == LOW && alarmTimer == 600)
  {
    Blynk.notify("SECURITY ALARM STILL GOING: 10 MIN.");
    Serial.println("SECURITY ALARM STILL GOING: 10 MIN.");
  }
}

void loop()
{
  Blynk.run();
  timer.run();
}
