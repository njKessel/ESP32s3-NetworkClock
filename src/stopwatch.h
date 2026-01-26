#ifndef STOPWATCH_H
#define STOPWATCH_H

#include <Arduino.h>

class Stopwatch {
    private:
        bool running;
        unsigned long long startTime;
        unsigned long long accumulatedTime;

    public:
        Stopwatch();
        void toggle();
        void reset();
        String getFormattedTime();
};

#endif