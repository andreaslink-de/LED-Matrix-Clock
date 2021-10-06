/*************************************************************************************
Initialises OTA process and prepares all necessary functions
/*************************************************************************************/
void initOTA()
{
  // Port defaults to 8266
  //ArduinoOTA.setPort(8266);
  
  // Hostname defaults to esp8266-[ChipID]
  // ArduinoOTA.setHostname("myesp8266");
  char otaHost[50];  
  sprintf(otaHost, "%s-%s-%s", "IoTDevice", flashIoTdeviceName, gIdentifierAry);
  ArduinoOTA.setHostname(otaHost);
  
  
  // No authentication by default
  ArduinoOTA.setPassword(otaPassword);
    
  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f197a57a5a743894a0e40815fc3
  // ArduinoOTA.setPasswordHash("21232f197a57a5a743894a0e40815fc3");
    
  ArduinoOTA.onStart([]()
  {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) 
    {
      type = "sketch";
    } else { // U_FS
      type = "filesystem";
    }

    // NOTE: if updating FS this would be the place to unmount FS using FS.end()
    DEBUGln("OTA: Start updating " + type);

    #ifdef USE_MQTT
      // If connected, send message
      //Connected MQTT-client ID
      char topicFirmwareUpdate[MAX_TOPIC_LENGTH];
      sprintf(topicFirmwareUpdate, "%s/%s", mqttBaseTopic, "firmware");
      sendMQTTmsg(topicFirmwareUpdate, "'Firmware upgrade '" + type + "' via OTA initiated...'", false);    
    #endif
  });
  
  ArduinoOTA.onEnd([]() 
  {
    DEBUGln(F("\nFinshed OTA Update"));
  });
  
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) 
  {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  
  ArduinoOTA.onError([](ota_error_t error) 
  {
    Serial.printf("Error[%u]: ", error);
    
    if (error == OTA_AUTH_ERROR) 
    {
      DEBUGln(F("OTA: Authentication Failed"));
    } else if (error == OTA_BEGIN_ERROR) 
    {
      DEBUGln(F("OTA: Begin Failed"));
    } else if (error == OTA_CONNECT_ERROR) 
    {
      DEBUGln(F("OTA: Connect Failed"));
    } else if (error == OTA_RECEIVE_ERROR) 
    {
      DEBUGln(F("OTA: Receive Failed"));
    } else if (error == OTA_END_ERROR) 
    {
      DEBUGln(F("OTA: End Failed"));
    }
  });
  
  ArduinoOTA.begin();

  DEBUGln();
  DEBUGln(F("ESP is ready for an OTA update!"));
}
