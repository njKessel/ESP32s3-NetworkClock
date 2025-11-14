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
  initTime("EST5EDT,M3.2.0/2,M11.1.0/2");

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
