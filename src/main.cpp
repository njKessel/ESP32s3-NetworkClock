#include <Arduino.h> // Core types and functions for arduino 
#include "secrets.h" // WiFi Cred
#include <WiFi.h>    // Router Connection
#include "time.h"    // POSIX time + SNTP
#include <Wire.h>

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

const uint8_t I2C_Address = 0x70; // Scan agrees 10.26

void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  WiFisetup();
  initTime("EST5EDT,M3.2.0/2,M11.1.0/2");

  delay(200);

  Wire.begin(4, 5); // keep I2C active for manual writes later
  Wire.beginTransmission(I2C_Address);
    Wire.write(0x21);
    Wire.endTransmission();
}

void loop() {
  

  struct tm ti;
  if (getLocalTime(&ti)) {
    char tbuf[9];
    strftime(tbuf, sizeof(tbuf), "%H:%M:%S", &ti);

    char zbuf[17];
    strftime(zbuf, sizeof(zbuf), "%Z %z", &ti);

    int len = strlen(zbuf);
    (void)len; // silence unused warning for now
  }
  delay(1000);
}
