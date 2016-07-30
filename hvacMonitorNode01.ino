#include <SimpleTimer.h>
#define BLYNK_PRINT Serial    // Comment this out to disable prints and save space
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#define ONE_WIRE_BUS 2        // ESP-01 GPIO 2
#include <TimeLib.h>          // Used by WidgetRTC.h
#include <WidgetRTC.h>

#include <ESP8266mDNS.h>        // Required for OTA
#include <WiFiUdp.h>            // Required for OTA
#include <ArduinoOTA.h>         // Required for OTA

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

DeviceAddress ds18b20house = { 0x28, 0xFF, 0x35, 0x11, 0x01, 0x16, 0x04, 0x25 }; // House temperature probe
DeviceAddress ds18b20attic = { 0x28, 0xC6, 0x89, 0x1E, 0x00, 0x00, 0x80, 0xAA }; // Attic temperature probe

char auth[] = "fromBlynkApp";
char ssid[] = ssid;
char pass[] = "pw";

int buzzerPin = 0;              // ESP-01 GPIO 0
bool alarmFlag;                 // TRUE if alarm is active. Allows for one alarm notification per instance.
int dailyHouseHigh = 0;         // Kicks off with overly low high
int dailyHouseLow = 200;        // Kicks off with overly high low
int dailyAtticHigh = 0;         // Kicks off with overly low high
int tempHouseHighAlarm = 100;   // Default house high temp alarm setpoint until selection is made in app
int alarmTimer, yHigh, yLow, yHighAttic, yMonth, yDate, yYear;
float tempHouse, tempAttic; // Current house and attic temps


bool manualFlag;                // TRUE if manual update mode is active.
bool v22flag, v23flag, v24flag; // TRUE is an update for a vPin is active.

SimpleTimer timer;

WidgetRTC rtc;
BLYNK_ATTACH_WIDGET(rtc, V8);
WidgetTerminal terminal(V26);

