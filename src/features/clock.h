#ifndef CLOCK_H
#define CLOCK_H

#include <Arduino.h>
#include <Preferences.h>
#include <MCP7940.h>

class Clock {
    private:
        MCP7940_Class RTC;                          // Bring in RTC functions
        unsigned long lastRTCSync;
        bool rtcInitialized;

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
        
        void begin();
        void onButtonPress();                           // What to do when the encoder button is pressed
        void onModButtonPress();                        // What to do when the modifier button is pressed
        void onHomeButtonPress();                       // What to do when the home button is pressed

        String getClockDisplay();                       // Build the display
        void setManualTime(int year, int month, int day, int hour, int minute);
                                                        // Handle manual time setting
        bool is24Hour();

};

#endif