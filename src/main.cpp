/*
ESP32 NETWORK CLOCK             NATHANIEL KESSEL
WORK IN PROGRESS                ESP32-S3
*/

#include <Arduino.h>          // ARDUINO      
#include <WiFi.h>             // WIFI
#include <SPI.h>              // SPI FOR 74HC595
#include <time.h>             // TIME FUNCTIONS
#include <esp_timer.h>        // FOR DEBOUNCE (HARDWARE TIMER)

#include "secrets.h"          // WIFI CRED          
#include "display_font.h"     // CHAR DISPLAY HANDLER
#include "selection_util.h"

#include "features/stopwatch.h"        // STOPWATCH CLASS
#include "features/alarm.h"
#include "features/timezone.h"
#include "features/notification.h"
#include "features/timer.h"

#include <string>

// --- STATE MACHINE ---
enum SystemState {
  CLOCK_CLEAN,     // CLOCK WITHOUT NAVIGATON
  NAV_MODE,        // MENU WITH NAVIGATION ARROWS, IND 0 IS CLOCK + NAV, IND 1 IS TIME ZONE
  TZ_SELECT,      // TIME ZONE MENU
  STOPWATCH,
  ALARM,
  MODE_TIMER,
  NOTIFICATION,
};

SystemState currentState = CLOCK_CLEAN; // DEFAULT TO BASIC CLOCK
unsigned long menuTimeout = 0;          // INIT MENU TIMEOUT

// --- GLOBALS ---
uint64_t toDisplayWords[12];            // INIT ARRAY FOR THE PATTERNS SENT TO THE SHIFT REGISTERS
unsigned long lastUpdate = 0;           // INIT TIME SINCE THE LAST SCREEN MUX
bool hour24;
int activeAlarm = -1;

// --- PIN DEFINITIONS ---
constexpr int PIN_COPI  = 13;           // SPI SERIAL
constexpr int PIN_LATCH = 40;           // SPI RCLK
constexpr int PIN_OE    = 4;            // 74HC595 OUTPUT ENABLE (BRIGHTNESS VIA PWM)
constexpr int PIN_SCK   = 12;           // SPI CLOCK

// --- ENCODER ---
constexpr int PIN_ENCODER_PUSH = 8;     // PIN FOR PRESSING ENCODER
constexpr int PIN_ENCODER_A = 9;        // PIN A ON ENCODER
constexpr int PIN_ENCODER_B = 10;       // PIN B ON ENCODER

// --- BUTTONS ---
constexpr int PIN_HOME_BUTTON = 5;
constexpr int PIN_MOD_BUTTON  = 6;

volatile long encoderRawCount = 0;      // INIT FOR TRACKING PULSES FROM ENCODER
long lastEncoderRead = 0;               // INIT TIMER SINCE LAST ENCODER READ

volatile int menuIndex = 0;             // INIT MENU INDEX
volatile bool encoderMoved = false;     // INIT ENCODER MOVEMENT
bool lastEncState = false;              // INIT PREVIOUS ENCODER MOVEMENT
int timeLastPressed = 0;                // INIT TIMER SINCE LAST ENCODER PRESS

SPISettings srSettings(10000000, MSBFIRST, SPI_MODE0); // SET SPI

// --- TIMER POLLING ---
static const int8_t encoder_states[] = {
  0, -1,  1,  0,
  1,  0,  0, -1,
 -1,  0,  0,  1,
  0,  1, -1,  0
};

void IRAM_ATTR onTimer(void* arg) {        
  static uint8_t old_AB = 0;                                                    // init old_AB = 0b00000000 
  old_AB <<= 2;                                                                 // MOVES BITS LEFT 2
  old_AB |= (digitalRead(PIN_ENCODER_A) << 1) | digitalRead(PIN_ENCODER_B);     // READ PINS A AND B, PUT PIN A VAL IN POS 1 AND PIN B VAL IN POS 2, NOW WE HAVE 0b0000ABAB WHERE FIRST AB IS OLDAB AND SECOND AB IS NEW AB
  old_AB &= 0x0f;                                                               // ZEROS THE EXTRA BITS SO MAX 4 BITS

  int change = encoder_states[old_AB];                                          // LOOK TO SEE IF VALID MOVE
  if (change != 0) {                                                            // IF VALID MOVE
    encoderRawCount += change;                                                  // INCREMENT encoderRawCount BY VALUE IN encoder_states
    encoderMoved = true;                                                        // SIGNAL THE ENCODER CHANGE
  }
}

