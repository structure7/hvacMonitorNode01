/* Node01 responsibilities:
   - Send notification if security alarm goes off and tweet it, too.
   - Reports tstat's and attic's temperature.
   - Ability send notification alarm if certain temp setpoint reached.
*/

#include <SimpleTimer.h>
#define BLYNK_PRINT Serial      // Comment this out to disable prints and save space
#include <ESP8266mDNS.h>        // Required for OTA
#include <WiFiUdp.h>            // Required for OTA
#include <ArduinoOTA.h>         // Required for OTA
#include <BlynkSimpleEsp8266.h>
#include <TimeLib.h>            // Used by WidgetRTC.h
#include <WidgetRTC.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#define ONE_WIRE_BUS 14         // WeMos D5
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

DeviceAddress ds18b20house = { 0x28, 0xFF, 0x35, 0x11, 0x01, 0x16, 0x04, 0x25 }; // House temperature probe
DeviceAddress ds18b20attic = { 0x28, 0xFF, 0xA3, 0x0B, 0x82, 0x16, 0x03, 0x28 }; // Attic temperature probe

char auth[] = "fromBlynkApp";
char ssid[] = "ssid";
char pass[] = "pw";

SimpleTimer timer;
WidgetTerminal terminal(V26);
WidgetRTC rtc;
WidgetBridge bridge1(V50);

int buzzerPin = 0;              // WeMos D3
bool alarmFlag;                 // TRUE if alarm is active. Allows for one alarm notification per instance.
int dailyHouseHigh = 0;         // Kicks off with overly low high
int dailyHouseLow = 200;        // Kicks off with overly high low
int dailyAtticHigh = 0;         // Kicks off with overly low high
int dailyAtticLow = 200;
int tempHouseHighAlarm = 100;   // Default house high temp alarm setpoint until selection is made in app
int alarmTimer, yHigh, yLow, yHighAttic, yMonth, yDate, yYear;
double tempHouse, tempAttic;    // Current house and attic temps
int tempHouseInt, tempAtticInt;
int todaysDate;
int lastMinute;

int houseLast24high = 0;
int houseLast24low = 200;               // Rolling high/low temps in last 24-hours.
int houseLast24hoursTemps[288];         // Last 24-hours temps recorded every 5 minutes.
int houseArrayIndex = 0;

int atticLast24high = 0;
int atticLast24low = 200;               // Rolling high/low temps in last 24-hours.
int atticLast24hoursTemps[288];         // Last 24-hours temps recorded every 5 minutes.
int atticArrayIndex = 0;

bool ranOnce;
//bool blynkConnected;
long startupTime;

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
  ArduinoOTA.setHostname("Node01AT-WeMosD1mini");

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

  timer.setTimeout(5000, setupArray);               // Sets entire array to temp at startup for a "baseline"
  timer.setTimeout(1000, vsync1);                   // Syncs back vPins to survive hardware reset
  timer.setTimeout(2000, vsync2);                   // Syncs back vPins to survive hardware reset

  timer.setInterval(1000L, sendAlarmStatus);
  timer.setInterval(5000L, uptimeReport);
  timer.setInterval(2011L, sendHouseTemp);
  timer.setInterval(5012L, sendAtticTemp);
  timer.setInterval(30000L, sendControlTemp);      // Sends temp to hvacMonitor via bridge for control
  timer.setInterval(300000L, recordHighLowTemps);  // 'Last 24 hours' array and 'since midnight' variables updated ~5 minutes. TODO: find a way to make the array do both!
}

void loop()
{
  Blynk.run();
  timer.run();
  ArduinoOTA.handle();

  if (ranOnce == false && year() != 1970) {
    todaysDate = day();
    lastMinute = minute();
    ranOnce = true;
    startupTime = now();
  }

  if (todaysDate != day()) {
    timer.setTimeout(120000, resetHiLoTemps);
    todaysDate = day();
  }
}

void sendControlTemp() {
  bridge1.virtualWrite(V127, 1, tempHouse);    // Writing "1" for this node, then the temp.
}

