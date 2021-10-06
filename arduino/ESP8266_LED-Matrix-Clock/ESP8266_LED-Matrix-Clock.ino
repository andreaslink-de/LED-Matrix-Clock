/*------------------------------------------------------------------------------------
 * ESP8266 - LED-Matrix Clock
 * 
 * Author:  Dipl.-Inform. (FH) Andreas Link
 * Date:    21.07.2021 Harrislee, Germany
 * Web:     Link-Technologies.de
 * License: GNU General Public License, Version 3
 * 
 * Usage:
 *   Boots, connects to WiFi or asks for WiFi access, connects to internet to get time from
 *   NTP server. Connects to MQTT server to send events if buttons are pushed/touched and to 
 *   subscribe to outside temperature.
 *   Firmwareupdates via OTA are possbile.
 *
 * Remind: 
 *   Consider to preset your "config.h" with relevant passswords for accessing the 
 *   WiFi Manager on first boot.
 ------------------------------------------------------------------------------------*/


#define DEBUG_OUTPUT      // Uncomment to enable serial IO debug messages
#define USE_MQTT          // Comment to deactivate MQTT (which is biggest part)
#define USE_NTP_TIME      // Comment to deactivate use of internet time to get correct local date and time
#define USE_SHT21         // Comment to deactivate use of SHT21 Temperature and Humidity sensor
//#define USE_BH1750        // Comment to deactivate BH1750 brightness integration <-- NOT YET FULLY IMPLEMENTED!



/* *********************************************** <<< DEBUG >>> ****************************************************/
////////////////////////
// Debug Serial Magic //
////////////////////////
#ifdef DEBUG_OUTPUT
  #define DEBUG(input)   { Serial.print(input); }
  #define DEBUGln(input) { Serial.println(input); }
#else
  #define DEBUG(input) ;
  #define DEBUGln(input) ;
#endif

/* *********************************************** >>> DEBUG <<< ****************************************************/


#define FW_VERSION "0.3"
#include "config.h"

#define MQTT_MAX_PACKET_SIZE 1024 // Remind: If you mess with this value and go too low, the code throws weird exceptions
#define BUTTON1   15              // GPIO15 aka D8
#define BUTTON2   16              // GPIO16 aka D0


#include <FS.h>                   // This needs to be first, or it all crashes and burns...
#include <ESP8266WiFi.h>          // https://github.com/esp8266/Arduino
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>          // https://github.com/tzapu/WiFiManager
#include <ArduinoJson.h>          // https://github.com/bblanchon/ArduinoJson

#include <Adafruit_GFX.h>         // https://github.com/adafruit/Adafruit-GFX-Library
#include <Max72xxPanel.h>         // https://github.com/markruys/arduino-Max72xxPanel

#include <AceButton.h>
using namespace ace_button;

#ifdef USE_MQTT
  #include <PubSubClient.h>
#endif

#ifdef USE_NTP_TIME
  #include <NTPClient.h>
  #include <WiFiUdp.h>
  #include "RTClib.h"             //Makes DateTime calculations possible first
#endif

#ifdef USE_SHT21
  #include "SparkFunHTU21D.h"     // https://github.com/sparkfun/SparkFun_HTU21D_Breakout_Arduino_Library
#endif

#ifdef USE_BH1750
  #include <BH1750.h>             // https://github.com/claws/BH1750
#endif


//Read power via ADC
ADC_MODE(ADC_VCC);

/* *********************************************** <<< Config >>> ****************************************************/
// ----------------------------------------- MQTT ------------------------------------------ //
#ifdef USE_MQTT
  //MQTT-Topic/Data Max-Length
  #define MAX_TOPIC_LENGTH 100
  #define MAX_DATA_LENGTH 100
  
  //MQTT-Setup
  char mqttBaseTopic[MAX_TOPIC_LENGTH];
  char mqttButton1Topic[MAX_TOPIC_LENGTH];
  char mqttButton2Topic[MAX_TOPIC_LENGTH];
  char mqttLightTopic[MAX_TOPIC_LENGTH];
  char mqttTempTopic[MAX_TOPIC_LENGTH];
  char mqttHumidityTopic[MAX_TOPIC_LENGTH];
  
  //Dedicated subscribe topics
  //const char* mqttSubTopicOutTemp = "zuhause/garten/kinderhaus/aussen/temperatur";
    //const char* q = "abc";
    //const char*& c = q;
  
  const char* &mqttSubTopicOutTemp = outsideTemperatureMQTTtopic;
  const char* mqttSubTopicDate    = "system/date";
  const char* mqttSubTopicTime    = "system/time";
