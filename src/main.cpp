#include <Arduino.h>
#include <TaskScheduler.h>
#include "PinChangeInterrupt.h"

// Input pins
#define RC_CH3_PIN 2    // Brightness
#define RC_CH5_PIN 3    // RGB color
#define RC_CH9_PIN 4    // ON/OFF

// Output pins
#define ONOFF_LED_PIN 5
#define BRIGHTNESS_LED_PIN 6
#define RGB_BLUE_PIN 7
#define RGB_GREEN_PIN 8
#define RGB_RED_PIN 9

// Constants
#define PULSE_CENTER 1500
#define PULSE_THRESHOLD 200
#define MIN_PULSE 1050
#define MAX_PULSE 1950

// Volatile variables for interrupt handling
volatile uint16_t pulseWidth[3] = {1500, 1500, 1500};
volatile uint32_t startTime[3] = {0, 0, 0};

Scheduler runner;

// Interrupt handlers
void ch3Change() { 
  bool state = digitalRead(RC_CH3_PIN);
  if (state) startTime[0] = micros(); 
  else pulseWidth[0] = micros() - startTime[0]; 
}

void ch5Change() { 
  bool state = digitalRead(RC_CH5_PIN);
  if (state) startTime[1] = micros(); 
  else pulseWidth[1] = micros() - startTime[1]; 
}

void ch9Change() { 
  bool state = digitalRead(RC_CH9_PIN);
  if (state) startTime[2] = micros(); 
  else pulseWidth[2] = micros() - startTime[2]; 
}

// Task functions
void updateLEDs() {
  // Brightness control based on CH3
  analogWrite(BRIGHTNESS_LED_PIN, map(pulseWidth[0], MIN_PULSE, MAX_PULSE, 255, 0));

  // RGB LED control based on CH5
  uint16_t ch5 = pulseWidth[1];
  digitalWrite(RGB_RED_PIN, ch5 < (PULSE_CENTER - PULSE_THRESHOLD) ? HIGH : LOW);
  digitalWrite(RGB_GREEN_PIN, (ch5 >= (PULSE_CENTER - PULSE_THRESHOLD) && ch5 <= (PULSE_CENTER + PULSE_THRESHOLD)) ? HIGH : LOW);
  digitalWrite(RGB_BLUE_PIN, ch5 > (PULSE_CENTER + PULSE_THRESHOLD) ? HIGH : LOW);
  
  // ON/OFF LED control based on CH9
  digitalWrite(ONOFF_LED_PIN, pulseWidth[2] < (PULSE_CENTER - PULSE_THRESHOLD) ? HIGH : LOW);
}

void printDebugInfo() {
  Serial.print("CH3: ");
  Serial.print(pulseWidth[0]);
  Serial.print(" | CH5: ");
  Serial.print(pulseWidth[1]);
  Serial.print(" | CH9: ");
  Serial.println(pulseWidth[2]);
}

// Tasks
Task taskUpdateLEDs(10, TASK_FOREVER, &updateLEDs);
Task taskPrintDebug(1000, TASK_FOREVER, &printDebugInfo);

void setup() {
  Serial.begin(9600);
  
  // Configure pins
  pinMode(RC_CH3_PIN, INPUT);
  pinMode(RC_CH5_PIN, INPUT);
  pinMode(RC_CH9_PIN, INPUT);
  
  pinMode(ONOFF_LED_PIN, OUTPUT);
  pinMode(BRIGHTNESS_LED_PIN, OUTPUT);
  pinMode(RGB_RED_PIN, OUTPUT);
  pinMode(RGB_GREEN_PIN, OUTPUT);
  pinMode(RGB_BLUE_PIN, OUTPUT);
  
  // Attach interrupts
  attachInterrupt(digitalPinToInterrupt(RC_CH3_PIN), ch3Change, CHANGE);
  attachInterrupt(digitalPinToInterrupt(RC_CH5_PIN), ch5Change, CHANGE);
  attachPCINT(digitalPinToPCINT(RC_CH9_PIN), ch9Change, CHANGE);
  
  // Initialize scheduler
  runner.init();
  runner.addTask(taskUpdateLEDs);
  runner.addTask(taskPrintDebug);
  taskUpdateLEDs.enable();
  taskPrintDebug.enable();
  
  Serial.println("RC LED Control System Initialized");
}

void loop() {
  runner.execute();
}