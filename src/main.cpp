#include <Arduino.h>
#include <TaskScheduler.h>
#include "PinChangeInterrupt.h"

//===========================================
// PIN DEFINITIONS
//===========================================
// RC Input pins - Using pins 2-4 to read channels 3, 5, 9 from receiver
#define RC_CH3_PIN 2    // Channel 3 - Using hardware interrupt INT0 //brightness
#define RC_CH5_PIN 3    // Channel 5 - Using hardware interrupt INT1 //rgb
#define RC_CH9_PIN 4    // Channel 9 - Using pin change interrupt PCINT0 //ON/OFF

// LED Output pins
#define ONOFF_LED_PIN 5        // Digital ON/OFF LED
#define BRIGHTNESS_LED_PIN 6   // PWM pin for brightness control
#define RGB_BLUE_PIN 7         // PWM pin for blue component
#define RGB_GREEN_PIN 8       // PWM pin for green component
#define RGB_RED_PIN 9         // PWM pin for red component

//===========================================
// PWM SIGNAL VARIABLES
//===========================================
// In helicopter mode, the signal range might differ from standard
volatile uint16_t ch3PulseWidth = 1500;  // Default center (typically 1000-2000 μs)
volatile uint16_t ch5PulseWidth = 1500;  // Default center
volatile uint16_t ch9PulseWidth = 1500;  // Default center
volatile uint32_t ch3StartTime = 0;
volatile uint32_t ch5StartTime = 0;
volatile uint32_t ch9StartTime = 0;

// Adjusted ranges for helicopter mode (may need calibration)
const uint16_t pulseCenter = 1500; // Center position

//===========================================
// TASK SCHEDULER
//===========================================
Scheduler runner;

// Function prototypes
void updateLEDs();
void printDebugInfo();
void checkSignalStatus();
void setRgbColorFromHue(int hue);

//===========================================
// INTERRUPT HANDLERS
//===========================================
// Interrupt handler for channel 3
void ch3Change() {
  if (digitalRead(RC_CH3_PIN) == HIGH) { // Rising edge
    ch3StartTime = micros();
  } else { // Falling edge
    ch3PulseWidth = micros() - ch3StartTime;
  }
}

// Interrupt handler for channel 5
void ch5Change() {
  if (digitalRead(RC_CH5_PIN) == HIGH) { // Rising edge
    ch5StartTime = micros();
  } else { // Falling edge
    ch5PulseWidth = micros() - ch5StartTime;
  }
}

// Interrupt handler for channel 9
void ch9Change() {
  if (digitalRead(RC_CH9_PIN) == HIGH) { // Rising edge
    ch9StartTime = micros();
  } else { // Falling edge
    ch9PulseWidth = micros() - ch9StartTime;
  }
}

//===========================================
// TASKS
//===========================================
Task taskUpdateLEDs(20, TASK_FOREVER, &updateLEDs);            // 50Hz update rate
Task taskPrintDebug(1000, TASK_FOREVER, &printDebugInfo);      // Debug output once per second

//===========================================
// SETUP FUNCTION
//===========================================
void setup() {
  // Initialize serial for debugging
  Serial.begin(9600);
  Serial.println("Starting RC PWM Reader for Helicopter Mode...");
  
  // Initialize RC input pins
  pinMode(RC_CH3_PIN, INPUT);
  pinMode(RC_CH5_PIN, INPUT);
  pinMode(RC_CH9_PIN, INPUT);
  
  // Initialize LED output pins
  pinMode(ONOFF_LED_PIN, OUTPUT);
  pinMode(BRIGHTNESS_LED_PIN, OUTPUT);
  pinMode(RGB_RED_PIN, OUTPUT);
  pinMode(RGB_GREEN_PIN, OUTPUT);
  pinMode(RGB_BLUE_PIN, OUTPUT);
  
  // Set initial LED states
  digitalWrite(ONOFF_LED_PIN, LOW);
  analogWrite(BRIGHTNESS_LED_PIN, 0);
  analogWrite(RGB_RED_PIN, 0);
  analogWrite(RGB_GREEN_PIN, 0);
  analogWrite(RGB_BLUE_PIN, 0);
  
  // Attach interrupts for RC PWM reading
  // Hardware interrupts for most important channels
  attachInterrupt(digitalPinToInterrupt(RC_CH3_PIN), ch3Change, CHANGE);
  attachInterrupt(digitalPinToInterrupt(RC_CH5_PIN), ch5Change, CHANGE);
  
  // Pin change interrupt for auxiliary channel
  attachPCINT(digitalPinToPCINT(RC_CH9_PIN), ch9Change, CHANGE);
  
  // Initialize task scheduler
  runner.init();
  
  // Add tasks to scheduler
  runner.addTask(taskUpdateLEDs);
  runner.addTask(taskPrintDebug);
  
  // Enable tasks
  taskUpdateLEDs.enable();
  taskPrintDebug.enable();
  
  Serial.println("RC LED Control System for Helicopter Mode Initialized");
}

//===========================================
// MAIN LOOP
//===========================================
void loop() {
  // Run the task scheduler
  runner.execute();
}

//===========================================
// TASK FUNCTIONS
//===========================================
// Task function to update LEDs based on RC input
void updateLEDs() {  
    // Determine switch position (works with both 2 and 3-position switches)
    if (ch5PulseWidth < (pulseCenter - 200)) {
        analogWrite(RGB_RED_PIN, 255);   // Red color
        analogWrite(RGB_GREEN_PIN, 0); // Turn off green
        analogWrite(RGB_BLUE_PIN, 0);  // Turn off blue
    } else if (ch5PulseWidth > (pulseCenter + 200)) {
        analogWrite(RGB_GREEN_PIN, 255); // Green color
        analogWrite(RGB_RED_PIN, 0);   // Turn off red
        analogWrite(RGB_BLUE_PIN, 0);  // Turn off blue
    } else {
        analogWrite(RGB_BLUE_PIN, 255);  // Blue color
        analogWrite(RGB_RED_PIN, 0);   // Turn off red
        analogWrite(RGB_GREEN_PIN, 0); // Turn off green
    }
    
    if (ch9PulseWidth < (pulseCenter -200)) {
        digitalWrite(ONOFF_LED_PIN, HIGH); // ON position
        } else {
        digitalWrite(ONOFF_LED_PIN, LOW);  // OFF position
    }
    
    // Process channel 8 for brightness control (adapted for helicopter mode)
    // Map pulse width to brightness, accounting for possible different ranges
    analogWrite(BRIGHTNESS_LED_PIN, map(ch3PulseWidth, 1096, 1920, 255, 0));
}

// Task function to print debug information
void printDebugInfo() {
  Serial.print("CH3: ");
  Serial.print(ch3PulseWidth);
  Serial.print(" | CH5: ");
  Serial.print(ch5PulseWidth);
  Serial.print(" | CH9: ");
  Serial.print(ch9PulseWidth);
}