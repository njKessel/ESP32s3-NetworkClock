#ifndef ALARM_H
#define ALARM_H

#include <Arduino.h>
#include <Preferences.h>
#include "selection_util.h"

struct alarmData {
    int alarmID; 
    uint8_t alarmHours; 
    uint8_t alarmMinutes; 
    uint8_t alarmDays;
};

class Alarm {
    private:
        alarmData table[3];

        int currentAlarm;
        int editField;
        int pageIndex;
        int lastTriggeredMinute;

        selectionUtility selector;
        Preferences prefs;
    
    public:
        Alarm();

        void onKnobTurn(int direction);
        void onButtonPress();

        void reset();

        void begin(); 
        void save();  
        void factoryReset(); 

        String getAlarmDisplay(bool hour24);

        String getDayString();
        String getTimeString(bool hour24);

        String selected();
        bool shouldRing(int alarmIndex);
};


#endif