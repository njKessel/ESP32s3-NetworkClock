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
    focusRemain = 0;
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
        paused = !paused;
    }
}

void Clock::onModButtonPress() {
    if (page == 0) {
        page = 1;
        if (!running && !paused) {
            running = true;
        }
    } else if (page == 1) {
        if (running) {
            running = false;
        }
        if (paused) {
            paused = false;
        }
        editMode = true;
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
        String(clockBuffer) = timeUtil.formatTime(ti, hour24);
        return String(clockBuffer);
    } else if (page == 1) {
        unsigned long long currentDuration = focusElapsed;

        if (running) {
            currentDuration += (millis() - focusStart);
        }

        unsigned long totalSeconds = currentDuration / 1000;
    
        int SWhours   = totalSeconds / 3600;
        int SWminutes = (totalSeconds / 60) % 60;
        int SWseconds = totalSeconds % 60;
        int SWmillis  = currentDuration % 1000;

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