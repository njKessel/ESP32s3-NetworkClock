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

#include <string>

// --- STATE MACHINE ---
enum SystemState {
  CLOCK_CLEAN,     // CLOCK WITHOUT NAVIGATON
  NAV_MODE,        // MENU WITH NAVIGATION ARROWS, IND 0 IS CLOCK + NAV, IND 1 IS TIME ZONE
  TZ_SELECT,      // TIME ZONE MENU
  STOPWATCH,
  ALARM,
  A_TIMESET,
  A_REPEAT
};

SystemState currentState = CLOCK_CLEAN; // DEFAULT TO BASIC CLOCK
unsigned long menuTimeout = 0;          // INIT MENU TIMEOUT

// --- GLOBALS ---
uint64_t toDisplayWords[12];            // INIT ARRAY FOR THE PATTERNS SENT TO THE SHIFT REGISTERS
unsigned long lastUpdate = 0;           // INIT TIME SINCE THE LAST SCREEN MUX

// --- PIN DEFINITIONS ---
constexpr int PIN_COPI  = 13;           // SPI SERIAL
constexpr int PIN_LATCH = 40;           // SPI RCLK
constexpr int PIN_OE    = 4;            // 74HC595 OUTPUT ENABLE (BRIGHTNESS VIA PWM)
constexpr int PIN_SCK   = 12;           // SPI CLOCK

// --- ENCODER ---
constexpr int PIN_ENCODER_PUSH = 8;     // PIN FOR PRESSING ENCODER
constexpr int PIN_ENCODER_A = 9;        // PIN A ON ENCODER
constexpr int PIN_ENCODER_B = 10;       // PIN B ON ENCODER

volatile long encoderRawCount = 0;      // INIT FOR TRACKING PULSES FROM ENCODER
long lastEncoderRead = 0;               // INIT TIMER SINCE LAST ENCODER READ

volatile int menuIndex = 0;             // INIT MENU INDEX
volatile int alarmMenuIndex = 1;
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
  struct tm timeinfo;                                                                              // INIT STRUCT FOR TIME DETAILS
  if (getLocalTime(&timeinfo, 0)) {                                                                // DOES ESP32 HAVE SYNCED TIME AND IF SO WHAT IS IT
    displayBuilder((char*)formatTime(timeinfo, lastEncState).c_str(), toDisplayWords, showArrows); // BUILD toDisplayWords FORMATTED
  }
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
  displayBuilder("  SYNCING   ", toDisplayWords, false);                        // BUILD toDisplayWords TO SHOW SYNC MESSAGE
  configTzTime(tz, "pool.ntp.org");                                             // SYNC TIME WITH ntp
  
  tm ti{};
  unsigned long start = millis();                                               // TIMESTAMP FOR 1 SEC MESSAGE
  while (!getLocalTime(&ti, 0) && (millis() - start < 10000)) {                 // IF NOT CONNECTED AND HASNT BEEN ONE SEC
    renderDisplay(toDisplayWords);                                              // DISPLAY SYNC MESSAGE
  }
}

// --- MENU DATA ---
struct TZEntry { const char* posix; const char* tz; const char* offset; const char* city; }; // SET UP POSIX TABLE

