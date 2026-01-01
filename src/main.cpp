/*
ESP32 NETWORK CLOCK             NATHANIEL KESSEL
WORK IN PROGRESS                        ESP32-S3
https://github.com/njKessel/ESP32s3-NetworkClock
*/


// main.cpp

#include <Arduino.h>                // Core types and functions for arduino 
#include <WiFi.h>                   // Router Connection
#include <SPI.h>                    // SPI
#include <time.h>                   // POSIX time + SNTP

#include "secrets.h"                // WiFi Cred, change to secrets.example.h
#include "display_font.h"           // Alphanumeric font

enum SystemState {
  CLOCK_HOME,      // TIME
  SETTINGS_NAV,    // SETTINGS
  TZ_SELECT        // TZ LIST
};

SystemState currentState = CLOCK_HOME;
unsigned long menuTimeout = 0;

uint64_t displayWords[12];
unsigned long lastUpdate = 0;

const uint8_t I2C_Address = 0x70; // Scan agrees 10.26
const int TZDISPLAY = 21; // GPIO PIN 21
int TZDISPLAY_status = 0;

// DISPLAY PINS
constexpr int PIN_COPI  = 13;  // SERIAL INPUT
constexpr int PIN_LATCH = 40;  // RCLK
constexpr int PIN_OE    = 4;   // BRIGHTNESS (PWM)
constexpr int PIN_SCK   = 12;  // SCK

// BUTTON PINS
constexpr int PIN_ENCODER_PUSH = 8;
constexpr int PIN_ENCODER_A = 9;
constexpr int PIN_ENCODER_B = 10;
bool lastEncState = false;

volatile int menuIndex = 0; // DEFAULT TO TIME SCREEN
volatile bool encoderMoved = false;

uint64_t toDisplayWords[12];

void IRAM_ATTR readEncoder(){
  int aState = digitalRead(PIN_ENCODER_A);
  int bState = digitalRead(PIN_ENCODER_B);
  
  if (aState == LOW) {
    if (bState == HIGH) {
      menuIndex++;
    } else {
      menuIndex--;
    }
    encoderMoved = true;
  }
}

SPISettings srSettings(10000000, MSBFIRST, SPI_MODE0);

static void latchPulse() {
  digitalWrite(PIN_LATCH, HIGH);
  delayMicroseconds(1);
  digitalWrite(PIN_LATCH, LOW);
  delayMicroseconds(1);
}

static void latchLow() {
  digitalWrite(PIN_LATCH, LOW);
}

static void latchHigh() {
  digitalWrite(PIN_LATCH, HIGH);
}

void spiWrite64(uint64_t data) {
  uint32_t high = (uint32_t)(data >> 32);
  uint32_t low = (uint32_t)(data & 0xFFFFFFFF);

  latchLow();
  SPI.beginTransaction(srSettings);
  SPI.transfer32(high);
  SPI.transfer32(low);
  SPI.endTransaction();

  asm volatile("nop;nop;nop;nop");
  latchPulse();
}

// SNTP sync
void initTime(const char* tz) {
  displayBuilder("  SYNCING   ", toDisplayWords, false);
  tm ti{}; // Zero initialization 
  configTzTime(tz, "pool.ntp.org"); // Sets timezone and starts SNTP

  const int maxAttempts = 150; // 150 * 200ms is 30s max wait
  int attempts = 0;
  while (!getLocalTime(&ti) && (attempts < maxAttempts)) {
    attempts++;
    for(int i=0; i<20; i++) displayScan(toDisplayWords);
  }
  unsigned long start = millis();
  while(millis() - start < 1000) {
      displayScan(toDisplayWords);
  }
}

unsigned long timeSinceRotate = millis();

bool navDelay() {
  int currentTime = millis();
  if (encoderMoved) {
    timeSinceRotate = millis();
    encoderMoved = false;
    return true;
  } else if (!encoderMoved && currentTime - timeSinceRotate < 5000) {
    return true;
  } else {
    return false;
  }
}

