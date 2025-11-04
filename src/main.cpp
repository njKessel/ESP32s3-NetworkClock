#include <Arduino.h> // Core types and functions for arduino 
#include "secrets.h" // WiFi Cred, change to secrets.example.h
#include <WiFi.h>    // Router Connection
#include "time.h"    // POSIX time + SNTP
#include <Wire.h>

const uint8_t I2C_Address = 0x70; // Scan agrees 10.26

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
  0x67  // 9
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

void displayTime(char tbuf[]) {
  bool decimalCarry = 1;
  int displayIndexTracer = strlen(tbuf);
  for(int displayIterator = strlen(tbuf); displayIterator >= 0; displayIterator--;) {
    Wire.beginTransmission(I2C_Address);
    if (tbuf[displayIterator] == ':') {
            decimalCarry = 1;
            displayIndexTracer--;
            Wire.endTransmission();
    } else {
      if (decimalCarry == 0) {
        Wire.write(displayIndex[displayIndexTracer]);
        Wire.write(displayPattern[tbuf[displayIterator]]);
        Wire.endTransmission();
        
      } else {
        Wire.write(displayIndex[displayIndexTracer]);
        Wire.write(addDecimal(displayPattern(tbuf[displayIterator])));
        Wire.endTransmission();
        displayIndexTracer--;
        decimalCarry
      }
      displayIndexTracer--;
      
    }
  }
}


void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  WiFisetup();
  initTime("EST5EDT,M3.2.0/2,M11.1.0/2");

  delay(200);

  displayInitialize();
  
  // Clear Display
  displayClean();

  
}

void loop() {
  struct tm ti;
  if (getLocalTime(&ti)) {
    char tbuf[9];
    strftime(tbuf, sizeof(tbuf), "%H:%M:%S", &ti); // FORMATS TIME AS HH:MM:SS

    char zbuf[17];
    strftime(zbuf, sizeof(zbuf), "%Z %z", &ti);

    int len = strlen(zbuf);
    (void)len;
  }
  displayTime(tbuf[]);
  delay(500);
}