void setupEncoderTimer() {                                                      
  const esp_timer_create_args_t periodic_timer_args = {                         // TIMER SETTINGS
    .callback = &onTimer,                                                       // CREATE A FUNCTION FOR THE TIMER TO FORCE INTERUPT 
    .name = "encoder_timer"                                                     // TIMER NAME
  };
  esp_timer_handle_t encoder_timer;                                             // TIMER ID
  esp_timer_create(&periodic_timer_args, &encoder_timer);                       // CREATES TIMER IN MEMORY USING SETTINGS AND ID
  esp_timer_start_periodic(encoder_timer, 1000);                                // STARTS TIMER BY ID AND W/O STOP (PERIODIC)
}

// --- LOW LEVEL HARDWARE ---
static void latchPulse() {                                                    
  digitalWrite(PIN_LATCH, HIGH);                                                // SET LATCH PIN HIGH
  delayMicroseconds(1);                                                         // WAIT
  digitalWrite(PIN_LATCH, LOW);                                                 // SET LATCH PIN LOW
  delayMicroseconds(1);                                                         // WAIT TO PREVENT BEING TOO FAST
}

void spiWrite64(uint64_t data) {  
  uint32_t high = (uint32_t)(data >> 32);                                       // BREAK HALF OF THE 64 BITS OFF
  uint32_t low = (uint32_t)(data & 0xFFFFFFFF);                                 // BREAK HALF OF THE 64 BITS OFF
  digitalWrite(PIN_LATCH, LOW);                                                 // DO NOT DISPLAY DATA
  SPI.beginTransaction(srSettings);                                             // OPEN SPI TRANSMISSION
  SPI.transfer32(high);                                                         // SEND MSB PART
  SPI.transfer32(low);                                                          // SEND LSB PART
  SPI.endTransaction();                                                         // CLOSE SPI TRANSMISSION
  asm volatile("nop;nop;nop;nop");                                              // SHORT DELAY 
  latchPulse();                                                                 // LATCH SHIFT REGISTERS
}

// --- RENDER FUNCTION ---
void renderDisplay(uint64_t* currentBuffer) {
  for (int i = 0; i < 12; i++) {                                                // LOOP 12 POSITIONS
    spiWrite64(currentBuffer[i]);                                               // WRITE THE DATA FOR EACH CHAR
    delayMicroseconds(300);                                                     // MUX REFRESH RATE
  }
}

// --- HELPERS ---
String formatTime(const tm& ti, bool hour24) {
  char buf[16];                                                                 // INIT DISPLAY CHAR BUFFER
  if (!hour24) strftime(buf, sizeof(buf), " %I:%M:%S  %p ", &ti);               // WRITE TEXT TO DISPLAY TO CHAR BUFFER FORMATTED FOR 12HR
  else strftime(buf, sizeof(buf), "   %H:%M:%S   ", &ti);                       // WRITE TEXT TO DISPLAY TO CHAR BUFFER FORMATTER FOR 24HR
  return String(buf);                                                           // RETURN THE TEXT TO DISPLAY AS A STRING
}

// --- FUNCTIONS ---
void displayBufferTime(bool showArrows) {
  if (lastEncState) {hour24 = true;} else {hour24 = false;};

  struct tm timeinfo;                                                                              // INIT STRUCT FOR TIME DETAILS

  if (getLocalTime(&timeinfo, 0)) {                                                                // DOES ESP32 HAVE SYNCED TIME AND IF SO WHAT IS IT
    displayBuilder((char*)formatTime(timeinfo, hour24).c_str(), toDisplayWords, showArrows);       // BUILD toDisplayWords FORMATTED
  } else {
    displayBuilder("  SYNCING   ", toDisplayWords, false);
  }
}

bool buttonDetect(bool buttonPressed, unsigned long now) {
  if (buttonPressed && (now - timeLastPressed > 250)) return true;
  return false;
}

