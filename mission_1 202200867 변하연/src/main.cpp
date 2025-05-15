/*
 * RC LED 제어 시스템
 * ------------------------------
 * 이 코드는 RC 수신기(R9D)의 채널 신호를 Arduino를 통해 수신하여
 * LED의 밝기, RGB 색상, ON/OFF 상태를 제어하는 시스템입니다.
 * - CH2: LED 밝기 제어
 * - CH3: RGB LED 색상 제어 (연속적인 색상 변화)
 * - CH9: LED 전원 ON/OFF 제어
 */

 #include <Arduino.h>                 // Arduino 기본 라이브러리
 #include <TaskScheduler.h>           // 스케줄링을 위한 라이브러리
 #include "PinChangeInterrupt.h"      // 추가 인터럽트 핀 지원을 위한 라이브러리

//AT9의 헬리콥터 모드 사용

 // 입력 핀 정의 - RC 수신기 연결
 #define RC_CH2_PIN 2    // 밝기 제어용 채널 (Arduino 디지털 핀 2에 연결)
 #define RC_CH3_PIN 3    // RGB 색상 제어용 채널 (Arduino 디지털 핀 3에 연결)
 #define RC_CH9_PIN 4    // ON/OFF 제어용 채널 (Arduino 디지털 핀 4에 연결)
 
 // 출력 핀 정의 - LED 연결
 #define ONOFF_LED_PIN 5         // ON/OFF LED
 #define BRIGHTNESS_LED_PIN 6    // 밝기 제어
 #define RGB_BLUE_PIN 7          // RGB LED 파란색
 #define RGB_GREEN_PIN 8         // RGB LED 초록색
 #define RGB_RED_PIN 9           // RGB LED 빨간색  //여기까지 변하연, 아래로는 고광채 작성
 
 // RC 수신기 신호 관련 상수
 #define PULSE_CENTER 1500       // RC 신호 중앙값 (μs)
 #define PULSE_THRESHOLD 200     // ON/OFF 구분 임계값 (μs)
 #define MIN_PULSE 1050          // RC 신호 최소값 (μs)
 #define MAX_PULSE 1950          // RC 신호 최대값 (μs)
 
 // 인터럽트 처리를 위한 volatile 변수
 // 각 채널의 펄스 폭을 저장 (기본값: 중앙 위치 1500μs)
 volatile uint16_t pulseWidth[3] = {1500, 1500, 1500};
 // 각 채널의 펄스 시작 시간을 저장
 volatile uint32_t startTime[3] = {0, 0, 0};
 
 // TaskScheduler 객체 생성
 Scheduler runner;
 
 // 인터럽트 핸들러 함수 - 채널 1 (밝기 제어)
 void ch2Change() { 
   bool state = digitalRead(RC_CH2_PIN);
   if (state) startTime[0] = micros();   // 신호가 HIGH로 변경될 때 타이머 시작
   else pulseWidth[0] = micros() - startTime[0];  // 신호가 LOW로 변경될 때 펄스 폭 계산
 }
 
 // 인터럽트 핸들러 함수 - 채널 3 (RGB 색상 제어)
 void ch3Change() { 
   bool state = digitalRead(RC_CH3_PIN);
   if (state) startTime[1] = micros();   // 신호가 HIGH로 변경될 때 타이머 시작
   else pulseWidth[1] = micros() - startTime[1];  // 신호가 LOW로 변경될 때 펄스 폭 계산
 }
 
 // 인터럽트 핸들러 함수 - 채널 9 (ON/OFF 제어)
 void ch9Change() { 
   bool state = digitalRead(RC_CH9_PIN);
   if (state) startTime[2] = micros();   // 신호가 HIGH로 변경될 때 타이머 시작
   else pulseWidth[2] = micros() - startTime[2];  // 신호가 LOW로 변경될 때 펄스 폭 계산
 }
 
 /**
  * RGB 색상 생성 함수
  * pos 값(0-765)에 따라 연속적인 색상 변화를 위한 RGB 값을 계산
  * 
  * pos: 색상 위치 (0-765)
  * r: 빨간색 값이 저장될 참조 변수 (0-255)
  * g: 초록색 값이 저장될 참조 변수 (0-255)
  * b: 파란색 값이 저장될 참조 변수 (0-255)
  */
 void setRGB(int pos, int& r, int& g, int& b) {
   // 입력값을 0-765 범위로 제한
   pos = constrain(pos, 0, 765);
 
   // 0-255: 빨강(255,0,0) → 노랑(255,255,0)
   if (pos < 255) {
     r = 255;
     g = pos;        // g는 0에서 255로 증가
     b = 0;
   } 
   // 255-510: 노랑(255,255,0) → 청록(0,255,255)
   else if (pos < 510) {
     r = 255 - (pos - 255);  // r은 255에서 0으로 감소
     g = 255;
     b = pos - 255;          // b는 0에서 255로 증가
   } 
   // 510-765: 청록(0,255,255) → 파랑(0,0,255)
   else {
     r = 0;
     g = 255 - (pos - 510);  // g는 255에서 0으로 감소
     b = 255;
   }
 }
 
 /**
  * LED 업데이트 함수
  * 모든 LED 상태를 업데이트합니다. TaskScheduler에 의해 주기적으로 호출됩니다.
  */
 void updateLEDs() {
   // 1. 밝기 제어 (CH2)
   // RC 신호(1050-1950μs)를 PWM 값(255-0)으로 매핑
   analogWrite(BRIGHTNESS_LED_PIN, map(pulseWidth[0], MIN_PULSE, MAX_PULSE, 255, 0));
 
   // 2. RGB LED 색상 제어 (CH3)
   uint16_t ch3 = pulseWidth[1];
   // RC 신호(1050-1950μs)를 색상 위치(0-765)로 매핑
   int colorPos = map(ch3, MIN_PULSE, MAX_PULSE, 0, 765);
 
   // RGB 값 계산
   int r, g, b;
   setRGB(colorPos, r, g, b);
 
   // RGB LED 출력 - 연속적인 색상 표현
   analogWrite(RGB_RED_PIN, r);
   analogWrite(RGB_GREEN_PIN, g);
   analogWrite(RGB_BLUE_PIN, b);
   
   // 3. ON/OFF LED 제어 (CH9)
   // 임계값보다 낮으면 LED ON, 그렇지 않으면 OFF
   digitalWrite(ONOFF_LED_PIN, pulseWidth[2] < (PULSE_CENTER - PULSE_THRESHOLD) ? HIGH : LOW);
 }
 
 /**
  * 디버그 정보 출력 함수
  * 각 채널의 PWM 값을 시리얼 모니터에 출력합니다.
  */
 void printDebugInfo() {
   Serial.print("CH2: ");
   Serial.print(pulseWidth[0]);   // 밝기 제어 채널
   Serial.print(" | CH3: ");
   Serial.print(pulseWidth[1]);   // RGB 색상 제어 채널
   Serial.print(" | CH9: ");
   Serial.println(pulseWidth[2]); // ON/OFF 제어 채널
 }
 
 // TaskScheduler 작업 정의
 // LED 업데이트 작업: 10ms 간격으로 무한 반복
 Task taskUpdateLEDs(10, TASK_FOREVER, &updateLEDs);
 // 디버그 정보 출력 작업: 1000ms(1초) 간격으로 무한 반복
 Task taskPrintDebug(1000, TASK_FOREVER, &printDebugInfo);
 
 /**
  * 초기 설정 함수
  * 핀 모드 설정, 인터럽트 연결, 스케줄러 초기화
  */
 void setup() {
   // 시리얼 통신 초기화 (9600bps)
   Serial.begin(9600);
   
   // 입력 핀 모드 설정
   pinMode(RC_CH2_PIN, INPUT);
   pinMode(RC_CH3_PIN, INPUT);
   pinMode(RC_CH9_PIN, INPUT);
   
   // 출력 핀 모드 설정
   pinMode(ONOFF_LED_PIN, OUTPUT);
   pinMode(BRIGHTNESS_LED_PIN, OUTPUT);
   pinMode(RGB_RED_PIN, OUTPUT);
   pinMode(RGB_GREEN_PIN, OUTPUT);
   pinMode(RGB_BLUE_PIN, OUTPUT);
   
   // 인터럽트 연결
   // 디지털 핀 2, 3은 기본 인터럽트 핀 사용
   attachInterrupt(digitalPinToInterrupt(RC_CH2_PIN), ch2Change, CHANGE);
   attachInterrupt(digitalPinToInterrupt(RC_CH3_PIN), ch3Change, CHANGE);
   // 디지털 핀 4는 PinChangeInterrupt 라이브러리 사용
   attachPCINT(digitalPinToPCINT(RC_CH9_PIN), ch9Change, CHANGE);
   
   // TaskScheduler 초기화
   runner.init();
   // 작업 등록
   runner.addTask(taskUpdateLEDs);
   runner.addTask(taskPrintDebug);
   // 작업 활성화
   taskUpdateLEDs.enable();
   taskPrintDebug.enable();
   
   // 초기화 완료 메시지
   Serial.println("RC LED Control System Initialized");
 }
 
 /**
  * 메인 루프 함수
  * TaskScheduler가 등록된 작업을 실행합니다.
  */
 void loop() {
   // TaskScheduler를 실행하여 등록된 작업을 스케줄에 맞게 처리
   runner.execute();
 }
