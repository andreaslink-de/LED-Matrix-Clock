/* 
 ---------------------- Global configuration of the smart LED-Matrix Clock ---------------------- 
 
 If MQTT is used, multiple commands can be send to the clock including a foreign temperature, like the current outside
 temperature. Intention is to always see the time and when wanting to know how cold/warm outside, just touch the first 
 button once.
 
 If display is too bride in the night, hold first button until display turns off. If time is required at night then, just
 touch first button again. Turn display on my holding first button.

 Second button status is only send via MQTT to be used for further smarter home integration by interpreting the 
 button topic it can be "touched", "doubletouched" or "long" and is always having 1 as a value.
 Example: zuhause/haus/schlafzimmer/iotdevice-clock/AndreasClockDevice/button2/touched 1

 It is possible to send commands to the clock to influence it'S behaviour or to send in important infos from another system
 like the housebus system. Remind "showtext" does only show 5 chars on a usual 4x1 LED matrix display.
 
 Example usage:
   mosquitto_pub -h 192.168.42.253 -t "zuhause/haus/schlafzimmer/iotdevice-clock/AndreasClockDevice/command/showtext" -m "Alarm"
   mosquitto_pub -h 192.168.42.253 -t "zuhause/haus/schlafzimmer/iotdevice-clock/AndreasClockDevice/command/setledmatrixbrightness" -m "15"
   mosquitto_pub -h 192.168.42.253 -t "zuhause/haus/schlafzimmer/iotdevice-clock/AndreasClockDevice/command/showmessage" -m "I am scrolling two times..."
   mosquitto_pub -h 192.168.42.253 -t "zuhause/haus/schlafzimmer/iotdevice-clock/AndreasClockDevice/command/setledmatrixstatus" -m "OFF"
 */


//Device Type/Name
char myFlashIoTdeviceName[50] = "iotdevice-clock";


/***** MQTT-Settings *****/
// Outside Temperature to show on demand (button 1 click):
const char* outsideTemperatureMQTTtopic = "zuhause/garten/kinderhaus/aussen/temperatur";

/* MQTT Topic Path:
  flashMQTTbaseTopic + flashIoTdeviceName + flashMQTTdeviceTopic
  zuhause/haus/raum/ + iotdevice-clock + /MyClockDevice
*/

// MQTT Server and base topic as preset values to load into the WiFi-Manager at first setup:
char myFlashMQTTserver[40]       = "192.168.0.253";
char myFlashMQTTport[6]          = "1883";
char myFlashMQTTbaseTopic[50]    = "zuhause/haus/raum/"; //Remind the trailing slash "/"
char myFlashMQTTdeviceTopic[50]  = "/MyClockDevice";     //Remind the leading slash "/"



/***** WiFi-Manager-Settings *****/
//WiFi-Manager-Password
const char* wifiMngrPassword =  "MyWiFiManagerPasswd";



/***** OTA-Settings *****/
//OTA-Password
const char* otaPassword = "MyOTA-password";