BLYNK_CONNECTED() {
  bridge1.setAuthToken(auth); // Place the AuthToken of the second hardware here
}

void vsync1()
{
  Blynk.syncVirtual(V22);    // dailyHouseHigh
  Blynk.syncVirtual(V23);    // dailyHouseLow
}

void vsync2()
{
  Blynk.syncVirtual(V30);    // tempHouseHighAlarm
}

BLYNK_WRITE(V22)
{
  dailyHouseHigh = param.asInt();
}

BLYNK_WRITE(V23)
{
  dailyHouseLow = param.asInt();
}

BLYNK_WRITE(V30) {
  switch (param.asInt())
  {
    case 1: { // Alarm Off
        tempHouseHighAlarm = 200;
        break;
      }
    case 2: {
        tempHouseHighAlarm = 80;
        break;
      }
    case 3: {
        tempHouseHighAlarm = 82;
        break;
      }
    case 4: {
        tempHouseHighAlarm = 84;
        break;
      }
    case 5: {
        tempHouseHighAlarm = 86;
        break;
      }
    default: {
        Serial.println("Unknown item selected");
        //tempHouseHighAlarm = param.asInt();
      }
  }
}

void notifyAndOff()
{
  Blynk.notify(String("House is ") + tempHouse + "F. Alarm disabled until reset."); // Send notification.
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
  long minDur = (now() - startupTime) / 60L;
  long hourDur = (now() - startupTime) / 3600L;
  if (minDur < 121)
  {
    terminal.print(String("Node01 (AT): ") + minDur + " min/");
    terminal.print(WiFi.RSSI());
    terminal.print("/");
    terminal.println(WiFi.localIP());
  }
  else if (minDur > 120)
  {
    terminal.print(String("Node01 (AT): ") + hourDur + " hrs/");
    terminal.print(WiFi.RSSI());
    terminal.print("/");
    terminal.println(WiFi.localIP());
  }
  terminal.flush();
}

void uptimeReport() {
  if (lastMinute != minute()) {
    Blynk.virtualWrite(101, minute());
    lastMinute = minute();
  }
}

void sendAtticTemp()
{
  sensors.requestTemperatures(); // Polls the sensors
  tempAttic = sensors.getTempF(ds18b20attic);

  // Conversion of tempAttic to tempAtticInt
  int xAtticInt = (int) tempAttic;
  double xTempAttic10ths = (tempAttic - xAtticInt);
  if (xTempAttic10ths >= .50)
  {
    tempAtticInt = (xAtticInt + 1);
  }
  else
  {
    tempAtticInt = xAtticInt;
  }

  if (tempAttic >= 0) // Different than above due to very high attic temps
  {
    Blynk.virtualWrite(7, tempAttic);
    if (tempAttic < 80)
    {
      Blynk.setProperty(V7, "color", "#04C0F8"); // Blue
    }
    else if (tempAttic >= 81 && tempAttic <= 100)
    {
      Blynk.setProperty(V7, "color", "#ED9D00"); // Yellow
    }
    else if (tempAttic > 100)
    {
      Blynk.setProperty(V7, "color", "#D3435C"); // Red
    }
  }
  else
  {
    Blynk.virtualWrite(7, "ERR");
    Blynk.setProperty(V7, "color", "#D3435C"); // Red
  }
}

