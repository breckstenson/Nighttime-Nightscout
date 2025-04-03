#include <Adafruit_GFX.h>
#include <Adafruit_GrayOLED.h>
#include <Adafruit_SPITFT.h>
#include <Adafruit_SPITFT_Macros.h>
#include <gfxfont.h>
#include <Adafruit_SSD1306.h>
#include <splash.h>
#include <TimeLib.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <Wire.h>
#include "bgArrows.h"
#include "credentials.h"

//declaring screen stuff
#define SCREEN_WIDTH 128 //if using a different screen, change these
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C

#define BUTTON D6

//bg arrow font key:
// u = up arrow
// d = down arrow
// r = right arrow
// s = diagonal up arrow
// t = diagonal down arrow

const char* ssid = WIFI_SSID; //define the wifi network;
const char* pass = WIFI_PASS;
const char* host = NS_SITE; //define the website we are connecting to;
const char* url = "/pebble";
const uint16_t port = 443;
const int timezone = TIMEZONE;
const int timezoneOffset = timezone * SECS_PER_MIN; //timezone multiplied by minutes and seconds

//declare bg variables
int bg;
String delta;
String bgDirection;
time_t nsNow;
time_t bgTime;
String iob;

int nsHour;
int nsMin;

int bgHour;
int bgMin;

int lastReading; //initialize time since last reading variable
int lastReadingMin; //minute that the last reading was retrieved

WiFiClientSecure client; //create a wifi secure object

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

bool isDaylightSavings(time_t nsNow) {
  if (month(nsNow)< 3 || month(nsNow) > 11) { //if we are not in dst months
     return false;
  }

  int secSunMarch = 14 - ((5 * year(nsNow) / 4 + 1) % 7); //calculate the beginning
  int firSunNov = 7 - ((5 * year(nsNow) / 4 + 1) % 7); //calculate the end

  if (month(nsNow) == 3) { //if it's march
    if (day(nsNow) < secSunMarch) { //if the date is before the second sunday in march
      return false;
    } else if (day(nsNow) > secSunMarch) { //if the date is after the second sunday in march
      return true;
    } else if (day(nsNow) == secSunMarch && hour(nsNow) >= 2) { //if the date is the second sunday in march and it's after 2 am
      //adjusting for the start of dst
      return true;
    }
    return false;
  }

  if (month(nsNow) == 11) { //if it's november
    if (day(nsNow) < firSunNov) { //if the date is before the first sun in november
      return true;
    } else if (day(nsNow) > firSunNov) { //if the date is after the first sun in november
      return false;
    } else if (day(nsNow) == firSunNov && hour(nsNow) >= 2) { //if the date is the first sun in november and it's after 2 am
      //this adjusts for the end of dst
      return false; 
    } 
  }
  return true;
}

time_t adjustTimezone (time_t nsNow) {
   time_t adjustedTime = nsNow + timezoneOffset * SECS_PER_MIN; //offset for timezone
  if(isDaylightSavings(nsNow)) { //if it is daylight savings
    adjustedTime += SECS_PER_HOUR; //add an hour in seconds
  }
  return adjustedTime;
}

String adjust12hr () { //convert to 12 hour time and add a leading 0 if under 10
  int hourTemp;
  if (hour() > 12) {
    hourTemp = hour() -12;
  } else {
    hourTemp = hour();
  }

  if (hourTemp < 10) {
    return "0" + String(hourTemp);
  } else {
    return String(hourTemp);
  }
}

String adjustMin() {
  if (minute() < 10) {
    return "0" + String(minute());
  } else {
    return String(minute());
  }
}

String amPm() {
  if (hour() >= 12) {
    return "p";
  } else {
    return "a";
  }
}

String adjustBg(int bgInput) { //adds a space to 2 digit bgs
  if (bgInput < 100) {
    return " " + String(bgInput);
  } else {
    return String(bgInput);
  }
}

void initializeDisplay() {
  display.clearDisplay();

  display.dim(true);

  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(28,25);
  display.cp437(true);
  display.println("Connecting...");

  display.display();
}

void sleepyTime() {
  Serial.println("goodnight");
  ESP.deepSleep(5000);
  Serial.println("good morning!");
}

String selectArrow(String input) {
  if (String(input) == "Flat") {
    return "r";
  } else if (String(input) == "FortyFiveUp") {
    return "s";
  } else if (String(input) == "FortyFiveDown") {
    return "t";
  } else if (String(input) == "SingleUp") {
    return " u";
  } else if (String(input) == "SingleDown") {
    return " d";
  } else if (String(input) == "DoubleUp") {
    return "uu";
  } else if (String(input) == "DoubleDown") {
    return "dd";
  } else {
    return "";
  }
}

