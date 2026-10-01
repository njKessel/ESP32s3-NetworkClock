#ifndef ALARM_H
#define ALARM_H

#include <Arduino.h>
#include <Preferences.h>
#include "selection_util.h"

struct alarmData {
    int alarmID; 
    uint8_t alarmHours;                         // What hour the alarm triggers
    uint8_t alarmMinutes;                       // What minute the alarm triggers
    uint8_t alarmDays;                          // What days the alarm triggers
}; 

class Alarm {
    private:
        alarmData table[3];

        int currentAlarm;                       // What alarm is being edited
        int editField;                          // What field in the menu is being edited
        int pageIndex;                          // Time config or day config page
        int lastTriggeredMinute;                // When the alarm last went off

        selectionUtility selector;              // Cursor
        Preferences prefs;                      // Save states
    
    public:
        Alarm();

        void onKnobTurn(int direction);         // Encoder rotation handler
        void onButtonPress();                   // Choice confirm

        void reset();                           // Exit handler

        void begin();                           // Initialize
        void save();                            // Save states
        void factoryReset();                    // Factory reset to defaults

        String getAlarmDisplay(bool hour24);    // Figure out if the time screen or day screen should be rendered

        String getDayString();                  // Display string creator that handles which days are active
        String getTimeString(bool hour24);      // Display string creator that handles both 12 and 24 hour time

        bool shouldRing(int alarmIndex);        // Check if the alarm should be ringing

        bool isAlarmInNextHour();               // Check if an alarm is coming soon
};


#endif