#endif


// ------------------------------------ WiFi-Manager --------------------------------------- //

//WiFiManager Vars:
bool shouldSaveConfigToFlash = false;  //Flag for saving data

//define your default values here, if there are different values in config.json, they are overwritten.
//length should be max size + 1
char flashIoTdeviceName[50];
char flashMQTTserver[40];
char flashMQTTport[6];
char flashMQTTbaseTopic[50];
char flashMQTTdeviceTopic[50];

//Vars:
float gESPVCC = 0.0;
String gIP = "not connected";
String gWiFiStatus = "not connected";

String curDate = "";
String curTime = "";
String gFormerTimeNow = "";
uint8_t gShowTimeOnce = false;

String gMacAdr = "00:00:00:00:00:00";
char gIdentifierAry[7];
String gIdentifier = "000000";
String gSSID = "";
uint8_t mqttConnectFailCounter = 0;
unsigned long lastDoSomethingEvent = 0;
unsigned long lastReconnectAttempt = 0;
boolean gForceWiFiManager = false;

//LED-Matrix ON/OFF status
uint8_t gLEDmatrixTurnedOn = true;
uint8_t gLEDmatrixWasTurnedOffBefore = false;

//Receive a message, and show it on the display vars
String gMessage = "";
uint8_t gNewMessageToShow = false;

//Blink Short Urgent Text vars
unsigned long lastShowTextTimeSlot = 0;
String gShortText = "";
uint8_t gNewTextToShow = false;
uint8_t gCurrentlyShowingText = false;
uint8_t gTextBlinkToggle = false;
unsigned long lastShowTextBlinkTimeSlot = 0;

//Show outside temp on display
String gOutsideTempStr = "N/A"; //Max char size: -00.00
unsigned long lastShowOutTempTimeSlot = 0;
uint8_t gNewOutTempToShow = false;
uint8_t gCurrentlyShowingOutsideTemp = false;


#ifdef USE_SHT21
  float gTempFl = 0.0;
  float gHumidityFl = 0;
  String gTempStr = "N/A";
  String gHumidityStr = "N/A";
  unsigned long lastTempHumiditySent = 0;

  //Defining SHT21 via SparkFun-Lib
  HTU21D sht21; // I²C-Address: 0x40
#endif

#ifdef USE_BH1750
  //Define BH1750 lightmeter
  BH1750 lightMeter(0x23);  // I²C-Address: 0x23

  unsigned long lastBrightnessSent = 0;
  uint16_t gLux = 0;
  String gLuxStr = "N/A";
#endif

#ifdef USE_NTP_TIME  
  WiFiUDP ntpUDP;

  // You can specify the time server pool and the offset (in seconds, can be
  // changed later with setTimeOffset() ). Additionaly you can specify the
  // update interval (in milliseconds, can be changed using setUpdateInterval() ).
  NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", 3600, 60000);

  long gTargetDateLong = 0;
  boolean gDisplaySleeping = false;
#endif



//Define an ESP-WiFi-Client for MQTT-connection
WiFiClient espClient;

#ifdef USE_MQTT
  PubSubClient mqttClient(espClient);
#endif



