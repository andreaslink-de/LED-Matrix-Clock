#ifdef USE_MQTT
/*************************************************************************************
Send a MQTT message to former connected MQTT broker
/*************************************************************************************/
boolean sendMQTTmsg(const char* pTopic, String pValue, boolean pRetained)
{
  if (mqttClient.connected())
  {
    DEBUG(F("Sending via MQTT... '"));
    DEBUG(pTopic);
    DEBUG(F("': '"));
    DEBUG(pValue);
    
    char msg[MAX_DATA_LENGTH];
    pValue.toCharArray(msg, sizeof(msg));
    
    boolean sendStatus = mqttClient.publish(pTopic, msg, pRetained);
    //boolean sendStatus = mqttClient.publish(pTopic, (byte*)msg, MAX_DATA_LENGTH, pRetained);
    
    if (sendStatus)
    {
      DEBUGln("' ...done");
      //DEBUGln("' OK");
    } else {
      DEBUGln("' ...failed");
      //DEBUGln("' ERROR");
    }      
    //DEBUGln();
  
    return sendStatus;
  } else {
    DEBUGln(F("WARNING: Trying to send MQTT-data, but broker is currently not connected! (MQTT-Status: "));
    DEBUG(getMqttStatusCode());
    DEBUGln(F(")")); 
  }
  
  return false;
}


/*************************************************************************************
MQTT preparation function to assign SPIFFS flash saved values to right variables
and ensure connections and topics are prepared correctly
Contruct:
  1. base topic + device name + device topic
  2. base topic + device name + temp topic
/*************************************************************************************/
void prepareMQTTparams()
{
  /*
  char flashIoTdeviceName[50]   = "iotdevice";
  char flashMQTTserver[40]      = "192.168.42.253";
  char flashMQTTport[6]         = "1883";
  
  char flashMQTTbaseTopic[50]   = "DEVELOPMENT/zuhause/haus/";
  char flashMQTTdeviceTopic[50] = "/raum/sensor";
  char flashMQTTtemp1Topic[50]   = "/raum/temperatur";

  char* mqttServer = "192.168.0.253";
  char* mqttBaseTopic       = "DEVELOPMENT/zuhause/haus/gaestezimmer/raum/desksensor";
  char* mqttTempTopic       = "DEVELOPMENT/zuhause/haus/gaestezimmer/raum/temperatur";
  */

  /* MQTT Topic Path:
    flashMQTTbaseTopic + flashIoTdeviceName + flashMQTTdeviceTopic
    zuhause/haus/raum/ + iotdevice-clock + /MyClockDevice
  */

  //Base Topic with device name for default values
  char baseTopic[MAX_TOPIC_LENGTH];
  sprintf(baseTopic, "%s%s%s", flashMQTTbaseTopic, flashIoTdeviceName, flashMQTTdeviceTopic);
  sprintf(mqttBaseTopic, "%s", baseTopic);

  //Button 1
  char button1Topic[MAX_TOPIC_LENGTH];
  sprintf(button1Topic, "%s%s", mqttBaseTopic, "/button1");
  sprintf(mqttButton1Topic, "%s", button1Topic);

  //Button 2
  char button2Topic[MAX_TOPIC_LENGTH];
  sprintf(button2Topic, "%s%s", mqttBaseTopic, "/button2");
  sprintf(mqttButton2Topic, "%s", button2Topic);

  #ifdef USE_SHT21
  //SHT21-Temperature
  char tempTopic[MAX_TOPIC_LENGTH];
  sprintf(tempTopic, "%s%s", mqttBaseTopic, "/temperature");
  sprintf(mqttTempTopic, "%s", tempTopic);

  //SHT21-Humidity
  char humiTopic[MAX_TOPIC_LENGTH];
  sprintf(humiTopic, "%s%s", mqttBaseTopic, "/humidity");
  sprintf(mqttHumidityTopic, "%s", humiTopic);
  #endif

  #ifdef USE_BH1750
  //BH1750-Brightness
  char lightTopic[MAX_TOPIC_LENGTH];
  sprintf(lightTopic, "%s%s", mqttBaseTopic, "/brightness");
  sprintf(mqttLightTopic, "%s", lightTopic);
  #endif
}



