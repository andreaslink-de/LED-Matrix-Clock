/*************************************************************************************
Callback notifying of the need to save config to Flash/EEPROM
/*************************************************************************************/
void saveConfigCallback() 
{
  DEBUGln("Changes made to data, remind to save config!");
  shouldSaveConfigToFlash = true;
}


/*************************************************************************************
Read data from SPIFFS aka Flash/EEPROM and use values for connecting device
SPIFFS = (S)erial (P)eripheral (I)nterface (F)lash (F)ile (S)ystem
/*************************************************************************************/
boolean readDataFromSPIFFS()
{
  //Read configuration from JSON file from SPIFFS/Flash memory  
  DEBUG(F("Mounting SPIFFS flash memory... "));

  boolean returnValue = false;

  if (SPIFFS.begin()) 
  {
    DEBUGln(F("Done! Successfully mounted file system."));

    // Find config file, if existing, use it (else save a new one later with new data)
    if (SPIFFS.exists("/config.json")) 
    {
      //File exists, read it and load JSON values
      DEBUG(F("Reading config file... "));
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile) 
      {
        DEBUGln(F("success!"));
        size_t size = configFile.size();
        
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);
        
        DynamicJsonDocument jsonDoc(1024);
        auto jsonError = deserializeJson(jsonDoc, buf.get());        
        if (!jsonError)
        {
          DEBUGln(F("JSON data successfully parsed"));
          returnValue = true;

          //Grab values from JSON and store in corresponding variables
          if(jsonDoc["devicename"]) strcpy(flashIoTdeviceName, jsonDoc["devicename"]);
          
          #ifdef USE_MQTT
          if(jsonDoc["mqttserver"]) strcpy(flashMQTTserver,    jsonDoc["mqttserver"]);
          if(jsonDoc["mqttport"])   strcpy(flashMQTTport,      jsonDoc["mqttport"]);

          if(jsonDoc["mqttbasetopic"])   strcpy(flashMQTTbaseTopic,   jsonDoc["mqttbasetopic"]);
          if(jsonDoc["mqttdevicetopic"]) strcpy(flashMQTTdeviceTopic, jsonDoc["mqttdevicetopic"]);          
          #endif
                    

          if (gForceWiFiManager)
          {
            DEBUGln(F("WARNING: Some required setup is missing in the locally stored data, so the WiFi Manager will be forced to start up, so you can fix it."));
          }
          
          
        } else {          
          DEBUGln(F("ERROR! Failed to open and load JSON config file."));
          DEBUGln(F("Info: If this is the first run, there is no file yet. Please enter data via IoT-Device WiFi-Manager."));
          scrollMessage("No config found");
        }
      }
    }
  } else {    
    DEBUGln(F("ERROR: Failed to mount file system."));
    scrollMessage("No config found");
  }

  //Current values to proceed with:
  DEBUGln(F("Data to proceed with:"));
  DEBUG(F("  IoT Device Name: "));
  DEBUGln(flashIoTdeviceName);

  
  #ifdef USE_MQTT
  DEBUG(F("  MQTT-Server: "));
  DEBUG(flashMQTTserver);
  DEBUG(F(":"));
  DEBUGln(flashMQTTport);
  DEBUG(F("  Topic: "));
  DEBUG(flashMQTTbaseTopic);
  DEBUG(flashIoTdeviceName);
  DEBUGln(flashMQTTdeviceTopic);
  #endif
    
  return returnValue;
}