/* *********************************************** Setup ****************************************************/
void setup()  
{
  //Basic assign of vars from config.h
  strncpy(flashIoTdeviceName, myFlashIoTdeviceName, 50);
  strncpy(flashMQTTserver, myFlashMQTTserver, 40);
  strncpy(flashMQTTport, myFlashMQTTport, 6);
  strncpy(flashMQTTbaseTopic, myFlashMQTTbaseTopic, 50);
  strncpy(flashMQTTdeviceTopic, myFlashMQTTdeviceTopic, 50);

  
  Serial.begin(115200);
  delay(100);

  DEBUGln();
  DEBUGln(F("------------------------------------------------------------"));
  DEBUG  (F("  Andreas Smart Clock v"));
  DEBUGln(FW_VERSION);
  DEBUG  (F("  ESP8266 Chip-ID: "));
  DEBUGln(ESP.getChipId());
  DEBUG  (F("  MAC Address: "));
  DEBUGln(WiFi.macAddress());
  DEBUGln();
  DEBUG  (F("  Sketch size (kB): "));
  DEBUG  (ESP.getSketchSize()/1000.0);
  DEBUG  (F(" ("));
  DEBUG  (ESP.getSketchSize());
  DEBUGln(F(" Byte)"));
  DEBUG  (F("  Free space (kB): "));
  DEBUG  (ESP.getFreeSketchSpace()/1000.0);
  DEBUG  (F(" ("));
  DEBUG  (ESP.getFreeSketchSpace());
  DEBUGln(F(" Byte)"));
  DEBUG  (F("  Free Mem/Heap (kB): "));
  DEBUG  (ESP.getFreeHeap()/1000.0);
  DEBUG  (F(" ("));
  DEBUG  (ESP.getFreeHeap());
  DEBUGln(F(" Byte)"));
  DEBUGln();
  DEBUGln(F("           Dipl.-Inform. (FH) Andreas Link"));  
  DEBUGln(F("               Release Build 07.2021"));
  DEBUGln(F("                www.andreaslink.de"));
  DEBUGln(F("             www.link-technologies.de"));
  DEBUGln(F("------------------------------------------------------------"));
  DEBUGln();

  // Prepare Debug-LED GPIO2 (TX1) for testing/measuring (IF NO SERIAL2 IS USED!)
  pinMode(LED_BUILTIN, OUTPUT); // Initialize digital pin LED_BUILTIN as an output.
  digitalWrite(LED_BUILTIN, HIGH); // Turn the LED off (HIGH = off, LOW = on)
  //pinMode(2, OUTPUT); // LED vorbereiten
  //digitalWrite(2, 1); // Turn LED off
  
  ledDoubleFlash();

  //Set unique MAC oriented device identifier
  gIdentifier = getUniqueDeviceIdentifier(); //Predefine unique identifier (from MAC)

  //Setup attached buttons
  initButtons();

  // Read current voltage
  gESPVCC = ESP.getVcc()/1000.0;
  DEBUG(F("Spannung: "));
  DEBUG(gESPVCC);
  DEBUGln(F(" V"));

  //Init LED Matrix Display
  initLEDmatrix();


#ifdef USE_NTP_TIME
  timeClient.begin();
#endif

#ifdef USE_SHT21
  //Initiate I²C
  Serial.println("Init I²C for e.g. SHT21 and/or BH1750...");
  Wire.begin();

  //Init SHT21
  DEBUG(F("Init/Prepare SHT21 Temp/Humidity sensor and read first values..."));
  sht21.begin();  // default: sda=4, scl=5
  updateSHT21();
  DEBUGln(F("done."));

  //Print formerly read temp/humidity
  DEBUGln();
  DEBUGln(F("Temperature and humidity from SHT21 sensor:"))
  DEBUG(F("  Temperature: "));
  DEBUG(gTempStr);
  DEBUGln(F(" C"));
  DEBUG(F("  Humidity: "));
  DEBUG(gHumidityStr);
  DEBUG(F(" %"));
  DEBUGln();
#endif

#ifdef USE_BH1750
  //Initiate I²C
  DEBUG(F("Init I²C + BH1750... "));
  Wire.begin();

  // Init BH1750
  lightMeter.begin(BH1750_CONTINUOUS_HIGH_RES_MODE_2);  // Possible modes: BH1750_CONTINUOUS_LOW_RES_MODE, BH1750_CONTINUOUS_HIGH_RES_MODE (default), BH1750_CONTINUOUS_HIGH_RES_MODE_2
                                                        //                 BH1750_ONE_TIME_LOW_RES_MODE,   BH1750_ONE_TIME_HIGH_RES_MODE,             BH1750_ONE_TIME_HIGH_RES_MODE_2  
  DEBUGln(F("done."));

  //Update current brightness value
  updateLight();
  DEBUGln();
  DEBUG(F(" Measured Light: "));
  DEBUG(gLuxStr);
  DEBUGln(F(" lux"));
#endif


  //Read Data from SPIFFS to prepare for WiFiManager
  DEBUGln(F("Reading from SPIFFS..."));
  if (readDataFromSPIFFS())
  {
    DEBUGln(F("Reading from SPIFFS was successful."));
  } else {
    DEBUGln(F("Reading from SPIFFS FAILED!"));
  }
  DEBUGln();

  // Initial Wifi mode setup and connection to WiFi:
  //Start WiFiManager or connect to WiFi
  DEBUGln(F("Starting WiFiManager and trying to connect to WiFi..."));
  scrollMessage("Connecting...");
  centerPrint("WiFi?");
  if (InitWiFiManager())
  {
    DEBUGln(F("WiFi successfully connected via WiFi-Manager!"));    
  } else {
    DEBUGln(F("WiFi connection via WiFi-Manager FAILED!"));
  }

  //Init protected OTA Update
  initOTA();

#ifdef USE_MQTT
  // Puzzle finale strings together to connect to mqtt server based on saved/entered input
  prepareMQTTparams();
  
  //Init MQTT broker details for MQTT broker and setup callback
  DEBUG(F("Init/Prepare MQTT-server: "));
  DEBUG(flashMQTTserver);
  DEBUG(F(":"));
  DEBUGln(flashMQTTport);
  mqttClient.setServer(flashMQTTserver, atoi(flashMQTTport));
  //mqttClient.setServer((const char *)"192.168.0.253", atoi(flashMQTTport));
  //mqttClient.setServer(mqttServer, atoi(flashMQTTport));
  //mqttClient.setServer(mqttServer, 1883);
  mqttClient.setCallback(mqttCallback);
#endif


#ifdef USE_NTP_TIME
  //Force Summertime --> Needs to be improved via code
  //setSummertime(true);
#endif  

  DEBUGln();
  DEBUGln(F("-- Initial setup is done --"));
  DEBUGln();
}