/*************************************************************************************
MQTT reconnect function, when connection to broker is lost 
with assignment to topic
/*************************************************************************************/
boolean mqttReconnect() 
{
  DEBUG(F("Attempting MQTT connection... (MQTT-Status: "));
  DEBUG(getMqttStatusCode());
  DEBUGln(F(")"));
    
  // Create a client ID
  String clientId = "IoTDevice-";
  //clientId += String(random(0xffff), HEX);
  clientId += gIdentifier;

  char topicStatus[MAX_TOPIC_LENGTH];
  sprintf(topicStatus, "%s/%s", mqttBaseTopic, "status"); //Concat topic
    
  // Attempt to connect
  //boolean connect (clientID, willTopic, willQoS, willRetain, willMessage)
  if (mqttClient.connect(clientId.c_str(), topicStatus, 0, true, "offline"))  //Last will
  {
    DEBUG(F("MQTT broker connected as "));
    DEBUG(clientId);
    DEBUG(F(" (Status Code: "));
    DEBUG(getMqttStatusCode());
    DEBUGln(")");

    delay(100);
    
    mqttConnectFailCounter = 0;
 
    // Once connected, publish an announcement...    
    sendMQTTmsg(topicStatus, F("online"), true);

    //Connected MQTT-client ID
    char topicMQTTid[MAX_TOPIC_LENGTH];
    sprintf(topicMQTTid, "%s/%s", mqttBaseTopic, "clientid"); //Concat topic
    sendMQTTmsg(topicMQTTid, clientId, false);

    //Device Name
    char topicDeviceName[MAX_TOPIC_LENGTH];
    sprintf(topicDeviceName, "%s/%s", mqttBaseTopic, "devicename"); //Concat topic
    sendMQTTmsg(topicDeviceName, (String)flashIoTdeviceName, false);

    char topicVer[MAX_TOPIC_LENGTH];
    sprintf(topicVer, "%s/%s", mqttBaseTopic, "version"); //Concat topic
    sendMQTTmsg(topicVer, (String)FW_VERSION, false);

    //Connected SSID
    char topicSSID[MAX_TOPIC_LENGTH];
    sprintf(topicSSID, "%s/%s", mqttBaseTopic, "ssid"); //Concat topic
    sendMQTTmsg(topicSSID, "'" + gSSID + "'", false);

    //IP
    char topicIP[MAX_TOPIC_LENGTH];
    sprintf(topicIP, "%s/%s", mqttBaseTopic, "ip"); //Concat topic
    sendMQTTmsg(topicIP, gIP, false);
    
    //Send MAC-Address
    char topicMAC[MAX_TOPIC_LENGTH];
    sprintf(topicMAC, "%s/%s", mqttBaseTopic, "mac"); //Concat topic
    sendMQTTmsg(topicMAC, gMacAdr, false);
   

    //Measure current voltage and send value
    gESPVCC = ESP.getVcc()/1000.0;
    char topicVoltage[MAX_TOPIC_LENGTH];
    sprintf(topicVoltage, "%s/%s", mqttBaseTopic, "voltage"); //Concat topic
    sendMQTTmsg(topicVoltage, String(gESPVCC, 3), false);

  #ifdef USE_SHT21
    //Send initial Temp/Humidity-Sensor-Values
    sendMQTTmsg(mqttTempTopic, gTempStr, false);
    sendMQTTmsg(mqttHumidityTopic, gHumidityStr, false);
  #endif
  
  #ifdef USE_BH1750
    //Send initial Light-Sensor-Value
    sendMQTTmsg(mqttLightTopic, gLuxStr, false);
  #endif

            
    DEBUGln();
    DEBUGln(F("Subscribing to MQTT topics... "));

    //Outside temperature (dedicated topic)
    DEBUG(F(" >Subscribe to:  '"));
    DEBUG(mqttSubTopicOutTemp);
    DEBUGln("'");
    mqttClient.subscribe(mqttSubTopicOutTemp);
    mqttClient.loop();

    //Reboot ESP
    char topicReboot[MAX_TOPIC_LENGTH];    
    sprintf(topicReboot, "%s/%s", mqttBaseTopic, "command/reboot"); //Concat topic
    DEBUG(F(" >Subscribe to:  '"));
    DEBUG(topicReboot);
    DEBUGln("'");
    mqttClient.subscribe(topicReboot);
    mqttClient.loop();

    //LED ON/OFF
    char topicLED[MAX_TOPIC_LENGTH];    
    sprintf(topicLED, "%s/%s", mqttBaseTopic, "command/led"); //Concat topic
    DEBUG(F(" >Subscribe to:  '"));
    DEBUG(topicLED);
    DEBUGln("'");
    mqttClient.subscribe(topicLED);
    mqttClient.loop();

    //Delete Wifi settings
    char topicDeleteWiFi[MAX_TOPIC_LENGTH];    
    sprintf(topicDeleteWiFi, "%s/%s", mqttBaseTopic, "command/deletewificonfig"); //Concat topic
    DEBUG(F(" >Subscribe to:  '"));
    DEBUG(topicDeleteWiFi);
    DEBUGln("'");
    mqttClient.subscribe(topicDeleteWiFi);
    mqttClient.loop();

    //Format SPIFFS flash memory
    char topicFormatSPIFFS[MAX_TOPIC_LENGTH];    
    sprintf(topicFormatSPIFFS, "%s/%s", mqttBaseTopic, "command/factoryreset"); //Concat topic
    DEBUG(F(" >Subscribe to:  '"));
    DEBUG(topicFormatSPIFFS);
    DEBUGln("'");
    mqttClient.subscribe(topicFormatSPIFFS);
    mqttClient.loop();

    //Scroll Text message on display
    char topicScrollText[MAX_TOPIC_LENGTH];    
    sprintf(topicScrollText, "%s/%s", mqttBaseTopic, "command/showmessage"); //Concat topic
    DEBUG(F(" >Subscribe to:  '"));
    DEBUG(topicScrollText);
    DEBUGln("'");
    mqttClient.subscribe(topicScrollText);
    mqttClient.loop();

    //Show short text on display
    char topicShowText[MAX_TOPIC_LENGTH];    
    sprintf(topicShowText, "%s/%s", mqttBaseTopic, "command/showtext"); //Concat topic
    DEBUG(F(" >Subscribe to:  '"));
    DEBUG(topicShowText);
    DEBUGln("'");
    mqttClient.subscribe(topicShowText);
    mqttClient.loop();

    //Turn display aka led matrix on or off
    char topicSetLEDmatrixStatus[MAX_TOPIC_LENGTH];    
    sprintf(topicSetLEDmatrixStatus, "%s/%s", mqttBaseTopic, "command/setledmatrixstatus");
    DEBUG(F(" >Subscribe to:  '"));
    DEBUG(topicSetLEDmatrixStatus);
    DEBUGln("'");
    mqttClient.subscribe(topicSetLEDmatrixStatus);
    mqttClient.loop();

    //Set display aka led matrix brightness
    char topicSetLEDmatrixBrightness[MAX_TOPIC_LENGTH];    
    sprintf(topicSetLEDmatrixBrightness, "%s/%s", mqttBaseTopic, "command/setledmatrixbrightness");
    DEBUG(F(" >Subscribe to:  '"));
    DEBUG(topicSetLEDmatrixBrightness);
    DEBUGln("'");
    mqttClient.subscribe(topicSetLEDmatrixBrightness);
    mqttClient.loop();
    
    //Subscribe to Date
    DEBUG(F(" >Subscribe to:  '"));
    DEBUG(mqttSubTopicDate);
    DEBUGln("'");
    mqttClient.subscribe(mqttSubTopicDate);
    mqttClient.loop();

    //Subscribe to Time
    DEBUG(F(" >Subscribe to:  '"));
    DEBUG(mqttSubTopicTime);
    DEBUGln("'");
    mqttClient.subscribe(mqttSubTopicTime);
    mqttClient.loop();

    DEBUGln();
    DEBUG(F("...done. MQTT is up and running now! (MQTT-Status: "));
    DEBUG(getMqttStatusCode());
    DEBUGln(F(")"));    
    DEBUGln();
    
  } else {
    //Did not connect, count max 20, then reboot
    mqttConnectFailCounter++;

    if (mqttConnectFailCounter > 20)
    {
      DEBUGln(F("ERROR: MQTT connect failed too often!"));
      DEBUG(F("Return Code: "));
      DEBUGln(getMqttStatusCode());

      DEBUGln(F("MQTT server does not seem to be accessable, starting WiFi-Manager..."));
      InitWiFiManager();
      //DEBUGln(F("Rebooting..."));
      //ESP.restart();
    }
  }
  
  return mqttClient.connected();
}


