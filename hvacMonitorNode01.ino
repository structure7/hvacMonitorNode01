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
int dayCountLatch = 0;
int alarmTimer, dailyHigh, dailyLow, nowTemp, todaysDate, yMonth, yDate, yYear;

SimpleTimer timer;

WidgetLED led1(V11);
WidgetRTC rtc;
BLYNK_ATTACH_WIDGET(rtc, V8);

void setup()
{
  Serial.begin(9600);
  Blynk.begin(auth, "ssid", "pw");

  sensors.begin();
  sensors.setResolution(ds18b20house, 10);
  sensors.setResolution(ds18b20attic, 10);

  while (Blynk.connect() == false) {
    // Wait until connected
  }
  rtc.begin();

  timer.setInterval(2000L, sendTemps); // Temperature sensor polling interval
  timer.setInterval(1000L, hiLoTemps);
  timer.setInterval(1000L, sendAlarmStatus);
  timer.setInterval(5000L, sendHeartbeat); // Blinks Blynk LED to reflect online status
}

void tweetHiLo() // Runs once 2 minutes after midnight. Called in hiLoTemps().
{
  Blynk.tweet(String("On ") + yMonth + "/" + yDate + "/" + yYear + " the house high/low temps were " + dailyHigh + "°F/" + dailyLow + "°F.");
}

void hiLoTemps()
{
  // Daily at 23:58 (arbitrary), "yesterday's date" is set.
  if (hour() == 23 && minute() == 58) 
  {
    yMonth = month();
    yDate = day();
    yYear = year();
  }

  // Sets the date once after reset/boot up, or when the date actually changes.
  if (dayCountLatch == 0)
  {
    todaysDate = day();
    dayCountLatch++;
  }

  // Records the high and low temperature. Love how simple this is.
  if (todaysDate == day())
  {
       if (nowTemp > dailyHigh)
      {
         dailyHigh = nowTemp;
      }
   else if (nowTemp < dailyLow)
      {
         dailyLow = nowTemp;
      }
  }
  else if (todaysDate != day())
  {
    timer.setTimeout(120000L, tweetHiLo); // Tweet hi/lo temps 2 minutes after midnight
    dayCountLatch = 0; // Allows today's date to be reset.
  }
}

void sendTemps()
{
  sensors.requestTemperatures(); // Polls the sensors
  float tempHouse = sensors.getTempF(ds18b20house);
  float tempAttic = sensors.getTempF(ds18b20attic);

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
