# LED-Matrix-Clock

This [Link-Technologies.de](http://link-tech.de) project is about a simple ESP8266 driven clock.
Ok, another clock you ask, what is the purpose and what makes it special?

Well, as I am shortsighted and as I am sleeping without my glasses, I needed to have a clock on my nightstand, which is not too big, but big enough to be read without glasses in the night. It also has to be readable, but not too bright to disturb. It has to be exact and probably also slightly smart - and so I decided to create my own.


_//Pictures here_


This project describes a smart **LED-Matrix Clock**, which is readable without glasses in the night including some smart touch buttons and a temp/humidity sensor.

This clock is able to...
- show local time in big letters
- change brightness
- grab exact current time from internet
- be turned off, if being to bright
- can be turned on with a touch
- connects to the local MQTT server
- has too touch buttons and posts the action to the MQTT server
- subscripe to a topic of outside temperature
- can show outside temperature on a touch
- has build sensor to measure rooms local temperature and humidity
- can show sliding messages to transmit information
- can show blinking alarm messages requiring attention


## Technical Components

This clock is made of/implements...
 - an ESP8266 (as WeMos D1 Mini) as core CPU
 - four chained LED matrix (8x8) elements up to a visible 8x32 LED-Matrix
 - a SHT21 for local temperature and humidity measurement
 - two touch buttons (one dedicated and one multipurpose)
 - Self designed black case (PLA) to dimm the brightness and hold it all together

----------

 _Remind, this is still work in progress_
 