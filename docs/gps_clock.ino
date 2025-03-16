#include <Wire.h>
#include <EEPROM.h>
#include <RTClib.h>
#include <SoftwareSerial.h>
#include <TinyGPSPlus.h>
#include <Rotary-encoder-easyC-SOLDERED.h>
#include <SHTC3-SOLDERED.h>
#include "MAX7219.h"

#define rtcINTR 2
#define gpsINTR 3
#define txPin 7
#define rxPin 6
#define buzzerPin 9
#define toneFrequency 500

MAX7219 max7219;
RTC_DS3231 rtc;
TinyGPSPlus gps;
SoftwareSerial gpsSerial(rxPin, txPin);
Rotary rotary;
SHTC3 shtcSensor;

struct {
  int8_t hour;
  int8_t minute;
  int8_t second;
} time;

int8_t* timePtr = (int8_t*)&time;

struct {
  bool FONT;
  int8_t BRT;
  bool DST;
  bool ALARM;
} matrix;

uint8_t* matrixPtr = (uint8_t*)&matrix;

enum menu {
  menuBlank,
  clock,
  alarm,
  timer,
  stopwatch,
  config
};
menu menuActive = clock;
menu menuPos = clock;

enum configMenu {
  configMenuBlank,
  font,
  brightness,
  daylightSavingTime,
  alarmOnOff
};
configMenu configMenuActive = configMenuBlank;
configMenu configMenuPos = font;

uint32_t prevTime = 0, startTime = 0;
bool alarmActive = false, timerActive = false, stopwatchActive = false, buzzerState = false;
volatile bool interruptFlag = false, usingRTC = true;


void setup() {

  max7219.begin();
  gpsSerial.begin(9600);

  shtcSensor.begin();
  rotary.begin();
  rotary.resetCounter();

  rtc.begin();
  rtc.disable32K();
  rtc.clearAlarm(1);
  rtc.clearAlarm(2);
  rtc.disableAlarm(1);
  rtc.disableAlarm(2);
  rtc.writeSqwPinMode(DS3231_SquareWave1Hz);

  pinMode(rtcINTR, INPUT_PULLUP);
  pinMode(gpsINTR, INPUT_PULLUP);
  pinMode(buzzerPin, OUTPUT);

  attachInterrupt(digitalPinToInterrupt(rtcINTR), RTC_ISR, FALLING);
  attachInterrupt(digitalPinToInterrupt(gpsINTR), GPS_ISR, RISING);

  matrix.FONT = EEPROM.read(1);
  matrix.BRT = EEPROM.read(2);
  matrix.DST = EEPROM.read(3);
  matrix.ALARM = EEPROM.read(4);

  max7219.setBrightness(matrix.BRT);
  max7219.font = matrix.FONT;
}


void loop() {

  uint8_t state = rotary.getState();

  if (millis() < 100) state = ROTARY_IDLE;

  if (!usingRTC) {
    while (gpsSerial.available()) {
      gps.encode(gpsSerial.read());
    }
  }

  if (rtc.alarmFired(1) && matrix.ALARM) turnOnAlarm();
  if (alarmActive) toggleBuzzer();

  if (interruptFlag) {
    if (menuActive == clock) {
      updateTime();
      if (menuPos == clock) {
        if (time.second <= 55) displayMenu();
        else if (time.second == 56) {
          shtcSensor.sample();
          max7219.display(temp2String(shtcSensor.readTempC()));
        }
      }
    } else {
      if (timerActive) updateTimer();
      if (stopwatchActive) updateStopwatch();
    }
    interruptFlag = false;
  }

  if (state != ROTARY_IDLE) {
    if (alarmActive) turnOffAlarm();
    else {
      switch (state) {
        case ROTARY_CW: clockwise(); break;
        case ROTARY_CCW: counterClockwise(); break;
        case BTN_CLICK: click(); break;
        case BTN_LONG_PRESS: longPress(); break;
        case BTN_DOUBLE_CLICK: doubleClick(); break;
      }
    }

    state = ROTARY_IDLE;
  }
}


