#ifndef TIMEZONE_H
#define TIMEZONE_H

#include <Arduino.h>

struct TZEntry { const char* posix; const char* tz; const char* offset; const char* city; }; // SET UP POSIX TABLE

class TimeZoneSetting {
    private:
        int currentIndex;

        int getCount();
    
    public:
        TimeZoneSetting();

        void onKnobTurn(int direction);
        void reset();

        String getDisplayString();

        const char* getSelectedPosix();
};

#endif