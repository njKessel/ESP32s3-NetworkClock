#ifndef TIMER_H
#define TIMER_H

#include <Arduino.h>

struct timerData {
    int timerID;
    uint8_t timerHours;
    uint8_t timerMinute;
};

class timer {
  private:
    timerData table[3];

    unsigned long startTime;
    unsigned long timeElapsed;
    
};

#endif