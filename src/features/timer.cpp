#include "timer.h"
#include <time.h>

Timer::Timer() {
    currentTimer = 0;
    editField = 0;
    editMode = false;

    runningT1 = false;
    runningT2 = false;
    runningT3 = false;

    pausedT1 = false;
    pausedT2 = false;
    pausedT3 = false;

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
        if (editField == 1) {    // CHANGE HOURS
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
        if (!runningT1 && !pausedT1) {
            if (editMode) {
                editField = 0;
            } else {
                editField = 1;
            }
            editMode = !editMode;
        } else {
            pausedT1 = false;
            runningT1 = false;
            timeElapsedT1 = 0;
        }
    } else if (currentTimer == 1) {
        if (!runningT2 && !pausedT2) {
            if (editMode) {
                editField = 0;
            } else {
                editField = 1;
            }
            editMode = !editMode;
        } else {
            pausedT2 = false;
            runningT2 = false;
            timeElapsedT2 = 0;
        }
    } else if (currentTimer == 2) {
        if (!runningT3 && !pausedT3) {
            if (editMode) {
                editField = 0;
            } else {
                editField = 1;
            }
            editMode = !editMode;
        } else {
            pausedT3 = false;
            runningT3 = false;
            timeElapsedT3 = 0;
        }
    }
}

