#ifndef SET_TIME_H
#define SET_TIME_H

#include <Arduino.h>
#include "selection_util.h"

class SetTime {
    private:
        int editField; 
        
        int sYear;
        int sMonth;
        int sDay;
        int sHour;
        int sMinute;
        int sSecond;
        
        selectionUtility selector;

    public:
        SetTime();
        void reset();
        void onKnobTurn(int direction);
        bool onButtonPress();
        String getDisplayString();
        
        int getYear() {return sYear;}
        int getMonth() {return sMonth;}
        int getDay() {return sDay;}
        int getHour() {return sHour;}
        int getMinute() {return sMinute;}
};

#endif