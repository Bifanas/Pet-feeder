//////////////////////////////////////////////////////////////////
//                          LIBRARIES                           //
//////////////////////////////////////////////////////////////////

//LOAD CELL LIB-------------------------------------------------------------------------------------
#include "HX711.h"
HX711 scale;

//OLED DISP LIB-------------------------------------------------------------------------------------

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Real Time Clock LIB---------------------------------------------------------------------

#include "Arduino.h"
#include "uRTCLib.h"
uRTCLib rtc(0x68);  // uRTCLib rtc;

//////////////////////////////////////////////////////////////////
//                           DEFINES                            //
//////////////////////////////////////////////////////////////////

// LOAD CELL DEFINES--------------------------------------------------------------------------------

#define LOADCELL_DOUT_PIN 18
#define LOADCELL_SCK_PIN 19
#define calibration_factor 106

int weigth = 0;

// OLED DISP DEFINES--------------------------------------------------------------------------------

#define SCREEN_WIDTH 128  // OLED display width, in pixels
#define SCREEN_HEIGHT 64  // OLED display height, in pixels

// Declaration for SSD1306 display connected using I2C
#define OLED_RESET -1  // Reset pin
#define SCREEN_ADDRESS 0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);


const int bufferSize = 4;  // Size of the circular buffer
struct Measurement {
  char Calendar[20];
  float weight;
};

Measurement measurements[bufferSize];  // Array to store the measurements
int currentIndex = 0;                  // Index to keep track of the current position in the buffer

// RTC DEFINES--------------------------------------------------------------------------------

//char Calendar[20];  // string UTF8 end zero

// Motor DEFINES -----------------------------------------------------------------------------

const int enable_motor = 15;  // Change to defines? #define enable_motor 15
const int step = 2;
const int dirPin = 4;

float cal = 49;  // auxiliary declaration of an initial Calibration value

int16_t Hour_A;  // 32767.. +32767  optimize the data type
int16_t Hour_B;

int16_t Minute_A;
int16_t Minute_B;

int16_t Meal_size_A;
int16_t Meal_size_B;

//////////////////////////////////////////////////////////////////
//                          FUNCTIONS                           //
//////////////////////////////////////////////////////////////////

void stepper(float Screw_turns, int motorpin, bool direction) {

  int speed = 600;  //NEEDS TO BE CALIBRATED !!!
  int full_turn = 4000;

  int steps = Screw_turns * full_turn;  // get how many steps

  digitalWrite(enable_motor, LOW);
  digitalWrite(dirPin, direction);

  for (int x = 0; x < steps; x++) {
    digitalWrite(motorpin, HIGH);
    delayMicroseconds(speed);
    digitalWrite(motorpin, LOW);
    delayMicroseconds(speed);
  }
  digitalWrite(enable_motor, HIGH);
}

//--------------------------------------------------------------------------------------

void feed(float cal, int amount) {
  //  to get how many turns to get 50g
  float turns = 50 / cal;
  for (int i = 0; i <= amount; i = i + 50) {
    stepper(turns, step, HIGH);  //1 turn = 4000 steps = 103g
    stepper(0.2, step, LOW);     //spins backwards chug control
  }
  digitalWrite(enable_motor, HIGH);
}

char now[20];

void addMeasurement(float newWeight, char current_calendar[]) {
  // Create a new measurement
  Measurement newMeasurement;
  strcpy(newMeasurement.Calendar, current_calendar);  // Use strcpy to copy the content
  newMeasurement.weight = newWeight;

  // Add the new measurement to the buffer at the current index
  measurements[currentIndex] = newMeasurement;

  // Increment the current index and wrap around if necessary
  currentIndex = (currentIndex + 1) % bufferSize;
}

