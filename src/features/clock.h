#ifndef CLOCK_H
#define CLOCK_H

#include <Arduino.h>
#include <Preferences.h>

class Clock {
    private:
        bool nav;
        int page;

    public:
        Clock();

        void onKnobTurn(int direction);
        void onButtonPress();
        void onModButtonPress();

        String getClockDisplay(bool hour24);

};

#endif