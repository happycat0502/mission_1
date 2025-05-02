#include <Arduino.h>
#include <TaskScheduler.h>
#include "PinChangeInterrupt.h"

// Pin definitions
#define ONOFF_LED_PIN 9    // LED for ON/OFF control
#define PWM_LED_PIN 10     // LED for brightness control
#define COLOR_LED_PIN 11   // LED for color patterns

#define BUTTON_ONOFF 2     // Button for ON/OFF control
#define BUTTON_MODE 3      // Button for mode selection
#define BUTTON_COLOR 4     // Button for color pattern selection

#define POTENTIOMETER_PIN A0 // Potentiometer for brightness control
#define SERIAL_BAUDRATE 9600 // Serial communication speed

// Mode definitions
enum Mode {
    NORMAL,    // Normal mode
    PWM_MODE,  // PWM control mode
    COLOR_MODE,// Color pattern mode
    OFF        // All LEDs off
};

// Color pattern definitions
enum ColorPattern {
    RED,
    GREEN,
    BLUE,
    RAINBOW,
    STROBE
};

// Global variables
Mode currentMode = NORMAL;            // Current mode initialization
int brightness = 255;                 // Brightness initialization (full)
ColorPattern currentColorPattern = RED;// Current color pattern
unsigned long colorDuration = 1000;    // Color change duration
unsigned long strobeDuration = 100;    // Strobe effect duration

// LED current values
int onOffLedValue = 0;                 // Current ON/OFF LED value
int pwmLedValue = 0;                   // Current PWM LED value
int colorLedValue = 0;                 // Current color LED value
int colorIntensity = 0;                // Current color intensity

// Button state variables
volatile bool onOffButtonPressed = false;  // ON/OFF button press flag
volatile bool modeButtonPressed = false;   // Mode button press flag
volatile bool colorButtonPressed = false;  // Color button press flag

// Function declarations
void normalSequence();     // Normal mode sequence
void pwmSequence();        // PWM mode sequence
void colorSequence();      // Color pattern sequence
void checkButtons();       // Button check function
void readPotentiometer();  // Read potentiometer value
void processSerial();      // Process serial input
void updateLEDs();         // Update LEDs

// Interrupt Service Routines
void onOffISR() {
    onOffButtonPressed = true;
}
void modeISR() {
    modeButtonPressed = true;
}
void colorISR() {
    colorButtonPressed = true;
}

// TaskScheduler object creation
Scheduler runner;

// Task objects creation
Task tNormal(colorDuration, TASK_FOREVER, &normalSequence, &runner, false);
Task tPWM(50, TASK_FOREVER, &pwmSequence, &runner, false);
Task tColor(colorDuration, TASK_FOREVER, &colorSequence, &runner, false);

Task tButtons(20, TASK_FOREVER, &checkButtons, &runner, true);
Task tPotentiometer(20, TASK_FOREVER, &readPotentiometer, &runner, true);
Task tSerial(20, TASK_FOREVER, &processSerial, &runner, true);
Task tUpdateLEDs(20, TASK_FOREVER, &updateLEDs, &runner, true);

// LED values setting function (internal state only)
void setLEDValues(int onOff, int pwm, int color) {
    onOffLedValue = onOff;
    pwmLedValue = pwm;
    colorLedValue = color;
}

// Update LEDs function (actual hardware control)
void updateLEDs() {
    // Apply brightness to LEDs
    analogWrite(ONOFF_LED_PIN, onOffLedValue);
    analogWrite(PWM_LED_PIN, pwmLedValue * brightness / 255);
    analogWrite(COLOR_LED_PIN, colorLedValue * brightness / 255);
}

// Normal mode sequence
void normalSequence() {
    // In normal mode, ON/OFF LED is on, others follow brightness
    setLEDValues(255, brightness, brightness);
    Serial.println("NORMAL_MODE_ACTIVE");
}

