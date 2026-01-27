#ifndef TIMER_H
#define TIMER_H

#include <Arduino.h>
#include <Preferences.h>
#include "selection_util.h"

struct timerData {
    int timerID;
    uint8_t timerHours;
    uint8_t timerMinute;
};

class Timer {
    private:
        timerData table[3];

        unsigned long startTimeT1; unsigned long startTimeT2; unsigned long startTimeT3;
        unsigned long currentRemainingT1; unsigned long currentRemainingT2; unsigned long currentRemainingT3;
        unsigned long timeElapsedT1; unsigned long timeElapsedT2; unsigned long timeElapsedT3;
        bool runningT1; bool runningT2; bool runningT3;

        bool editMode;

        int currentTimer;
        int editField;
        
        selectionUtility selector;
        Preferences prefs;
    
    public:
        Timer();

        void onKnobTurn(int direction);
        void onModButtonPress();
        void onButtonPress();

        void reset();

        void begin();
        void save();
        void factoryReset();

        String getTimerDisplay();

        bool shouldRing(int timerIndex);
};

#endif