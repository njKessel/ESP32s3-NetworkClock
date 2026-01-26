#include "stopwatch.h"

Stopwatch::Stopwatch() {
    running = false;
    accumulatedTime = 0;
    startTime = 0;
}

void Stopwatch::toggle() {
    if (running) {
        accumulatedTime += (millis() - startTime);
        running = false;
    } else {
        startTime = millis();
        running = true;
    }
}

void Stopwatch::reset() {
    running = false;
    accumulatedTime = 0;
}

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

    snprintf(stopwatchBuffer, sizeof(stopwatchBuffer), 
        " %02d:%02d:%02d:%03d", 
        SWhours, 
        SWminutes, 
        SWseconds, 
        SWmillis
    );

    return String(stopwatchBuffer); 
}