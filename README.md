# PWM Decoding

## 프로젝트 소개
이 프로젝트는 RC 수신기(R9D)의 채널 신호를 Arduino를 통해 수신하여 LED의 밝기, RGB 색상, 전원 상태를 제어하는 시스템입니다. 각 채널은 다음과 같은 기능을 수행합니다:
- CH2: LED 밝기 제어
- CH3: RGB LED 색상 제어 (연속적인 색상 변화)
- CH9: LED 전원 ON/OFF 제어

## 동작 영상 및 배선 설명
[![동작 영상 및 배선 설명](https://img.youtube.com/vi/cSw97ibA4u8/maxresdefault.jpg)](https://www.youtube.com/watch?v=cSw97ibA4u8)

## 배선 정보

### R9D 수신기와 Arduino 연결
| R9D 채널 | Arduino 핀 | 기능 |
|---------|-----------|------|
| CH2     | 2         | 밝기 제어 |
| CH3     | 3         | RGB 색상 제어 |
| CH9     | 4         | ON/OFF 제어 |
| GND     | GND       | 공통 접지 |
| VCC     | 5V        | 전원 공급 (5V) |

### Arduino와 LED 연결
| Arduino 핀 | 연결 대상 | 기능 |
|-----------|----------|------|
| 5         | ON/OFF LED | ON/OFF 제어 |
| 6         | brightness LED | 밝기 제어 |
| 7         | RGB LED (Blue) | RGB LED 파란색 |
| 8         | RGB LED (Green) | RGB LED 초록색 |
| 9         | RGB LED (Red) | RGB LED 빨간색 |

### RGB LED 배선
RGB LED를 사용할 경우, 각 핀과 Arduino 사이에 적절한 저항(약 220Ω)을 연결하여 과전류로부터 LED를 보호해야 합니다.

```
Arduino 핀 5 --- (+)ON/OFF LED(-) --- 220Ω 저항 --- GND
Arduino 핀 6 --- (+)brightness LED(-) --- 220Ω 저항 --- GND
Arduino 핀 7 --- (+)RGB LED Blue 핀
Arduino 핀 8 --- (+)RGB LED Green 핀
Arduino 핀 9 --- (+)RGB LED Red 핀
GND --- 220Ω 저항 --- RGB LED Common (GND)
```

## 기능 설명
1. **밝기 제어 (CH2)**
   - RC 송신기의 CH2 채널을 조정하여 LED의 밝기를 변경할 수 있습니다.
   - PWM 신호로 밝기를 제어합니다.
   - 스틱 위치에 따라 0~255 사이의 밝기 값으로 매핑됩니다.

2. **RGB 색상 제어 (CH3)**
   - RC 송신기의 CH3 채널을 조정하여 RGB LED의 색상을 연속적으로 변경할 수 있습니다.
   - 수신된 PWM 신호를 0~765 범위의 색상 위치로 매핑하여 빨강→노랑→초록→청록→파랑으로 연속적인 색상 변화를 구현합니다.
   - 각 색상 성분에 대해 PWM 신호를 사용하여 부드러운 색상 전환을 제공합니다.

3. **ON/OFF 제어 (CH9)**
   - RC 송신기의 CH9 채널(일반적으로 스위치)를 사용하여 LED 시스템을 켜거나 끌 수 있습니다.
   - 신호 값이 임계값(PULSE_CENTER - PULSE_THRESHOLD)보다 낮으면 LED가 켜지고, 그렇지 않으면 꺼집니다.

## 소스 코드 주요 기능

소스 코드는 다음과 같은 주요 기능을 포함하고 있습니다:

1. TaskScheduler 라이브러리를 사용한 비차단 작업 스케줄링
2. RC 수신기의 PWM 신호를 감지하기 위한 인터럽트 처리
3. 연속적인 RGB 색상 변화를 위한 색상 매핑 알고리즘
4. 디버깅을 위한 시리얼 모니터 출력

## 필요한 라이브러리
- TaskScheduler
- PinChangeInterrupt

## 사용 방법
1. Arduino IDE에 필요한 라이브러리를 설치합니다.
2. 위 배선 정보에 따라 R9D 수신기, Arduino, LED를 연결합니다.
3. 코드를 Arduino에 업로드합니다.
4. RC 송신기를 켜고 각 채널을 조작하여 LED를 제어합니다.

## 참고 사항
- 이 프로젝트는 Arduino Uno R3와 호환됩니다.
- RC 수신기의 신호 범위는 약 1050μs에서 1950μs입니다. 다른 수신기를 사용할 경우 이 값을 조정해야 할 수 있습니다.