void Main_screan_print() {
  Serial.println("Buffer:");
  // Print the contents of the buffer
  for (int i = 0; i < bufferSize; i++) {
    Serial.print("Timestamp: ");
    Serial.print(measurements[i].Calendar);
    Serial.print(" Weight: ");
    Serial.println(measurements[i].weight, 2);
  }
  Serial.println();

  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.print(rtc.hour());
  display.print(":");
  display.print(rtc.minute());
  display.print(":");
  display.print(rtc.second());
  display.setTextSize(1);
  display.print(" ");
  display.print(rtc.day());
  display.print("/");
  display.print(rtc.month());

  display.setCursor(0, 25);
  display.setTextSize(1);

  for (int i = 0; i < bufferSize; i++) {
    //display.print("");
    
    display.print(measurements[i].Calendar);
    display.print(" ");
    display.print(measurements[i].weight);
    display.println(" g");    
  }


  display.display();
}

//////////////////////////////////////////////////////////////////
//                            SETUP                            //
//////////////////////////////////////////////////////////////////

void setup() {
  Serial.begin(9600);
  // MOTOR SETUP-----------------------------------------------------------------------------

  pinMode(enable_motor, OUTPUT);
  pinMode(step, OUTPUT);
  pinMode(dirPin, OUTPUT);

  // RTC SETUP-------------------------------------------------------------------------

  URTCLIB_WIRE.begin();
  // rtc.set(0, 52, 22, 4, 22, 11, 2023);  // rtc.set(second, minute, hour, dayOfWeek, dayOfMonth, month, year)

  //LOAD CELL SETUP-------------------------------------------------------------------------------------

  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  scale.set_scale(calibration_factor);      //Adjust to this calibration factor
  scale.tare();                             //Reset the scale to 0
  long zero_factor = scale.read_average();  //Get a baseline reading
  Serial.print("Zero factor: ");            //This can be used to remove the need to tare the scale. Useful in permanent scale projects.
  Serial.println(zero_factor);


  //OLED DISP SETUP-------------------------------------------------------------------------------------
  // initialize the OLED object
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ;  // Don't proceed, loop forever
  }
  // Clear the buffer.
  display.clearDisplay();

  //////////////////////////////////////////////////////////////////
  //                    MEAL SCHEDULE SETUP                       //
  //////////////////////////////////////////////////////////////////

  // Change the time and size of the first meal!

  Hour_A = 8;
  Minute_A = 0;
  Meal_size_A = 200;

  // Change the time and size of the first meal!

  Hour_B = 17;
  Minute_B = 0;
  Meal_size_B = 200;
}

//////////////////////////////////////////////////////////////////
//                             MAIN                             //
//////////////////////////////////////////////////////////////////

int old_weigth = 0;
int new_weigth = 0;

void loop() {

  //Updates the data ---------------------------------------------------------------------------------------

  rtc.refresh();  // Check the current time.

  sprintf(now, "%02d/%02d %02d:%02d", rtc.day(), rtc.month(), rtc.hour(), rtc.minute());
  Serial.println(now);
  Main_screan_print();

  Serial.println(scale.get_units(5));  // Mesures how much food is inside.

  //schedule detection -------------------------------------------------------------------------------------
  if (Hour_A == rtc.hour() && Minute_A == rtc.minute() && 0 == rtc.second()) {

    old_weigth = scale.get_units(5);

    feed(cal, Meal_size_A);
    delay(1000);

    new_weigth = scale.get_units(5);
    old_weigth = old_weigth - new_weigth;

    addMeasurement(old_weigth, now);
  }
  if (Hour_B == rtc.hour() && Minute_B == rtc.minute() && 0 == rtc.second()) {
    
    old_weigth = scale.get_units(5);
    
    feed(cal, Meal_size_B);
    delay(1000);

    new_weigth = scale.get_units(5);
    old_weigth = old_weigth - new_weigth;

    addMeasurement(old_weigth, now);
  }
  delay(500);
  digitalWrite(enable_motor, HIGH);  //turn off motor

}
