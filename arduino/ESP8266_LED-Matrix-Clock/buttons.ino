/*
 * This file is handling the touch buttons and their related behaviour.
 * If a button is pressed the press-status is send via MQTT according to the defined topic.
 *
 * Examples:
 *  Button 1 clicked/touched
 *  'zuhause/haus/schlafzimmer/iotdevice-clock/AndreasClockDevice/button1/touched': '1'
 *  
 *  Button 1 long pressed
 *  Turning LED Matrix off... good night!
 *  'zuhause/haus/schlafzimmer/iotdevice-clock/AndreasClockDevice/button1/long': '1'
 *  
 *  Button 2 clicked/touched
 *  'zuhause/haus/schlafzimmer/iotdevice-clock/AndreasClockDevice/button2/touched': '1'
 *  
 *  Button 2 double clicked/touched
 *  'zuhause/haus/schlafzimmer/iotdevice-clock/AndreasClockDevice/button2/doubletouched': '1'
 *  
 *  Button 2 long pressed
 *  'zuhause/haus/schlafzimmer/iotdevice-clock/AndreasClockDevice/button2/long': '1'
 *
 * Remind:
 *  If Button 1 is pushed during boot, it will force the WiFi Manager to start up, so setup can be changed!
 *  
 */


// Create 2 ButtonConfigs. The System ButtonConfig is not used.
ButtonConfig config1;
ButtonConfig config2;

// Two buttons, explicitly bound to their respective ButtonConfig, instead
// of the default System ButtonConfig.
AceButton button1(&config1);
AceButton button2(&config2);

// Two event handlers.
void handleEvent1(AceButton*, uint8_t, uint8_t);
void handleEvent2(AceButton*, uint8_t, uint8_t);


void initButtons()
{
  // Buttons use the built-in pull up register.
  //pinMode(BUTTON1, INPUT_PULLUP);
  //pinMode(BUTTON2, INPUT_PULLUP);

  // Initialize the buttons
  button1.init(BUTTON1, LOW);
  button2.init(BUTTON2, LOW);

  // Configure ButtonConfig 1 with the event handler, and enable all higher
  // level events.
  config1.setEventHandler(handleEvent1);
  config1.setFeature(ButtonConfig::kFeatureClick);
  //config1.setFeature(ButtonConfig::kFeatureDoubleClick);
  config1.setFeature(ButtonConfig::kFeatureLongPress);
  //config1.setFeature(ButtonConfig::kFeatureRepeatPress);
  //config1.setFeature(ButtonConfig::kFeatureSuppressClickBeforeDoubleClick);

  // Configure ButtonConfig 2 with just Click.
  config2.setEventHandler(handleEvent2);
  config2.setFeature(ButtonConfig::kFeatureClick);
  config2.setFeature(ButtonConfig::kFeatureDoubleClick);
  config2.setFeature(ButtonConfig::kFeatureLongPress);
  config2.setClickDelay(250); // increase click delay from default 200 ms
  config2.setFeature(ButtonConfig::kFeatureSuppressClickBeforeDoubleClick);


  // Check if the button was pressed while booting
  if (button1.isPressedRaw()) 
  {
    DEBUGln(F("Button 1 was pressed while booting the device"));
    gForceWiFiManager=true;
    DEBUGln(F("Force WiFi Manager as button was pressed during boot..."));
  }

  
  if (button2.isPressedRaw()) 
  {
    DEBUGln(F("Button 2 was pressed while booting the device"));
    //Consider force WiFiManager here?
  }

  DEBUGln(F("Touch buttons are set up and initiated now"));
}


void checkButtonStates()
{
  // Should be called every 4-5ms or faster, for the default debouncing time of ~20ms.
  button1.check();
  button2.check();
}


//Handle all Button 1 Events
void handleEvent1(AceButton* button, uint8_t eventType, uint8_t buttonState) 
{
  // Control the LED only for the Pressed and Released events of Button 1.
  // Notice that if the MCU is rebooted while the button is pressed down, no
  // event is triggered and the LED remains off.
  switch (eventType) 
  {
    case AceButton::kEventClicked:
      {
        DEBUGln(F("Button 1 clicked/touched"));

        //When display is on, show outside temp, else just briefly show the time
        if (gLEDmatrixTurnedOn)
        {
          gNewOutTempToShow = true;
        } else {
          //Show clock for a second
          gShowTimeOnce = true;
        }        
        

        #ifdef USE_MQTT
        //Send MQTT value
        char topicMQTTbtnTouch[MAX_TOPIC_LENGTH];
        sprintf(topicMQTTbtnTouch, "%s/%s", mqttButton1Topic, "touched"); //Concat topic
        sendMQTTmsg(topicMQTTbtnTouch, "1", false);
        #endif
      }
      break;

    case AceButton::kEventLongPressed:
      {
        DEBUGln(F("Button 1 long pressed"));

        if (gLEDmatrixTurnedOn)
        {
          //Turn display off:
          setLEDmatrixOnOffStatus(false);
        } else {
          //Turn display on:
          setLEDmatrixOnOffStatus(true);
          gFormerTimeNow = "";
        }

        #ifdef USE_MQTT
        //Send MQTT value
        char topicMQTTbtnLong[MAX_TOPIC_LENGTH];
        sprintf(topicMQTTbtnLong, "%s/%s", mqttButton1Topic, "long"); //Concat topic
        sendMQTTmsg(topicMQTTbtnLong, "1", false);
        #endif
      }
    break;

    case AceButton::kEventDoubleClicked:
      DEBUGln(F("Button 1 double clicked!"));
      //ledFlash();
      break;
  }
}


//Handle all Button 2 Events
void handleEvent2(AceButton* button, uint8_t eventType, uint8_t buttonState) 
{
  // Control the LED only for the Pressed and Released events of Button 1.
  // Notice that if the MCU is rebooted while the button is pressed down, no
  // event is triggered and the LED remains off.
  switch (eventType) 
  {
    case AceButton::kEventClicked:
      {
        DEBUGln(F("Button 2 clicked/touched"));

        #ifdef USE_MQTT
        //Send MQTT value
        char topicMQTTbtnTouch[MAX_TOPIC_LENGTH];
        sprintf(topicMQTTbtnTouch, "%s/%s", mqttButton2Topic, "touched");
        sendMQTTmsg(topicMQTTbtnTouch, "1", false);
        #endif
      }
      break;

    case AceButton::kEventDoubleClicked:
      {
        DEBUGln(F("Button 2 double clicked/touched"));
        
        #ifdef USE_MQTT
          //Send MQTT value
          char topicMQTTbtnDbleTouch[MAX_TOPIC_LENGTH];
          sprintf(topicMQTTbtnDbleTouch, "%s/%s", mqttButton2Topic, "doubletouched");
          sendMQTTmsg(topicMQTTbtnDbleTouch, "1", false);
          #endif
      }
      break;

    case AceButton::kEventLongPressed:
      {
        DEBUGln(F("Button 2 long pressed"));
        
        #ifdef USE_MQTT
          //Send MQTT value
          char topicMQTTbtnLong[MAX_TOPIC_LENGTH];
          sprintf(topicMQTTbtnLong, "%s/%s", mqttButton2Topic, "long");
          sendMQTTmsg(topicMQTTbtnLong, "1", false);
          #endif
      }
      break;
  }
}
