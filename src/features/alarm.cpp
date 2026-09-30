////////////////////////////////////////////////////////////
// INCLUDES  ///////////////////////////////////////////////
////////////////////////////////////////////////////////////
#include "alarm.h"                                        // Alarm header
#include <time.h>                                         // Arduino time functions

////////////////////////////////////////////////////////////
// ALARM  //////////////////////////////////////////////////
////////////////////////////////////////////////////////////
Alarm::Alarm() {
    currentAlarm = 0;
    editField = 0;
    pageIndex = 1;
    lastTriggeredMinute = -1;

             // ALARM #    HOUR    MINUTE    DAYS         // Initial alarm settings
    table[0] = {1,         6,      30,       0b00111110}; // Alarm one
    table[1] = {2,         22,     0,        0b00111110}; // Alarm two
    table[2] = {3,         0,      0,        0b00000000}; // Alarm three
                                             // Day Bits
                                             // 0   0   0   0   0   0   0   0
                                             // X   Sun Mon Tue Wed Thu Fri Sat (X is don't care)
}

// Initialize  /////////////////////////////////////////////
void Alarm::begin() {
    prefs.begin("alarm-data", false);

    if (prefs.isKey("table")) {                           // Are there alarms saved?
        prefs.getBytes("table", table, sizeof(table));    // Pull the saved alarms
    }
}

// Save Settings  //////////////////////////////////////////
void Alarm::save() {
    Serial.println("DBG 091 ALARM: Saving alarm settings");
    prefs.putBytes("table", table, sizeof(table));
}

// Factory Reset  //////////////////////////////////////////
void Alarm::factoryReset() {
    Serial.println("DBG 09F ALARM: Alarm factory reset");
    prefs.clear();

             // ALARM #    HOUR    MINUTE    DAYS         // Restore defaults
    table[0] = {1,         6,      30,       0b00111110};
    table[1] = {2,         22,     0,        0b00111110};
    table[2] = {3,         0,      0,        0b00000000};
}

////////////////////////////////////////////////////////////
// Config Menu /////////////////////////////////////////////
////////////////////////////////////////////////////////////

// Change Page /////////////////////////////////////////////
void Alarm::onKnobTurn(int direction) {
    if (editField == 0) {
        pageIndex += direction;
        if (pageIndex > 2) pageIndex = 1;
        if (pageIndex < 1) pageIndex = 2;

    } else {

        if (pageIndex == 1) {                             // Configure alarm time

            switch (editField) {
                case 1:                                   // Choose alarm
                    currentAlarm += direction;
                    if (currentAlarm > 2) currentAlarm = 0;
                    if (currentAlarm < 0) currentAlarm = 2;
                    break;

                case 2:                                   // Configure alarm hour
                    table[currentAlarm].alarmHours += direction;
                    if (table[currentAlarm].alarmHours > 23) table[currentAlarm].alarmHours = 0;
                    if (table[currentAlarm].alarmHours == 255) table[currentAlarm].alarmHours = 23;
                    break;

                case 3:                                   // Configure alarm minute
                    table[currentAlarm].alarmMinutes += direction;
                    if (table[currentAlarm].alarmMinutes > 59) table[currentAlarm].alarmMinutes = 0;
                    if (table[currentAlarm].alarmMinutes == 255) table[currentAlarm].alarmMinutes = 59;
                    break;
                    
            }
        }

        if (pageIndex == 2) {                             // Configure alarm days to trigger
            table[currentAlarm].alarmDays ^= (1 << (editField - 1));
        }
    }
}

// Confirm Choice //////////////////////////////////////////
void Alarm::onButtonPress() {
    editField++;                                          // Move to next field
    switch (pageIndex) {
        case 1:                                           // 3 different configurables on the time page
            if (editField > 3) editField = 0;
            break;
        case 2:
            if (editField > 7) editField = 0;             // 7 different configurables on the day page
            break;
    }

}

// Menu Exit ///////////////////////////////////////////////
void Alarm::reset() {
    currentAlarm = 0;
    editField = 0;
    pageIndex = 1;
}

////////////////////////////////////////////////////////////
// Display Creation ////////////////////////////////////////
////////////////////////////////////////////////////////////

// Display router //////////////////////////////////////////
String Alarm::getAlarmDisplay(bool hour24) {
    selector.update();
    if (pageIndex == 1) {
        return getTimeString(hour24);
    } else {
        return getDayString();
    }
}

// Time Config Display /////////////////////////////////////
String Alarm::getTimeString(bool hour24) {
    int displayAlarmHours;
    char amPM[3];

    if (!hour24) {                                         // If in 12 hour mode

        if ((table[currentAlarm].alarmHours - 12) < 0) {   // If before noon add AM
          strcpy(amPM, "AM"); 
          displayAlarmHours = table[currentAlarm].alarmHours;
          if (displayAlarmHours == 0) displayAlarmHours = 12;

        } else {                                           // If after noon add PM
          strcpy(amPM, "PM"); 
          displayAlarmHours = table[currentAlarm].alarmHours - 12;
          if (displayAlarmHours == 0) displayAlarmHours = 12;
        }

    } else {                                               // If in 24hr mode
        strcpy(amPM, "  ");
        displayAlarmHours = table[currentAlarm].alarmHours;
    }

    char timeBuffer[20];
    snprintf(timeBuffer, sizeof(timeBuffer), " %s  %s:%s %s", 
       selector.getBlinkText(editField == 1, table[currentAlarm].alarmID,      1).c_str(),
       selector.getBlinkText(editField == 2, displayAlarmHours,                2).c_str(),
       selector.getBlinkText(editField == 3, table[currentAlarm].alarmMinutes, 2).c_str(),
       amPM
    );
    return String(timeBuffer);
}

// Day Config Display //////////////////////////////////////
String Alarm::getDayString() {
    char days[8] = "SMTWTFS";
    String dayString = "  ";                              // Left padding

    for (int i = 0; i < 7; i++) {
        String letter = String(days[i]);
        
        if (table[currentAlarm].alarmDays & (1 << i)) {  // Add selected dot if the day is active
            letter += ":";
        }

        bool isSelected = (editField == i + 1);
        
        dayString += selector.getBlinkText(isSelected, letter);
                                                         // Blinking cursor
    }

    dayString += "   ";
    return dayString;
}

////////////////////////////////////////////////////////////
// Ring Check //////////////////////////////////////////////
////////////////////////////////////////////////////////////

bool Alarm::shouldRing(int alarmIndex) {
    time_t now;
    time(&now);
    struct tm timeinfo;
    localtime_r(&now, &timeinfo); 

    if (timeinfo.tm_year < 116) {
        return false;
    }                                                     // Fail fast if time is invalid

    if (timeinfo.tm_min == lastTriggeredMinute) return false;
                                                          // Fail if its already gone off in this minute 
    
    int todayIndex = timeinfo.tm_wday;

    bool isEnabledToday = (table[alarmIndex].alarmDays >> todayIndex) & 1;
                                                          // Is it supposed to go off today?

    bool ring = isEnabledToday &&
        (timeinfo.tm_hour == table[alarmIndex].alarmHours) && 
        (timeinfo.tm_min  == table[alarmIndex].alarmMinutes) &&
        (timeinfo.tm_sec  == 0);
                                                          // Is it supposed to go off at this time?

    if (ring) {                                           // If so, ring
        lastTriggeredMinute = timeinfo.tm_min;            // Prevent it from ringing multiple times in a minute
        return true;
    }
    return false;
}