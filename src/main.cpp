#include <Arduino.h> // Core types and functions for arduino 
#include <LiquidCrystal.h> // lcd driver
#include "secrets.h" // WiFi Cred
#include <WiFi.h> // Router Connection
#include "time.h" // POSIX for handling time locally, SNTP for acquiring UTC time from the internet

// PIN MAPPING
const int rs = 15, en = 17, d7 = 12, d6 = 11, d5 = 10, d4 = 9; 
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

// SNTP sync
void initTime(const char* tz) {
  tm ti{}; // Zero initialization 
  configTzTime(tz, "pool.ntp.org"); // Sets timezone and starts SNTP

  const int maxAttempts = 150; // 150 * 200ms is 30s max wait
  int attempts = 0;
  while (!getLocalTime(&ti) && (attempts < maxAttempts)) {
    attempts++;
    delay(200); } // Waits for valid time
}

String formatTime(const tm& ti) {
  char buf[9];  // "HH:MM:SS" + empty
  strftime(buf, sizeof(buf), "%H:%M:%S", &ti);
  return String(buf);
}


void WiFisetup(){
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while ( WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
}

void setTimeFF(int yr, int month, int mday, int hr, int minute, int sec, int isDst){
  struct tm tmd{};

  tmd.tm_year = yr - 1900;    // Years since 1900
  tmd.tm_mon = month-1;
  tmd.tm_mday = mday;
  tmd.tm_hour = hr;
  tmd.tm_min = minute;
  tmd.tm_sec = sec;
  tmd.tm_isdst = isDst;
  time_t t = mktime(&tmd);
  struct timeval now = { .tv_sec = t};
  settimeofday(&now, NULL);

}

void setup() {
  lcd.begin(16,2);
  delay(50);

  Serial.begin(115200);
  Serial.setDebugOutput(true);
  WiFisetup();
  
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Syncing Time...");
  initTime("EST5EDT,M3.2.0/2,M11.1.0/2");

  lcd.clear();
  lcd.print("int...");
  delay(200);


}

void loop() {
  lcd.setCursor(0,0);
  struct tm ti;
  if (getLocalTime(&ti)) {
    char tbuf[9];
    strftime(tbuf, sizeof(tbuf), "%H:%M:%S", &ti);
    lcd.setCursor(0, 0);
    lcd.print(tbuf);

    char zbuf[17];
    strftime(zbuf, sizeof(zbuf), "%Z %z", &ti);
    lcd.setCursor(0, 1);
    lcd.print(zbuf);

    int len = strlen(zbuf);
    for (int i = len; i < 16; ++i) lcd.print(' ');
  }
  delay(1000);
}
