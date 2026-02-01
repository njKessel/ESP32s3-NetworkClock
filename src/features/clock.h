#ifndef CLOCK_H
#define CLOCK_H

#include <Arduino.h>
#include <Preferences.h>

class Clock {
    private:
        bool nav;
        bool hour24;
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

        void onKnobTurn(int direction);
        void onButtonPress();
        void onModButtonPress();

        String getClockDisplay(bool hour24);

};

#endif