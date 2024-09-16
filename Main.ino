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

#define LOADCELL_DOUT_PIN 13
#define LOADCELL_SCK_PIN 15
float calibration_factor = 23.5; //need to czech

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


 // Motor A connections
#define enA 17
#define in1 16
#define in2 14

float cal = 17.7;  // auxiliary declaration of an initial Calibration value
// calibration for 12V 28rpm float cal = 17.7;

int16_t Hour_A;  // 32767.. +32767  optimize the data type
int16_t Hour_B;

int16_t Minute_A;
int16_t Minute_B;

int16_t Meal_size_A;
int16_t Meal_size_B;


//////////////////////////////////////////////////////////////////
//                          FUNCTIONS                           //
//////////////////////////////////////////////////////////////////

void MotorControl(float time, bool direrction) {
  analogWrite(enA, 255); // Set motors to maximum speed

  if (direrction) {
    digitalWrite(in1, HIGH);  // 1 Forward
    digitalWrite(in2, LOW);
  } else if (!direrction) {  // 0 Reverse
    digitalWrite(in1, LOW);
    digitalWrite(in2, HIGH);
  }
  delay(time); // Keep spinning (Food control) 
  
  digitalWrite(in1, LOW); // Turn off motors
  digitalWrite(in2, LOW);

  analogWrite(enA, 0); // Disable the motors
}

//--------------------------------------------------------------------------------------


void feed(float cal, int amount) {
   float turns = 2900 * (50/ cal); //  (2900) = 1 tunr, 50g = portion size, cal = 18g per turn
  for (int i = 0; i < amount; i = i + 50) { // feeds in 50g pulses
  MotorControl(turns, 1); // Spins forward
  MotorControl(1000, 0);  // Spins backwards
  }
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
  //Serial.println("Buffer:");
  // Print the contents of the buffer

  /*
  for (int i = 0; i < bufferSize; i++) {
    Serial.print("Timestamp: ");
    Serial.print(measurements[i].Calendar);
    Serial.print(" Weight: ");
    Serial.println(measurements[i].weight, 2);
  }
  Serial.println();
  */

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


void setup() {

  Serial.begin(9600);
  // MOTOR SETUP-----------------------------------------------------------------------------

  // Set all the motor control pins to outputs
  pinMode(enA, OUTPUT);
  pinMode(in1, OUTPUT);
  pinMode(in2, OUTPUT);

  // Turn off motors - Initial state
  digitalWrite(in1, LOW);
  digitalWrite(in2, LOW);

  // RTC SETUP-------------------------------------------------------------------------

  URTCLIB_WIRE.begin();

  //rtc.set(0,50,21, 6, 31, 8, 2024);  //(second, minute, hour, dayOfWeek, dayOfMonth, month, year)

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

unsigned long seconds = 1000L;  
unsigned long minutes = seconds * 60;



void loop() {
    //Updates the data ---------------------------------------------------------------------------------------

  rtc.refresh();  // Check the current time.

  sprintf(now, "%02d/%02d %02d:%02d", rtc.day(), rtc.month(), rtc.hour(), rtc.minute());
  Serial.println(now);

  Main_screan_print();

 // Serial.println(scale.get_units(5));  // Mesures how much food is inside.

  //schedule detection -------------------------------------------------------------------------------------
  if (Hour_A == rtc.hour() && Minute_A == rtc.minute() && 0 == rtc.second()) {

    old_weigth = scale.get_units(5);

    feed(cal, Meal_size_A);
    delay(5*minutes);

    new_weigth = scale.get_units(5);
    old_weigth = old_weigth - new_weigth;

    addMeasurement(old_weigth, now);
  }
  if (Hour_B == rtc.hour() && Minute_B == rtc.minute() && 0 == rtc.second()) {
    
    old_weigth = scale.get_units(5);
    
    feed(cal, Meal_size_B);
    delay(5*minutes);

    new_weigth = scale.get_units(5);
    old_weigth = old_weigth - new_weigth;
    addMeasurement(old_weigth, now);
  }
  delay(500);
}