const TZEntry TZ_menu[] = { // POSIX TABLE FOR POSIX STRINGS AND MENUS
  // POSIX Code                                               Label           Offset        City/Region
  {"AOE12",                                                   "  AOE",        "  -12",      "  Baker Island"},           // 0
  {"NUT11",                                                   "  NUT",        "  -11",      "  American Samoa"},         // 1
  {"HST10",                                                   "  HST",        "  -10",      "  Hawaii"},                 // 2
  {"MART9:30",                                                "  MART",       "  -9:30",    "  French Polynesia"},       // 3
  {"AKST9AKDT,M3.2.0/2:00:00,M11.1.0/2:00:00",                "  AKST",       "  -9",       "  Alaska"},                 // 4
  {"PST8PDT,M3.2.0/2:00:00,M11.1.0/2:00:00",                  "  PDT",        "  -8",       "  Los Angeles"},            // 5
  {"MST7MDT,M3.2.0/2:00:00,M11.1.0/2:00:00",                  "  MST MDT",    "  -7",       "  Denver"},                 // 6
  {"MST7",                                                    "  MST",        "  -7",       "  Phoenix"},                // 7
  {"CST6CDT,M3.2.0/2:00:00,M11.1.0/2:00:00",                  "  CST",        "  -6",       "  Chicago"},                // 8
  {"EST5EDT,M3.2.0/2,M11.1.0/2",                              "  EST",        "  -5",       "  New York"},               // 9
  {"AST4ADT,M3.2.0/2:00:00,M11.1.0/2:00:00",                  "  AST",        "  -4",       "  Halifax"},                // 10
  {"BRT3",                                                    "  BRT",        "  -3",       "  São Paulo"},              // 11
  {"FKST3FKDT,M9.1.0/2:00:00,M4.3.0/2:00:00",                 "  FKST",       "  -3",       "  Falkland Islands"},       // 12
  {"GRNL2",                                                   "  GRL",        "  -2",       "  South Georgia"},          // 13
  {"AZOT1AZOST,M3.5.0/0:00:00,M10.5.0/1:00:00",               "  AZOT",       "  -1",       "  Azores"},                 // 14
  {"GMT0",                                                    "  GMT",        "  0",        "  London"},                 // 15
  {"GMT0BST,M3.5.0/1:00:00,M10.5.0/2:00:00",                  "  BST",        "  +1",       "  United Kingdom"},         // 16
  {"CET-1CEST,M3.5.0/2:00:00,M10.5.0/3:00:00",                "  CET",        "  +1",       "  Berlin"},                 // 17
  {"WAT-1",                                                   "  WAT",        "  +1",       "  Nigeria"},                // 18
  {"EET-2EEST,M3.5.0/3:00:00,M10.5.0/4:00:00",                "  EET",        "  +2",       "  Athens"},                 // 19
  {"CAT-2",                                                   "  CAT",        "  +2",       "  South Africa"},           // 20
  {"MSK-3",                                                   "  MSK",        "  +3",       "  Moscow"},                 // 21
  {"AST-3",                                                   "  AST",        "  +3",       "  Saudi Arabia"},           // 22
  {"GST-4",                                                   "  GST",        "  +4",       "  Dubai"},                  // 23
  {"AZT-4AZST,M3.5.0/4:00:00,M10.5.0/5:00:00",                "  AZT",        "  +4",       "  Azerbaijan"},             // 24
  {"AFT-4:30",                                                "  AFT",        "  +4:30",    "  Afghanistan"},            // 25
  {"PKT-5",                                                   "  PKT",        "  +5",       "  Pakistan"},               // 26
  {"IST-5:30",                                                "  IST",        "  +5:30",    "  India"},                  // 27
  {"NPT-5:45",                                                "  NPT",        "  +5:45",    "  Nepal"},                  // 28
  {"BST-6",                                                   "  BST",        "  +6",       "  Bangladesh"},             // 29
  {"MMT-6:30",                                                "  MMT",        "  +6:30",    "  Myanmar"},                // 30
  {"ICT-7",                                                   "  ICT",        "  +7",       "  Thailand"},               // 31
  {"WIB-7",                                                   "  WIB",        "  +7",       "  Indonesia West"},         // 32
  {"CST-8",                                                   "  CST CHINA",  "  +8",       "  China"},                  // 33
  {"AWST-8",                                                  "  AWST",       "  +8",       "  Perth"},                  // 34
  {"HKT-8",                                                   "  HKT",        "  +8",       "  Hong Kong"},              // 35
  {"WITA-8",                                                  "  WITA",       "  +8",       "  Indonesia Central"},      // 36
  {"JST-9",                                                   "  JST",        "  +9",       "  Japan"},                  // 37
  {"KST-9",                                                   "  KST",        "  +9",       "  South Korea"},            // 38
  {"ACST-9:30ACDT,M10.1.0/2:00:00,M4.1.0/3:00:00",            "  ACST",       "  +9:30",    "  Adelaide"},               // 39
  {"CST-9:30",                                                "  CST N TERR", "  +9:30",    "  Northern Territory"},     // 40
  {"AEST-10AEDT,M10.1.0/2:00:00,M4.1.0/3:00:00",              "  AEST",       "  +10",      "  Sydney"},                 // 41
  {"CHUT-10",                                                 "  CHUT",       "  +10",      "  Chuuk"},                  // 42
  {"WIT-9",                                                   "  WIT",        "  +9",       "  Indonesia East"},         // 43
  {"SBT-11",                                                  "  SBT",        "  +11",      "  Solomon Islands"},        // 44
  {"NCT-11",                                                  "  NCT",        "  +11",      "  New Caledonia"},          // 45
  {"NZST-12NZDT,M9.5.0/2:00:00,M4.1.0/3:00:00",               "  NZST",       "  +12",      "  New Zealand"},            // 46
  {"FJT-12FJDT,M11.1.0/2:00:00,M1.2.0/3:00:00",               "  FJT",        "  +12",      "  Fiji"},                   // 47
  {"TOT-13",                                                  "  TOT",        "  +13",      "  Tonga"},                  // 48
  {"PHOT-13",                                                 "  PHOT",       "  +13",      "  Phoenix Islands"},        // 49
  {"LINT-14",                                                 "  LINT",       "  +14",      "  Line Islands"}            // 50
};