/* *********************************************** Loop ****************************************************/

void loop()
{
  //Check the button states
  checkButtonStates();
  
  unsigned long now = millis();

  //Clear all time slot counters, if millis reached the end of unsigned long some point in time
  if (now < lastDoSomethingEvent)
    lastDoSomethingEvent = 0;

  if (now < lastShowTextTimeSlot)
    lastShowTextTimeSlot = 0;

  if (now < lastShowTextBlinkTimeSlot)
    lastShowTextBlinkTimeSlot = 0;    

  if (now < lastReconnectAttempt)
    lastReconnectAttempt = 0;

  #ifdef USE_SHT21
  if (now < lastTempHumiditySent)
    lastTempHumiditySent = 0;
  #endif

  #ifdef USE_BH1750
  if (now < lastBrightnessSent)
    lastBrightnessSent = 0;
  #endif
    
  
  //WiFi is still up:
  if (WiFi.status() != WL_CONNECTED)
  {
    DEBUG(timestamp());
    DEBUGln(F(" ERROR: WiFi lost! Rebooting..."));
    ESP.reset();
  }

  //Handle OTA Update progress
  ArduinoOTA.handle();

  #ifdef USE_NTP_TIME
    timeClient.update();
  #endif

 
  #ifdef USE_MQTT
  // Handle MQTT messages, if connected
  if (!mqttClient.connected())
  {    
    if (now - lastReconnectAttempt > 5000) 
    {
      lastReconnectAttempt = now;
      
      // Attempt to reconnect
      if (mqttReconnect()) 
      {
        lastReconnectAttempt = 0;
        mqttClient.loop();
      }
      ledFlash();
      delay(200);
      ledFlash();
    }
  } else {
    // MQTT-Client is connected:
    mqttClient.loop();
      
    #ifdef USE_BH1750
      //Send brightness every ~30 seconds
      if (now - lastBrightnessSent > 30000) 
      {
        lastBrightnessSent = now;
  
        DEBUGln();
        DEBUGln(F("Reading light from BH1750 sensor..."));
        updateLight();
        DEBUG(F("  Measured Light: "));
        DEBUG(gLuxStr);
        DEBUGln(F(" lux"));
                
        sendMQTTmsg(mqttLightTopic, gLuxStr, false);
      }      
    #endif


    // Shall I show the received outside temp?
    if (gNewOutTempToShow)
    {
      gNewOutTempToShow = false;
      
      String temp = gOutsideTempStr + " " + (char)247 + "C";  //Prints: "-12.34 °C"
      scrollMessage(temp,35);      
      gFormerTimeNow = "";

      /*
      //Starting new timeslot
      if (!gCurrentlyShowingOutsideTemp)
      {
        lastShowOutTempTimeSlot = now;
        gCurrentlyShowingOutsideTemp = true;
        setLEDmatrixBrightness(5);
      }
  
      //As long as we are in the time slot
      if (now - lastShowOutTempTimeSlot < 3000)
      {
        centerPrint(gOutsideTempStr);
      } else {
        gCurrentlyShowingOutsideTemp = false;
        gNewOutTempToShow = false;
        gFormerTimeNow = "";
        setLEDmatrixBrightness(1);
      }
      */
    }


    #ifdef USE_SHT21
      if (now - lastTempHumiditySent > 60000)
      {
        lastTempHumiditySent = now;
  
        DEBUGln();
        DEBUG(timestamp());
        DEBUGln(F("Reading temperature and humidity from SHT21 sensor..."));
        updateSHT21();
        DEBUG(F("  Measured Temperature: "));
        DEBUG(gTempStr);
        DEBUG(F(" "));
        //DEBUG((char)247);
        DEBUGln(F("C"));
  
        DEBUG(F("  Measured Humidity: "));      
        DEBUG(gHumidityStr);
        DEBUGln(F(" %"));
        DEBUGln();
        
        //Send values via MQTT
        sendMQTTmsg(mqttTempTopic, gTempStr, false);
        sendMQTTmsg(mqttHumidityTopic, gHumidityStr, false);
      }
    #endif
    
    
  } // End of MQTT connected
  #else
    // If compiled without MQTT support, then show date on demand
    // Shall I show the date?
    if (gNewOutTempToShow)
    {
      gNewOutTempToShow = false;

      //Calc date:
      unsigned long unixNow = getNowUnixTimestamp();
      DateTime rightNow(unixNow);

      String dateToday = "";      
      if (rightNow.day() < 10)
      {
        dateToday = "0" + (String)rightNow.day();
      } else {
        dateToday = (String)rightNow.day();
      }
      dateToday += ".";
      
      if (rightNow.month() < 10)
      {
        dateToday += "0" + (String)rightNow.month();
      } else {
        dateToday += (String)rightNow.month();
      }
      dateToday += "." + (String)rightNow.year();

      DEBUG("Showing the date: ")
      DEBUGln(dateToday);
      
      scrollMessage(dateToday,35);      
      gFormerTimeNow = "";    
    }  
  #endif


  // Is there a message to show in the message buffer?
  if (gNewMessageToShow)
  {
    //Show message once and reset trigger
    gNewMessageToShow = false;
    scrollMessage(gMessage);
    scrollMessage(gMessage);    

    //Just to be sure, if this scrolling message interrupted a short text display to kick in again
    if (gNewTextToShow)
    {
      now = millis();
      lastShowTextTimeSlot = now;
      lastDoSomethingEvent = now;
    } else {
      gFormerTimeNow = "";
    }
  }


  // Is there a short text to show in the message buffer? If yes, show it for some seconds and then reset the trigger
  if (gNewTextToShow)
  {
    //Starting new timeslot
    if (!gCurrentlyShowingText)
    {
      lastShowTextTimeSlot = now;
      gCurrentlyShowingText = true;
      setLEDmatrixBrightness(10);

      //If the display is off, turn it on
      if (!gLEDmatrixTurnedOn)
      {
        //Remind old status
        gLEDmatrixWasTurnedOffBefore = true;
        
        //Turn display on:
        setLEDmatrixOnOffStatus(true);
      }        
    }

    //As long as we are in the time slot
    if (now - lastShowTextTimeSlot < 3000)
    {
      // Toggle blinker
      if (now - lastShowTextBlinkTimeSlot > 250)
      {
        gTextBlinkToggle = !gTextBlinkToggle;
        lastShowTextBlinkTimeSlot = now;
      }

      //Show text depending on toggle state
      if (gTextBlinkToggle)
      {
        centerPrint(gShortText);
      } else {
        clearLEDmatrix();
      }
    } else {
      gCurrentlyShowingText = false;
      gNewTextToShow = false;      
      gFormerTimeNow = "";
      setLEDmatrixBrightness(1);

      if (gLEDmatrixWasTurnedOffBefore)
      {
        //Turn display off again and reset the reminder
        setLEDmatrixOnOffStatus(false);
        gLEDmatrixWasTurnedOffBefore = false;
      }
    }
  }
  
   
  
  
  //Check, if time slot event is reached and usually show current time here:
  if (now - lastDoSomethingEvent > 500)
  {
    lastDoSomethingEvent = now;

    //Evaluate if we are in summer or winter time
    doWeHaveSummerOrWinterTime();

    //Get current time from NTP
    String timeNow = getNowSimpleFormatedTime();
    
    /*
    //Calculate the date values and time span
    unsigned long unixNow = getNowUnixTimestamp();
    DateTime rightNow(unixNow);
    printDateOnSerial("Current DateTime: ", rightNow);
    */

    if ((gFormerTimeNow != timeNow && gLEDmatrixTurnedOn) || gShowTimeOnce)
    {
      //Forceing to show the time briefly, when display is turned off
      if (gShowTimeOnce)
      {
        setLEDmatrixOnOffStatus(true);
        setLEDmatrixBrightness(0);
      }

      //Print the time centered
      centerPrint(timeNow);
      gFormerTimeNow = timeNow;

      if (gShowTimeOnce)
      {
        delay(1500);        
        setLEDmatrixOnOffStatus(false);
        setLEDmatrixBrightness(1);
        gShowTimeOnce = false;
      }
    }
  } 
}
