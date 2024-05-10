//Digi-physi Musical Intrument
//our instrument utilizes an accelerometer and a water sensor to detect the motion and water content of
//the cleaning sponge. When the sponge is pressed, more water is detected by the water sensor and the music
//starts playing. The measurements from the accelorometer are then used to calculate the velocity of the
//sponge's movement in three-dimensional space, enabling the generation of musical tones corresponding
//to the speed of the sponge.

//library definitions

#include "pitches.h"
#include "I2Cdev.h"
#include "MPU6050.h"
#include <cmath>

MPU6050 accelgyro;

#define OUTPUT_READABLE_ACCELGYRO

//variable definitions
int16_t current_x_axis_placement, current_y_axis_placement, current_z_axis_placement , ignore1 , ignore2 , ignore3;
int16_t previous_x_axis_placement = 0, previous_y_axis_placement = 0, previous_z_axis_placement = 0;
int distance_check_interval = 5;
int velocity_threshold = 10000;
int velocityArray[5];
int waterArray[5];
int velocity_array_size = sizeof(velocityArray) / sizeof(velocityArray[0]);
int water_array_size = sizeof(waterArray) / sizeof(waterArray[0]);
int average_velocity_counter = 0;
int average_water_counter = 0;
int averageVelocity;
int averageWater;
double scaledVelocity;
static unsigned long previousMillis = 0;
static boolean is_playing = false;
#define TONE_OUTPUT_PIN 26
// The ESP32 has 16 channels which can generate 16 independent waveforms
// We'll just choose PWM channel 0 here

const int TONE_PWM_CHANNEL = 0;


int melody[] = {
  NOTE_C4, NOTE_G3, NOTE_G3, NOTE_A3, NOTE_G3, 0, NOTE_B3, NOTE_C4
};

int melody_array_size = sizeof(melody) / sizeof(melody[0]);

int noteDurations[] = {
  4, 8, 8, 4, 4, 4, 4, 4
};

int water_threshold = 1800;

static int thisNote = 0;

boolean pressed_sponge_flag = false;

//setup method
void setup() {
  // put your setup code here, to run once:
  Wire.begin();
  Serial.begin(115200);
  ledcAttachPin(TONE_OUTPUT_PIN, TONE_PWM_CHANNEL);
  Serial.println("Initializing I2C devices...");
  accelgyro.initialize();
  Serial.println("Testing device connections...");
  Serial.println(accelgyro.testConnection() ? "MPU6050 connection successful" : "MPU6050 connection failed");

}

//main loop
void loop() {
  //read sensor values
  int water_sensor_value = analogRead(34);
  averageWater = calculateAverageWater(water_sensor_value);
  //calculate velocity and update distance values
  accelgyro.getMotion6(&current_x_axis_placement, &current_y_axis_placement, &current_z_axis_placement , &ignore1 , &ignore2 , &ignore3);
  scaledVelocity = calculateScaledVelocity(current_x_axis_placement, current_y_axis_placement, current_z_axis_placement);

  Serial.print("Scaled: ");
  Serial.println(scaledVelocity);
  //check water threshold
  pressed_sponge_flag = (averageWater > water_threshold);
  Serial.println(pressed_sponge_flag);
  Serial.println(water_sensor_value);
  //if water threshold, play music
  if( pressed_sponge_flag ) {    
      playMelody(melody[thisNote] , scaledVelocity);
  }
  //else stop music
  else{
    ledcWrite(TONE_PWM_CHANNEL, 0);
    is_playing = false;

  }
  
}

//music playing function
void playMelody(int note , double scaledVelocity) {
  unsigned long currentMillis = millis();
  int noteDuration = (1000 / noteDurations[thisNote]) / scaledVelocity;
  int pauseBetweenNotes = noteDuration * 0.20;
  if(!is_playing && currentMillis - previousMillis >= pauseBetweenNotes){
    previousMillis = millis();
    is_playing = true;
    ledcWriteTone(TONE_PWM_CHANNEL, note);
    thisNote = (thisNote + 1) % (melody_array_size);
  }
  else if(is_playing && currentMillis - previousMillis >= noteDuration) {
    previousMillis = millis();
    is_playing = false;
    ledcWrite(TONE_PWM_CHANNEL, 0);
  }
}

//This function calculates the average water value based on a rolling window of water readings.
//It maintains a sum of the latest water values within the window and updates the average accordingly.
int calculateAverageWater(int new_water_value) {
  static int sum = 0;
  static int count = 0;

  // Subtract the old value from the sum before it's overwritten
  sum -= waterArray[average_water_counter];
  // Add the new velocity
  waterArray[average_water_counter] = new_water_value;
  sum += new_water_value;

  // Increment the counter and wrap it around if necessary
  average_water_counter = (average_water_counter + 1) % water_array_size;

  // Ensure we don't divide by zero and handle buffer size correctly
  if (count < water_array_size) count++;
  
  return sum / count; // Calculate average based on current count
}

//This function calculates the average velocity based on a rolling window of velocity readings.
//Similar to calculateAverageWater, it maintains a sum of the latest velocity values within the window
//and updates the average accordingly.
int calculateAverageVelocity(int newVelocity) {
  static int sum = 0;
  static int count = 0;

  // Subtract the old value from the sum before it's overwritten
  sum -= velocityArray[average_velocity_counter];
  // Add the new velocity
  velocityArray[average_velocity_counter] = newVelocity;
  sum += newVelocity;

  // Increment the counter and wrap it around if necessary
  average_velocity_counter = (average_velocity_counter + 1) % velocity_array_size;

  // Ensure we don't divide by zero and handle buffer size correctly
  if (count < velocity_array_size) count++;
  
  return sum / count; // Calculate average based on current count
}

//This function scales the average velocity based on certain thresholds.
//It adjusts the scaling factor to map velocities to audible frequencies,
//ensuring a pleasant range for the musical output.
double scaleVelocity(int averageVelocity) {
    if (averageVelocity < 300) {
        return 0.25; // Clamp velocities below 900 to the minimum scale
    } else if (averageVelocity > 5500 || averageVelocity < 0) {
        return 3; // Clamp velocities above 10000 or below 0 to the maximum scale
    } else if (averageVelocity <= 3500) {
        // Scale more aggressively in the 900 to 8000 range
        return 0.25 + (2.25 * (averageVelocity - 300) / (3500 - 300));
    } else {
        // Scale less aggressively in the 8000 to 10000 range
        return 2.5 + (0.5 * (averageVelocity - 3500) / (5500 - 3500));
    }
}

//This function calculates the scaled velocity based on changes in the position of the cleaning sponge.
//It computes the distance traveled by the sponge and calculates the velocity,
//then scales it using the scaleVelocity function to produce a value suitable for controlling
//the tempo of the musical output.
double calculateScaledVelocity(int16_t current_x_axis_placement, int16_t current_y_axis_placement, int16_t current_z_axis_placement){
  int distance = sqrt(sq(current_x_axis_placement - previous_x_axis_placement) + sq(current_y_axis_placement - previous_y_axis_placement) + sq(current_z_axis_placement - previous_z_axis_placement));
  int velocity = distance ;    
    previous_x_axis_placement = current_x_axis_placement;
    previous_y_axis_placement = current_y_axis_placement;
    previous_z_axis_placement = current_z_axis_placement;
  
    averageVelocity = calculateAverageVelocity(velocity);
    return (scaleVelocity(averageVelocity));
}
