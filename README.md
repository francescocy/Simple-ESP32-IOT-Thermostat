# Simple-ESP32-IOT-Thermostat

DISCLAIMER - This project was originally developed in Italian, I'll try to translate as much of it as possible in English; code and comments are English-only already.

PLEASE USE THE LATEST VERSION OF THE .ino FILE! New features and bugfixes are listed as comments at the beggining of each .ino file.


INTRODUCTION

This project was born out of a simple idea: replacing our old and clunky living room thermostat with a modern IOT one, while keeping it simple, DIY and fully open-source. I didn't want to rely on a home assistant or a dodgy app-controlled pre-built thermostat, as they tend to have inexistent long-term support and come with too many options.


DEVELOPMENT CHOICES

Main objective is to end up with an extremely simple LCD interface in the living room, with remote control possible using an HTML page.
The unit must fit within the original thermostat's volume and mimick its look; a 3D printed faceplate will be designed for this.
The LCD must show and allow editing of all key paramenters (set temperature, current temperature, furnace status...)
Advanced options can be added later and will be controlled through HTML.


HARDWARE:

NodeMCU-32s

Dallas DS18B20 digital temperature sensor

ST7735 SPI LCD 1.8" 128x160 pixel

10A 220V relay, with optocoupler, 5V powered, 3.3V and 5V signal input

HiLink HLK-PM01 Step-Down module, isolated 220V AC -> 5V DC 0.6A


SOFTWARE:

Developed with Arduino IDE 1.8.19

Included libraries: WiFi.h; AsyncTCP.h; ESPAsyncWebServer.h; Wire.h; OneWire.h; DallasTemperature.h; AsyncElegantOTA.h; Arduino_GFX_Library.h

OTA update: https://github.com/ayushsharma82/ElegantOTA


HOW TO USE:

Open the .ino file with Arduino IDE or Atom, edit it to suit your WiFi settings, Board, LCD type/size/rotation, pinout etc... Compile it and upload it!
I hope I'll have time to upload a more extensive guide soon.

Bonus: You can try the pre-compiled .bin file to test the code, but it may not play well with different ESP boards and LCDs/OLEDs; starting from V 1.2.5 you can create a temporary guest wifi network from your router using the baked-in wifi credentials SSID="wifi" PASSWORD="password" (intended for testing only, please download the .ino file, compile it locally with your own wifi credentials and upload it to your esp32).
