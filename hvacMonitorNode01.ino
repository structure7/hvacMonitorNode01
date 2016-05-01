#include <SimpleTimer.h>
#define BLYNK_PRINT Serial    // Comment this out to disable prints and save space
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#define ONE_WIRE_BUS 2  // ESP-01 GPIO 2
#include <TimeLib.h> // Used by WidgetRTC.h
#include <WidgetRTC.h>

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

DeviceAddress ds18b20house = { 0x28, 0xFF, 0x35, 0x11, 0x01, 0x16, 0x04, 0x25 }; // House temperature probe
DeviceAddress ds18b20attic = { 0x28, 0xC6, 0x89, 0x1E, 0x00, 0x00, 0x80, 0xAA }; // Attic temperature probe

char auth[] = "fromBlynkApp";

int buzzerPin = 0;  // ESP-01 GPIO 0
int alarmLatch = 0;
int dailyHigh = 0; // Kicks of with overly low high
int dailyLow = 200; // Kicks of with overly high low
int alarmTimer, dailyHighAttic, yHigh, yLow, yHighAttic, yMonth, yDate, yYear;
float tempHouse, tempAttic;

SimpleTimer timer;

WidgetLED led1(V11); // Heartbeat
WidgetRTC rtc;
BLYNK_ATTACH_WIDGET(rtc, V8);

void setup()
{
  Serial.begin(9600);
  Blynk.begin(auth, "ssid", "pw");
  
  while (Blynk.connect() == false) {
    // Wait until connected
  }

  sensors.begin();
  sensors.setResolution(ds18b20house, 10);
  sensors.setResolution(ds18b20attic, 10);
  
  rtc.begin();

  timer.setInterval(2000L, sendTemps); // Temperature sensor polling interval
  timer.setInterval(1000L, sendAlarmStatus);
  timer.setInterval(5000L, sendHeartbeat); // Blinks Blynk LED to reflect online status
  timer.setInterval(60000L, hiLoTemps);
  timer.setInterval(1000L, setHiLoTemps);
}

void setHiLoTemps()
{
  // Daily at 23:59, "yesterday's date" and high/low temp are recorded.
  if (hour() == 23 && minute() == 59 && second() == 59)
  {
    yMonth = month();
    yDate = day();
    yYear = year();
    yHigh = dailyHigh;
    yLow = dailyLow;
    yHighAttic = dailyHighAttic;
    dailyHigh = 0; // Resets daily high temp
    dailyLow = 200; // Resets daily low temp
    dailyHighAttic = 0; // Resets attic daily high temp
    timer.setTimeout(120000L, tweetHiLo); // Tweet hi/lo temps 2 minutes after midnight
  }
}

void hiLoTemps()
{
  if (tempHouse > dailyHigh)
  {
    dailyHigh = tempHouse;
    Blynk.virtualWrite(22, dailyHigh);
  }
  
  if (tempHouse < dailyLow)
  {
    dailyLow = tempHouse;
    Blynk.virtualWrite(23, dailyLow);
  }

    if (tempAttic > dailyHighAttic)
  {
    dailyHighAttic = tempAttic;
    Blynk.virtualWrite(24, dailyHighAttic);
  }
}

void tweetHiLo() // Runs once 2 minutes after midnight. Called in hiLoTemps().
{
  Blynk.tweet(String("On ") + yMonth + "/" + yDate + "/" + yYear + " the house high/low temps were " + yHigh + "°F/" + yLow + "°F. Attic high was" + yHighAttic + "°F.");
}

void sendTemps()
{
  sensors.requestTemperatures(); // Polls the sensors
  tempHouse = sensors.getTempF(ds18b20house);
  tempAttic = sensors.getTempF(ds18b20attic);

  if (tempHouse >= 30 && tempHouse <= 120)
  {
    Blynk.virtualWrite(3, tempHouse);
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
  }
  else if (digitalRead(buzzerPin) == LOW && alarmLatch == 0)
  {
    Blynk.notify("SECURITY ALARM TRIGGERED.");

    if (minute() < 10 && hour() < 12)
    {
      Blynk.tweet(String(month()) + "/" + day() + " " + hourFormat12() + ":" + "0" + minute() + "AM - SECURITY ALARM TRIGGERED");
    }
    else if (minute() < 10 && hour() > 11)
    {
      Blynk.tweet(String(month()) + "/" + day() + " " + hourFormat12() + ":" + "0" + minute() + "PM - SECURITY ALARM TRIGGERED");
    }
    else if (minute() > 9 && hour() < 12)
    {
      Blynk.tweet(String(month()) + "/" + day() + " " + hourFormat12() + ":" + minute() + "AM - SECURITY ALARM TRIGGERED");
    }
    else if (minute() > 9 && hour() > 11)
    {
      Blynk.tweet(String(month()) + "/" + day() + " " + hourFormat12() + ":" + minute() + "PM - SECURITY ALARM TRIGGERED");
    }

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
  }
  if (digitalRead(buzzerPin) == LOW && alarmTimer == 300)
  {
    Blynk.notify("SECURITY ALARM STILL GOING: 5 MIN.");
  }
  if (digitalRead(buzzerPin) == LOW && alarmTimer == 600)
  {
    Blynk.notify("SECURITY ALARM STILL GOING: 10 MIN.");
  }
}

void loop()
{
  Blynk.run();
  timer.run();
}
