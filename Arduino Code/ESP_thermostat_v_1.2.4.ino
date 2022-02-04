/*
 * New in release 1.2.4:
 * Added emojis -> more intuitive HTML view
 * Starting a timer will now automatically enable thermostat
 * 
 * New in release 1.2.3:
 * Removed debug variables introduced in 1.2.2 to track down brownout-like problems
 * Added auto-refresh of main page after variable input. A fallback link is also present if the page is not loaded
 * Improved timer readability on built-in LCD by aligning single digits to the right
 */

#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Wire.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <AsyncElegantOTA.h>; //this enables OTA updates at IP/update

//setting up GFX for TFT Display starts here
#include <Arduino_GFX_Library.h>
#include "FreeMono8pt7b.h"

/* More dev device declaration: https://github.com/moononournation/Arduino_GFX/wiki/Dev-Device-Declaration */
#if defined(DISPLAY_DEV_KIT)
Arduino_GFX *gfx = create_default_Arduino_GFX();
#else /* !defined(DISPLAY_DEV_KIT) */

/* More data bus class: https://github.com/moononournation/Arduino_GFX/wiki/Data-Bus-Class */
Arduino_DataBus *bus = create_default_Arduino_DataBus();

/* More display class: https://github.com/moononournation/Arduino_GFX/wiki/Display-Class */
Arduino_GFX *gfx = new Arduino_ST7735(bus, TFT_RST, 1 /* rotation */, false /* IPS */);

#endif /* !defined(DISPLAY_DEV_KIT) */
/*******************************************************************************
 * End of Arduino_GFX setting
 ******************************************************************************/
const float tscale = 0.88; /*time scaling to adjust for clock inaccuracy; won't be perfect,
but saves us from using NTP or an RTC (lower values make the timer run faster)*/
float   timer = 0;  //time in min
float   tcnt = 0;   //actual time counter
int     hrrem = 0;  //H remaining
int     minrem = 0; //Min remaining
int     secrem = 0; //Sec remaining
#define  maxtemp 30 //Max allowed temperature in input field
#define  mintemp 5  //min allowed temperature in input field
float   settemp = 15;
#define temp_offset -3.6 //sensor offset in Celsius

// PWM or DAC specs for BL control
#define TFT_BL     25
#define maxdutycycle 255
#define mindutycycle 210
#define BL_cntDownMax 50
int     dutycycle = 255;
int     BTN_Press = 0;
int     BL_cntDown = BL_cntDownMax;

//Hardware Buttons
int del = 0; //general delay to run at beginning of loop
#define btndelays 2 //avoids double press
#define btndelayl 10 //delay for longer operations, like resetting timer
#define BTN1      13 //on/off
#define BTN2      14 //timer
#define BTN3      16 //temp +
#define BTN4      17 //temp -

const char* ssid = "YOUR_SSID_HERE";   //replace with your SSID
const char* password = "YOUR_PASSWORD_HERE"; //password

String input_field = "15.0";
String last_temperature;
String enable_trigger = "checked";
String input_field2 = "true";
String input_field3 = "0.0";
String reboot = "";
String input_field4 = "false";
String anim = "+";
String heater = "";
String hostname = "Termostato";
String firmware = "V1.2.4 04/02/22";
int caldaia = 0;

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html><head>
  <title>termostato</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">

  <style>
    html {font-family: Times New Roman; display: inline-block; text-align: center;}
    h2 {font-size: 2.0rem;}
    h3 {font-size: 2.0rem; color: #FF0000;}
  </style>
  
  </head><body>
  <h2>Temperatura Attuale:</h2> 
  <h3>%TEMPERATURE% &deg;C</h3>
  <a>Caldaia: %HEATER%</a><br><br>
  <form action="/get">
    &#127777;&#65039; Temperatura Desiderata:<br><input type="number" step="0.5" name="threshold_input" value="%THRESHOLD%" required><br><br>
    &#9201;&#65039; Timer (countdown in Min):<br><input type="number" step="0.1" name="timer_input" value="%TIMER%" required><br><br>
    Spunta per accendere<input type="checkbox" name="enable_triggerInput" value="true" %ENABLE_TRIGGER_INPUT%><br><br>
    Riavvia (in caso di blocco)<input type="checkbox" name="reboot_input" value="true" %ENABLE_REBOOT%><br><br>
    Premi Conferma una volta inserite<br>le impostazioni desiderate<br>
    <input type="submit" value="Conferma">
  </form><br>
  <a href='/'>&#128260; Ricarica per aggiornare i dati</a><br><br>
  <a>Hardware: NodeMCU-32s; DS18B20</a><br>
  <a>Firmware: %FW%</a><br><br>
  <a href='https://cloud.lilik.it/s/Az2rG9LDqwCJ4ME'>Guida, Sorgente e Info Utili</a><br><br>
  <a href='/update'>&#11014;&#65039; OTA Firmware_Update</a>
</body></html>)rawliteral";

void notFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "Not found");
}

void(* resetFunc) (void) = 0; // create a standard reset function

AsyncWebServer server(80);

String processor(const String& var){

  if(var == "TEMPERATURE"){
    return last_temperature;
  }
  else if(var == "THRESHOLD"){
    return String(settemp, 1);
  }
  else if(var == "ENABLE_TRIGGER_INPUT"){
    return enable_trigger;
  }
  else if(var == "TIMER"){
    return String((timer), 1);
  }
  else if(var == "ENABLE_REBOOT"){
    return reboot;
  }
  else if(var == "FW"){
    return firmware;
  }
  else if(var == "HEATER"){
    return heater;
  }
//starting here we add extra variables to monitor!
  else if(var == "BLCNTD"){
    return String((BL_cntDown), 3);
  }
  else if(var == "TCNT"){
    return String((tcnt), 3);
  }
  else if(var == "DEL"){
    return String((del), 3);
  }
//and they end here
  return String();
}

bool triggerActive = false;

const char* input_paramter1 = "threshold_input";
const char* input_paramter2 = "enable_triggerInput";
const char* input_paramter3 = "timer_input";
const char* input_paramter4 = "reboot_input";

unsigned long previousMillis = 0;     
const long interval = 5;    


const int ESP_output = 12;
const int oneWireBus = 32;     
OneWire oneWire(oneWireBus);
DallasTemperature sensors(&oneWire);

//Create a function to check when WiFi drops using events and automatically re-connect
void Wifi_disconnected(WiFiEvent_t event, WiFiEventInfo_t info){
  WiFi.begin(ssid, password);
}

void setup() {

//Hardware Buttons setup
pinMode(BTN1, INPUT_PULLUP);
digitalWrite(BTN1, HIGH);
pinMode(BTN2, INPUT_PULLUP);
digitalWrite(BTN2, HIGH);
pinMode(BTN3, INPUT_PULLUP);
digitalWrite(BTN3, HIGH);
pinMode(BTN4, INPUT_PULLUP);
digitalWrite(BTN4, HIGH);

//Brightness DAC control setup
#if defined(LCD_PWR_PIN)
pinMode(LCD_PWR_PIN, OUTPUT);    // sets the pin as output
digitalWrite(LCD_PWR_PIN, HIGH); // power on
#endif
pinMode(TFT_BL, OUTPUT);
//digitalWrite(TFT_BL, HIGH);
dacWrite(TFT_BL,dutycycle);
  
//initializing TFT LCD
// Use this initializer if using a 1.8" TFT screen:
gfx->begin();
gfx->fillScreen(BLACK);

//setting the font
gfx->setFont(&FreeMono8pt7b);
    //gfx->setCursor(10, 10);
    gfx->setTextColor(WHITE, BLACK);
    gfx->setCursor(0, 20);
    gfx->println("Ciao!");
    gfx->setCursor(0, 40);
    gfx->println("Sono il tuo");
    gfx->setCursor(0, 55);
    gfx->println("termostato IOT");
    gfx->setCursor(0, 75);
    gfx->println("F.W.Version:");
    gfx->setCursor(0, 90);
    gfx->setTextColor(GREEN, BLACK);
    gfx->println(firmware);
    gfx->setCursor(0, 120);
    gfx->setTextColor(WHITE, BLACK);
    gfx->println("FOSS by Barb");

    delay(1000); // 1sec
//End of TFT LCD initialization
  
//Starting from now, a wifi connection is established and its parameters are printed on the LCD
  WiFi.mode(WIFI_STA);
  WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE, INADDR_NONE);
  WiFi.setHostname(hostname.c_str()); //define hostname
  WiFi.onEvent(Wifi_disconnected, SYSTEM_EVENT_STA_DISCONNECTED); //call on wifi reconnection if disconnected
  WiFi.begin(ssid, password);
  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    gfx->println("Connecting...");
    return;
  }
  delay(1000); // 1sec
  gfx->fillScreen(BLACK);
  delay(100);