void getBg() {
  
  Serial.print("connecting to ");
  Serial.println(host);
  while (!client.connect(host, port)) { //if we can't connect to the host using the secure https port...
    Serial.println("connection failed.");
    //Serial.println(client.getLastSSLError(NULL,15 ));
    delay(10000);
  }

  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
      "Host: " + host + "\r\n" +
      "User-Agent: ESP8266\r\n"
      "Connection: close\r\n\r\n"); //this giberish establishes the connection to the website aparently

  client.find("\r\n\r\n"); //from the text we get, find the blank line (i.e. the end of the header)
  client.find("\n"); //then from there, find the next new line
  String json = client.readStringUntil('\n'); //read this line until we get to the next new line. this will be our json!!! 
  
  //Serial.println(json);

  JsonDocument ns;

  deserializeJson(ns, json);

  bg = ns["bgs"][0]["sgv"]; //get all our variables from the json
  delta = ns["bgs"][0]["bgdelta"].as<String>();
  bgDirection = ns["bgs"][0]["direction"].as<String>();
  nsNow = ns["status"][0]["now"]; //current time according to ns in unix
  bgTime = ns["bgs"][0]["datetime"]; //time since last reading in unix
  iob = ns["bgs"][0]["iob"].as<String>();

  nsNow = nsNow/1000; //ns times are in miliseconds. convert to seconds
  bgTime = bgTime/1000;

  nsNow = adjustTimezone(nsNow);
  bgTime = adjustTimezone(bgTime);
  setTime(nsNow);
  
  nsHour = hour(); //hour of the time given from nightscout
  nsMin = minute(); //minute of the current time

  bgHour = hour(bgTime); //hour of the last reading time
  bgMin = minute(bgTime); //minute of the last reading time

  lastReading; //initialize time since last reading variable

  if (nsHour != bgHour) { //if it's a new hour...
    int minBeforeHour = 60 - bgMin; //number of minutes before the new hour that the last bg was taken
    lastReading = minBeforeHour + nsMin; //add that to the number of minutes after to get the number of mins since last reading
  } else {
    lastReading = nsMin - bgMin;
  }

  lastReadingMin = minute();
}

void displayBg() { //displays bg information for 10 seconds
  Serial.print(hour());
  Serial.print(":");
  Serial.println(minute());

  Serial.println( "bg: " + bg);
  Serial.println("delta: " + delta);
  Serial.println("direction: " + bgDirection);
  Serial.println("iob: " + iob);
  Serial.println(nsMin);
  Serial.println(bgMin);
  Serial.println(lastReading);

  //draw clock
  display.clearDisplay();
  display.fillRect(0,0,128,18,SSD1306_WHITE);
  display.setTextSize(2);
  display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
  display.setCursor(32,1);
  display.print(adjust12hr());
  display.print(":");
  display.print(adjustMin());
  display.print(amPm());

  //draw bg
  display.setTextSize(4);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(5,22);
  display.println(adjustBg(bg));

  //draw arrow
  display.setTextSize(2);
  display.setCursor(80,40);
  display.setFont(&bgArrows);
  display.print(selectArrow(bgDirection));
  display.setFont();
  display.setTextSize(1);
  display.setCursor(110,33);
  display.print(delta);

  //draw iob and last reading
  display.fillRect(0,54,128,10,SSD1306_WHITE);
  display.setTextSize(1);
  display.setTextColor(SSD1306_BLACK,SSD1306_WHITE);
  display.setCursor(10,56);
  display.print("IOB: ");
  display.print(iob);
  display.setCursor(105,56);
  display.print(lastReading);
  display.print("m");
  
  display.display();
}

void setup() {
  display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS); //the first param generates the voltage from 3.3v

  pinMode(BUTTON,INPUT_PULLUP); //define the mode for the button
  initializeDisplay();
  
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);

  client.setInsecure();

  getBg(); //get nightscout information and set initial time
  display.clearDisplay();
  display.display();
  
  
}

void loop() {
  if(lastReadingMin != minute()) { //if the minute has changed since the last reading, get a new one
    getBg(); //get bg information
  }
    
  if(digitalRead(BUTTON) == LOW) {
    displayBg();
    delay(10000);
    display.clearDisplay();
    display.display();
  }

  delay(100);
 //while (hour() >= 19 || hour() < 10) { //check to make sure we are within sleeping hours
    //getBg(); //get bg information
    //delay(60000); //wait a minute
  //}
  //display.clearDisplay(); //clear the display during the day
  //display.display();
  //delay(60000*60); //delay an hour before checking again
  
}
