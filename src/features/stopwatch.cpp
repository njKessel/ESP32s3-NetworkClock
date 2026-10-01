//////////////////////////////////////////////////////////////
// INCLUDES  /////////////////////////////////////////////////
//////////////////////////////////////////////////////////////
#include "stopwatch.h"                                      // Stopwatch header

//////////////////////////////////////////////////////////////
// INITIALIZATION ////////////////////////////////////////////
//////////////////////////////////////////////////////////////
Stopwatch::Stopwatch() {
    running = false;
    accumulatedTime = 0;
    startTime = 0;
}

//////////////////////////////////////////////////////////////
// CORE LOGIC ////////////////////////////////////////////////
//////////////////////////////////////////////////////////////

// Start / Pause Toggle //////////////////////////////////////
void Stopwatch::toggle() {
    if (running) {
        accumulatedTime += (millis() - startTime);          // Pause: Bank the active time into accumulatedTime so we can resume later
        running = false;
    } else {
        startTime = millis();                               // Start/Resume: Drop an anchor at the current millis() tick
        running = true;
    }
}

// Full Reset ////////////////////////////////////////////////
void Stopwatch::reset() {
    running = false;
    accumulatedTime = 0;
}

//////////////////////////////////////////////////////////////
// DISPLAY FORMATTING ////////////////////////////////////////
//////////////////////////////////////////////////////////////
String Stopwatch::getFormattedTime() { 
    unsigned long long currentDuration = accumulatedTime;

    if (running) {
        currentDuration += (millis() - startTime);
    }

    unsigned long totalSeconds = currentDuration / 1000;
  
    int SWhours   = totalSeconds / 3600;
    int SWminutes = (totalSeconds / 60) % 60;
    int SWseconds = totalSeconds % 60;
    int SWmillis  = currentDuration % 1000;

    char stopwatchBuffer[20]; 

    snprintf(stopwatchBuffer, sizeof(stopwatchBuffer),      // Enforces leading 0s
        " %02d:%02d:%02d:%03d", 
        SWhours, 
        SWminutes, 
        SWseconds, 
        SWmillis
    );

    return String(stopwatchBuffer); 
}