#include <Arduino.h> // Core types and functions for arduino 
#include "secrets.h" // WiFi Cred, change to secrets.example.h
#include <WiFi.h>    // Router Connection
#include "time.h"    // POSIX time + SNTP
#include <Wire.h>

const uint8_t I2C_Address = 0x70; // Scan agrees 10.26
const int TZDISPLAY = 21; // GPIO PIN 21
int TZDISPLAY_status = 0;

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

int addDecimal(int numberVal) {
  int increasedNumVal = numberVal | 0x80;
  return increasedNumVal;
}

const int displayPattern[] = {
  0x3F, // 0
  0x06, // 1
  0x5B, // 2
  0x4F, // 3
  0x66, // 4
  0x6D, // 5
  0x7d, // 6
  0x07, // 7
  0x7f, // 8 
  0x67, // 9
  0x77, // A
  0x7C, // B
  0x39, // C
  0x5E, // D
  0x79, // E
  0x71, // F
  0x39, // G
  0x74, // H
  0x05, // I
  0x0D, // J
  0x78, // K
  0x38, // L
  0xD4, // M
  0x54, // N
  0x3F, // O
  0x73, // P
  0x67, // Q
  0x7B, // R
  0x6D, // S
  0x78, // T
  0x1C, // U
  0x3E, // V
  0x9C, // W
  0x76, // X
  0x66, // Y
  0x5B  // Z
};

const int displayIndex[] = { // Simple list of the hex values for transmission that correspond to digits
  0x00, // CHAR 0
  0x02, // CHAR 1
  0x04, // CHAR 2
  0x06, // CHAR 3
  0x08, // CHAR 4
  0x0A, // CHAR 5
  0x0C, // CHAR 6
  0x0E  // CHAR 7
};