void WiFisetup(){                                                         
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);                                         // BEGIN CONNECTING TO WIFI USING GIVEN SSID AND PASSWORD
  displayBuilder(" CONNECTING ", toDisplayWords, false);                        // BUILD toDisplayWords TO SHOW CONNECTING MESSAGE
  while (WiFi.status() != WL_CONNECTED) {                                       // WHILE NOT CONNECTED
    for(int i = 0; i < 50; i++) renderDisplay(toDisplayWords);                  // DISPLAY CONNECTING MESSAGE
  }
  unsigned long start = millis();                                               // TIMESTAMP FOR 1 SEC MESSAGE
  while(millis() - start < 1000) renderDisplay(toDisplayWords);                 // SHOW CONNECTING MESSAGE FOR ONE SECOND TO PREVENT FLICKER
}

void initTime(const char* tz) {                                                 
  configTzTime(tz, "pool.ntp.org");                                             // SYNC TIME WITH ntp
}

Stopwatch stopwatchTool;
Alarm alarmTool;
TimeZoneSetting tzTool;
Notification notifTool;
Timer timerTool;

// --- SETUP ---
void setup() {
  Serial.begin(115200);                                                         // START SERIAL MONITOR AT BAUD RATE 115200
  pinMode(PIN_ENCODER_PUSH, INPUT_PULLUP);                                      // DEFINE ENCODER BUTTON AS INPUT
  if (digitalRead(PIN_ENCODER_PUSH) == LOW) {
    displayBuilder("  RESETING  ", toDisplayWords, false);
    
    alarmTool.begin();      
    alarmTool.factoryReset();
    
    unsigned long startReset = millis();
    while (millis() - startReset < 2000) {
      renderDisplay(toDisplayWords); 
    }
  }
  alarmTool.begin();
  pinMode(PIN_HOME_BUTTON, INPUT_PULLUP);
  pinMode(PIN_MOD_BUTTON, INPUT_PULLUP);
  pinMode(PIN_LATCH, OUTPUT);                                                   // DEFINE LATCH AS OUTPUT
  pinMode(PIN_OE, OUTPUT);                                                      // DEFINE OE AS OUTPUT
  digitalWrite(PIN_OE, LOW);                                                    // SET OE LOW FOR MAX BRIGHTNESS
  SPI.begin(PIN_SCK, -1, PIN_COPI, PIN_LATCH);                                  // INDICATE WHAT PINS ARE WHICH TO SPI FUNCTIONS
  initFontTable();                                                              // BRING FONT TABLE INTO MEMORY
  WiFisetup();                                                                  // CONNECT TO WIFI
  pinMode(PIN_ENCODER_A, INPUT_PULLUP);                                         // DEFINE ENCODER ROTATION DETECTION
  pinMode(PIN_ENCODER_B, INPUT_PULLUP);               
  
  setupEncoderTimer();                                                          // START TIMER FOR DEBOUNCE
  
  initTime("EST5EDT");                                                          // DEFAULT TO EST TIME ZONE AND SYNC TIME
}

