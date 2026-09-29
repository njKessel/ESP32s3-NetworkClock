#include "set_time.h"
#include <time.h>

SetTime::SetTime() {
    editField = 1;
}

// MENU RESET //////////////////////////////////////////////
void SetTime::reset() {
    time_t now;
    time(&now);
    struct tm ti;
    localtime_r(&now, &ti);
    
    sYear = ti.tm_year + 1900;
    if (sYear < 2024) sYear = 2026;
    sMonth = ti.tm_mon + 1;
    sDay = ti.tm_mday;
    sHour = ti.tm_hour;
    sMinute = ti.tm_min;
    sSecond = ti.tm_sec;
    
    editField = 1;
}

// TIME ADJUST ENCODER HANDLING ////////////////////////////
void SetTime::onKnobTurn(int direction) {
    switch (editField) {
        case 1:                                           // Configure year, between 2024 - 2099
            sYear += direction;
            if (sYear > 2099) sYear = 2024;
            if (sYear < 2024) sYear = 2099;
            break;
        case 2:
            sMonth += direction;                          // Configure month
            if (sMonth > 12) sMonth = 1;
            if (sMonth < 1) sMonth = 12;
            break;
        case 3:                                           // Configure day
            sDay += direction;
            if (sDay > 31) sDay = 1;
            if (sDay < 1) sDay = 31;
            break;
        case 4:                                           // Configure hour
            sHour += direction;
            if (sHour > 23) sHour = 0;
            if (sHour < 0) sHour = 23;
            break;
        case 5:                                           // Configure minute
            sMinute += direction;
            if (sMinute > 59) sMinute = 0;
            if (sMinute < 0) sMinute = 59;
            break;
        case 6:                                           // Configure seconds
            sSecond += direction;
            if (sSecond > 59) sSecond = 0;
            if (sSecond < 0) sSecond = 59;
            break;
    }
}

// MOVE THROUGH CONFIG /////////////////////////////////////
bool SetTime::onButtonPress() {
    editField++;
    if (editField > 6) {
        return true;                                      // If the end of the menu was reached, exit
    }
    return false;
}

// PREPARE DISPLAY /////////////////////////////////////////
String SetTime::getDisplayString() {
    selector.update();
    char buffer[20];
    
    if (editField <= 3) { 
        snprintf(buffer, sizeof(buffer), " %s.%s.%s ",
            selector.getBlinkText(editField == 1, sYear, 4).c_str(),
            selector.getBlinkText(editField == 2, sMonth, 2).c_str(),
            selector.getBlinkText(editField == 3, sDay, 2).c_str()
        );
    } else { 
        snprintf(buffer, sizeof(buffer), "   %s:%s:%s   ",
            selector.getBlinkText(editField == 4, sHour, 2).c_str(),
            selector.getBlinkText(editField == 5, sMinute, 2).c_str(),
            selector.getBlinkText(editField == 6, sSecond, 2).c_str()
        );
    }
    return String(buffer);
}