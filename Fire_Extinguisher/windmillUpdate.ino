/* Surprising Input-Output Relationships
  This Arduino project uses a heat sensor to control an LED strip and servos. 
  As the temperature increases, the LEDs change color on a scale of green to red depending on the current temperature.
  When a threshold is reached, a fan activates, and two servos move to cool/extinguish the heat source until the DHT detects a reduce in temperature.

  sensors:
  1. left right servo motor - pin 22
  2. up down servo motor - pin 23
  3. DHT heat sensor - pin 16
  4. LED strip - pin 21

  Video link: https://www.youtube.com/watch?v=Y29MlKcLL6I

  Created by:
  1. Nimrod Boazi - 208082735
  2. Noam Shildekraut - 315031864
*/
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#include <Adafruit_NeoPixel.h>
#include <ESP32Servo.h>

#define LED_PIN 21
#define LED_COUNT 12
#define DHTPIN 16
#define DHTTYPE DHT22
#define T0 4

Servo leftRightServo;
Servo upDownServo;
const int leftRightServoPin = 22;
const int upDownServoPin = 23;
static int leftRightPos = 120;
unsigned static long previousMillis = 0;
unsigned static long currentMillis;
static int whatTurn = 0;
const int shortInterval = 50;
const int longInterval = 100;

const int tempratureThreshHold = 11;
int roomTemp;
int ledCount;
int currentTemp;

//DHT decleration
sensors_event_t event;
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);
DHT_Unified dht(DHTPIN, DHTTYPE);
uint32_t delayMS;

//LED color array
const uint32_t colorArray[] = {
    strip.Color(0, 255, 0),
    strip.Color(85, 255, 0),
    strip.Color(170, 255, 0),
    strip.Color(255, 255, 0),
    strip.Color(255, 215, 0),
    strip.Color(255, 170, 0),
    strip.Color(255, 130, 0),
    strip.Color(255, 85, 0),
    strip.Color(255, 45, 0),
    strip.Color(255, 20, 0),
    strip.Color(255, 10, 0),
    strip.Color(255, 0, 0)
};

// Function declarations
void initTemperatureSensor();
void initLEDs();
void initServos();
void activateFan();
int updateLedCount(int roomTemp, int currentTemp);
void updateLED(int ledCount);

void setup() {
  Serial.begin(115200);
  delay(1000);

  initTemperatureSensor();
  initLEDs();
  initServos();
}

void loop() {
  sensors_event_t event;
  dht.temperature().getEvent(&event);

  if (isnan(event.temperature)) {
    Serial.println(F("Error reading temperature!"));
  } else {
    currentTemp = event.temperature; // Updated current temperature
    Serial.print(F("Temperature: "));
    Serial.print(event.temperature);
    ledCount = updateLedCount(roomTemp, currentTemp);
    updateLED(ledCount);
  }

  if (ledCount >= tempratureThreshHold) {
    activateFan();
  }
}

// Initialize the temperature sensor
void initTemperatureSensor() {
  dht.begin();
  sensor_t sensor;
  dht.temperature().getSensor(&sensor);
  dht.temperature().getEvent(&event);

  if (isnan(event.temperature)) {
    Serial.println(F("Error reading temperature!"));
  } else {
    roomTemp = event.temperature; // Store initial room temperature
  }
}

// Initialize the LEDs
void initLEDs() {
  strip.begin();
  strip.show();
  strip.setBrightness(150); // Set LED brightness
}

// Initialize the servos
void initServos() {
  leftRightServo.setPeriodHertz(50);
  leftRightServo.attach(leftRightServoPin, 500, 2500);  // Attach left-right servo
  upDownServo.setPeriodHertz(50);
  upDownServo.attach(upDownServoPin, 500, 2500);    // Attach up-down servo

  leftRightServo.write(90);  // Center left-right servo
  upDownServo.write(90);     // Center up-down servo
}

// Activate fan and move servos to simulate extinguishing a fire
void activateFan() {
  int upPos = 120;
  int downPos = 20;
  currentMillis = millis();

  switch (whatTurn) {
    case 0:
      if(currentMillis - previousMillis > longInterval) {
        leftRightServo.write(leftRightPos);
        leftRightPos += 15;
        whatTurn++;
        previousMillis = millis(); 
      }
      break;
    case 1:
      if(currentMillis - previousMillis > shortInterval) {
        upDownServo.write(downPos);
        whatTurn++;
        previousMillis = millis();
      }
      break;
    case 2:
      if(currentMillis - previousMillis > longInterval) {
        upDownServo.write(upPos);
        if(leftRightPos > 105) {
          leftRightPos = 105;
          whatTurn++; 
        } else {
          whatTurn = 0;
        }
        previousMillis = millis(); 
      }
      break;
    case 3:
      if(currentMillis - previousMillis > longInterval) {
        leftRightServo.write(leftRightPos);
        leftRightPos -= 15;
        whatTurn++;
        previousMillis = millis();  
      }
      break;
    case 4:
      if(currentMillis - previousMillis > shortInterval) {
        upDownServo.write(downPos);
        whatTurn++;
        previousMillis = millis();  
      }
      break;
    case 5:
      if(currentMillis - previousMillis > longInterval) {
        upDownServo.write(upPos);
        if(leftRightPos < 75) {
          leftRightPos = 75;
          whatTurn = 0; 
        } else {
          whatTurn = 3;
        }
        previousMillis = millis();  
      }
      break;
  }
}

// Update the number of LEDs lit based on temperature difference
int updateLedCount(int roomTemp, int currentTemp) {
  if (currentTemp - roomTemp > 11) {
    ledCount = 11;
  } else if (currentTemp - roomTemp < 0) {
    ledCount = 0;
  } else {
    ledCount = currentTemp - roomTemp;
  }
  
  return ledCount;
}

// Update LED colors based on temperature
void updateLED(int ledCount) {
  for (int i = 0; i <= ledCount; i++) {
    strip.setPixelColor(i, colorArray[ledCount]);
  }
  for (int i = 11; i > ledCount; i--) {
    uint32_t color = strip.Color(0, 0, 0); // Default color is red
    strip.setPixelColor(i, color);
  }
  strip.show();
}