const String TZ_menu[] = {                                    // TIME ZONE POSIX STRINGS, [POSIX, TIME ZONE, OFFSET, CITY]
  ["AOE12", "AOE", "-12", "Baker Island"],                                              // 0 AOE
  ["NUT11", "NUT", "-11", "American Samoa"],                                            // 1 NUT
  ["HST11HDT,M3.2.0/2:00:00,M11.1.0/2:00:00", "HST", "-10", "Hawaii"],                  // 2 HST
  ["MART9:30,M3.2.0/2:00:00,M11.1.0/2:00:00", "MART", "-9:30", "French Polynesia"],     // 3 MART

  ["AKST9AKDT,M3.2.0/2:00:00,M11.1.0/2:00:00", "AKST", "-9", "Alaska"],                 // 4 AKST
  ["PST8PDT,M3.2.0/2:00:00,M11.1.0/2:00:00", "PDT", "-8", "Los Angeles"],               // 5 PDT
  ["MST7MDT,M3.2.0/2:00:00,M11.1.0/2:00:00", "MST", "-7", "Denver"],                    // 6 MST/DST
  ["MST7", "MST", "-7", "Phoenix"],                                                     // 7 MST
  ["CST6CDT,M3.2.0/2:00:00,M11.1.0/2:00:00", "CST", "-6", "Chicago"],                   // 8 CST
  ["EST5EDT,M3.2.0/2,M11.1.0/2", "EST", "-5", "New York"],                              // 9 EST

  ["AST4ADT,M3.2.0/2:00:00,M11.1.0/2:00:00", "AST", "-4", "Halifax"],                   // 10 AST
  ["BRT3", "BRT", "-3", "São Paulo"],                                                   // 11 BRT
  ["FKST3FKDT,M9.1.0/2:00:00,M4.3.0/2:00:00", "FKST", "-3", "Falkland Islands"],        // 12 FKST
  ["GRNL2", "GRL", "-2", "South Georgia"],                                              // 13 GRL

  ["AZOT1AZOST,M3.5.0/0:00:00,M10.5.0/1:00:00", "AZOT", "-1", "Azores"],                // 14 AZOT
  ["GMT0", "GMT", "0", "London"],                                                       // 15 GMT
  ["GMT0BST,M3.5.0/1:00:00,M10.5.0/2:00:00", "BST", "+1", "United Kingdom"],            // 16 GMT/DST
  ["CET-1CEST,M3.5.0/2:00:00,M10.5.0/3:00:00", "CET", "+1", "Berlin"],                  // 17 CET
  ["WAT-1", "WAT", "+1", "Nigeria"],                                                    // 18 WAT
  ["EET-2EEST,M3.5.0/3:00:00,M10.5.0/4:00:00", "EET", "+2", "Athens"],   

  ["CAT-2", "CAT", "+2", "South Africa"],                                               // 19 CAT
  ["MSK-3", "MSK", "+3", "Moscow"],                                                     // 20 MSK
  ["AST-3", "AST", "+3", "Saudi Arabia"],                                               // 21 AST

  ["GST-4", "GST", "+4", "Dubai"],                                                      // 22 GST
  ["AZT-4AZST,M3.5.0/4:00:00,M10.5.0/5:00:00", "AZT", "+4", "Azerbaijan"],              // 23 AZT
  ["AFT-4:30", "AFT", "+4:30", "Afghanistan"],                                          // 24 AFT
  ["PKT-5", "PKT", "+5", "Pakistan"],                                                   // 25 PKT
  ["IST-5:30", "IST", "+5:30", "India"],                                                // 26 IST
  ["NPT-5:45", "NPT", "+5:45", "Nepal"],                                                // 27 NPT

  ["BST-6", "BST", "+6", "Bangladesh"],                                                 // 28 BST
  ["MMT-6:30", "MMT", "+6:30", "Myanmar"],                                              // 29 MMT
  ["ICT-7", "ICT", "+7", "Thailand"],                                                   // 30 ICT
  ["WIB-7", "WIB", "+7", "Indonesia West"],                                             // 31 WIB

  ["CST-8", "CST", "+8", "China"],                                                      // 32 CST
  ["AWST-8", "AWST", "+8", "Perth"],                                                    // 33 AWST
  ["HKT-8", "HKT", "+8", "Hong Kong"],                                                  // 34 HKT
  ["WITA-8", "WITA", "+8", "Indonesia Central"],                                        // 35 WITA

  ["JST-9", "JST", "+9", "Japan"],                                                      // 36 JST
  ["KST-9", "KST", "+9", "South Korea"],                                                // 37 KST
  ["ACST-9:30ACDT,M10.1.0/2:00:00,M4.1.0/3:00:00", "ACST", "+9:30", "Adelaide"],        // 38 ACST/ACDT
  ["CST-9:30", "CST", "+9:30", "Northern Territory"],                                   // 39 ACST

  ["AEST-10AEDT,M10.1.0/2:00:00,M4.1.0/3:00:00", "AEST", "+10", "Sydney"],              // 40 AEDT
  ["CHUT-10", "CHUT", "+10", "Chuuk"],                                                  // 41 CHUT
  ["WIT-9", "WIT", "+9", "Indonesia East"],                                             // 42 WIT

  ["SBT-11", "SBT", "+11", "Solomon Islands"],                                          // 43 SBT
  ["NCT-11", "NCT", "+11", "New Caledonia"],                                            // 44 NCT
  ["NZST-12NZDT,M9.5.0/2:00:00,M4.1.0/3:00:00", "NZST", "+12", "New Zealand"],          // 45 NZST
  ["FJT-12FJDT,M11.1.0/2:00:00,M1.2.0/3:00:00", "FJT", "+12", "Fiji"],                  // 46 FJT

  ["TOT-13", "TOT", "+13", "Tonga"],                                                    // 47 TOT
  ["PHOT-13", "PHOT", "+13", "Phoenix Islands"],                                        // 48 PHOT
  ["LINT-14", "LINT", "+14", "Line Islands"]                                            // 49 LINT
};

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