void sendHouseTemp() {
  sensors.requestTemperatures(); // Polls the sensors
  tempHouse = sensors.getTempF(ds18b20house);

  // Conversion of tempHouse to tempHouseInt
  int xHouseInt = (int) tempHouse;
  double xHouse10ths = (tempHouse - xHouseInt);
  if (xHouse10ths >= .50)
  {
    tempHouseInt = (xHouseInt + 1);
  }
  else
  {
    tempHouseInt = xHouseInt;
  }

  if (tempHouse >= 30 && tempHouse <= 120)
  {
    Blynk.virtualWrite(3, tempHouse);
    if (tempHouse < 78)
    {
      Blynk.setProperty(V3, "color", "#04C0F8"); // Blue
    }
    else if (tempHouse >= 78 && tempHouse <= 80)
    {
      Blynk.setProperty(V3, "color", "#ED9D00"); // Yellow
    }
    else if (tempHouse > 80)
    {
      Blynk.setProperty(V3, "color", "#D3435C"); // Red
    }
  }
  else
  {
    Blynk.virtualWrite(3, "ERR");
    Blynk.setProperty(V3, "color", "#D3435C"); // Red
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

void setupArray()
{
  for (int i = 0; i < 288; i++)
  {
    houseLast24hoursTemps[i] = tempHouseInt;
  }

  houseLast24high = tempHouseInt;
  houseLast24low = tempHouseInt;

  Blynk.setProperty(V3, "label", "House");

  for (int i = 0; i < 288; i++)
  {
    atticLast24hoursTemps[i] = tempAtticInt;
  }

  atticLast24high = tempAtticInt;
  atticLast24low = tempAtticInt;

  Blynk.setProperty(V7, "label", "Attic");
}

void recordHighLowTemps()
{
  // HOUSE TEMPERATURES
  if (houseArrayIndex < 288)  // Records house temperature to the current array position. 287 positions covers 24 hours every 5 minutes.
  {
    houseLast24hoursTemps[houseArrayIndex] = tempHouseInt;
    ++houseArrayIndex;
  }
  else                        // Restart array to index 0 when it reached the end.
  {
    houseArrayIndex = 0;
    houseLast24hoursTemps[houseArrayIndex] = tempHouseInt;
    ++houseArrayIndex;
  }

  houseLast24high = -200; // Set high/low in prep for below.
  houseLast24low = 200;

  for (int i = 0; i < 288; i++) // Find the high/low in the array.
  {
    if (houseLast24hoursTemps[i] > houseLast24high)
    {
      houseLast24high = houseLast24hoursTemps[i];
    }

    if (houseLast24hoursTemps[i] < houseLast24low)
    {
      houseLast24low = houseLast24hoursTemps[i];
    }
  }

  if (tempHouse > dailyHouseHigh) // Sets a high low which reset at midnight.
  {
    dailyHouseHigh = tempHouse;
    Blynk.virtualWrite(22, dailyHouseHigh);
  }

  if (tempHouse < dailyHouseLow)
  {
    dailyHouseLow = tempHouse;
    Blynk.virtualWrite(23, dailyHouseLow);
  }

  Blynk.setProperty(V3, "label", String("House ") + houseLast24high + "/" + houseLast24low);  // Sets label with high/low temps.

  // ATTIC TEMPERATURES
  if (atticArrayIndex < 288)                   // Mess with array size and timing to taste!
  {
    atticLast24hoursTemps[atticArrayIndex] = tempAtticInt;
    ++atticArrayIndex;
  }
  else
  {
    atticArrayIndex = 0;
    atticLast24hoursTemps[atticArrayIndex] = tempAtticInt;
    ++atticArrayIndex;
  }

  atticLast24high = -200;
  atticLast24low = 200;

  for (int i = 0; i < 288; i++)
  {
    if (atticLast24hoursTemps[i] > atticLast24high)
    {
      atticLast24high = atticLast24hoursTemps[i];
    }

    if (atticLast24hoursTemps[i] < atticLast24low)
    {
      atticLast24low = atticLast24hoursTemps[i];
    }
  }

  if (tempAttic > dailyAtticHigh)
  {
    dailyAtticHigh = tempAttic;
    Blynk.virtualWrite(24, dailyAtticHigh);
  }

  if (tempAtticInt < dailyAtticLow)
  {
    dailyAtticLow = tempAtticInt;
  }

  Blynk.setProperty(V7, "label", String("Attic ") + atticLast24high + "/" + atticLast24low);  // Sets label with high/low temps.
}

void resetHiLoTemps()
{
  dailyHouseHigh = 0;     // Resets daily high temp
  dailyHouseLow = 200;    // Resets daily low temp
  dailyAtticHigh = 0;     // Resets daily high temp
  dailyAtticLow = 200;    // Resets daily low temp
}