const size_t TZ_MENU_COUNT = sizeof(TZ_menu) / sizeof(TZ_menu[0]);              // TZ_menu SIZE CALCULATION

struct alarmData {int alarm; uint8_t alarmHours; uint8_t alarmMinutes; uint8_t repeatDays;};

alarmData alarmTable[] = {
// ALARM #    HOUR    MINUTE    DAYS
  {1,         6,      30,       0b00111110},
  {2,         22,     0,        0b00111110},
  {3,         0,      0,        0b00000000}
};

String stopwatchFormat(unsigned long long stopwatchTime) {
  unsigned long totalSeconds = stopwatchTime / 1000;
  
  int SWhours   = totalSeconds / 3600;
  int SWminutes = (totalSeconds / 60) % 60;
  int SWseconds = totalSeconds % 60;
  int SWmillis  = stopwatchTime % 1000;

  char timeBuffer[20]; 
  snprintf(timeBuffer, sizeof(timeBuffer), "%02d:%02d:%02d:%03d", SWhours, SWminutes, SWseconds, SWmillis);

  return String(timeBuffer) + "  ";
}

// String alarmFormat(menuIndex) {

// }


// String timesetHandler(menuIndex) {
//   if (menuIndex == 1) {
//       char alarmBuffer[20];
//       snprintf(alarmBuffer, sizeof(alarmBuffer), " %01d  %02d:%02d %s", alarmTable[alarmMenuIndex].alarm, alarmTable[alarmMenuIndex].alarmHours, alarmTable[alarmMenuIndex].alarmMinutes);
//   }
// }

