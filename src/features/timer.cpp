#include "timer.h"
#include <time.h>

Timer::Timer() {
    currentTimer = 0;
    editField = 0;
    editMode = false;
    timeElapsedT1 = 0;
    timeElapsedT2 = 0;
    timeElapsedT3 = 0;

              //TIMER #  HOUR     MINUTE
    table[0] = {1,       0,       30};
    table[1] = {2,       1,       0};
    table[2] = {3,       8,       0};
}

void Timer::begin() {
    prefs.begin("timer-data", false);

    if (prefs.isKey("table")) {
        prefs.getBytes("table", table, sizeof(table));
    }
}

void Timer::save() {
    prefs.putBytes("table", table, sizeof(table));
}

void Timer::factoryReset() {
    prefs.clear();

              //TIMER #  HOUR     MINUTE
    table[0] = {1,       0,       30};
    table[1] = {2,       1,       0};
    table[2] = {3,       8,       0};
}

void Timer::onKnobTurn(int direction) {
    if (editMode) {
        if (editField == 0) {           // CHANGE TIMERS
            currentTimer += direction;
            if (currentTimer > 2) currentTimer = 0;
            if (currentTimer < 0) currentTimer = 2;

        } else if (editField == 1) {    // CHANGE HOURS
            table[currentTimer].timerHours += direction;
            if (table[currentTimer].timerHours > 99) table[currentTimer].timerHours = 0;
            if (table[currentTimer].timerHours == 255) table[currentTimer].timerHours = 99;

        } else if (editField == 2) {    // CHANGE MINUTES
            table[currentTimer].timerMinute += direction;
            if (table[currentTimer].timerMinute > 59) table[currentTimer].timerMinute = 0;
            if (table[currentTimer].timerMinute == 255) table[currentTimer].timerMinute = 59;

        } 
    } else {
        currentTimer += direction;
        if (currentTimer > 2) currentTimer = 0;
        if (currentTimer < 0) currentTimer = 2;
    }
}

void Timer::onModButtonPress() {
    if (currentTimer == 0) {
        if (!runningT1) {
            editMode = true;
        } else {
            runningT1 = false;
            timeElapsedT1 = 0;
        }
    } else if (currentTimer == 1) {
        if (!runningT2) {
            editMode = true;
        } else {
            runningT2 = false;
            timeElapsedT2 = 0;
        }
    } else if (currentTimer == 2) {
        if (!runningT3) {
            editMode = true;
        } else {
            runningT3 = false;
            timeElapsedT3 = 0;
        }
    }
}

void Timer::onButtonPress() {
    if (editMode) {
        editField++;
        if (editField > 2) editField = 0;
    } else {
        if (currentTimer == 0) {
            if (!runningT1) {
                startTimeT1 = millis();
                runningT1 = true;
            } else {
                timeElapsedT1 += (millis() - startTimeT1);
                runningT1 = false;
            }
            delay(250);
        } else if (currentTimer == 1) {
            if (!runningT2) {
                startTimeT2 = millis();
                runningT2 = true;
            } else {
                timeElapsedT2 += (millis() - startTimeT2);
                runningT2 = false;
            }
            delay(250);
        } else if (currentTimer == 2) {
            if (!runningT3) {
                startTimeT3 = millis();
                runningT3 = true;
            } else {
                timeElapsedT3 += (millis() - startTimeT3);
                runningT3 = false;
            }
            delay(250);
        }
    }
}

void Timer::reset() {
    currentTimer = 0;
    editField = 0;
    editMode = false;
}