//Print main menu elements only once, this helps reduce flicker
    gfx->setTextColor(WHITE, BLACK);
    gfx->setCursor(0, 30);
    gfx->println("T Impost.");
    gfx->setCursor(0, 50);
    gfx->println("T Attuale");
    gfx->setCursor(0, 70);
    gfx->println("Caldaia");
    gfx->setCursor(0, 90);
    gfx->println("Timer");
    //show button layout on screen
    gfx->setCursor(0, 125);
    gfx->println("I/0 Timer T+  T-");

pinMode(ESP_output, OUTPUT);
digitalWrite(ESP_output, LOW);
  

sensors.begin();
  
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);
  });

  server.on("/get", HTTP_GET, [] (AsyncWebServerRequest *request) {
    if (request->hasParam(input_paramter1)) {
      input_field = request->getParam(input_paramter1)->value();
      settemp = input_field.toFloat();
    }
    if (request->hasParam(input_paramter3)) {
      input_field = request->getParam(input_paramter3)->value();
      timer = input_field.toFloat();
    }
      if (request->hasParam(input_paramter2)) {
        input_field2 = request->getParam(input_paramter2)->value();
        enable_trigger = "checked";
        triggerActive = true;
      }
      else {
        input_field2 = "false";
        enable_trigger = "";
        triggerActive = false;
      }
      if (request->hasParam(input_paramter4)) {
        input_field4 = request->getParam(input_paramter4)->value();
        reboot = "checked";
      }
      else {
        reboot = "";
      }
    request->send_P(200, "text/html", index_html, processor); // Re-send main page, with data!
    //request->send(200, "text/html", "<h1 style="font-size: 2rem">href='/'>Return to Home Page</h1>");  //requires manual click, but font is larger
  });
  server.onNotFound(notFound);
  AsyncElegantOTA.begin(&server); //enables OTA updates from the browser
  server.begin();
}

