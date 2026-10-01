//////////////////////////////////////////////////////////////
// INCLUDES  /////////////////////////////////////////////////
//////////////////////////////////////////////////////////////
#include "clock.h"                                          // Clock header
#include <Arduino.h>                                        // Base arduino functions
#include <time.h>                                           // Arduino time functions
#include "time_util.h"                                      // My time formatting header
#include "timezone.h"                                       // Timezone selection and handler
#include <sys/time.h>                                       // UNIX/POSIX handling

extern TimeUtil timeUtil;
extern volatile bool globalWiFiConnected;                   // Is wifi connected?

// Initialize  ///////////////////////////////////////////////
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
        if (rtcNow.year() >= 2024) {                        // Check if the RTC is accurate, if so sync to it
            struct timeval tv;
            tv.tv_sec = rtcNow.unixtime();
            tv.tv_usec = 0;
            settimeofday(&tv, NULL);
            Serial.println("DBG 062 CLOCK: Loaded time from RTC on boot");
        } else {                                            // If not just wait for NTP
            Serial.println("DBG 063 CLOCK: RTC time invalid, waiting for NTP");
        }
    } else {                                                // Fail status
        Serial.println("DBG 064 CLOCK: RTC not found on I2C bus");
    }
}
//////////////////////////////////////////////////////////////
// INPUTS ////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////

// Encoder Button ////////////////////////////////////////////
void Clock::onButtonPress() {
    if (page == 0) {                                        // Toggle 24hr and 12hr time
        hour24 = !hour24;
    } else if (page == 1) {
        if (running) {                                      // Toggle if the focus timer is running
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

// Modifier Button ///////////////////////////////////////////
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

// Home button ///////////////////////////////////////////
void Clock::onHomeButtonPress() {
    if (page == 1) {
        editMode = false;
        page = 0;         
    }
}

//////////////////////////////////////////////////////////////
// DISPLAY ///////////////////////////////////////////////////
//////////////////////////////////////////////////////////////
String Clock::getClockDisplay() {
    if (page == 0) {
        time_t now;
        time(&now);
        
        struct tm ti;
        localtime_r(&now, &ti); 
        
        if (rtcInitialized) {
            static bool initialSyncDone = false;
            unsigned long currentMillis = millis();
            
                                                            // Sync immediately once NTP is valid, then only every 12 hours (43,200,000 ms)
            if (!initialSyncDone || (currentMillis - lastRTCSync >= 43200000)) {    
                
                if (globalWiFiConnected && ti.tm_year >= 116) {
                    lastRTCSync = currentMillis;
                    initialSyncDone = true;                 // Lock out the rapid syncing
                    
                                                            // extract the UTC time so timezones don't corrupt the RTC
                    struct tm *utc_ti = gmtime(&now);
                    RTC.adjust(DateTime(utc_ti->tm_year + 1900, utc_ti->tm_mon + 1, utc_ti->tm_mday, utc_ti->tm_hour, utc_ti->tm_min, utc_ti->tm_sec));
                    Serial.println("DBG 065 CLOCK: Synced hardware RTC to NTP time");
                } 
                else if (!globalWiFiConnected && !initialSyncDone) {
                                                            // If there is no wifi on boot, correct to the RTC time
                    DateTime rtcNow = RTC.now();
                    if (rtcNow.year() >= 2024) {
                        struct timeval tv;
                        tv.tv_sec = rtcNow.unixtime();
                        tv.tv_usec = 0;
                        settimeofday(&tv, NULL);
                        
                        lastRTCSync = currentMillis;
                        initialSyncDone = true; 
                        Serial.println("DBG 066 CLOCK: Corrected ESP32 clock from RTC");
                    }
                }
            }
        }
        
        return timeUtil.formatTime(ti, hour24);
        
    } else if (page == 1) {                                 // Focus timer screen
        long currentRemain = focusRemain;

        if (running) {                                      // If its running show remaining time
            long elapsed = millis() - focusStart;
            currentRemain -= elapsed;
            
            if (currentRemain <= 0) {
                currentRemain = 0;
                running = false;
                focusRemain = 0;
            }
        }

        unsigned long totalSeconds = currentRemain / 1000;  // Calculate remaining time
    
        int SWhours   = totalSeconds / 3600;
        int SWminutes = (totalSeconds / 60) % 60;
        int SWseconds = totalSeconds % 60;
        int SWmillis  = currentRemain % 1000;

        char stopwatchBuffer[20]; 
        snprintf(stopwatchBuffer, sizeof(stopwatchBuffer),  // Build output
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

//////////////////////////////////////////////////////////////
// MANUAL TIME SET ///////////////////////////////////////////
//////////////////////////////////////////////////////////////
void Clock::setManualTime(int year, int month, int day, int hour, int minute) {
    struct tm ti;
    ti.tm_year = year - 1900;                               // Correct offset since tm expects years since 1900
    ti.tm_mon = month - 1;                                  // Correct for 0-11 index
    ti.tm_mday = day;
    ti.tm_hour = hour;
    ti.tm_min = minute;
    ti.tm_sec = 0;
    ti.tm_isdst = -1;

    time_t t = mktime(&ti);
    struct timeval tv;
    tv.tv_sec = t;
    tv.tv_usec = 0;
    settimeofday(&tv, NULL);

    if (rtcInitialized) {
        struct tm *utc_ti = gmtime(&t);
        RTC.adjust(DateTime(utc_ti->tm_year + 1900, utc_ti->tm_mon + 1, utc_ti->tm_mday, utc_ti->tm_hour, utc_ti->tm_min, utc_ti->tm_sec));
        Serial.println("DBG 067 CLOCK: Manual time saved to RTC");
    }
}