// PWM sequence for brightness demonstration
void pwmSequence() {
    static int pwmDirection = 5;
    static int pwmValue = 0;
    
    // Update PWM value
    pwmValue += pwmDirection;
    
    // Reverse direction at limits
    if (pwmValue >= 255 || pwmValue <= 0) {
        pwmDirection = -pwmDirection;
        pwmValue = constrain(pwmValue, 0, 255);
    }
    
    // Set LED values
    setLEDValues(255, pwmValue, 0);
    Serial.print("PWM_VALUE:");
    Serial.println(pwmValue);
}

// Color pattern sequence
void colorSequence() {
    static int colorState = 0;
    static int rainbowHue = 0;
    static bool strobeState = false;
    
    switch (currentColorPattern) {
        case RED:
            setLEDValues(0, 0, 255);
            Serial.println("COLOR:RED");
            break;
            
        case GREEN:
            setLEDValues(0, 255, 0);
            Serial.println("COLOR:GREEN");
            break;
            
        case BLUE:
            setLEDValues(255, 0, 0);
            Serial.println("COLOR:BLUE");
            break;
            
        case RAINBOW:
            // Simple rainbow effect simulation
            rainbowHue = (rainbowHue + 1) % 3;
            switch (rainbowHue) {
                case 0: setLEDValues(0, 0, 255); Serial.println("RAINBOW:RED"); break;
                case 1: setLEDValues(0, 255, 0); Serial.println("RAINBOW:GREEN"); break;
                case 2: setLEDValues(255, 0, 0); Serial.println("RAINBOW:BLUE"); break;
            }
            tColor.setInterval(colorDuration);
            break;
            
        case STROBE:
            // Strobe effect (on/off all LEDs)
            if (strobeState) {
                setLEDValues(255, 255, 255);
                Serial.println("STROBE:ON");
            } else {
                setLEDValues(0, 0, 0);
                Serial.println("STROBE:OFF");
            }
            strobeState = !strobeState;
            tColor.setInterval(strobeDuration);
            break;
    }
}

// Set operating mode
void setMode(Mode newMode) {
    // Disable all mode tasks
    tNormal.disable();
    tPWM.disable();
    tColor.disable();
    
    // Enable appropriate task based on new mode
    switch (newMode) {
        case NORMAL:
            tNormal.enable();
            Serial.println("MODE:NORMAL");
            break;
            
        case PWM_MODE:
            tPWM.enable();
            Serial.println("MODE:PWM");
            break;
            
        case COLOR_MODE:
            tColor.enable();
            Serial.println("MODE:COLOR");
            break;
            
        case OFF:
            setLEDValues(0, 0, 0);
            Serial.println("MODE:OFF");
            break;
    }
    
    currentMode = newMode;
}

// Set color pattern
void setColorPattern(ColorPattern newPattern) {
    currentColorPattern = newPattern;
    
    // Reset interval when pattern changes
    if (newPattern == RAINBOW) {
        tColor.setInterval(colorDuration);
    } else if (newPattern == STROBE) {
        tColor.setInterval(strobeDuration);
    } else {
        tColor.setInterval(colorDuration);
    }
    
    // Print pattern change
    Serial.print("COLOR_PATTERN:");
    switch (newPattern) {
        case RED: Serial.println("RED"); break;
        case GREEN: Serial.println("GREEN"); break;
        case BLUE: Serial.println("BLUE"); break;
        case RAINBOW: Serial.println("RAINBOW"); break;
        case STROBE: Serial.println("STROBE"); break;
    }
}