String Timer::getTimerDisplay() {
    millisT1 = ((table[0].timerMinute*60000) + (table[0].timerHours*3600000));
    millisT2 = ((table[1].timerMinute*60000) + (table[1].timerHours*3600000));
    millisT3 = ((table[2].timerMinute*60000) + (table[2].timerHours*3600000));

    unsigned long currentTotalElapsedT1 = timeElapsedT1;
    unsigned long currentTotalElapsedT2 = timeElapsedT2;
    unsigned long currentTotalElapsedT3 = timeElapsedT3;

    if (runningT1) {
        currentTotalElapsedT1 += (millis() - startTimeT1);
    } else if (runningT2) {
        currentTotalElapsedT2 += (millis() - startTimeT2);
    } else if (runningT3) {
        currentTotalElapsedT3 += (millis() - startTimeT3);
    }

    currentRemainingT1 = millisT1 - currentTotalElapsedT1;
    currentRemainingT2 = millisT2 - currentTotalElapsedT2;
    currentRemainingT3 = millisT3 - currentTotalElapsedT3;

    int minutesRemainingT1 = ((currentRemainingT1 / 1000) / 60) % 60;
    int minutesRemainingT2 = ((currentRemainingT2 / 1000) / 60) % 60;
    int minutesRemainingT3 = ((currentRemainingT3 / 1000) / 60) % 60;

    int hoursRemainingT1   = ((currentRemainingT1 / 1000) /3600);
    int hoursRemainingT2   = ((currentRemainingT2 / 1000) /3600);
    int hoursRemainingT3   = ((currentRemainingT3 / 1000) /3600);

    char timerBuffer[24];
    char timerIDDisp[2];
    char timerHourDisp[3];
    char timerMinuteDisp[3];
    if (currentTimer == 0) {
        if (runningT1) {
            snprintf(timerIDDisp, sizeof(timerIDDisp), " ");
            snprintf(timerHourDisp,   sizeof(timerHourDisp),   "%02d", hoursRemainingT1);
            snprintf(timerMinuteDisp, sizeof(timerMinuteDisp), "%02d", minutesRemainingT1);

            snprintf(timerBuffer, sizeof(timerBuffer), "%s   %s:%s    ", timerIDDisp, timerHourDisp, timerMinuteDisp);
        } else {
            snprintf(timerBuffer, sizeof(timerBuffer), "%s   %s:%s    ",
                selector.getBlinkText(editField == 0, table[0].timerID,     1).c_str(),
                selector.getBlinkText(editField == 1, table[0].timerHours,  2).c_str(),
                selector.getBlinkText(editField == 2, table[0].timerMinute, 2).c_str()
            );
        }
    } else if (currentTimer == 1) {
        if (runningT2) {
            snprintf(timerIDDisp, sizeof(timerIDDisp), " ");
            snprintf(timerHourDisp,   sizeof(timerHourDisp),   "%02d", hoursRemainingT2);
            snprintf(timerMinuteDisp, sizeof(timerMinuteDisp), "%02d", minutesRemainingT2);

            snprintf(timerBuffer, sizeof(timerBuffer), "%s   %s:%s    ", timerIDDisp, timerHourDisp, timerMinuteDisp);
        } else {
            snprintf(timerBuffer, sizeof(timerBuffer), "%s   %s:%s    ",
                selector.getBlinkText(editField == 0, table[1].timerID,     1).c_str(),
                selector.getBlinkText(editField == 1, table[1].timerHours,  2).c_str(),
                selector.getBlinkText(editField == 2, table[1].timerMinute, 2).c_str()
            );
        }
    } else if (currentTimer == 2) {
        if (runningT3) {
            snprintf(timerIDDisp, sizeof(timerIDDisp), " ");
            snprintf(timerHourDisp,   sizeof(timerHourDisp),   "%02d", hoursRemainingT3);
            snprintf(timerMinuteDisp, sizeof(timerMinuteDisp), "%02d", minutesRemainingT3);

            snprintf(timerBuffer, sizeof(timerBuffer), "%s   %s:%s    ", timerIDDisp, timerHourDisp, timerMinuteDisp);
        } else {
            snprintf(timerBuffer, sizeof(timerBuffer), "%s   %s:%s    ",
                selector.getBlinkText(editField == 0, table[2].timerID,     1).c_str(),
                selector.getBlinkText(editField == 1, table[2].timerHours,  2).c_str(),
                selector.getBlinkText(editField == 2, table[2].timerMinute, 2).c_str()
            );
        }
    }
    return String(timerBuffer);
}

bool Timer::shouldRing(int timerIndex) {
    if (timerIndex == 1) {
        if (timeElapsedT1 >= millisT1) {
            return true;
        } else {
            return false;
        }

    } else if (timerIndex == 2) {
        if (timeElapsedT2 >= millisT2) {
            return true;
        } else {
            return false;
        }

    } else if (timerIndex == 3) {
        if (timeElapsedT3 >= millisT3) {
            return true;
        } else {
            return false;
        }

    } else {
        return false;
    }
}