void clockwise() {

  switch (menuActive) {
    case menuBlank:
    case stopwatch: break;
    case clock:
      menuPos = (menu)((int)menuPos + 1);
      if (menuPos > config) menuPos = clock;
      displayMenu();
      break;

    case alarm:
    case timer:
      if (!timerActive && !stopwatchActive) {
        (*timePtr)++;
        checkTime();
        displayMenu();
      }
      break;

    case config:
      if (configMenuActive == configMenuBlank) {
        configMenuPos = (configMenu)((int)configMenuPos + 1);
        if (configMenuPos > alarmOnOff) configMenuPos = font;
        displayMenu();
      } else {
        switch (configMenuActive) {
          case configMenuBlank: break;

          case font:
            matrix.FONT = !matrix.FONT;
            max7219.font = matrix.FONT;
            displayMenu();
            break;

          case brightness:
            matrix.BRT++;
            if (matrix.BRT > 15) matrix.BRT = 1;
            max7219.setBrightness(matrix.BRT);
            displayMenu();
            break;

          case daylightSavingTime:
            matrix.DST = !matrix.DST;
            displayMenu();
            break;

          case alarmOnOff:
            matrix.ALARM = !matrix.ALARM;
            displayMenu();
            break;
        }
      }
      break;
  }
}


void counterClockwise() {

  switch (menuActive) {
    case menuBlank:
    case stopwatch: break;
    case clock:
      menuPos = (menu)((int)menuPos - 1);
      if (menuPos < clock) menuPos = config;
      displayMenu();
      break;

    case alarm:
    case timer:
      if (!timerActive && !stopwatchActive) {
        (*timePtr)--;
        checkTime();
        displayMenu();
      }
      break;

    case config:
      if (configMenuActive == configMenuBlank) {
        configMenuPos = (configMenu)((int)configMenuPos - 1);
        if (configMenuPos < font) configMenuPos = alarmOnOff;
        displayMenu();
      } else {
        switch (configMenuActive) {
          case configMenuBlank: break;
          case font:
            matrix.FONT = !matrix.FONT;
            max7219.font = matrix.FONT;
            displayMenu();
            break;

          case brightness:
            matrix.BRT--;
            if (matrix.BRT < 1) matrix.BRT += 15;
            max7219.setBrightness(matrix.BRT);
            displayMenu();
            break;

          case daylightSavingTime:
            matrix.DST = !matrix.DST;
            displayMenu();
            break;
          case alarmOnOff:
            matrix.ALARM = !matrix.ALARM;
            displayMenu();
            break;
        }
      }
      break;
  }
}

void click() {

  uint8_t timeSize = sizeof(time);

  switch (menuActive) {
    case menuBlank:
    case stopwatch: break;
    case clock:
      menuActive = menuPos;
      if (menuActive == timer || menuActive == stopwatch) {
        time.hour = 0;
        time.minute = 0;
        time.second = 0;
      }
      displayMenu();
      break;

    case alarm:
    case timer:
      if (!timerActive && !stopwatchActive) {
        timePtr++;
        if (timePtr == (int8_t*)&time + timeSize) timePtr -= timeSize;
      }
      break;

    case config:
      if (configMenuActive == configMenuBlank) configMenuActive = configMenuPos;
      displayMenu();
      break;
  }
}


void longPress() {

  switch (menuActive) {
    case menuBlank:
    case clock: break;
    case alarm:
      rtc.writeSqwPinMode(DS3231_OFF);
      rtc.setAlarm1(DateTime(2024, 5, 2, time.hour, time.minute, time.second), DS3231_A1_Hour);
      rtc.writeSqwPinMode(DS3231_SquareWave1Hz);
      menuActive = clock;
      menuPos = clock;
      timePtr = (int8_t*)&time;
      break;

    case timer:
      timerActive = true;
      timePtr = (int8_t*)&time;
      menuActive = timer;
      menuPos = menuBlank;
      displayMenu();
      break;

    case stopwatch:
      stopwatchActive = true;
      menuActive = stopwatch;
      menuPos = menuBlank;
      displayMenu();
      break;

    case config:
      if (configMenuActive != configMenuBlank) {
        EEPROM.update((uint8_t)configMenuActive, *(matrixPtr + (uint8_t)configMenuActive - 1));
        configMenuActive = configMenuBlank;
        configMenuPos = font;
        displayMenu();
      }
      break;
  }
}


void doubleClick() {

  switch (menuActive) {
    case menuBlank: break;
    case clock:
    case alarm:
    case timer:
    case stopwatch:
      menuActive = clock;
      menuPos = clock;
      timerActive = false;
      stopwatchActive = false;
      timePtr = (int8_t*)&time;
      displayMenu();
      break;

    case config:
      if (configMenuActive == configMenuBlank) {
        menuActive = clock;
        menuPos = clock;
        displayMenu();
        break;
      } else {
        configMenuActive = configMenuBlank;
        displayMenu();
      }
      break;
  }
}


void turnOnAlarm() {

  gpsSerial.end();
  rtc.clearAlarm(1);
  alarmActive = true;
  menuActive = menuBlank;
}


void turnOffAlarm() {

  gpsSerial.begin(9600);
  noTone(buzzerPin);
  max7219.allOff();
  alarmActive = false;
  timerActive = false;
  menuActive = clock;
  menuPos = clock;
}


