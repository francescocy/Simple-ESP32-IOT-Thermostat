# Simple-ESP32-IOT-Thermostat
INTRODUZIONE:
Questo progetto nasce dalla necessità di sostituire il nostro vetusto e farraginoso cronotermostato di casa con un moderno dispositivo IOT, senza fare però affidamento su prodotti di terze parti basati su App proprietarie.

SCELTE PROGETTUALI:
L'obiettivo è ottenere un'interfaccia locale estremamente semplice, che possa essere capita a colpo d'occhio. Funzioni più avanzate (come programmazione settimanale e aggiunta sensori e controllo pompe di calore delle camere) potranno essere aggiunte in seguito e controllate tramite interfaccia HTML.
Il termostato deve essere integrato nella parete, all'interno della scatolina Bticino LivingLight
Deve essere presente un display LCD che permetta di conoscere e modificare i parametri principali direttamente sul termostato, senza bisogno di accedere alla pagina WEB; il display e la pagina devono aggiornarsi l'un l'altra in tempo reale.

HARDWARE
NodeMCU-32s
Dallas DS18B20 digital temperature sensor
ST7735 SPI LCD 1.8"
10A 220V relay, with optocoupler, 5V powered, 3.3V and 5V signal input
Step-Down module, isolated 220V AC -> 5V DC 0.6A

SOFTWARE:
Developed with Arduino IDE 1.8.19
Included libraries: WiFi.h; AsyncTCP.h; ESPAsyncWebServer.h; Wire.h; OneWire.h; DallasTemperature.h; AsyncElegantOTA.h; Arduino_GFX_Library.h