/*************************************************************************************
MQTT callback function, when receiving a MQTT message
--> Handle incoming MQTT events
/*************************************************************************************/
void mqttCallback(char* topic, byte* payloadByte, unsigned int len)
{
    //No data, no interest
    if (len == 0) return;

    // Prepare Payload into char* "message"
    char* payload = (char *)payloadByte;
    char message[len + 1];
    strlcpy(message, (char *)payloadByte, len + 1);

    String topicStr = String(topic);

    //Date
    if (topicStr.startsWith(mqttSubTopicDate))
    {
      curDate = String(message);
    }

    //Time
    if (topicStr.startsWith(mqttSubTopicTime))
    {
      curTime = String(message);
    }


    //Outside Temperature
    if (topicStr.startsWith(mqttSubTopicOutTemp))
    {
      //Outside temperature received
      DEBUG(timestamp());
      DEBUG(F("MQTT: Outside-Temp via MQTT received with value: "));
      DEBUGln(message);
      gOutsideTempStr = String(message);
    }
    

    //Reboot
    char checkTopic[MAX_TOPIC_LENGTH];
    sprintf(checkTopic, "%s/%s", mqttBaseTopic, "command/reboot");
    if (topicStr.startsWith(checkTopic))
    {
      //Reboot detected
      unsigned char value = relayParsePayload(payload);
      //DEBUG("Parsed value (0=off,1=on,2=toggle): ");
      DEBUG(timestamp());
      DEBUG(F("MQTT: ESP-Reboot via MQTT received with value: "));
      DEBUG(value);
      DEBUGln(F(" ('1' as value triggers reboot of device)"));
            
      if (value == 1)
      {
        DEBUG(F("Restarting ESP8266..."));
        delay(500);
        ESP.restart();
      }      
    }
    

    //Show scrolling message on display
    memset(checkTopic, 0, sizeof(checkTopic));
    sprintf(checkTopic, "%s/%s", mqttBaseTopic, "command/showmessage");
    if (topicStr.startsWith(checkTopic))
    {
      //New message to show received, so prepare buffer
      DEBUG(timestamp());
      DEBUG(F("MQTT: New Message to show received via MQTT with value: "));
      DEBUGln(message);
      gMessage = String(message);
      gNewMessageToShow = true;
    }


    //Show short text centered on display
    memset(checkTopic, 0, sizeof(checkTopic));
    sprintf(checkTopic, "%s/%s", mqttBaseTopic, "command/showtext");
    if (topicStr.startsWith(checkTopic))
    {
      //New short text to show received, so prepare buffer
      DEBUG(timestamp());
      DEBUG(F("MQTT: New short text to show received via MQTT with value: "));
      DEBUGln(message);
      gShortText = String(message);
      gNewTextToShow = true;
    }


    //Set status of LED matrix, turn it on or off
    memset(checkTopic, 0, sizeof(checkTopic));
    sprintf(checkTopic, "%s/%s", mqttBaseTopic, "command/setledmatrixstatus");
    if (topicStr.startsWith(checkTopic))
    {
      //New status received, check and set it
      DEBUG(timestamp());
      DEBUG(F("MQTT: New LED-Matrix status received via MQTT with value: "));
      DEBUGln(message);

      unsigned char ledMatrixStatus = relayParsePayload(payload);
      
      if (ledMatrixStatus == 1 || ledMatrixStatus)
      {
        DEBUG(F("LED-Matrix value '"));
        DEBUG(ledMatrixStatus);
        DEBUGln(F("' is turning the display ON"));
        
        setLEDmatrixOnOffStatus(true);
      } else if (ledMatrixStatus == 0 || !ledMatrixStatus)
      {
        DEBUG(F("LED-Matrix value '"));
        DEBUG(ledMatrixStatus);
        DEBUGln(F("' is turning the display OFF"));
        
        setLEDmatrixOnOffStatus(false);
      }
    }


    //Set brightness of LED matrix
    memset(checkTopic, 0, sizeof(checkTopic));
    sprintf(checkTopic, "%s/%s", mqttBaseTopic, "command/setledmatrixbrightness");
    if (topicStr.startsWith(checkTopic))
    {
      //Analyse new received brightness value
      DEBUG(timestamp());
      DEBUG(F("MQTT: New brightness level received via MQTT with value: "));
      DEBUGln(message);

      //Convert char text to small unsigned integer
      uint8_t brightnessValue = atoi(message);
      
      if (brightnessValue >= 0 && brightnessValue <= 15)
      {
        // Set brightness on LED matrix display: Use a value between 0 and 15 for brightness
        //  0 = dark
        //  15 = very bright

        setLEDmatrixBrightness(brightnessValue);
      } else {
        DEBUG(F("Error: Given brightness level '"));
        DEBUG(brightnessValue);
        DEBUGln(F("' is out of the allowed boundaries (0-15)!"));
      }
    }
  

    //Reset and delete WiFi settings
    memset(checkTopic, 0, sizeof(checkTopic));
    sprintf(checkTopic, "%s/%s", mqttBaseTopic, "command/deletewificonfig");
    if (topicStr.startsWith(checkTopic))
    {
      //Format of memory detected
      unsigned char value = relayParsePayload(payload);
      //DEBUG("Parsed value (0=off,1=on,2=toggle): ");
      DEBUG(timestamp());
      DEBUG(F("MQTT: Deletion and reset of saved wifi data received via MQTT with value: "));
      DEBUG(value);
      DEBUGln(F(" ('1' triggers wifi reset and reboot of device)"));
            
      if (value == 1)
      {
        DEBUG(F("Deleting WiFi settings..."));              
        WiFi.disconnect();
        DEBUGln(F("done!"));
        delay(1000);        
        
        DEBUG(F("Reset and restart ESP8266..."));
        delay(2000);
        ESP.reset();  // Reset and reboot ESP8266
      }      
    }


    //Format SPIFFS flash memory - WARNING RADICAL Factory Reset!
    memset(checkTopic, 0, sizeof(checkTopic));
    sprintf(checkTopic, "%s/%s", mqttBaseTopic, "command/factoryreset");
    if (topicStr.startsWith(checkTopic))
    {
      //Format of memory detected
      unsigned char value = relayParsePayload(payload);
      //DEBUG("Parsed value (0=off,1=on,2=toggle): ");
      DEBUG(timestamp());
      DEBUG(F("MQTT: Full factory reset and deletion of all(!) saved data received via MQTT with value: "));
      DEBUG(value);
      DEBUGln(F(" ('1' triggers format and reboot of device)"));
            
      if (value == 1)
      {
        DEBUG(F("Formating SPIFFS memory and deleting everything inkl. WiFi settings..."));
        SPIFFS.format();        
        WiFi.disconnect();
        DEBUGln(F("done!"));
        delay(1000);        
        
        DEBUG(F("Reset and restart ESP8266..."));
        delay(2000);
        ESP.reset();  // Reset and reboot ESP8266
      }
    }

    // LED-ON/OFF
    memset(checkTopic, 0, sizeof(checkTopic));
    sprintf(checkTopic, "%s/%s", mqttBaseTopic, "command/led");
    if (topicStr.startsWith(checkTopic))
    {
      unsigned char value = relayParsePayload(payload);
      //DEBUG("Parsed value (0=off,1=on,2=toggle): ");
      DEBUG(timestamp());
      DEBUG(F("MQTT: LED command via MQTT received with value: "));
      DEBUG(value);
      DEBUGln(F(" ('1' turns LED on, '0' turns LED off, '2' toggles LED)"));
            
      if (value == 1)
      {
        DEBUGln(F("Switching LED to ON"));
        digitalWrite(LED_BUILTIN, LOW); // LED on
      } else if (value == 0)
      {
        DEBUGln(F("Switching LED to OFF"));
        digitalWrite(LED_BUILTIN, HIGH); // LED off
      } else if (value == 2)
      {
        DEBUG(F("Toggle LED: "));
        int curStat = digitalRead(LED_BUILTIN);

        if (curStat == HIGH)
        {
          DEBUGln(F("Switching LED to ON"));
          digitalWrite(LED_BUILTIN, LOW); // LED on          
        } else {
          DEBUGln(F("Switching LED to OFF"));
          digitalWrite(LED_BUILTIN, HIGH); // LED off
        }
      }
    }
}




