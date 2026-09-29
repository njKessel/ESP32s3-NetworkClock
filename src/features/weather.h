#ifndef WEATHER_H
#define WEATHER_H

#include <Arduino.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

class Weather {
    private:
        static volatile int currentTemp;
        static volatile char currentIcon;
        static volatile bool lastFetchSuccessful;
        static volatile unsigned long lastFetchTime;

        static void weatherTask(void *pvParameters);

    public:
        Weather();
        void begin();
        String getDisplayString();
};

#endif