// --- SETUP ---
void setup() {
  Serial.begin(115200);                                                         // START SERIAL MONITOR AT BAUD RATE 115200
  pinMode(PIN_LATCH, OUTPUT);                                                   // DEFINE LATCH AS OUTPUT
  pinMode(PIN_OE, OUTPUT);                                                      // DEFINE OE AS OUTPUT
  digitalWrite(PIN_OE, LOW);                                                    // SET OE LOW FOR MAX BRIGHTNESS
  SPI.begin(PIN_SCK, -1, PIN_COPI, PIN_LATCH);                                  // INDICATE WHAT PINS ARE WHICH TO SPI FUNCTIONS
  initFontTable();                                                              // BRING FONT TABLE INTO MEMORY
  WiFisetup();                                                                  // CONNECT TO WIFI
  pinMode(PIN_ENCODER_PUSH, INPUT_PULLUP);                                      // DEFINE ENCODER BUTTON AS INPUT
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

  // 1. INPUTS
  if (hasMoved) {                                                           // IF ENCODER HAS CHANGED STATE
    menuTimeout = now;                                                          // RESET TIMEROUT
    
    long movement = encoderRawCount / 4;                                        // DETECTS IF THE ENCODER HAS MOVED 
    
    if (movement != lastEncoderRead) {                                          // IF MOVEMENT CHANGES FROM LAST
        if (movement > lastEncoderRead) menuIndex++; else menuIndex--;          // CHANGE MENU + IF INCREASE AND - IF DECREASE
        lastEncoderRead = movement;                                             // SET LAST TO CURRENT
        
        if (currentState == CLOCK_CLEAN) {                                      // IF ON THE CLOCK
           currentState = NAV_MODE;                                             // BRING UP NAVIGATION
           menuIndex = 0;                                                       // SET MENU 0
        }
    }
  }

  // 2. LOGIC (50ms gate)
  static unsigned long lastLogic = 0;                                                                 // INIT LAST LOGIC CHAGE
  if (now - lastLogic > 50) {                                                                         // IF ITS BEEN 50ms
    lastLogic = now;                                                                                  // TIMESTAMP LAST LOGIC

    // Timeout
    unsigned long timeoutDuration = (currentState == NAV_MODE && menuIndex == 1) ? 10000 : 5000;      // IF ON SETTINGS MENU SET TIMEOUT TO 10s, IF ON CLOCK SET TIMEOUT TO 5s
    if (currentState != CLOCK_CLEAN && currentState != STOPWATCH && (now - menuTimeout > timeoutDuration)) {                       // IF NOT ON THE CLEAN CLOCK PAGE AND ITS BEEN LONGER THAN TIMEOUT GO TO CLOCK PAGE
      currentState = CLOCK_CLEAN;
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
        } else if (menuIndex == 1) {
          displayBuilder(" STOPWATCH  ", toDisplayWords, true);
          if (buttonPressed && (now - timeLastPressed > 250)) {                                       // IF BUTTON IS PRESSED
            currentState = STOPWATCH;                                                                 // GO TO TIME ZONE SELECT MENU
            timeLastPressed = now;                                                                    // TIMESTAMP BUTTON PRESS
          }
        } else if (menuIndex == 2) {
          displayBuilder(" TIMER      ", toDisplayWords, true);

        } else if (menuIndex == 3) {
          displayBuilder(" TIME ZONE  ", toDisplayWords, true);                                       // IF NOT ON THE TIME PAGE THEN GET toDisplayWords FOR TIME ZONE OPTION
          if (buttonPressed && (now - timeLastPressed > 250)) {                                       // IF BUTTON IS PRESSED
            currentState = TZ_SELECT;                                                                 // GO TO TIME ZONE SELECT MENU
            menuIndex = 9;                                                                            // DEFAULT TO EST OPTION
            
            encoderRawCount = 9 * 4;                                                                  // SET encoderRawCount TO POSITION FOR EST
            lastEncoderRead = 9;                                                                      // SET lastEncoderRead TO 9 SO IT SCROLLS THROUGH LIST AT CORRECT POSITION
            
            timeLastPressed = now;                                                                    // TIMESTAMP BUTTON PRESS
            menuTimeout = now;                                                                        // TIMESTAMP TIMEOUT
          }

        } else if (menuIndex == 4) {
          displayBuilder("  ALARM     ", toDisplayWords, true);
          if (buttonPressed && (now - timeLastPressed > 250)) {
            currentState = ALARM;
          }
        }
        break;

      case TZ_SELECT:
        if (menuIndex < 0) menuIndex = TZ_MENU_COUNT - 1;                                             // WRAP TO END OF MENU IF YOU GO BEFORE THE FIRST OPTION
        if (menuIndex >= (int)TZ_MENU_COUNT) menuIndex = 0;                                           // WRAP TO START OF MENU IF YOU GO AFTER LAST OPTION
        
        displayBuilder(TZ_menu[menuIndex].tz, toDisplayWords, true);                                  // GET toDisplayWords FOR CURRENT OPTION WITH NAV ARROWS
        
        if (buttonPressed && (now - timeLastPressed > 250)) {                                         // IF YOU SELECT OPTION
          initTime(TZ_menu[menuIndex].posix);                                                         // RESYNC TIME WITH THAT TIME ZONE
          currentState = CLOCK_CLEAN;                                                                 // GO BACK TO CLEAN CLOCK
          timeLastPressed = now;                                                                      // TIMESTAMP BUTTON PRESS
        }
        break;
        
      case STOPWATCH: { 
        static bool swRunning = false;
        static unsigned long long swStartTime = 0;
        static unsigned long long swAccumulatedTime = 0;


        unsigned long long currentDuration = swAccumulatedTime;
        if (swRunning) {
          currentDuration += (millis() - swStartTime);
          menuTimeout = now; 
        }

        displayBuilder((char*)stopwatchFormat(currentDuration).c_str(), toDisplayWords, true);

        if (buttonPressed && (now - timeLastPressed > 250)) {
          timeLastPressed = now; 

          if (swRunning) {
            swAccumulatedTime += (millis() - swStartTime);
            swRunning = false;
          } else {
            swStartTime = millis();
            swRunning = true;
          }
        }

        if (hasMoved) {
          currentState = NAV_MODE;

          swRunning = false;
          swAccumulatedTime = 0; 
          
          encoderMoved = false; 
        }
        
        break;
      }
      case ALARM: {
        menuIndex = 1;
        switch (menuIndex) {
          case A_TIMESET: {
            if (buttonPressed && (now - timeLastPressed > 250)) {
              timeLastPressed = now;
            }
          }
        }
      }
    }
  }
  renderDisplay(toDisplayWords);                                                                      // RENDER CURRENT SCREEN STATE
}



// 2026-19-1 | 10:26 - 11:34
