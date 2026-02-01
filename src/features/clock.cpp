#include "clock.h"
#include <Arduino.h>
#include <time.h>

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
        if (!running) {
            running = true;
        }
    }
}