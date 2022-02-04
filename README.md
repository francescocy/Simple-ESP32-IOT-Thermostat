# Simple-ESP32-IOT-Thermostat
INTRODUCTION:
This project was born out of a simple idea; replacing our old and clunky thermostat in the living room with a modern IOT device, while keeping it simple, DIY and fully open-source. I didn't want to rely on a home assistant or a dodgy app-controlled pre-built thermostat, as they tend to have no long-term support and also have too many options.

DEVELOPMENT CHOICES:
It's always good practice to make a list of mandatory specs and features, so that both software and hardware are developed from the ground-up to include them.
Main objective is to end up with an extremely simple LCD interface in the living room.
The unit must fit within the original thermostat's volume and mimick its look; a 3D printed faceplate will be designed for this.
The LCD must show and allow editing of all key paramenters (set temperature, current temperature, furnace status...)
Advanced options can be added later and will be controlled through an HTML page.

HARDWARE:
NodeMCU-32s
Dallas DS18B20 digital temperature sensor
ST7735 SPI LCD 1.8"
10A 220V relay, with optocoupler, 5V powered, 3.3V and 5V signal input
Step-Down module, isolated 220V AC -> 5V DC 0.6A

SOFTWARE:
Developed with Arduino IDE 1.8.19
Included libraries: WiFi.h; AsyncTCP.h; ESPAsyncWebServer.h; Wire.h; OneWire.h; DallasTemperature.h; AsyncElegantOTA.h; Arduino_GFX_Library.h

HOW TO USE:
You have two possible choices:
1-[NOT RECOMMENDED!] Download the .bin file in the Arduino Code folder and upload to a NodeMCU-32s or clone board, buy identical hardware and wire it up exactly the same as mine (pinout can be seen in the .ino file, I'll try to add a schematic later).
2-[RECCOMENDED] Open the .ino file with Arduino IDE or Atom, edit it to suit your controller, LCD type/size, pinout etc... Compile it and upload it!
