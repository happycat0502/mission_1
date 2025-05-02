#include <Arduino.h>
#include <TaskScheduler.h>
#include "PinChangeInterrupt.h"

//===========================================
// PIN DEFINITIONS
//===========================================
// RC Input pins - Best pins for reading PWM signals
#define RC_CH1_PIN 2    // Steering - Using hardware interrupt INT0
#define RC_CH2_PIN 3    // Throttle - Using hardware interrupt INT1
#define RC_CH3_PIN 4    // Aux channel - Using pin change interrupt

// LED Output pins
#define ONOFF_LED_PIN 7        // Digital ON/OFF LED
#define BRIGHTNESS_LED_PIN 9   // PWM pin for brightness control
#define RGB_RED_PIN 11         // PWM pin for red component
#define RGB_GREEN_PIN 10       // PWM pin for green component
#define RGB_BLUE_PIN 6         // PWM pin for blue component

//===========================================
// PWM SIGNAL VARIABLES
//===========================================
volatile uint16_t ch1PulseWidth = 1500;  // Default center (1000-2000 μs)
volatile uint16_t ch2PulseWidth = 1500;  // Default center
volatile uint16_t ch3PulseWidth = 1500;  // Default center
volatile uint32_t ch1StartTime = 0;
volatile uint32_t ch2StartTime = 0;
volatile uint32_t ch3StartTime = 0;

// Failsafe variables
unsigned long lastValidRcSignalTime = 0;
const unsigned long rcSignalTimeout = 500; // ms

//===========================================
// LED CONTROL VARIABLES
//===========================================
bool ledState = false;         // ON/OFF state
int ledBrightness = 0;         // Brightness level (0-255)
int rgbHue = 0;                // Color hue (0-359)

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
// Interrupt handler for channel 1 (steering)
void ch1Change() {
  if (digitalRead(RC_CH1_PIN) == HIGH) {
    // Rising edge
    ch1StartTime = micros();
  } else {
    // Falling edge
    uint32_t pulseWidth = micros() - ch1StartTime;
    
    // Only update if within valid range (filter glitches)
    if (pulseWidth >= 900 && pulseWidth <= 2100) {
      ch1PulseWidth = pulseWidth;
      lastValidRcSignalTime = millis();
    }
  }
}

// Interrupt handler for channel 2 (throttle)
void ch2Change() {
  if (digitalRead(RC_CH2_PIN) == HIGH) {
    // Rising edge
    ch2StartTime = micros();
  } else {
    // Falling edge
    uint32_t pulseWidth = micros() - ch2StartTime;
    
    // Only update if within valid range (filter glitches)
    if (pulseWidth >= 900 && pulseWidth <= 2100) {
      ch2PulseWidth = pulseWidth;
      lastValidRcSignalTime = millis();
    }
  }
}

// Interrupt handler for channel 3 (aux)
void ch3Change() {
  if (digitalRead(RC_CH3_PIN) == HIGH) {
    // Rising edge
    ch3StartTime = micros();
  } else {
    // Falling edge
    uint32_t pulseWidth = micros() - ch3StartTime;
    
    // Only update if within valid range (filter glitches)
    if (pulseWidth >= 900 && pulseWidth <= 2100) {
      ch3PulseWidth = pulseWidth;
      lastValidRcSignalTime = millis();
    }
  }
}

//===========================================
// TASKS
//===========================================
Task taskUpdateLEDs(20, TASK_FOREVER, &updateLEDs);            // 50Hz update rate
Task taskPrintDebug(1000, TASK_FOREVER, &printDebugInfo);      // Debug output once per second
Task taskCheckSignal(100, TASK_FOREVER, &checkSignalStatus);   // Check signal validity 10 times per second