/*************************************************************************************
Prepare and init WiFiManager
/*************************************************************************************/
bool InitWiFiManager()
{
  // The extra parameters to be configured (can be either global or just in the setup)
  // After connecting, parameter.getValue() will get you the configured value
  // id/name placeholder/prompt default length
  //Prepare Setup screen
  String portalHello = "<hr><center><b>link-tech.de</b><br />Smart IoT-Device v" + (String)FW_VERSION + "</center><br />Please define device details:<br />";
  char portalHelloChar[portalHello.length()];
  portalHello.toCharArray(portalHelloChar, sizeof(portalHelloChar));  
  WiFiManagerParameter custom_head_text(portalHelloChar);
  
  WiFiManagerParameter custom_mqtt_devicename("devicename", "IoT Device", flashIoTdeviceName, 50);
  #ifdef USE_MQTT
  WiFiManagerParameter custom_mqtt_server("mqttserver", "MQTT Server", flashMQTTserver, 40);
  WiFiManagerParameter custom_mqtt_port("mqttport", "MQTT Port (1883)", flashMQTTport, 5);

  WiFiManagerParameter custom_mqtt_text("<hr><b>Please define MQTT values:</b><br /><small>Base-Topic + IoT-Device + MQTT Device Topic + |Btn1|Btn2|Light|Temp|command-Topic</small><br />");
  WiFiManagerParameter custom_mqtt_baseTopic   ("mqttbasetopic", "Base topic: myhome/room/", flashMQTTbaseTopic, 50);
  WiFiManagerParameter custom_mqtt_deviceTopic ("mqttdevicetopic", "Device topic: /deviceX", flashMQTTdeviceTopic, 50);
  #endif
  
  WiFiManagerParameter custom_mqtt_textfooter ("<hr><center><small>Implemented 07.2021 by AndreasLink.de</small></center>");
  

  // WiFiManager: Local initialization. (Once its business is done, there is no need to keep it around.)
  WiFiManager wifiManager;

  // Set config-save notify callback function
  wifiManager.setSaveConfigCallback(saveConfigCallback);
  
  //Order setup screen on potal page:
  wifiManager.addParameter(&custom_head_text);
  
  wifiManager.addParameter(&custom_mqtt_devicename);
  #ifdef USE_MQTT
  wifiManager.addParameter(&custom_mqtt_server);
  wifiManager.addParameter(&custom_mqtt_port);
  
  wifiManager.addParameter(&custom_mqtt_text);
  wifiManager.addParameter(&custom_mqtt_baseTopic);
  wifiManager.addParameter(&custom_mqtt_deviceTopic);
  wifiManager.addParameter(&custom_mqtt_textfooter);
  #endif

    
  //reset settings - for testing
  //wifiManager.resetSettings();

  //DEBUG if needed:
  wifiManager.setDebugOutput(false);

  //Set minimum quality of signal so it ignores AP's under that quality --> defaults to 8%
  wifiManager.setMinimumSignalQuality();
  
  // Sets timeout in seconds until configuration portal gets turned off (useful to make it all retry or go to sleep)
  wifiManager.setTimeout(240); // 4 Mins

  //If MQTT-connects fail too often, force portal to start up to be able to change MQTT values, else do as always
  if (mqttConnectFailCounter > 20)
  {
    shouldSaveConfigToFlash = false;
    String wifiName = "IoT-Device-" + gIdentifier;
    if (!wifiManager.startConfigPortal(wifiName.c_str(), wifiMngrPassword)) 
    {
      DEBUGln(F("Failed to connect with given data. Rebooting device for another try..."));      
      delay(3000);
      ESP.reset();  // Reset and reboot ESP8266    
    }
  } else {
  
    if (gForceWiFiManager)
    {
      DEBUGln(F("Forced WiFi Manager is starting"));
      scrollMessage("Please setup WiFi");
      String wifiName = "IoT-Device-" + gIdentifier;      
      if (!wifiManager.startConfigPortal(wifiName.c_str(), wifiMngrPassword))
      {
        DEBUGln(F("Failed to connect with given data. Rebooting device for another try..."));      
        delay(3000);
        ESP.reset();  // Reset and reboot ESP8266    
      }      
    } else {
      DEBUGln(F("WiFi Manager auto connect is in progress"));
      // "autoConnect" fetches SSID and pass and tries to connect to known WiFi.
      // If it does not connect it starts an access point with the specified name and password.
      // Remind: Device goes into a blocking loop awaiting configuration until former timeout is reached
      String wifiName = "IoT-Device-" + gIdentifier;
      if (!wifiManager.autoConnect(wifiName.c_str(), wifiMngrPassword))
      {
        DEBUGln(F("Failed to connect within timeout timeframe. Rebooting device for another try..."));
        delay(3000);
        ESP.reset();  // Reset and reboot ESP8266
      }
    }
  }


  // !! -- At this point WiFi is connected successfully -- !!
  DEBUGln();
  DEBUGln(F("WiFi connected!"));
  scrollMessage("WiFi OK");
  DEBUGln();

  //Init random generator
  randomSeed(micros());
  
  // Print SSID, IP and MAC address
  DEBUG(F(" SSID: "));
  DEBUGln(WiFi.SSID());
  DEBUG(F(" IP address: "));
  DEBUGln(WiFi.localIP());
  DEBUG(F(" Gateway address: "));
  DEBUGln(WiFi.gatewayIP());  
  DEBUG(F(" MAC address: "));
  DEBUGln(WiFi.macAddress());
  DEBUGln();

  //Update global Strings
  gWiFiStatus = "OK! Connected!";
  gSSID = (String)WiFi.SSID();
  gIP = WiFi.localIP().toString();
  gMacAdr = (String)WiFi.macAddress();
        
  
  //Reset MQTT-connection counter
  mqttConnectFailCounter = 0;
    
  //Read updated parameters
  strcpy(flashIoTdeviceName, custom_mqtt_devicename.getValue());
  
  #ifdef USE_MQTT
  strcpy(flashMQTTserver, custom_mqtt_server.getValue());
  strcpy(flashMQTTport, custom_mqtt_port.getValue());

  strcpy(flashMQTTbaseTopic, custom_mqtt_baseTopic.getValue());
  strcpy(flashMQTTdeviceTopic, custom_mqtt_deviceTopic.getValue());  
  #endif
  
  
  //Save the custom parameters to Flash/EEPROM
  if (shouldSaveConfigToFlash) 
  {
    DEBUGln(F("Configuration updated! Saving config to flash..."));
    
    DynamicJsonDocument jsonDoc(1024);    
    jsonDoc["devicename"] = flashIoTdeviceName;
    #ifdef USE_MQTT
    jsonDoc["mqttserver"] = flashMQTTserver;
    jsonDoc["mqttport"] = flashMQTTport;
    
    jsonDoc["mqttbasetopic"] = flashMQTTbaseTopic;
    jsonDoc["mqttdevicetopic"] = flashMQTTdeviceTopic;    
    #endif
    
    jsonDoc["version"] = FW_VERSION;
    jsonDoc["author"] = "AndreasLink.de";
    

    //Debug-Data output:
    serializeJson(jsonDoc, Serial);
    DEBUGln();
  
    // Save to config JSON file
    File configFile = SPIFFS.open("/config.json", "w");
    if (configFile) 
    {
      serializeJson(jsonDoc, configFile);
      configFile.close();
      DEBUGln(F("Success! New values saved to config file on SPIFFS flash memory."));      
    } else {
      DEBUGln(F("ERROR: Failed to open config file for writing on SPIFFS flash memory to save data. New values are NOT saved!"));      
    }
    DEBUGln();
  }

  return true;
}
