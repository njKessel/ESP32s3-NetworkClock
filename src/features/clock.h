#ifndef CLOCK_H
#define CLOCK_H

#include <Arduino.h>
#include <Preferences.h>

class Clock {
    private:

        bool hour24;
        bool nav;
        int page;
        // PAGE 0 = CLOCK
        // PAGE 1 = FOCUS TIMER

        unsigned long focusStart;
        unsigned long focusRemain;
        unsigned long focusElapsed;
        unsigned long focusMillis;
        bool running;
        bool paused;

        bool latch;
        bool editMode;
    public:
        Clock();

        void onButtonPress();
        void onModButtonPress();
        void onHomeButtonPress();

        String getClockDisplay(bool hour24);

};

#endif