bool hour24 = false;

String formatTime(const tm& ti, bool hour24) {
  char buf[16];  // "HH:MM:SS" + NUL
  if (!hour24) {
    strftime(buf, sizeof(buf), " %I:%M:%S  %p ", &ti);
  } else {
    strftime(buf, sizeof(buf), "   %H:%M:%S   ", &ti);
  }
  return String(buf);
}

void WiFisetup(){
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  displayBuilder((char*)String(" CONNECTING ").c_str(), toDisplayWords, false);

  while (WiFi.status() != WL_CONNECTED) {
    for(int i = 0; i < 50; i++) {
        displayScan(toDisplayWords); 
    }
  }
  unsigned long start = millis();
  while(millis() - start < 1000) {
      displayScan(toDisplayWords);
  }
}

void settingsDisplay(String text) {
  displayBuilder((char*)String(text).c_str(), toDisplayWords, true);
  displayScan(toDisplayWords);
}

void setTimeFF(int yr, int month, int mday, int hr, int minute, int sec, int isDst){
  struct tm tmd{};
  tmd.tm_year = yr - 1900;
  tmd.tm_mon  = month - 1;
  tmd.tm_mday = mday;
  tmd.tm_hour = hr;
  tmd.tm_min  = minute;
  tmd.tm_sec  = sec;
  tmd.tm_isdst = isDst;
  time_t t = mktime(&tmd);
  struct timeval now = { .tv_sec = t };
  settimeofday(&now, NULL);
}

struct TZEntry {
  const char* posix;   // POSIX TZ string
  const char* tz;      // short label for menu
  const char* offset;  // human-readable offset
  const char* city;    // example city/region
};