//===========================================
// SETUP FUNCTION
//===========================================
void setup() {
  // Initialize serial for debugging
  Serial.begin(115200);
  Serial.println(F("Starting RC PWM Reader..."));
  
  // Initialize RC input pins
  pinMode(RC_CH1_PIN, INPUT);
  pinMode(RC_CH2_PIN, INPUT);
  pinMode(RC_CH3_PIN, INPUT);
  
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
  attachInterrupt(digitalPinToInterrupt(RC_CH1_PIN), ch1Change, CHANGE);
  attachInterrupt(digitalPinToInterrupt(RC_CH2_PIN), ch2Change, CHANGE);
  
  // Pin change interrupt for auxiliary channel
  attachPCINT(digitalPinToPCINT(RC_CH3_PIN), ch3Change, CHANGE);
  
  // Initialize task scheduler
  runner.init();
  
  // Add tasks to scheduler
  runner.addTask(taskUpdateLEDs);
  runner.addTask(taskPrintDebug);
  runner.addTask(taskCheckSignal);
  
  // Enable tasks
  taskUpdateLEDs.enable();
  taskPrintDebug.enable();
  taskCheckSignal.enable();
  
  Serial.println(F("RC LED Control System Initialized"));
  Serial.println(F("Connect RC receiver channels as follows:"));
  Serial.println(F("- Channel 1 (ON/OFF toggle) -> Arduino Pin 2"));
  Serial.println(F("- Channel 2 (brightness)    -> Arduino Pin 3"));
  Serial.println(F("- Channel 3 (color)         -> Arduino Pin 4"));
  Serial.println(F("- RC receiver ground        -> Arduino GND"));
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
  // Check if RC signal is valid
  bool validRcSignal = (millis() - lastValidRcSignalTime < rcSignalTimeout);
  
  if (validRcSignal) {
    // Process channel 1 for ON/OFF LED
    // Toggle LED when stick is moved to one extreme
    static bool prevAboveThreshold = false;
    if (ch1PulseWidth > 1800) {
      // Only toggle when the signal crosses the threshold (prevents multiple toggles)
      if (!prevAboveThreshold) {
        ledState = !ledState;
        digitalWrite(ONOFF_LED_PIN, ledState ? HIGH : LOW);
        Serial.print(F("LED toggled: "));
        Serial.println(ledState ? F("ON") : F("OFF"));
        prevAboveThreshold = true;
      }
    } else {
      // Reset the toggle detection when signal goes below threshold
      prevAboveThreshold = false;
    }
    
    // Process channel 2 for brightness control
    // Map 1000-2000μs to 0-255 brightness
    ledBrightness = map(constrain(ch2PulseWidth, 1000, 2000), 1000, 2000, 0, 255);
    analogWrite(BRIGHTNESS_LED_PIN, ledBrightness);
    
    // Process channel 3 for RGB LED color
    // Map 1000-2000μs to 0-359 degrees in color wheel
    rgbHue = map(constrain(ch3PulseWidth, 1000, 2000), 1000, 2000, 0, 359);
    setRgbColorFromHue(rgbHue);
  } else {
    // Failsafe - set LEDs to a safe state
    digitalWrite(ONOFF_LED_PIN, LOW);
    analogWrite(BRIGHTNESS_LED_PIN, 0);
    analogWrite(RGB_RED_PIN, 0);
    analogWrite(RGB_GREEN_PIN, 0);
    analogWrite(RGB_BLUE_PIN, 0);
  }
}

// Task function to print debug information
void printDebugInfo() {
  Serial.print(F("CH1: "));
  Serial.print(ch1PulseWidth);
  Serial.print(F(" | CH2: "));
  Serial.print(ch2PulseWidth);
  Serial.print(F(" | CH3: "));
  Serial.print(ch3PulseWidth);
  Serial.print(F(" | Brightness: "));
  Serial.print(ledBrightness);
  Serial.print(F(" | Hue: "));
  Serial.print(rgbHue);
  Serial.print(F(" | Signal: "));
  Serial.println((millis() - lastValidRcSignalTime < rcSignalTimeout) ? F("VALID") : F("LOST"));
}

// Task function to check signal status and provide feedback
void checkSignalStatus() {
  static bool lastSignalStatus = true;
  bool currentSignalStatus = (millis() - lastValidRcSignalTime < rcSignalTimeout);
  
  // Only print message when status changes
  if (currentSignalStatus != lastSignalStatus) {
    if (currentSignalStatus) {
      Serial.println(F("RC signal restored"));
    } else {
      Serial.println(F("RC signal lost - failsafe active"));
    }
    lastSignalStatus = currentSignalStatus;
  }
}

// Function to set RGB LED color based on hue value (0-359 degrees)
void setRgbColorFromHue(int hue) {
  // Constrain hue to valid range
  hue = constrain(hue, 0, 359);
  
  // Calculate RGB values from hue
  int r, g, b;
  
  // Convert hue to RGB using the HSV color model (S=100%, V=100%)
  int sector = hue / 60;
  int remainder = hue % 60;
  int c = 255;
  int x = (c * (60 - remainder)) / 60;
  int y = (c * remainder) / 60;
  
  switch (sector) {
    case 0:  r = c; g = y; b = 0; break;
    case 1:  r = x; g = c; b = 0; break;
    case 2:  r = 0; g = c; b = y; break;
    case 3:  r = 0; g = x; b = c; break;
    case 4:  r = y; g = 0; b = c; break;
    case 5:  r = c; g = 0; b = x; break;
    default: r = 0; g = 0; b = 0; break;
  }
  
  // Set RGB LED pins
  analogWrite(RGB_RED_PIN, r);
  analogWrite(RGB_GREEN_PIN, g);
  analogWrite(RGB_BLUE_PIN, b);
}