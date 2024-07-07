/*
Digital-Physical Web Server
This Arduino-based disco ball combines the magic of colorful LED lights, smooth servo motor movements, and looping sound to create a dynamic and engaging experience. 
Controlled via the Blynk app, it allows for easy remote operation of the LED brightness, sound playback, and servo movements. 

Sensors:
1. LED strip - pin 22
2. Servo motor - pin 15
3. I2S sound - DIN pin 14, LRCLK pin 27, BCLK pin 26

Video link: https://youtu.be/2o8rinIdgPk

  Created by:
  1. Nimrod Boazi - 208082735
  2. Noam Shildekraut - 315031864
*/

#include <Arduino.h>

#define BLYNK_TEMPLATE_ID "TMPL6Pd6Pw10v"
#define BLYNK_TEMPLATE_NAME "MyDevice"
char auth[] = "zxW5oD8QA05piWQHBBimIRgOTjVhgzRZ";

char ssid[] = "ChinaNet-2.4G-0A20"; 
char password[] = "0548020878";

#define DELAY_MS 10 

String sound = "sound.mp3"; 


#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include <Adafruit_NeoPixel.h>
#include <ESP32Servo.h>
#include "Audio.h"
#include "SPIFFS.h"

#define BLYNK_PRINT Serial

#define SERVOPIN 15 // Servo Pin
#define LEDPIN 22   // LED Strip
#define DIN 14      // MP3 Player
#define LRCLK 27    // MP3 Player
#define BCLK 26     // MP3 Player

#define NUMPIXELS 12 // no of LEDs

Adafruit_NeoPixel pixels(NUMPIXELS, LEDPIN, NEO_GRB + NEO_KHZ800);

Audio audio;

Servo servo;

BlynkTimer timer;

// Flags to keep state
int servoFlag = 0;
int LEDFlag = 0;
int soundFlag = 0;
int LEDBrightness = 10;
bool playSound = false;

// old state variables
int servoPrevious = 0;
int soundPrevios = 0;
int LEDPrevious = 10;

// rainbow helping variables
uint16_t j = 0;               // Used to keep track of the position in the rainbow cycle
unsigned long lastUpdate = 0; // Store the last update time

// Servo movement variables
unsigned long lastMoveTime = 0;
const unsigned long moveInterval = 20; // Time in milliseconds for each movement
int servoPosition = 0;
int servoDirection = 1;

BLYNK_WRITE(V0)
{
  servoFlag = param.asInt();
}

BLYNK_WRITE(V1)
{
  LEDFlag = param.asInt();
}

BLYNK_WRITE(V2)
{
  soundFlag = param.asInt();
}

BLYNK_WRITE(V3)
{
  LEDBrightness = param.asInt();
}

uint32_t Wheel(byte WheelPos)
{
  WheelPos = 255 - WheelPos;
  if (WheelPos < 85)
  {
    return pixels.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if (WheelPos < 170)
  {
    WheelPos -= 85;
    return pixels.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return pixels.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}

void rainbow(uint8_t wait)
{
  if (millis() - lastUpdate > wait)
  {
    lastUpdate = millis();
    for (uint16_t i = 0; i < pixels.numPixels(); i++)
    {
      pixels.setPixelColor(i, Wheel(((i * 256 / pixels.numPixels()) + j) & 255));
    }
    pixels.show();
    j++;
    if (j >= 256 * 5)
    {
      j = 0; // Reset the cycle after completing 5 cycles of all colors
    }
  }
}

void pixelOff()
{
  for (uint16_t i = 0; i < pixels.numPixels(); i++)
  {
    pixels.setPixelColor(i, pixels.Color(0, 0, 0));
  }
  pixels.show();
}

void setup()
{
  servo.attach(SERVOPIN, 500, 2400);
  servo.write(0);

  pixels.begin();
  pixels.setBrightness((LEDBrightness * 20) + 55);
  pixels.show();

  Serial.begin(115200);

  if (!SPIFFS.begin(true))
  {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  audio.setPinout(BCLK, LRCLK, DIN);
  audio.setVolume(21); // 0...21

  Blynk.begin(auth, ssid, password, "blynk.cloud", 80);
  // timer.setInterval(1000L, senddata);
}

void loop()
{
  Blynk.run();
  timer.run();
  audio.loop();

//Servo click
  if (servoFlag != servoPrevious)
  {
    Serial.println("Move Servo");
    servoPrevious = servoFlag;
  }

//Servo movement
  if (servoFlag)
  {
    unsigned long currentTime = millis();
    if (currentTime - lastMoveTime >= moveInterval)
    {
      lastMoveTime = currentTime;
      servo.write(servoPosition);
      servoPosition += servoDirection;

      if (servoPosition >= 180 || servoPosition <= 0)
      {
        servoDirection = -servoDirection; // Reverse direction
      }
    }
  }

//Sound click
  if (soundFlag != soundPrevios)
  {
    if (soundFlag)
    {
      audio.connecttoFS(SPIFFS, sound.c_str());
      playSound = true;
    }
    else
    {
      audio.stopSong();
      playSound = false;
    }
    soundPrevios = soundFlag;
  }

//LED brightness change
  if (LEDBrightness != LEDPrevious)
  {
    Serial.println("Brightness Changed");
    pixels.setBrightness((LEDBrightness * 20) + 55);
    LEDPrevious = LEDBrightness;
  }

//LED case handle
  if (LEDFlag)
  {
    rainbow(DELAY_MS);
  }
  else
  {
    pixelOff();
  }

//MP3 loop
  if (playSound)
  {
    if (!audio.isRunning())
    {
      audio.connecttoFS(SPIFFS, sound.c_str());
    }
  }
}