#include "clock.h"
#include <Arduino.h>
#include <time.h>
#include "time_util.h"
#include "timezone.h"

extern TimeUtil timeUtil;

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
        struct tm ti;
        if (!getLocalTime(&ti)) {
            return "Time Error"; 
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