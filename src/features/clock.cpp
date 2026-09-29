#include "clock.h"
#include <Arduino.h>
#include <time.h>
#include "time_util.h"
#include "timezone.h"
#include <sys/time.h>

extern TimeUtil timeUtil;
extern volatile bool globalWiFiConnected;

Clock::Clock() {
    page = 0;
    hour24 = false;
    nav = false;
    
    focusStart = 0;
    focusRemain = 1500000;
    focusElapsed = 0;
    focusMillis = 0;
    running = false;
    paused = false;

    latch = false;
    editMode = false;

    lastRTCSync = 0;
    rtcInitialized = false;
}

void Clock::begin() {
    if (RTC.begin()) {
        rtcInitialized = true;
        RTC.deviceStart(); 

        DateTime rtcNow = RTC.now();
        if (rtcNow.year() >= 2024) {
            struct timeval tv;
            tv.tv_sec = rtcNow.unixtime();
            tv.tv_usec = 0;
            settimeofday(&tv, NULL);
            Serial.println("DBG 062 CLOCK: Loaded time from RTC on boot");
        } else {
            Serial.println("DBG 063 CLOCK: RTC time invalid, waiting for NTP");
        }
    } else {
        Serial.println("DBG 064 CLOCK: RTC not found on I2C bus");
    }
}

void Clock::onButtonPress() {
    if (page == 0) {
        hour24 = !hour24;
    } else if (page == 1) {
        if (running) {
            focusRemain -= (millis() - focusStart);
            running = false;
        } else {
            if (focusRemain > 0) {
                focusStart = millis();
                running = true;
            }
        }
    }
}

void Clock::onModButtonPress() {
    if (page == 0) {
        page = 1;
    } else if (page == 1) {
        running = false;
        focusRemain = 1500000; 
        focusStart = 0;
        editMode = false;
    }
}

void Clock::onHomeButtonPress() {
    if (page == 1) {
        editMode = false; 
        page = 0;         
    }
}

String Clock::getClockDisplay() {
    if (page == 0) {
        time_t now;
        time(&now);
        
        struct tm ti;
        localtime_r(&now, &ti); 
        
        if (rtcInitialized) {
            unsigned long currentMillis = millis();
            if (currentMillis - lastRTCSync >= 300000) { // 300,000 ms = 5 minutes
                lastRTCSync = currentMillis;

                if (globalWiFiConnected && ti.tm_year >= 116) {
                    // Wi-Fi is active and NTP is synced: Update RTC from ESP32
                    // extract the UTC time so timezones don't corrupt the RTC
                    struct tm *utc_ti = gmtime(&now);
                    RTC.adjust(DateTime(utc_ti->tm_year + 1900, utc_ti->tm_mon + 1, utc_ti->tm_mday, utc_ti->tm_hour, utc_ti->tm_min, utc_ti->tm_sec));
                    Serial.println("DBG 065 CLOCK: Synced hardware RTC to NTP time");
                } 
                else if (!globalWiFiConnected) {
                    DateTime rtcNow = RTC.now();
                    if (rtcNow.year() >= 2024) {
                        struct timeval tv;
                        tv.tv_sec = rtcNow.unixtime();
                        tv.tv_usec = 0;
                        settimeofday(&tv, NULL);
                        Serial.println("DBG 066 CLOCK: Corrected ESP32 clock from RTC");
                    }
                }
            }
        }
        
        return timeUtil.formatTime(ti, hour24);
        
    } else if (page == 1) {
        long currentRemain = focusRemain;

        if (running) {
            long elapsed = millis() - focusStart;
            currentRemain -= elapsed;
            
            if (currentRemain <= 0) {
                currentRemain = 0;
                running = false;
                focusRemain = 0;
            }
        }

        unsigned long totalSeconds = currentRemain / 1000;
    
        int SWhours   = totalSeconds / 3600;
        int SWminutes = (totalSeconds / 60) % 60;
        int SWseconds = totalSeconds % 60;
        int SWmillis  = currentRemain % 1000;

        char stopwatchBuffer[20]; 
        snprintf(stopwatchBuffer, sizeof(stopwatchBuffer), 
            " %02d:%02d:%02d:%03d", 
            SWhours, 
            SWminutes, 
            SWseconds, 
            SWmillis
        );

        return String(stopwatchBuffer); 
   }
   return "";
}