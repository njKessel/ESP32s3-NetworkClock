/*
ESP32 NETWORK CLOCK             NATHANIEL KESSEL
WORK IN PROGRESS                        ESP32-S3
https://github.com/njKessel/ESP32s3-NetworkClock
*/

#include <Arduino.h>                // Core types and functions for arduino 
#include <WiFi.h>                   // Router Connection
#include <SPI.h>                    // SPI
#include <Wire.h>                   // I2C
#include <time.h>                   // POSIX time + SNTP

#include "secrets.h"                // WiFi Cred, change to secrets.example.h
#include "display_font.h"           // Alphanumeric font

uint64_t displayWords[12];
const char* displayStr = "   12.34.AB   ";
unsigned long lastUpdate = 0;

const uint8_t I2C_Address = 0x70; // Scan agrees 10.26
const int TZDISPLAY = 21; // GPIO PIN 21
int TZDISPLAY_status = 0;

// DISPLAY PINS
constexpr int PIN_COPI  = 13;  // SERIAL INPUT
constexpr int PIN_LATCH = 40;  // RCLK
constexpr int PIN_OE    = 4;   // BRIGHTNESS (PWM)
constexpr int PIN_SCK   = 12;  // SCK

SPISettings srSettings(1000000, MSBFIRST, SPI_MODE0);

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
  tm ti{}; // Zero initialization 
  configTzTime(tz, "pool.ntp.org"); // Sets timezone and starts SNTP

  const int maxAttempts = 150; // 150 * 200ms is 30s max wait
  int attempts = 0;
  while (!getLocalTime(&ti) && (attempts < maxAttempts)) {
    attempts++;
    delay(200); // Waits for valid time
  }
}

String formatTime(const tm& ti) {
  char buf[14];  // "HH:MM:SS" + NUL
  strftime(buf, sizeof(buf), "<<<%H:%M:%S>>>", &ti);
  return String(buf);
}

uint64_t toDisplayWords[12];

void timeSpace(char formattedTime[], char type) {
  int dI = 0;
  int len = strlen(formattedTime);

  for (int i = 0; i < 12; i++) toDisplayWords[i] = 0;

  if (type == 't') {
    for (int i = 0; i < len && dI < 12; i++) {
      
      if (i + 1 < len && formattedTime[i + 1] == ':') {
        toDisplayWords[dI] = getSegmentPattern(formattedTime[i], true);
        i++;
      } else {
        toDisplayWords[dI] = getSegmentPattern(formattedTime[i], false);
      }
      dI++;
    }
  }
}