// Check button states and update modes
void checkButtons() {
    if (onOffButtonPressed) {
        Serial.println("ON/OFF button pressed");
        if (currentMode == OFF) {
            setMode(NORMAL);
        } else {
            setMode(OFF);
        }
        onOffButtonPressed = false;
    }
    
    if (modeButtonPressed) {
        Serial.println("Mode button pressed");
        // Cycle through modes
        switch (currentMode) {
            case NORMAL: setMode(PWM_MODE); break;
            case PWM_MODE: setMode(COLOR_MODE); break;
            case COLOR_MODE: setMode(NORMAL); break;
            case OFF: setMode(NORMAL); break;
        }
        modeButtonPressed = false;
    }
    
    if (colorButtonPressed) {
        Serial.println("Color button pressed");
        if (currentMode == COLOR_MODE) {
            // Cycle through color patterns
            switch (currentColorPattern) {
                case RED: setColorPattern(GREEN); break;
                case GREEN: setColorPattern(BLUE); break;
                case BLUE: setColorPattern(RAINBOW); break;
                case RAINBOW: setColorPattern(STROBE); break;
                case STROBE: setColorPattern(RED); break;
            }
        }
        colorButtonPressed = false;
    }
}

// Read potentiometer and update brightness
void readPotentiometer() {
    int potValue = analogRead(POTENTIOMETER_PIN);
    int newBrightness = map(potValue, 0, 1023, 0, 255);
    
    // Update only if there's a significant change (noise reduction)
    if (abs(newBrightness - brightness) > 2) {
        brightness = newBrightness;
        Serial.print("BRIGHTNESS:");
        Serial.println(brightness);
    }
}

// Process serial commands
void processSerial() {
    if (Serial.available() > 0) {
        String command = Serial.readStringUntil('\n');
        command.trim();
        int separatorPos = command.indexOf(':');
        
        if (separatorPos > 0) {
            String param = command.substring(0, separatorPos);
            String value = command.substring(separatorPos + 1);
            
            if (param == "MODE") {
                if (value == "NORMAL") setMode(NORMAL);
                else if (value == "PWM") setMode(PWM_MODE);
                else if (value == "COLOR") setMode(COLOR_MODE);
                else if (value == "OFF") setMode(OFF);
            }
            else if (param == "COLOR") {
                if (value == "RED") setColorPattern(RED);
                else if (value == "GREEN") setColorPattern(GREEN);
                else if (value == "BLUE") setColorPattern(BLUE);
                else if (value == "RAINBOW") setColorPattern(RAINBOW);
                else if (value == "STROBE") setColorPattern(STROBE);
            }
            else if (param == "BRIGHTNESS") {
                brightness = value.toInt();
                Serial.print("BRIGHTNESS_SET:");
                Serial.println(brightness);
            }
            else if (param == "COLOR_DURATION") {
                colorDuration = value.toInt();
                Serial.print("COLOR_DURATION_SET:");
                Serial.println(colorDuration);
            }
            else if (param == "STROBE_DURATION") {
                strobeDuration = value.toInt();
                Serial.print("STROBE_DURATION_SET:");
                Serial.println(strobeDuration);
            }
        }
    }
}

void setup() {
    // Pin mode setup
    pinMode(ONOFF_LED_PIN, OUTPUT);
    pinMode(PWM_LED_PIN, OUTPUT);
    pinMode(COLOR_LED_PIN, OUTPUT);
    pinMode(BUTTON_ONOFF, INPUT_PULLUP);
    pinMode(BUTTON_MODE, INPUT_PULLUP);
    pinMode(BUTTON_COLOR, INPUT_PULLUP);
    pinMode(POTENTIOMETER_PIN, INPUT);
    
    // Interrupt setup for buttons
    attachInterrupt(digitalPinToInterrupt(BUTTON_ONOFF), onOffISR, FALLING);
    attachInterrupt(digitalPinToInterrupt(BUTTON_MODE), modeISR, FALLING);
    
    // PinChangeInterrupt for the third button (which might not be on a hardware interrupt pin)
    attachPCINT(digitalPinToPCINT(BUTTON_COLOR), colorISR, FALLING);
    
    // Start serial communication
    Serial.begin(SERIAL_BAUDRATE);
    Serial.println("LED Control System Initialized");
    
    // Start in normal mode
    setMode(NORMAL);
}

void loop() {
    runner.execute(); // Execute TaskScheduler
}