/*************************************************************************************
Returns current MQTT status as readable String for easier debugging in case of 
conncetion errors or broken connections during usage
/*************************************************************************************/
String getMqttStatusCode()
{
  /*
  * mqttClient.state()
  * -4 : MQTT_CONNECTION_TIMEOUT - the server didn't respond within the keepalive time
  * -3 : MQTT_CONNECTION_LOST - the network connection was broken
  * -2 : MQTT_CONNECT_FAILED - the network connection failed
  * -1 : MQTT_DISCONNECTED - the client is disconnected cleanly
  *  0 : MQTT_CONNECTED - the client is connected
  *  1 : MQTT_CONNECT_BAD_PROTOCOL - the server doesn't support the requested version of MQTT
  *  2 : MQTT_CONNECT_BAD_CLIENT_ID - the server rejected the client identifier
  *  3 : MQTT_CONNECT_UNAVAILABLE - the server was unable to accept the connection
  *  4 : MQTT_CONNECT_BAD_CREDENTIALS - the username/password were rejected
  *  5 : MQTT_CONNECT_UNAUTHORIZED - the client was not authorized to connect
  */  

  int mqttCode = mqttClient.state();
  char retCode[42];

  switch (mqttCode)
  {
    case (0):
      sprintf(retCode, "%d: %s", mqttCode, "MQTT_CONNECTED");      
      return String(retCode);
      break;
    case (-4):
      sprintf(retCode, "%d: %s", mqttCode, "MQTT_CONNECTION_TIMEOUT");
      return String(retCode);
      break;
    case (-3):
      sprintf(retCode, "%d: %s", mqttCode, "MQTT_CONNECTION_LOST");
      return String(retCode);
      break;
    case (-2):
      sprintf(retCode, "%d: %s", mqttCode, "MQTT_CONNECT_FAILED");
      return String(retCode);
      break;
    case (-1):
      sprintf(retCode, "%d: %s", mqttCode, "MQTT_DISCONNECTED");
      return String(retCode);
      break;
    case (1):
      sprintf(retCode, "%d: %s", mqttCode, "MQTT_CONNECT_BAD_PROTOCOL");
      return String(retCode);
      break;
    case (2):
      sprintf(retCode, "%d: %s", mqttCode, "MQTT_CONNECT_BAD_CLIENT_ID");
      return String(retCode);
      break;
    case (3):
      sprintf(retCode, "%d: %s", mqttCode, "MQTT_CONNECT_UNAVAILABLE");
      return String(retCode);
      break;
    case (4):
      sprintf(retCode, "%d: %s", mqttCode, "MQTT_CONNECT_BAD_CREDENTIALS");
      return String(retCode);
      break;
    case (5):
      sprintf(retCode, "%d: %s", mqttCode, "MQTT_CONNECT_UNAUTHORIZED");
      return String(retCode);
      break;      
  }

  // If nothing fits, return the integer as char*  
  //sprintf(retCode, "%d", mqttCode);
  //DEBUG(F("No MQTT Return Code found, returning bare value: "));
  //DEBUGln(retCode);
  return String(mqttCode);
}

