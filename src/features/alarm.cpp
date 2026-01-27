#include "alarm.h"

Alarm::Alarm() {
    currentAlarm = 0;
    editField = 0;
    pageIndex = 1;

             // ALARM #    HOUR    MINUTE    DAYS
    table[0] = {1,         6,      30,       0b00111110};
    table[1] = {2,         22,     0,        0b00111110};
    table[2] = {3,         0,      0,        0b00000000};
}

void Alarm::begin() {
    prefs.begin("alarm-data", false);

    if (prefs.isKey("table")) {
        prefs.getBytes("table", table, sizeof(table));
    }
}

void Alarm::save() {
    prefs.putBytes("table", table, sizeof(table));
}

void Alarm::factoryReset() {
    prefs.clear();

             // ALARM #    HOUR    MINUTE    DAYS
    table[0] = {1,         6,      30,       0b00111110};
    table[1] = {2,         22,     0,        0b00111110};
    table[2] = {3,         0,      0,        0b00000000};
}


void Alarm::onKnobTurn(int direction) {
    if (editField == 0) {
        pageIndex += direction;
        if (pageIndex > 2) pageIndex = 1;
        if (pageIndex < 1) pageIndex = 2;

    } else {

        if (pageIndex == 1) { // 

            switch (editField) {
                case 1: // CYCLE ALARMS
                    currentAlarm += direction;
                    if (currentAlarm > 2) currentAlarm = 0;
                    if (currentAlarm < 0) currentAlarm = 2;
                    break;

                case 2: // CHANGE HOUR
                    table[currentAlarm].alarmHours += direction;
                    if (table[currentAlarm].alarmHours > 23) table[currentAlarm].alarmHours = 0;
                    if (table[currentAlarm].alarmHours == 255) table[currentAlarm].alarmHours = 23;
                    break;

                case 3: // CHANGE MINUTE
                    table[currentAlarm].alarmMinutes += direction;
                    if (table[currentAlarm].alarmMinutes > 59) table[currentAlarm].alarmMinutes = 0;
                    if (table[currentAlarm].alarmMinutes == 255) table[currentAlarm].alarmMinutes = 59;
                    break;
                    
            }
        }

        if (pageIndex == 2) {
            table[currentAlarm].alarmDays ^= (1 << (editField - 1));
        }
    }
}

void Alarm::onButtonPress() {
    editField++;
    switch (pageIndex) {
        case 1:
            if (editField > 3) editField = 0;
            break;
        case 2:
            if (editField > 7) editField = 0;
            break;
    }

}

void Alarm::reset() {
    currentAlarm = 0;
    editField = 0;
    pageIndex = 1;
}

String Alarm::getAlarmDisplay(bool hour24) {
    selector.update();
    if (pageIndex == 1) {
        return getTimeString(hour24);
    } else {
        return getDayString();
    }
}

String Alarm::getTimeString(bool hour24) {
    int displayAlarmHours;
    char amPM[3];

    if (!hour24) { // 12HR TIME

        if ((table[currentAlarm].alarmHours - 12) < 0) { // CHECK IF AM, FORMAT
          strcpy(amPM, "AM"); 
          displayAlarmHours = table[currentAlarm].alarmHours;
          if (displayAlarmHours == 0) displayAlarmHours = 12;

        } else {                                         // FORMAT PM
          strcpy(amPM, "PM"); 
          displayAlarmHours = table[currentAlarm].alarmHours - 12;
          if (displayAlarmHours == 0) displayAlarmHours = 12;
        }

    } else { // 24H TIME
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

String Alarm::getDayString() {
    char days[8] = "SMTWTFS";
    String dayString = "  ";

    for (int i = 0; i < 7; i++) {
        String letter = String(days[i]);
        
        if (table[currentAlarm].alarmDays & (1 << i)) {
            letter += ":";
        }

        bool isSelected = (editField == i + 1);
        
        dayString += selector.getBlinkText(isSelected, letter);
    }

    dayString += "   ";
    return dayString;
}