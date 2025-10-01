#include <Arduino.h>
#include <LiquidCrystal.h>
#include "secrets.h"
#include <WiFi.h>
#include "time.h"

const int rs = 15, en = 17, d7 = 12, d6 = 11, d5 = 10, d4 = 9;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

void initTime(const char* tz) {
  configTzTime(tz, "pool.ntp.org");
  tm ti; while (!getLocalTime(&ti)) { delay(200); }
}

String formatTime(const tm& ti) {
  char buf[9];                       // "HH:MM:SS" + NUL
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
  struct tm tm;

  tm.tm_year = yr - 1900;
  tm.tm_mon = month-1;
  tm.tm_mday = mday;
  tm.tm_hour = hr;
  tm.tm_min = minute;
  tm.tm_sec = sec;
  tm.tm_isdst = isDst;
  time_t t = mktime(&tm);
  struct timeval now = { .tv_sec = t};
  settimeofday(&now, NULL);

}

void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  WiFisetup();
  initTime("EST5EDT,M3.2.0/2,M11.1.0/2");

  lcd.begin(16,2);
  lcd.print("int...");
  delay(200);


}

void loop() {
  lcd.setCursor(0,0);
  tm timeinfo;
  if (getLocalTime(&timeinfo)) {
    lcd.setCursor(0, 0);
    lcd.print(formatTime(timeinfo));
  }
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
