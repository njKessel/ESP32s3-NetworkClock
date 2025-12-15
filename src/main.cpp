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

const uint8_t I2C_Address = 0x70; // Scan agrees 10.26
const int TZDISPLAY = 21; // GPIO PIN 21
int TZDISPLAY_status = 0;

// DISPLAY PINS
constexpr int PIN_COPI  = 11;  // SERIAL INPUT
constexpr int PIN_LATCH = 40;  // RCLK
constexpr int PIN_OE    = 4;   // BRIGHTNESS (PWM)
constexpr int PIN_SCK   = 12;  // SCK

SPISettings srSettings(10000000, MSBFIRST, SPI_MODE0);

static void latchPulse() {
  digitalWrite(PIN_LATCH, HIGH);
  digitalWrite(PIN_LATCH, LOW);
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
  char buf[9];  // "HH:MM:SS" + NUL
  strftime(buf, sizeof(buf), "%H:%M:%S", &ti);
  return String(buf);
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

void displayInitialize(){
  Wire.begin(4, 5); // keep I2C active for manual writes later
  Wire.beginTransmission(I2C_Address);
    Wire.write(0x21); // Oscillator on
    Wire.endTransmission();
  Wire.beginTransmission(I2C_Address);
    Wire.write(0x81); // Display on
    Wire.endTransmission();
}

void displayClean(){
  for (int positionWiper = 0x00; positionWiper <= 0x0E; positionWiper+=2) {
    Wire.beginTransmission(I2C_Address);
      Wire.write(positionWiper);
      Wire.write(0x00);
      Wire.endTransmission();
  }
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


enum anodeBit : uint32_t {
  ANODE0 = 1u << 25,  // 74HC595 U4 pin ANODE1 on 12C-LTP-587HR_rev1
  ANODE1 = 1u << 26,
  ANODE2 = 1u << 27,
  ANODE3 = 1u << 28,
  ANODE4 = 1u << 29,
  ANODE5 = 1u << 30,
  ANODE6 = 1u << 31,
  ANODE7 = 1u << 32,
  ANODE8 = 1u << 33,
  ANODE9 = 1u << 34,
  ANODE10 = 1u << 35,
  ANODE11 = 1u << 26, // 74HC595 U5 pin ANODE12 on 12C-LTP-587HR_rev1
};

void displayScan(uint32_t displayCodes[]) {
  for (int digit = 0; digit < 12, digit++;) {
    switch(uint32_t digit) {
      case 0:
        displayCodes[0] | ANODE0;
        break;
      case 1:
        displayCodes[1] | ANODE1;
        break;
      case 2:
        displayCodes[2] | ANODE2;
        break;
      case 3:
        displayCodes[3] | ANODE3;
        break;
      case 4:
        displayCodes[4] | ANODE4;
        break;
      case 5:
        displayCodes[5] | ANODE5;
        break;
      case 6:
        displayCodes[6] | ANODE6;
        break;
      case 7:
        displayCodes[7] | ANODE7;
        break;
      case 8:
        displayCodes[8] | ANODE8;
        break;
      case 9:
        displayCodes[9] | ANODE9;
        break;
      case 10:
        displayCodes[10] | ANODE10;
        break;
      case 11:
        displayCodes[11] | ANODE11;
        break;
    }
  }
}

void displayTime(char tbuf[]) { // Displays the time as given by tbuf to the display
  bool decimalCarry = 0;
  int displayIndexTracer = strlen(tbuf) - 1;
  for(int displayIterator = 7; displayIterator >= 0; displayIterator--) {
    if (tbuf[displayIterator] == ':') {
            decimalCarry = 1;
    } else {
      Wire.beginTransmission(I2C_Address);
      if (decimalCarry == 0) {
        Wire.write(displayIndex[displayIndexTracer]);
        Wire.write(displayPattern[tbuf[displayIterator] - '0']);
        Wire.endTransmission();
      } else {
        Wire.write(displayIndex[displayIndexTracer]);
        Wire.write(addDecimal(displayPattern[tbuf[displayIterator] - '0']));
        Wire.endTransmission();
      }
      decimalCarry = 0;
      if (displayIndexTracer != 0) {
        displayIndexTracer--;
      } else {
        Serial.println("Index Tracer error in displayTime code 0x0002");
      }
    }
  }
}

void displayTZ() {
  Wire.beginTransmission(I2C_Address);
    Wire.write(displayIndex[2]);
    Wire.write(0x00);
    Wire.write(displayIndex[3]);
    Wire.write(0x00);
    Wire.write(displayIndex[4]);
    Wire.write(0x00);
    Wire.write(displayIndex[5]);
    Wire.write(displayPattern[14]);
    Wire.write(displayIndex[6]);
    Wire.write(displayPattern[28]);
    Wire.write(displayIndex[7]);
    Wire.write(displayPattern[29]);
    Wire.endTransmission();
}

void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  WiFisetup();
  initTime(TZ_menu[9, 0]);

  delay(200);

  displayInitialize();
  
  // Clear Display
  displayClean();

  pinMode(21, INPUT);
}

void loop() {
  TZDISPLAY_status = digitalRead(21);

  struct tm ti;
  char tbuf[9];
  if (getLocalTime(&ti)) {
    strftime(tbuf, sizeof(tbuf), "%H:%M:%S", &ti); // FORMATS TIME AS HH:MM:SS

    char zbuf[17];
    strftime(zbuf, sizeof(zbuf), "%Z %z", &ti);

    int len = strlen(zbuf);
    (void)len;
  }
  if (TZDISPLAY_status == HIGH) {
    displayTZ();
  } else {
    displayTime(tbuf);
  }
 
  delay(500);
}