void WiFisetup(){
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
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
  {"AOE12", "AOE", "-12", "Baker Island"},                                              // 0
  {"NUT11", "NUT", "-11", "American Samoa"},                                            // 1

  // Hawaii does NOT use DST; POSIX offset for UTC-10 is "HST10"
  {"HST10", "HST", "-10", "Hawaii"},                                                    // 2

  // Marquesas Islands (UTC-9:30) do NOT use DST
  {"MART9:30", "MART", "-9:30", "French Polynesia"},                                    // 3

  {"AKST9AKDT,M3.2.0/2:00:00,M11.1.0/2:00:00", "AKST", "-9", "Alaska"},                 // 4
  {"PST8PDT,M3.2.0/2:00:00,M11.1.0/2:00:00", "PDT", "-8", "Los Angeles"},              // 5
  {"MST7MDT,M3.2.0/2:00:00,M11.1.0/2:00:00", "MST", "-7", "Denver"},                   // 6
  {"MST7", "MST", "-7", "Phoenix"},                                                    // 7
  {"CST6CDT,M3.2.0/2:00:00,M11.1.0/2:00:00", "CST", "-6", "Chicago"},                  // 8
  {"EST5EDT,M3.2.0/2,M11.1.0/2", "EST", "-5", "New York"},                             // 9

  {"AST4ADT,M3.2.0/2:00:00,M11.1.0/2:00:00", "AST", "-4", "Halifax"},                  // 10
  {"BRT3", "BRT", "-3", "São Paulo"},                                                  // 11
  {"FKST3FKDT,M9.1.0/2:00:00,M4.3.0/2:00:00", "FKST", "-3", "Falkland Islands"},       // 12
  {"GRNL2", "GRL", "-2", "South Georgia"},                                             // 13

  {"AZOT1AZOST,M3.5.0/0:00:00,M10.5.0/1:00:00", "AZOT", "-1", "Azores"},               // 14
  {"GMT0", "GMT", "0", "London"},                                                      // 15
  {"GMT0BST,M3.5.0/1:00:00,M10.5.0/2:00:00", "BST", "+1", "United Kingdom"},           // 16
  {"CET-1CEST,M3.5.0/2:00:00,M10.5.0/3:00:00", "CET", "+1", "Berlin"},                 // 17
  {"WAT-1", "WAT", "+1", "Nigeria"},                                                   // 18
  {"EET-2EEST,M3.5.0/3:00:00,M10.5.0/4:00:00", "EET", "+2", "Athens"},                 // 19

  {"CAT-2", "CAT", "+2", "South Africa"},                                              // 20
  {"MSK-3", "MSK", "+3", "Moscow"},                                                    // 21
  {"AST-3", "AST", "+3", "Saudi Arabia"},                                              // 22

  {"GST-4", "GST", "+4", "Dubai"},                                                     // 23
  {"AZT-4AZST,M3.5.0/4:00:00,M10.5.0/5:00:00", "AZT", "+4", "Azerbaijan"},             // 24
  {"AFT-4:30", "AFT", "+4:30", "Afghanistan"},                                         // 25
  {"PKT-5", "PKT", "+5", "Pakistan"},                                                  // 26
  {"IST-5:30", "IST", "+5:30", "India"},                                               // 27
  {"NPT-5:45", "NPT", "+5:45", "Nepal"},                                               // 28

  {"BST-6", "BST", "+6", "Bangladesh"},                                                // 29
  {"MMT-6:30", "MMT", "+6:30", "Myanmar"},                                             // 30
  {"ICT-7", "ICT", "+7", "Thailand"},                                                  // 31
  {"WIB-7", "WIB", "+7", "Indonesia West"},                                            // 32

  {"CST-8", "CST", "+8", "China"},                                                     // 33
  {"AWST-8", "AWST", "+8", "Perth"},                                                   // 34
  {"HKT-8", "HKT", "+8", "Hong Kong"},                                                 // 35
  {"WITA-8", "WITA", "+8", "Indonesia Central"},                                       // 36

  {"JST-9", "JST", "+9", "Japan"},                                                     // 37
  {"KST-9", "KST", "+9", "South Korea"},                                               // 38
  {"ACST-9:30ACDT,M10.1.0/2:00:00,M4.1.0/3:00:00", "ACST", "+9:30", "Adelaide"},       // 39
  {"CST-9:30", "CST", "+9:30", "Northern Territory"},                                  // 40

  {"AEST-10AEDT,M10.1.0/2:00:00,M4.1.0/3:00:00", "AEST", "+10", "Sydney"},             // 41
  {"CHUT-10", "CHUT", "+10", "Chuuk"},                                                 // 42
  {"WIT-9", "WIT", "+9", "Indonesia East"},                                            // 43

  {"SBT-11", "SBT", "+11", "Solomon Islands"},                                         // 44
  {"NCT-11", "NCT", "+11", "New Caledonia"},                                           // 45
  {"NZST-12NZDT,M9.5.0/2:00:00,M4.1.0/3:00:00", "NZST", "+12", "New Zealand"},         // 46
  {"FJT-12FJDT,M11.1.0/2:00:00,M1.2.0/3:00:00", "FJT", "+12", "Fiji"},                 // 47

  {"TOT-13", "TOT", "+13", "Tonga"},                                                   // 48
  {"PHOT-13", "PHOT", "+13", "Phoenix Islands"},                                       // 49
  {"LINT-14", "LINT", "+14", "Line Islands"}                                           // 50
};

const size_t TZ_MENU_COUNT = sizeof(TZ_menu) / sizeof(TZ_menu[0]);

void displayScan(uint64_t displayWords[12]) {
    for (int i = 0; i < 12; i++) {
        spiWrite64(displayWords[i]);
        latchPulse();
    }
}

void displayWrite(uint64_t displayWords[12], unsigned long &lastUpdate, int refreshDelay = 2) {
    static int currentDigit = 0;
    if (millis() - lastUpdate >= refreshDelay) {
        spiWrite64(displayWords[currentDigit]);
        currentDigit = (currentDigit + 1) % 12;
        lastUpdate = millis();
    }
}



void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  WiFisetup();

  SPI.begin(PIN_SCK, -1, PIN_COPI, PIN_LATCH);
  pinMode(PIN_LATCH, OUTPUT);
  pinMode(PIN_OE, OUTPUT);
  digitalWrite(PIN_OE, LOW);

  delay(200);
  pinMode(TZDISPLAY, INPUT);

  initFontTable();
  initTime("EST5EDT");
}

void loop() {
  struct tm timeinfo;
  if (getLocalTime(&timeinfo)) {
    char timeStr[16];

    strftime(timeStr, sizeof(timeStr), "%H:%M:%S", &timeinfo); 
    
    displayBuilder(timeStr, toDisplayWords);
  }

  displayWrite(toDisplayWords, lastUpdate, 1); 
}
  
