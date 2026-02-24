#ifndef TIME_UTIL
#define TIME_UTIL

#include <Arduino.h>
#include <Preferences.h>
#include <time.h>

class TimeUtil {
    private:

    public:
        void initTime(const char* tz);
        String formatTime(const tm& ti, bool hour24);
};

#endif