#endif

//Parses I/O status:
//  Payload could be "OFF", "ON", "TOGGLE"
//  or its number equivalents: 0, 1 or 2
unsigned char relayParsePayload(const char* payload) 
{
    // Payload could be "OFF", "ON", "TOGGLE"
    // or its number equivalents: 0, 1 or 2

    if (payload[0] == '0') return 0;
    if (payload[0] == '1') return 1;
    if (payload[0] == '2') return 2;

    unsigned int lenPl = strlen(payload);
    if (lenPl>6) 
      lenPl=6;
    
    //Only focus on relevant payload
    char pyld[7];
    strncpy(pyld, payload, lenPl);
    pyld[6] = '\0';

    // trim payload
    char* p = ltrim((char *)pyld);

    // to lower
    unsigned int l = strlen(p);
    //if (l>6) l=6;
    for (unsigned char i=0; i<l; i++) 
    {
        p[i] = tolower(p[i]);
    }

    String compare = (String)p;
    unsigned int value = 0xFF;
    
    if (compare.startsWith("off")) { 
      value = 0;
    } else if (compare.startsWith("on")) { 
      value = 1;
    } else if (compare.startsWith("toggle")) { 
      value = 2;
    }

    return value;
}


/********************************************************************************************\
  Left trim (remove leading spaces)
/********************************************************************************************/
char* ltrim(char* s) 
{
    char *p = s;
    while ((unsigned char) *p == ' ') ++p;
    return p;
}
