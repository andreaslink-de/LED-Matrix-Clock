/*************************************************************************************
Blue On-Board LED short flash
/*************************************************************************************/
void ledFlash()
{
    digitalWrite(LED_BUILTIN, LOW); // LED on
    delay(130);
    digitalWrite(LED_BUILTIN, HIGH); // LED off
}


/*************************************************************************************
Blue On-Board LED double flash
/*************************************************************************************/
void ledDoubleFlash()
{
    digitalWrite(LED_BUILTIN, LOW); // LED on
    delay(70);
    digitalWrite(LED_BUILTIN, HIGH); // LED off
    delay(50);
    digitalWrite(LED_BUILTIN, LOW); // LED on
    delay(70);
    digitalWrite(LED_BUILTIN, HIGH); // LED off
}


/*************************************************************************************
Return current date/time timestamp if received from bus
/*************************************************************************************/
String timestamp()
{
  if (curDate == "" || curTime == "")
    return "";
    
  return curDate + " " + curTime + ": ";
}


/*************************************************************************************
Parses hours from current timestamp (if received from bus) and sets a sleep time
to protect the OLED a little bit over night
/*************************************************************************************/
boolean weAreInSleeptime()
{
  if (curTime == "")
    return false;

  int hours = 0;
  String parsedHours = "";
  
  if (curTime.indexOf(':') <= 0)
    return false;

  //DEBUGln();
  DEBUG(F(" Sleeptime-Hours: "));
  parsedHours = curTime.substring(0,2);
  DEBUG(parsedHours);
  DEBUG(F(" (Sleeptime is between 2:00 and 6:00) --> "));

  hours = parsedHours.toInt();
  boolean sleeptime = (hours > 1 && hours < 6);

  if (sleeptime)
  {
    DEBUGln(F("true: sleeping"));
  } else {
    DEBUGln(F("false: be awake"));
  }
    
  return sleeptime;
}


/*************************************************************************************
Get MAC-Adress from ESP8266 by reading station MAC
/*************************************************************************************/
String getUniqueDeviceIdentifier() 
{
  uint8_t mac[6];
  WiFi.macAddress(mac);
  
  //Split and define global identifier
  char baseIdentifierFromMacChr[7] = {0};
  sprintf(baseIdentifierFromMacChr, "%02X%02X%02X", mac[3], mac[4], mac[5]);

  // Dirty implemenation for saving the char array value
  strcpy(gIdentifierAry, baseIdentifierFromMacChr);
  
  return String(baseIdentifierFromMacChr);
}


/*************************************************************************************
Decodes special HTML chars into "normal" chars
/*************************************************************************************/
String decodeHtmlString(String msg) 
{
  String decodedMsg = msg;
  // Restore special characters that are misformed to %char by the client browser
  decodedMsg.replace("+", " ");
  decodedMsg.replace("%21", "!");
  decodedMsg.replace("%22", "");
  decodedMsg.replace("%23", "#");
  decodedMsg.replace("%24", "$");
  decodedMsg.replace("%25", "%");
  decodedMsg.replace("%26", "&");
  decodedMsg.replace("%27", "'");
  decodedMsg.replace("%28", "(");
  decodedMsg.replace("%29", ")");
  decodedMsg.replace("%2A", "*");
  decodedMsg.replace("%2B", "+");
  decodedMsg.replace("%2C", ",");
  decodedMsg.replace("%2F", "/");
  decodedMsg.replace("%3A", ":");
  decodedMsg.replace("%3B", ";");
  decodedMsg.replace("%3C", "<");
  decodedMsg.replace("%3D", "=");
  decodedMsg.replace("%3E", ">");
  decodedMsg.replace("%3F", "?");
  decodedMsg.replace("%40", "@");
  decodedMsg.toUpperCase();
  decodedMsg.trim();
  return decodedMsg;
}



#ifdef USE_BH1750
  /*************************************************************************************
  Read and globally save I²C BH1750 lightmeter value
  /*************************************************************************************/
  void updateLight()
  {
    gLux = lightMeter.readLightLevel();
    gLuxStr = String(gLux);
  }
#endif