const TZEntry TZ_menu[] = {
  // POSIX Code                                             Label         Offset        City/Region
  {"AOE12",                                                 "  AOE",      "  -12",      "  Baker Island"},           // 0
  {"NUT11",                                                 "  NUT",      "  -11",      "  American Samoa"},         // 1
  {"HST10",                                                 "  HST",      "  -10",      "  Hawaii"},                 // 2
  {"MART9:30",                                              "  MART",     "  -9:30",    "  French Polynesia"},       // 3
  {"AKST9AKDT,M3.2.0/2:00:00,M11.1.0/2:00:00",              "  AKST",     "  -9",       "  Alaska"},                 // 4
  {"PST8PDT,M3.2.0/2:00:00,M11.1.0/2:00:00",                "  PDT",      "  -8",       "  Los Angeles"},            // 5
  {"MST7MDT,M3.2.0/2:00:00,M11.1.0/2:00:00",                "  MST",      "  -7",       "  Denver"},                 // 6
  {"MST7",                                                  "  MST",      "  -7",       "  Phoenix"},                // 7
  {"CST6CDT,M3.2.0/2:00:00,M11.1.0/2:00:00",                "  CST",      "  -6",       "  Chicago"},                // 8
  {"EST5EDT,M3.2.0/2,M11.1.0/2",                            "  EST",      "  -5",       "  New York"},               // 9
  {"AST4ADT,M3.2.0/2:00:00,M11.1.0/2:00:00",                "  AST",      "  -4",       "  Halifax"},                // 10
  {"BRT3",                                                  "  BRT",      "  -3",       "  São Paulo"},              // 11
  {"FKST3FKDT,M9.1.0/2:00:00,M4.3.0/2:00:00",               "  FKST",     "  -3",       "  Falkland Islands"},       // 12
  {"GRNL2",                                                 "  GRL",      "  -2",       "  South Georgia"},          // 13
  {"AZOT1AZOST,M3.5.0/0:00:00,M10.5.0/1:00:00",             "  AZOT",     "  -1",       "  Azores"},                 // 14
  {"GMT0",                                                  "  GMT",      "  0",        "  London"},                 // 15
  {"GMT0BST,M3.5.0/1:00:00,M10.5.0/2:00:00",                "  BST",      "  +1",       "  United Kingdom"},         // 16
  {"CET-1CEST,M3.5.0/2:00:00,M10.5.0/3:00:00",              "  CET",      "  +1",       "  Berlin"},                 // 17
  {"WAT-1",                                                 "  WAT",      "  +1",       "  Nigeria"},                // 18
  {"EET-2EEST,M3.5.0/3:00:00,M10.5.0/4:00:00",              "  EET",      "  +2",       "  Athens"},                 // 19
  {"CAT-2",                                                 "  CAT",      "  +2",       "  South Africa"},           // 20
  {"MSK-3",                                                 "  MSK",      "  +3",       "  Moscow"},                 // 21
  {"AST-3",                                                 "  AST",      "  +3",       "  Saudi Arabia"},           // 22
  {"GST-4",                                                 "  GST",      "  +4",       "  Dubai"},                  // 23
  {"AZT-4AZST,M3.5.0/4:00:00,M10.5.0/5:00:00",              "  AZT",      "  +4",       "  Azerbaijan"},             // 24
  {"AFT-4:30",                                              "  AFT",      "  +4:30",    "  Afghanistan"},            // 25
  {"PKT-5",                                                 "  PKT",      "  +5",       "  Pakistan"},               // 26
  {"IST-5:30",                                              "  IST",      "  +5:30",    "  India"},                  // 27
  {"NPT-5:45",                                              "  NPT",      "  +5:45",    "  Nepal"},                  // 28
  {"BST-6",                                                 "  BST",      "  +6",       "  Bangladesh"},             // 29
  {"MMT-6:30",                                              "  MMT",      "  +6:30",    "  Myanmar"},                // 30
  {"ICT-7",                                                 "  ICT",      "  +7",       "  Thailand"},               // 31
  {"WIB-7",                                                 "  WIB",      "  +7",       "  Indonesia West"},         // 32
  {"CST-8",                                                 "  CST",      "  +8",       "  China"},                  // 33
  {"AWST-8",                                                "  AWST",     "  +8",       "  Perth"},                  // 34
  {"HKT-8",                                                 "  HKT",      "  +8",       "  Hong Kong"},              // 35
  {"WITA-8",                                                "  WITA",     "  +8",       "  Indonesia Central"},      // 36
  {"JST-9",                                                 "  JST",      "  +9",       "  Japan"},                  // 37
  {"KST-9",                                                 "  KST",      "  +9",       "  South Korea"},            // 38
  {"ACST-9:30ACDT,M10.1.0/2:00:00,M4.1.0/3:00:00",          "  ACST",     "  +9:30",    "  Adelaide"},               // 39
  {"CST-9:30",                                              "  CST",      "  +9:30",    "  Northern Territory"},     // 40
  {"AEST-10AEDT,M10.1.0/2:00:00,M4.1.0/3:00:00",            "  AEST",     "  +10",      "  Sydney"},                 // 41
  {"CHUT-10",                                               "  CHUT",     "  +10",      "  Chuuk"},                  // 42
  {"WIT-9",                                                 "  WIT",      "  +9",       "  Indonesia East"},         // 43
  {"SBT-11",                                                "  SBT",      "  +11",      "  Solomon Islands"},        // 44
  {"NCT-11",                                                "  NCT",      "  +11",      "  New Caledonia"},          // 45
  {"NZST-12NZDT,M9.5.0/2:00:00,M4.1.0/3:00:00",             "  NZST",     "  +12",      "  New Zealand"},            // 46
  {"FJT-12FJDT,M11.1.0/2:00:00,M1.2.0/3:00:00",             "  FJT",      "  +12",      "  Fiji"},                   // 47
  {"TOT-13",                                                "  TOT",      "  +13",      "  Tonga"},                  // 48
  {"PHOT-13",                                               "  PHOT",     "  +13",      "  Phoenix Islands"},        // 49
  {"LINT-14",                                               "  LINT",     "  +14",      "  Line Islands"}            // 50
};


