// ------------ LED-Matrix Settings ------------
//  CLK -> D5 (SCK)  
//  CS  -> D6 
//  DIN -> D7 (MOSI)
const int pinCS = D6;         // Attach CS to this pin, DIN to MOSI and CLK to SCK (cf http://arduino.cc/en/Reference/SPI )
int gDisplayIntensity = 1;    // This can be set from 0 - 15
const int numberOfHorizontalDisplays = 4; // default 4 for standard 4 x 1 display Max size of 16
const int numberOfVerticalDisplays = 1;   // default 1 for a single row height

/* set ledRotation for LED Display panels (3 is default)
  0: no rotation
  1: 90 degrees clockwise
  2: 180 degrees
  3: 90 degrees counter clockwise (default)
*/
int ledRotation = 3;

//Create LED matrix var
Max72xxPanel matrix = Max72xxPanel(pinCS, numberOfHorizontalDisplays, numberOfVerticalDisplays);


// LED Settings
int refresh = 0;

//Font-Settings (used when scrolling)
int spacer = 1;  // Amount of dots between letters
int width = 5 + spacer; // The font width is 5 pixels + spacer


/*************************************************************************************
Initial Setup of LED Matrix and Hello-Screen
/*************************************************************************************/
void initLEDmatrix()
{
  DEBUG(F("Number of LED Displays: "));
  DEBUGln(String(numberOfHorizontalDisplays));
  
  // initialize LED
  matrix.setIntensity(0); // Use a value between 0 and 15 for brightness

  int maxPos = numberOfHorizontalDisplays * numberOfVerticalDisplays;
  for (int i = 0; i < maxPos; i++) 
  {
    matrix.setRotation(i, ledRotation);
    matrix.setPosition(i, maxPos - i - 1, 0);
  }

  DEBUGln(F("LED matrix created"));
  DEBUGln();
  
  matrix.fillScreen(LOW); // show black
  centerPrint("Hallo");

  // Brightness Fade
  for (int inx = 0; inx <= 15; inx++) 
  {
    matrix.setIntensity(inx);
    delay(100);
  }
  
  for (int inx = 15; inx >= 0; inx--) 
  {
    matrix.setIntensity(inx);
    delay(60);
  }
  
  delay(500);
  clearLEDmatrix();
  matrix.setIntensity(1);
}


/*************************************************************************************
Set brightness on LED matrix display: Use a value between 0 and 15 for brightness
0 = dark
15 = very bright
/*************************************************************************************/
void setLEDmatrixBrightness(uint8_t pBrightness)
{
  if (pBrightness > 15)
    pBrightness = 15;
    
  matrix.setIntensity(pBrightness);
}


/*************************************************************************************
Fully Clear LED-Matrix to black screen
/*************************************************************************************/
void clearLEDmatrix()
{
  matrix.fillScreen(LOW); // show black aka clear screen, all LEDs off
  matrix.write();
}


/*************************************************************************************
Turn LED Matrix on or off, depending on given value
pStatus = true  --> Display ON
pStatus = false --> Display OFF
/*************************************************************************************/
void setLEDmatrixOnOffStatus(uint8_t pStatus)
{
  matrix.shutdown(!pStatus);
  gLEDmatrixTurnedOn = pStatus;  

  // Log status
  if (pStatus)
  {
    DEBUGln(F("Turning LED Matrix on again"));
  } else {
    DEBUGln(F("Turning LED Matrix off... good night!"));  
  }  
}


/*************************************************************************************
Print centered text on screen and clear it before
/*************************************************************************************/
void centerPrint(String pMsg) 
{
  matrix.fillScreen(LOW); // show black aka clear screen, all LEDs off
  int x = (matrix.width() - (pMsg.length() * width)) / 2;

  matrix.setCursor(x, 0);
  matrix.print(pMsg);

  /*
    matrix.drawPixel(matrix.width() - 1, 6, HIGH);
    matrix.drawFastHLine(0, 7, numberOfLightPixels, HIGH);
  */

  matrix.write();
}



/*************************************************************************************
Clear screen and scroll message with given speed (if given)
Display Scroll Speed default to 25; In milliseconds --> Configurable to slow = 35, normal = 25, fast = 15, very fast = 5
/*************************************************************************************/
void scrollMessage(String pMsg)
{
  scrollMessage(pMsg, 25);
}

void scrollMessage(String pMsg, uint16_t pScrollSpeed)
{
  pMsg += " "; // add a space at the end

  if (pScrollSpeed <= 0)
    pScrollSpeed = 25;
  
  for (int i = 0 ; i < width * pMsg.length() + matrix.width() - 1 - spacer; i++) 
  {  
    ArduinoOTA.handle();

    #ifdef USE_MQTT
    // Handle MQTT messages, if connected
    if (mqttClient.connected())
    {
      // MQTT-Client is connected:
      mqttClient.loop();
    }
    #endif
  
    if (refresh == 1) 
      i = 0;
    
    refresh = 0;
    matrix.fillScreen(LOW);

    int letter = i / width;
    int x = (matrix.width() - 1) - i % width;
    int y = (matrix.height() - 8) / 2; // center the text vertically

    while (x + width - spacer >= 0 && letter >= 0) 
    {
      if (letter < pMsg.length())
      {
        matrix.drawChar(x, y, pMsg[letter], HIGH, LOW, 1);
      }

      letter--;
      x -= width;
    }

    matrix.write(); // Send bitmap to display
    delay(pScrollSpeed);
  }
  matrix.setCursor(0, 0);
}