// --- LOOP ---
void loop() {
  unsigned long now = millis();                                                 // TIMESTAMP START OF LOOP

  bool hasMoved = encoderMoved;

  if (hasMoved) encoderMoved = false;

  bool buttonPressed = (digitalRead(PIN_ENCODER_PUSH) == LOW);                  // DETERMINE STATE OF ENCODER BUTTON
  bool homeButtonPressed = (digitalRead(PIN_HOME_BUTTON) == LOW);
  bool modButtonPressed = (digitalRead(PIN_MOD_BUTTON) == LOW);

  // 1. INPUTS
  if (hasMoved) {
    menuTimeout = now; 
    
    long movement = encoderRawCount / 4;
    

    if (currentState == CLOCK_CLEAN) {
        if (movement != lastEncoderRead) {
            currentState = NAV_MODE;
            menuIndex = 0;
            
            lastEncoderRead = movement; 
        }
    } 

    else if (currentState == NAV_MODE) {
        if (movement != lastEncoderRead) {
            if (movement > lastEncoderRead) menuIndex++; else menuIndex--;
            lastEncoderRead = movement;
        }
    } 

    else if (currentState == ALARM) {
      if (movement != lastEncoderRead) {
        int direction = (movement > lastEncoderRead) ? 1 : -1;
              
        alarmTool.onKnobTurn(direction);
              
        lastEncoderRead = movement;
      }
    }

    else if (currentState == TZ_SELECT) {
        if (movement != lastEncoderRead) {
            int direction = (movement > lastEncoderRead) ? 1 : -1;

            tzTool.onKnobTurn(direction);

            lastEncoderRead = movement;
        }
    }

    else if (currentState == MODE_TIMER) {
        if (movement != lastEncoderRead) {
            int direction = (movement > lastEncoderRead) ? 1 : -1;

            timerTool.onKnobTurn(direction);

            lastEncoderRead = movement;
        }
    }
    
    menuTimeout = now;
  }

  // 2. LOGIC (50ms gate)
  static unsigned long lastLogic = 0;                                                                 // INIT LAST LOGIC CHAGE
  int logicRefreshSpeed = 50;

  if (currentState == STOPWATCH) {
    logicRefreshSpeed = 20; 
  } else {
    logicRefreshSpeed = 50; 
  }

  if (now - lastLogic > logicRefreshSpeed) {                                                                         // IF ITS BEEN 50ms
    lastLogic = now;                                                                                  // TIMESTAMP LAST LOGIC

    // Timeout
    unsigned long timeoutDuration = (currentState == ALARM) ? 20000 : ((currentState == NAV_MODE) ? 10000 : 5000);                              // IF ON SETTINGS MENU SET TIMEOUT TO 10s, IF ON CLOCK SET TIMEOUT TO 5s, if in alarm settings 20s
    if (currentState != CLOCK_CLEAN && currentState != STOPWATCH && currentState != NOTIFICATION && (now - menuTimeout > timeoutDuration)) {                       // IF NOT ON THE CLEAN CLOCK PAGE AND ITS BEEN LONGER THAN TIMEOUT GO TO CLOCK PAGE
      if (currentState == ALARM) {
          alarmTool.save();
      }
      currentState = CLOCK_CLEAN;
      alarmTool.reset();
    }

    if (buttonDetect(homeButtonPressed, now)) {
      alarmTool.reset();
      timerTool.reset();
      stopwatchTool.reset();
      currentState = CLOCK_CLEAN;
    }

    if (currentState != NOTIFICATION) {
      if (alarmTool.shouldRing(0)) {
        activeAlarm = 0;
        currentState = NOTIFICATION;
      } else if (alarmTool.shouldRing(1)) {
        activeAlarm = 1;
        currentState = NOTIFICATION;
      } else if (alarmTool.shouldRing(2)) {
        activeAlarm = 2;
        currentState = NOTIFICATION;
      }
    }

    // State Machine
    switch (currentState) {
      case CLOCK_CLEAN:                                                                               // IF ON CLEAN CLOCK PAGE
        displayBufferTime(false);                                                                     // RETURNS BUILT toDisplayWords WITHOUT NAV ARROWS
        if (buttonPressed && (now - timeLastPressed > 250)) {                                         // IF THE BUTTON IS PRESSED AND AFTER 250ms
          lastEncState = !lastEncState;                                                               // SWAP BUTTON STATE (TOGGLE SWITCH)
          timeLastPressed = now;                                                                      // TIMESTAMP BUTTON PRESS
        }
        break;

      case NAV_MODE:                                                                                  // IF ON NAV CLOCK PAGE
        if (menuIndex < 0) menuIndex = 4;                                                             // IF MENU IS LESS THAN 0 CORRECT TO 1
        if (menuIndex > 4) menuIndex = 0;                                                             // IF MENU IS MORE THAN 1 CORRECT TO 0

        if (menuIndex == 0) {                                                                         // IF ON TIME PAGE
          displayBufferTime(true);                                                                    // GET toDisplayWords FOR TIME WITH NAV ARROWS
          if (buttonPressed && (now - timeLastPressed > 250)) {                                         // IF THE BUTTON IS PRESSED AND AFTER 250ms
            lastEncState = !lastEncState;                                                               // SWAP BUTTON STATE (TOGGLE SWITCH)
            timeLastPressed = now;                                                                      // TIMESTAMP BUTTON PRESS
          }
        } else if (menuIndex == 1) {
          displayBuilder(" STOPWATCH  ", toDisplayWords, true);
          if (buttonPressed && (now - timeLastPressed > 250)) {                                       // IF BUTTON IS PRESSED
            currentState = STOPWATCH;                                                                 // GO TO TIME ZONE SELECT MENU
            timeLastPressed = now;                                                                    // TIMESTAMP BUTTON PRESS
          }
        } else if (menuIndex == 2) {
          displayBuilder(" TIMER      ", toDisplayWords, true);
          if (buttonDetect(buttonPressed, now)) {
            currentState = MODE_TIMER;

            timeLastPressed = now;
            menuTimeout = now;
          }
        } else if (menuIndex == 3) {
          displayBuilder(" TIME ZONE  ", toDisplayWords, true);                                       // IF NOT ON THE TIME PAGE THEN GET toDisplayWords FOR TIME ZONE OPTION
          if (buttonPressed && (now - timeLastPressed > 250)) {                                       // IF BUTTON IS PRESSED
            currentState = TZ_SELECT;                                                                 // GO TO TIME ZONE SELECT MENU
            
            timeLastPressed = now;                                                                    // TIMESTAMP BUTTON PRESS
            menuTimeout = now;                                                                        // TIMESTAMP TIMEOUT
          }

        } else if (menuIndex == 4) {
          displayBuilder(" ALARM     ", toDisplayWords, true);
          if (buttonPressed && (now - timeLastPressed > 250)) {
            timeLastPressed = now;
            currentState = ALARM;

            alarmTool.reset();

            encoderRawCount = 0;
            lastEncoderRead = 0;
          }
        }
        break;

      case TZ_SELECT:
        displayBuilder((char*)tzTool.getDisplayString().c_str(), toDisplayWords, true);

        if (buttonPressed && (now - timeLastPressed > 250)) {
          initTime(tzTool.getSelectedPosix());
          currentState = CLOCK_CLEAN;
        }
        break;
        

      case STOPWATCH: { 
        displayBuilder((char*)stopwatchTool.getFormattedTime().c_str(), toDisplayWords, false);

        if (buttonPressed && (now - timeLastPressed > 250)) {
          timeLastPressed = now; 
          stopwatchTool.toggle();
        }

        if (hasMoved) {
          currentState = NAV_MODE;
          stopwatchTool.reset();
          encoderMoved = false;
        }
        
        break;
      }
      case ALARM: {
        displayBuilder((char*)alarmTool.getAlarmDisplay(hour24).c_str(), toDisplayWords, true);

        if (buttonPressed && (now - timeLastPressed > 250)) {
          timeLastPressed = now;
          alarmTool.onButtonPress();
          menuTimeout = now;
        }

        break;
      }
      case MODE_TIMER: {
        displayBuilder((char*)timerTool.getTimerDisplay().c_str(), toDisplayWords, true);

        if (buttonDetect(buttonPressed, now)) {
          timeLastPressed = now;
          timerTool.onButtonPress();
          menuTimeout = now;
        }

        if (buttonDetect(modButtonPressed, now)) {
          timeLastPressed = now;
          timerTool.onModButtonPress();
          menuTimeout = now;
        }

        break;
      }
      case NOTIFICATION: {
        const char* message = "";
        if (activeAlarm == 0) {
          message = "ALARM 1";
        } else if (activeAlarm == 1) {
          message = "ALARM 2";  
        } else if (activeAlarm == 2) {
          message = "ALARM 3";
        }

        displayBuilder((char*)notifTool.getNotificationDisplay(message).c_str(), toDisplayWords, false);

        if (buttonPressed && (now - timeLastPressed > 250)) {
            activeAlarm = -1;
            timeLastPressed = now;
            currentState = CLOCK_CLEAN;
        }
      }
    }
  }
  renderDisplay(toDisplayWords);                                                                      // RENDER CURRENT SCREEN STATE
}



// 2026-19-01 | 10:26 - 11:23
// 2026-20-01 | 1:22 - 