const size_t TZ_MENU_COUNT = sizeof(TZ_menu) / sizeof(TZ_menu[0]);

// --- REPLACEMENT FOR displayWrite AND displayScan ---
void renderDisplay(uint64_t* currentBuffer) {

  for (int i = 0; i < 12; i++) {
    spiWrite64(currentBuffer[i]);

    delayMicroseconds(300); 
  }

}

void WiFisetup(){
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  displayBuilder(" CONNECTING ", toDisplayWords, false);

  while (WiFi.status() != WL_CONNECTED) {

    for(int i = 0; i < 50; i++) {
        renderDisplay(toDisplayWords); 
    }
  }
  
  // Show "Connected" state briefly
  unsigned long start = millis();
  while(millis() - start < 1000) {
      renderDisplay(toDisplayWords);
  }
}

void idleBehavior(bool hour24) {
  struct tm timeinfo;
  if (getLocalTime(&timeinfo)) {
    displayBuilder((char*)formatTime(timeinfo, hour24).c_str(), toDisplayWords, navDelay());
  }
}

int timeLastPressed = millis();

bool encoderButton(int type) {
  if (type == 0) { // TOGGLE BUTTON BEHAVIOR
    if (digitalRead(PIN_ENCODER_PUSH) == LOW && (millis() - timeLastPressed >= 150)) {
      lastEncState = !lastEncState;
      timeLastPressed = millis();
      return lastEncState;
    } 
    return lastEncState;
  } else if (type == 1) { // MOMENTARY BUTTON BEHAVIOR
    do {
      return true;
    } while (digitalRead(PIN_ENCODER_PUSH) == LOW);
  }
  return false;
}

void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);


  pinMode(PIN_LATCH, OUTPUT);
  pinMode(PIN_OE, OUTPUT);
  digitalWrite(PIN_OE, LOW); 

  SPI.begin(PIN_SCK, -1, PIN_COPI, PIN_LATCH);

  initFontTable();

  WiFisetup();

  pinMode(TZDISPLAY, INPUT);
  pinMode(PIN_ENCODER_PUSH, INPUT_PULLUP);
  pinMode(PIN_ENCODER_A, INPUT_PULLUP);
  pinMode(PIN_ENCODER_B, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PIN_ENCODER_A), readEncoder, FALLING);

}

void loop() {
  unsigned long now = millis();
  bool buttonPressed = (digitalRead(PIN_ENCODER_PUSH) == LOW);

  if (encoderMoved) {
    menuTimeout = now;
    if (currentState == CLOCK_HOME) { currentState = SETTINGS_NAV; menuIndex = 0; }
    encoderMoved = false;
  }

  static unsigned long lastLogic = 0;
  if (now - lastLogic > 50) {
    lastLogic = now;

    if (currentState != CLOCK_HOME && (now - menuTimeout > 10000)) currentState = CLOCK_HOME;

    switch (currentState) {
      case CLOCK_HOME:

        idleBehavior(lastEncState); 
        if (buttonPressed && (now - timeLastPressed > 250)) {
          lastEncState = !lastEncState;
          timeLastPressed = now;
        }
        break;

      case SETTINGS_NAV:
        displayBuilder(" TIME ZONE  ", toDisplayWords, true);
        if (buttonPressed && (now - timeLastPressed > 250)) {
          currentState = TZ_SELECT;
          menuIndex = 9; 
          timeLastPressed = now;
          menuTimeout = now;
        }
        break;

      case TZ_SELECT:
        if (menuIndex < 0) menuIndex = TZ_MENU_COUNT - 1;
        if (menuIndex >= (int)TZ_MENU_COUNT) menuIndex = 0;
        displayBuilder(TZ_menu[menuIndex].tz, toDisplayWords, true);
        
        if (buttonPressed && (now - timeLastPressed > 250)) {
          initTime(TZ_menu[menuIndex].posix); 
          currentState = CLOCK_HOME;          
          timeLastPressed = now;
        }
        break;
    }
  }

  renderDisplay(toDisplayWords);
}