void updateTimer() {

  time.second--;
  checkTime();
  if (time.hour == 0 && time.minute == 0 && time.second == 0) {
    gpsSerial.end();
    alarmActive = true;
    timerActive = false;
  }
  interruptFlag = false;
  displayMenu();
}


void updateStopwatch() {

  time.second++;
  checkTime();
  displayMenu();
  interruptFlag = false;
}


void toggleBuzzer() {

  if (millis() - prevTime > 500) {
    prevTime = millis();
    buzzerState = !buzzerState;

    if (buzzerState) {
      tone(buzzerPin, toneFrequency);
      max7219.allOn();
    } else {
      noTone(buzzerPin);
      max7219.allOff();
    }
  }
}


void updateTime() {

  if (usingRTC) {

    time.hour = rtc.now().hour();
    time.minute = rtc.now().minute();
    time.second = rtc.now().second();

  } else {

    if (gps.time.isValid()) {

      time.hour = gps.time.hour() + 1 + matrix.DST;
      time.minute = gps.time.minute();
      time.second = gps.time.second() + 1;
      checkTime();

      if (time.minute == 30 && time.second == 30) {
        rtc.adjust(DateTime(gps.date.year(), gps.date.month(), gps.date.day(), time.hour, 30, 30));
      }
    }
  }
}


void displayMenu() {

  if (menuActive != config) {

    if (menuPos != clock && menuActive == clock) {

      switch (menuPos) {
        case menuBlank:
        case clock: break;
        case alarm: matrix.FONT ? max7219.display("ALRM") : max7219.display("ALARM"); break;
        case timer: matrix.FONT ? max7219.display("TIMR") : max7219.display("TIMER"); break;
        case stopwatch: matrix.FONT ? max7219.display("STPW") : max7219.display("STPW"); break;
        case config: matrix.FONT ? max7219.display("CONF") : max7219.display("CONFIG"); break;
      }
    } else {

      switch (menuActive) {
        case menuBlank:
        case clock:
        case alarm:
        case timer:
        case stopwatch: max7219.display(time2String(time.hour, time.minute, time.second)); break;
        case config: max7219.display("FONT"); break;
      }
    }
  } else {

    if (configMenuPos != configMenuBlank && configMenuActive == configMenuBlank) {

      switch (configMenuPos) {
        case configMenuBlank: break;
        case font: max7219.display("FONT"); break;
        case brightness: max7219.display("BRT"); break;
        case daylightSavingTime: max7219.display("DST"); break;
        case alarmOnOff: max7219.display("ALRM"); break;
      }
    } else if (configMenuPos != configMenuBlank && configMenuActive != configMenuBlank) {

      switch (configMenuActive) {
        case configMenuBlank: break;
        case font: max7219.display(matrix.FONT ? "12 : 34" : "12 : 34 : 56"); break;
        case brightness: max7219.display(String(matrix.BRT)); break;
        case daylightSavingTime: max7219.display(matrix.DST ? "ON" : "OFF"); break;
        case alarmOnOff: max7219.display(matrix.ALARM ? "ON" : "OFF"); break;
      }
    }
  }
}


String time2String(uint8_t hour, uint8_t minute, uint8_t second) {

  String _hour = (hour < 10 ? "   " : String(hour / 10)) + String(hour % 10);
  String _minute = String(minute / 10) + String(minute % 10);
  String _second = String(second / 10) + String(second % 10);

  String timeStr;
  if (time.second % 2 == 0) timeStr = matrix.FONT ? _hour + " : " + _minute : _hour + " : " + _minute + " : " + _second;
  else timeStr = matrix.FONT ? _hour + "    " + _minute : _hour + "    " + _minute + "    " + _second;

  return timeStr;
}


String temp2String(float t) {

  String _t = (matrix.FONT ? "" : "     ") + String((int)t) + "." + String(((int)(t * 10) % 10)) + "c";

  return _t;
}


void RTC_ISR(void) {

  if (millis() - startTime > 1001) {
    usingRTC = true;
    interruptFlag = 1;
  }
}


void GPS_ISR(void) {

  startTime = millis();
  usingRTC = false;
  interruptFlag = 1;
}


void checkTime() {

  if (time.second > 59) {
    time.second -= 60;
    time.minute++;
  }

  if (time.minute > 59) {
    time.minute -= 60;
    time.hour++;
  }

  if (time.hour > 23) time.hour -= 24;

  if (time.second < 0) {
    time.second += 60;
    time.minute--;
  }

  if (time.minute < 0) {
    time.minute += 60;
    time.hour--;
  }

  if (time.hour < 0) time.hour += 24;
}