void Timer::onButtonPress() {
    if (editMode) {
        editField++;
        if (editField > 2) editField = 1;
    } else {
        if (currentTimer == 0) {
            if (!runningT1) {
                startTimeT1 = millis();
                runningT1 = true;
                pausedT1 = false;
            } else {
                timeElapsedT1 += (millis() - startTimeT1);
                runningT1 = false;
                pausedT1 = true;
            }
            delay(250);
        } else if (currentTimer == 1) {
            if (!runningT2) {
                startTimeT2 = millis();
                runningT2 = true;
                pausedT2 = false;
            } else {
                timeElapsedT2 += (millis() - startTimeT2);
                runningT2 = false;
                pausedT2 = true;
            }
            delay(250);
        } else if (currentTimer == 2) {
            if (!runningT3) {
                startTimeT3 = millis();
                runningT3 = true;
                pausedT3 = false;
            } else {
                timeElapsedT3 += (millis() - startTimeT3);
                runningT3 = false;
                pausedT3 = true;
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
    selector.update();

    millisT1 = (((unsigned long)table[0].timerMinute*60000) + ((unsigned long)table[0].timerHours*3600000));
    millisT2 = (((unsigned long)table[1].timerMinute*60000) + ((unsigned long)table[1].timerHours*3600000));
    millisT3 = (((unsigned long)table[2].timerMinute*60000) + ((unsigned long)table[2].timerHours*3600000));

    unsigned long currentTotalElapsedT1 = timeElapsedT1;
    unsigned long currentTotalElapsedT2 = timeElapsedT2;
    unsigned long currentTotalElapsedT3 = timeElapsedT3;

    if (runningT1) {
        currentTotalElapsedT1 += (millis() - startTimeT1);
    } 
    if (runningT2) {
        currentTotalElapsedT2 += (millis() - startTimeT2);
    } 
    if (runningT3) {
        currentTotalElapsedT3 += (millis() - startTimeT3);
    }

    if (currentTotalElapsedT1 > millisT1) {
        currentTotalElapsedT1 = millisT1;
    } 
    if (currentTotalElapsedT2 > millisT2) {
        currentTotalElapsedT2 = millisT2;
    }
    if (currentTotalElapsedT3 > millisT3) {
        currentTotalElapsedT3 = millisT3;
    }

    currentRemainingT1 = millisT1 - currentTotalElapsedT1;
    currentRemainingT2 = millisT2 - currentTotalElapsedT2;
    currentRemainingT3 = millisT3 - currentTotalElapsedT3;

    int secondsRemainingT1 = ((currentRemainingT1 / 1000) % 60);
    int secondsRemainingT2 = ((currentRemainingT2 / 1000) % 60);
    int secondsRemainingT3 = ((currentRemainingT3 / 1000) % 60);

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
    char timerSecondDisp[3];
    if (currentTimer == 0) {
        if (runningT1 || pausedT1) {
            snprintf(timerHourDisp,   sizeof(timerHourDisp),   "%02d", hoursRemainingT1);
            snprintf(timerMinuteDisp, sizeof(timerMinuteDisp), "%02d", minutesRemainingT1);
            snprintf(timerSecondDisp, sizeof(timerSecondDisp), "%02d", secondsRemainingT1);

            snprintf(timerBuffer, sizeof(timerBuffer), "  %s:%s:%s  ", timerHourDisp, timerMinuteDisp, timerSecondDisp);
        } else {
            String s_id = selector.getBlinkText(false, table[0].timerID,     1);
            String s_hr = selector.getBlinkText(editField == 1, table[0].timerHours,  2);
            String s_mn = selector.getBlinkText(editField == 2, table[0].timerMinute, 2);

            snprintf(timerBuffer, sizeof(timerBuffer), " %s  %s:%s    ",
                s_id.c_str(),
                s_hr.c_str(),
                s_mn.c_str()
            );
        }
    } else if (currentTimer == 1) {
        if (runningT2 || pausedT2) {
            snprintf(timerHourDisp,   sizeof(timerHourDisp),   "%02d", hoursRemainingT2);
            snprintf(timerMinuteDisp, sizeof(timerMinuteDisp), "%02d", minutesRemainingT2);
            snprintf(timerSecondDisp, sizeof(timerSecondDisp), "%02d", secondsRemainingT2);

            snprintf(timerBuffer, sizeof(timerBuffer), "  %s:%s:%s  ", timerHourDisp, timerMinuteDisp, timerSecondDisp);
        } else {
            String s_id = selector.getBlinkText(false, table[1].timerID,     1);
            String s_hr = selector.getBlinkText(editField == 1, table[1].timerHours,  2);
            String s_mn = selector.getBlinkText(editField == 2, table[1].timerMinute, 2);

            snprintf(timerBuffer, sizeof(timerBuffer), " %s  %s:%s    ",
                s_id.c_str(),
                s_hr.c_str(),
                s_mn.c_str()
            );
        }
    } else if (currentTimer == 2) {
        if (runningT3 || pausedT3) {
            snprintf(timerHourDisp,   sizeof(timerHourDisp),   "%02d", hoursRemainingT3);
            snprintf(timerMinuteDisp, sizeof(timerMinuteDisp), "%02d", minutesRemainingT3);
            snprintf(timerSecondDisp, sizeof(timerSecondDisp), "%02d", secondsRemainingT3);

            snprintf(timerBuffer, sizeof(timerBuffer), "  %s:%s:%s  ", timerHourDisp, timerMinuteDisp, timerSecondDisp);
        } else {
            String s_id = selector.getBlinkText(false, table[2].timerID,     1);
            String s_hr = selector.getBlinkText(editField == 1, table[2].timerHours,  2);
            String s_mn = selector.getBlinkText(editField == 2, table[2].timerMinute, 2);

            snprintf(timerBuffer, sizeof(timerBuffer), " %s  %s:%s    ",
                s_id.c_str(),
                s_hr.c_str(),
                s_mn.c_str()
            );
        }
    }
    return String(timerBuffer);
}

bool Timer::shouldRing(int timerIndex) {
    unsigned long target = 0;
    unsigned long currentElapsed = 0;

    if (timerIndex == 1) {
        target = ((unsigned long)table[0].timerMinute * 60000) + ((unsigned long)table[0].timerHours * 3600000);
        
        currentElapsed = timeElapsedT1;
        if (runningT1) currentElapsed += (millis() - startTimeT1);
        
    } else if (timerIndex == 2) {
        target = ((unsigned long)table[1].timerMinute * 60000) + ((unsigned long)table[1].timerHours * 3600000);
        currentElapsed = timeElapsedT2;
        if (runningT2) currentElapsed += (millis() - startTimeT2);

    } else if (timerIndex == 3) {
        target = ((unsigned long)table[2].timerMinute * 60000) + ((unsigned long)table[2].timerHours * 3600000);
        currentElapsed = timeElapsedT3;
        if (runningT3) currentElapsed += (millis() - startTimeT3);
    }

    if (currentElapsed >= target && target > 0) {
        return true;
    }
    
    return false;
}