void setup()
{
  Serial.begin(9600);
  Blynk.begin(auth, ssid, pass);

  while (Blynk.connect() == false) {
    // Wait until connected
  }

  sensors.begin();
  sensors.setResolution(ds18b20house, 10);
  sensors.setResolution(ds18b20attic, 10);

  // START OTA ROUTINE
  ArduinoOTA.setHostname("esp8266-Node01AT");

  ArduinoOTA.onStart([]() {
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.print("MAC address: ");
  Serial.println(WiFi.macAddress());
  // END OTA ROUTINE

  rtc.begin();

  timer.setInterval(2000L, sendTemps); // Temperature sensor polling interval
  timer.setInterval(1000L, sendAlarmStatus);
  timer.setInterval(30000L, hiLoTemps);
  timer.setInterval(5000L, setHiLoTemps);
  timer.setInterval(1000L, uptimeReport);

  Blynk.virtualWrite(22, "RST");
  Blynk.virtualWrite(23, "RST");
  Blynk.virtualWrite(24, "RST");
}

void loop()
{
  Blynk.run();
  timer.run();
  ArduinoOTA.handle();
}

BLYNK_WRITE(V30) {
  switch (param.asInt())
  {
    case 1: { // Alarm Off
        tempHouseHighAlarm = 200;
        break;
      }
    case 2: { // 80F Alarm
        tempHouseHighAlarm = 80;
        break;
      }
    case 3: { // 82F Alarm
        tempHouseHighAlarm = 82;
        break;
      }
    case 4: { // 84F Alarm;
        tempHouseHighAlarm = 84;
        break;
      }
    case 5: { // 86F Alarm
        tempHouseHighAlarm = 86;
        break;
      }
    default: {
        Serial.println("Unknown item selected");
      }
  }
}

void notifyAndOff()
{
  Blynk.notify(String("House is ") + tempHouse + "°F. Alarm disabled until reset."); // Send notification.
  Blynk.virtualWrite(V30, 1); // Rather than fancy timing, just disable alarm until reset.
}

BLYNK_WRITE(V27) // App button to report uptime
{
  int pinData = param.asInt();

  if (pinData == 0)
  {
    timer.setTimeout(3000L, uptimeSend);
  }
}

void uptimeSend()
{
  long minDur = millis() / 60000L;
  long hourDur = millis() / 3600000L;
  if (minDur < 121)
  {
    terminal.print(String("Node01 (AT): ") + minDur + " mins @ ");
    terminal.println(WiFi.localIP());
  }
  else if (minDur > 120)
  {
    terminal.print(String("Node01 (AT): ") + hourDur + " hrs @ ");
    terminal.println(WiFi.localIP());
  }
  terminal.flush();
}

void uptimeReport(){
  if (second() > 1 && second() < 6)
  {
    Blynk.virtualWrite(101, minute());
  }
}

void setHiLoTemps()
{
  // Daily at 00:01, "yesterday's date" and high/low temp are recorded.
  if (hour() == 00 && minute() == 01)
  {
    dailyHouseHigh = 0;     // Resets daily high temp
    dailyHouseLow = 200;    // Resets daily low temp
    dailyAtticHigh = 0;     // Resets attic daily high temp
  }
}

void hiLoTemps()
{
  if (tempHouse > dailyHouseHigh)
  {
    dailyHouseHigh = tempHouse;
    Blynk.virtualWrite(22, dailyHouseHigh);
  }

  if (tempHouse < dailyHouseLow)
  {
    dailyHouseLow = tempHouse;
    Blynk.virtualWrite(23, dailyHouseLow);
  }

  if (tempAttic > dailyAtticHigh)
  {
    dailyAtticHigh = tempAttic;
    Blynk.virtualWrite(24, dailyAtticHigh);
  }
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

  if (tempHouse >= tempHouseHighAlarm)
  {
    notifyAndOff();
  }
}

void sendAlarmStatus()
{
  if (digitalRead(buzzerPin) == HIGH) // Runs when alarm buzzer is OFF (pulled high)
  {
    alarmTimer = 0;
    alarmFlag = 0;
  }
  else if (digitalRead(buzzerPin) == LOW && alarmFlag == 0)
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

    alarmFlag = 1;
  }
  else if (digitalRead(buzzerPin) == LOW && alarmFlag != 0)
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

BLYNK_WRITE(V26)  // Manual updates to vPins
{
  if ( String("manual") == param.asStr() || String("MANUAL") == param.asStr())  // Enters manual update mode
  {
    terminal.println(""); terminal.println("");
    terminal.println(""); terminal.println("");
    terminal.println("    Variables you can manually update:");
    terminal.println("V22: House high temp.");
    terminal.println("V23: House low temp.");
    terminal.println("V24: Attic high temp.");
    terminal.println("");
    terminal.println("Enter v** to update, 'x' to exit:");
    manualFlag = 1;
  }

  if (v22flag == 1 && manualFlag == 1)                                // Data entry mode for vPins. This group must be above the selection mode below.
  {
    Blynk.virtualWrite(22, param.asInt());
    dailyHouseHigh = param.asInt();
    terminal.println("");
    terminal.println(String("Updated with ") + dailyHouseHigh + ".");
    terminal.println(""); terminal.println("Mode closed.");
    v22flag = 0;
    manualFlag = 0;
  }
  else if (v23flag == 1 && manualFlag == 1)
  {
    Blynk.virtualWrite(23, param.asInt());
    dailyHouseLow = param.asInt();
    terminal.println("");
    terminal.println(String("Updated with ") + dailyHouseLow + ".");
    terminal.println(""); terminal.println("Mode closed.");
    v23flag = 0;
    manualFlag = 0;
  }  
  else if (v24flag == 1 && manualFlag == 1)
  {
    Blynk.virtualWrite(24, param.asInt());
    dailyAtticHigh = param.asInt();
    terminal.println("");
    terminal.println(String("Updated with ") + dailyAtticHigh + ".");

    v24flag = 0;
    manualFlag = 0;
  }
  
  if ( (String("V22") == param.asStr() || String("v22") == param.asStr()) && manualFlag == 1)       // Enters vPin selection mode.
  {
    terminal.println(" "); terminal.println("Enter value to update V22 to:");
    v22flag = 1;
  }
  else if ( (String("V23") == param.asStr() || String("v23") == param.asStr()) && manualFlag == 1)
  {
    terminal.println(" "); terminal.println("Enter value to update V23 to:");
    v23flag = 1;
  }
  else if ( (String("V24") == param.asStr() || String("v24") == param.asStr()) && manualFlag == 1)
  {
    terminal.println(" "); terminal.println("Enter value to update V24 to:");
    v24flag = 1;
  }
  else if ( (String("x") == param.asStr() || String("X") == param.asStr()) && manualFlag == 1)
  {
    manualFlag = 0;
    terminal.println(""); terminal.println("Mode closed.");    
  }

  terminal.flush();
}
