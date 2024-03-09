#include <Adafruit_GFX.h>         // Core graphics library
#include <Adafruit_ST7789.h>      // Hardware-specific library for ST7789
#include <SdFat.h>                // SD card & FAT filesystem library
#include <Adafruit_SPIFlash.h>    // SPI / QSPI flash library
#include <Adafruit_ImageReader.h> // Image-reading functions
#include <Adafruit_SH110X.h>      // OLED screen
#include <Wire.h>
#include <SPI.h>
// #include <SD.h>                //substituted with SDFat
#include <sstream>
#include <iostream>
#include "RTClib.h"
using namespace std;

//---------- SD CARD AND TFT ----------
#define SD_CS    9 // SD card select pin
#define TFT_CS  12 // TFT select pin
#define TFT_DC   10 // TFT display/command pin
#define TFT_RST  11 // Or set to -1 and connect to Arduino RESET pin

SdFat                SD;         // SD card filesystem
Adafruit_ImageReader reader(SD); // Image-reader object, pass in SD filesys
File myFile;

Adafruit_ST7789      tft    = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);
Adafruit_Image       img;        // An image loaded into RAM
int32_t              width  = 0, // BMP image dimensions
                     height = 0;


//---------- ANALOG INPUTS ----------
int analogPin = A0;

//---------- LED pins ----------
#define RED_LED_PIN 13
int LED_pin = RED_LED_PIN;
#define LED_OFF LOW
#define LED_ON HIGH


//---------- OLED -----------
#define BUTTON_A  9
#define BUTTON_B  6
#define BUTTON_C  5

Adafruit_SH1107 display = Adafruit_SH1107(64, 128, &Wire);

//---------- RTC ----------
RTC_DS3231 rtc;


//---------- FILENAMING ----------
char filename[13] = "0001axxx.txt";


//---------- SETUP ----------

void setup(void) {

  ImageReturnCode stat; // Status from image-reading functions
  Serial.begin(9600);
  while(!Serial);       // Wait for Serial Monitor before continuing
  tft.init(135, 240);           // Init ST7789 320x240

  // SD card
  debug(F("Initializing filesystem..."));
  if(!SD.begin(SD_CS, SD_SCK_MHZ(10))) { // Breakouts require 10 MHz limit due to longer wires
    error(F("SD begin() failed"));
  }
  debug(F("OK!"));

  // TFT
  debug(F("Loading startup.bmp to screen..."));
  reader.drawBMP("/startup.bmp", tft, 0, 0);


  analogReadResolution(12);

  tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
  tft.setRotation(3);
  tft.setCursor(1, 1);

  // OLED
    display.begin(0x3C, true); // Address 0x3C default
  debug(F("OLED begun"));

  // Clear the buffer.
  display.clearDisplay();
  display.display();
  display.setRotation(1);

  pinMode(BUTTON_A, INPUT_PULLUP);
  pinMode(BUTTON_B, INPUT_PULLUP);
  pinMode(BUTTON_C, INPUT_PULLUP);

  // text display tests
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);
  display.setCursor(0,0);
  display.print("Connecting to SSID\n'adafruit':");
  display.print("connected!");
  display.println("IP: 10.0.1.23");
  display.println("Sending val #0");
  display.display(); // actually display all of the above

  //----------RTC----------
  if (! rtc.begin()) {
    while (1) delay(10);
  }

  if (rtc.lostPower()) {
    Serial.println("RTC lost power, let's set the time!");
    // When time needs to be set on a new device, or after a power loss, the
    // following line sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
    // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
  }
  DateTime now = rtc.now();
  filename[0] = firstDigit(now.day()) + 48; // +48 to convert number to ascii
  filename[1] = lastDigit(now.day()) + 48;
  filename[2] = firstDigit(now.month()) + 48;
  filename[3] = lastDigit(now.month()) + 48;
}


int startTime = micros();
const int size = 10;
double freq = 35000;
int run = 0;


void loop() {

  if(!digitalRead(BUTTON_A)){
    display.clearDisplay();
    display.setCursor(0,0);
    DateTime now = rtc.now();
    filename[0] = firstDigit(now.day()) + 48;
    filename[1] = lastDigit(now.day()) + 48;
    filename[2] = firstDigit(now.month()) + 48;
    filename[3] = lastDigit(now.month()) + 48;
    filename[4] = 'a';
    debug(F(filename));
    myFile.open(filename, FILE_READ);
    if(myFile){ // read from the file until there's nothing else in it:
      while (myFile.available()) {
       char letter = myFile.read(); // have to do it like this for ascii reasons?
       display.print(letter);
       Serial.print(letter);
     }
      myFile.close(); // close the file:
    } else {
      // if the file didn't open, print an error:
      debug(F("error opening file"));
    }
  } 
  if(!digitalRead(BUTTON_B)) display.print("B");
  if(!digitalRead(BUTTON_C)) display.print("C");
  delay(100);
  yield();
  display.display();


  
  
}



//---------- A small helper ----------
void error(const __FlashStringHelper*err) {
  if(Serial){
    Serial.println(err);
  }
  digitalWrite(LED_pin, LED_ON);
  while (1);
}

//----------- debug tools ----------
void debug(const __FlashStringHelper*text){
  if(Serial){
    Serial.println(text);
  }
}


//---------- Blink the red LED ----------
void blink_LED(uint32_t blink_ms){
  // blink the LED for the specified number of milliseconds

  uint32_t start_time = millis();

  // make sure the LED is off, initially.
  digitalWrite(LED_pin, LED_OFF);

  while(millis() - start_time < blink_ms)
  {
    digitalWrite(LED_pin, LED_ON);
    delay(50);
    digitalWrite(LED_pin, LED_OFF);
    delay(50);
  }
}

//---------- Check if string is a number ----------
bool is_number(const std::string& s){
    std::string::const_iterator it = s.begin();
    while (it != s.end() && std::isdigit(*it)) ++it;
    return !s.empty() && it == s.end();
}

//---------- get the first number from a string ----------
int extractIntegerWords(string str){
  stringstream ss;

  /* Storing the whole string into string stream */
  ss << str;

  /* Running loop till the end of the stream */
  string temp;
  int found;
  while (!ss.eof()) {
      /* extracting word by word from stream */
      ss >> temp;

      /* Checking the given word is integer or not */
      if (stringstream(temp) >> found){
          return found;
      }
    }
  return 0;
}


//---------- Helper functions for date to file name ----------
int firstDigit(int n) 
{ 
    if (n<10){
      return 0;
    }
    else{
      while (n >= 10){// Remove last digit from number til only one digit is left 
        n /= 10; 
      }  
      return n; // return the first digit 
    }
} 
  
int lastDigit(int n) 
{ 
    return (n % 10); // return the last digit 
} 