void loop() {

//generalised delay logic, makes it easier to count per cycle without interrupting everything every time
delay(del + 45);
if (del > 0) {
  del--;
}
else {
  del = 0;
}

//soft reset
if (reboot == "checked") {
  resetFunc();
}
else if (digitalRead(BTN1) == LOW && digitalRead(BTN2) == LOW) {
  resetFunc();
}

//safety limits to avoid over/under-heating
  if(settemp<mintemp){
    settemp=maxtemp;
  }
  if(settemp>maxtemp){
    settemp=mintemp;
  }
//Hardware Buttons Input Logic
  if(digitalRead(BTN1) == LOW && input_field2 == "false" && enable_trigger == "" && del == 0){
    triggerActive = true;
    input_field2 = "true";
    enable_trigger = "checked";
    BTN_Press = 1;
    del = btndelays;
  }
  else if(digitalRead(BTN1) == LOW && input_field2 == "true" && enable_trigger == "checked" && del == 0){
    triggerActive = false;
    input_field2 = "false";
    enable_trigger = "";
    BTN_Press = 1;
    del = btndelays;
  }

  if(digitalRead(BTN2) == LOW && timer < 361 && del == 0){
    //timer = timer+30*120*tscale; //scaling should be 120, as we count down in 500ms, but for some reason it's off by 5%
    timer = timer + 30;
    BTN_Press = 1;
    del = btndelays;
  }
  if(digitalRead(BTN2) == LOW && timer >= 361 && del == 0){
    timer = 0;
    BTN_Press = 1;
    del = btndelays;
  }
tcnt = timer*120*tscale;

//Beginning of Timer countdown logic
  if (tcnt == 0) {
    // Do nothing, default condition
  }
  else if (tcnt == 1){
    //End of countdown, disable thermostat, turn gas furnace off
    digitalWrite(ESP_output, LOW);
    triggerActive = false;
    input_field2 = "false";
    enable_trigger = "";
    tcnt = 0;
    //gfx->fillScreen(BLACK);
  }
  else if (tcnt > 0 && digitalRead(BTN2) == HIGH && del == 0){
    //added commands to enable thermostat when timer is active to simplify user input
    triggerActive = true;
    input_field2 = "true";
    enable_trigger = "checked";
    tcnt--;
  }
  else if (tcnt < 0){
    digitalWrite(ESP_output, LOW);
    triggerActive = false;
    input_field2 = "false";
    enable_trigger = "";
    tcnt = 0;
  }

//Computing time in minutes
timer = tcnt/(120*tscale);

//Increase and decrease temperature with hardware buttons
  if(digitalRead(BTN3) == LOW && del == 0){
    settemp=settemp+.5;
    BTN_Press = 1;
    del = btndelays;
  }
  if(digitalRead(BTN4) == LOW && del == 0){
    settemp=settemp-.5;
    BTN_Press = 1;
    del = btndelays;
  }
  input_field.concat(settemp); //Pass set temperature value to HTML string
  
//BL dimming logic
  if (BTN_Press == 1){
    BL_cntDown = BL_cntDownMax;
  }
  while (dutycycle < maxdutycycle && BL_cntDown == BL_cntDownMax){
    dutycycle++;
    dacWrite(25,dutycycle);
    delay(10);
  }

  if (BTN_Press == 0 && BL_cntDown > 0){
    BL_cntDown--;
  }
  while (dutycycle > mindutycycle && BL_cntDown == 1){
    dutycycle--;
    dacWrite(25,dutycycle);
    delay(10);
  }
  BTN_Press = 0;
//End of dimming logic
  
//Sensor integration logic and visual interface
//Static elements are printed only once during setup, this helps reduce flicker
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    sensors.requestTemperatures();
    // Temperature in Celsius degrees 
    float temperature = sensors.getTempCByIndex(0)+(temp_offset);
    gfx->setTextColor(WHITE, BLACK);
    gfx->setCursor(0, 10);
    //gfx->println("Stato Riscaldamento:");
    if (input_field2 == "true"){
      gfx->setTextColor(GREEN, BLACK);
      gfx->println("Mod. Invernale");
    }
    else {
      gfx->setTextColor(BLUE, BLACK);
      gfx->println("Spento        ");
    }
    gfx->setTextColor(WHITE, BLACK);
    //gfx->setCursor(0, 30);
    //gfx->println("T Impost.");
    gfx->setCursor(110, 30);
    //gfx->println(input_field.toFloat());
    gfx->println(settemp);
    //gfx->setCursor(0, 50);
    //gfx->println("T Attuale");
    gfx->setCursor(110, 50);
    gfx->println(temperature);
    //gfx->setCursor(0, 70);
    //gfx->println("Caldaia");
    gfx->setCursor(110, 70);
    if (caldaia == 1 && input_field2 == "true"){
      heater = "ON &#128293;";
      gfx->setTextColor(GREEN, BLACK);
      gfx->println("ON ");
    }
    else {
      heater = "OFF &#10060;";
      gfx->setTextColor(BLUE, BLACK);
      gfx->println("OFF");
    }
    gfx->setTextColor(WHITE, BLACK);
    //gfx->setCursor(0, 90);
    //gfx->println("Timer");
    gfx->setCursor(60, 90);
    gfx->println("             ");

    //computing remaining time
    if (timer > 0) {
      gfx->setTextColor(GREEN, BLACK);
    }
      hrrem = timer/(60); //make integer
      minrem = (timer-hrrem*60);
      secrem = 60*(timer-hrrem*60-minrem);
    
    //displaying remaining time, since v1.2.3 single digits are shifted by 9 pixels to align right

    gfx->setCursor(90, 90); gfx->println("  ");
    if (hrrem < 10) {
      gfx->setCursor(99, 90); gfx->println(hrrem);
    }
    else {
      gfx->setCursor(90, 90); gfx->println(hrrem);
    }
    
    gfx->setCursor(108, 90); gfx->println(":");
    gfx->setCursor(115, 90); gfx->println("  ");
    if (minrem < 10) {
      gfx->setCursor(124, 90); gfx->println(minrem);
    }
    else {
      gfx->setCursor(115, 90); gfx->println(minrem);
    }
    
    gfx->setCursor(133, 90); gfx->println(":");
    gfx->setCursor(140, 90); gfx->println("  ");
    if (secrem < 10) {
      gfx->setCursor(149, 90); gfx->println(secrem);
    }
    else {
      gfx->setCursor(140, 90); gfx->println(secrem);
    }
    
//Pass Current Temp to HTML string processor
    last_temperature = String(temperature);
    
//Thermostat control logic
    if(temperature > settemp+0.25){
      triggerActive = true;
      caldaia = 0;
      digitalWrite(ESP_output, LOW);
    }
    else if((temperature < settemp-0.25) && input_field2 == "true" && triggerActive) {
      triggerActive = false;
      caldaia = 1;
      digitalWrite(ESP_output, HIGH);
    }
    if(input_field2 != "true") {
      digitalWrite(ESP_output, LOW);
      triggerActive = false;
    }
  }
}
