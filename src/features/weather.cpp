#include "weather.h"
#include "secrets.h"
#include <WiFi.h>
#include <stdio.h>
#include <ArduinoJson.h> 

extern volatile bool globalWiFiConnected;

volatile int Weather::currentTemp = 0;
volatile char Weather::currentIcon = '\x80'; 
volatile bool Weather::lastFetchSuccessful = false;
volatile unsigned long Weather::lastFetchTime = 0;

Weather::Weather() {}

void Weather::begin() {
    xTaskCreatePinnedToCore(
        Weather::weatherTask,   // Function to run
        "WeatherTask",          // Task name
        8192,                   // Stack size (JSON parsing needs space)
        NULL,                   // Task parameters
        1,                      // Priority (1 is low priority)
        NULL,                   // Task handle
        0                       // Pin to Core 0
    );
}

void Weather::weatherTask(void *pvParameters) {
    while (true) {
        if (globalWiFiConnected) {
            unsigned long now = millis();
            
            if (!lastFetchSuccessful || (now - lastFetchTime >= 3600000)) {
                
                HTTPClient http;
                String url = "http://api.openweathermap.org/data/2.5/weather?lat=" + String(OWM_LAT) + "&lon=" + String(OWM_LON) + "&appid=" + String(OWM_API_KEY) + "&units=imperial";

                http.begin(url);
                int httpCode = http.GET();

                if (httpCode == 200) {
                    String payload = http.getString();
                    
                    JsonDocument doc;
                    DeserializationError error = deserializeJson(doc, payload);

                    if (!error) {
                        currentTemp = (int)round(doc["main"]["temp"].as<float>());
                        int weatherID = doc["weather"][0]["id"].as<int>();

                        if (weatherID >= 200 && weatherID < 300) currentIcon = '\x84';      // Thunderstorm
                        else if (weatherID >= 300 && weatherID < 600) currentIcon = '\x83'; // Rain
                        else if (weatherID >= 600 && weatherID < 700) currentIcon = '\x85'; // Snow
                        else if (weatherID >= 700 && weatherID < 800) currentIcon = '\x86'; // Fog/Mist
                        else if (weatherID == 800) currentIcon = '\x80';                    // Sunny/Clear
                        else if (weatherID == 801 || weatherID == 802) currentIcon = '\x81';// Partly Cloudy
                        else currentIcon = '\x82';                                          // Cloudy

                        lastFetchSuccessful = true;
                        lastFetchTime = millis();
                        Serial.println("DBG 0BF WTAPI: Weather sync successful");
                    } else {
                        Serial.println("DBG 0B3 WTAPI: JSON Parse Failed");
                    }
                }
                http.end();
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}

String Weather::getDisplayString() {
    char displayBuffer[24]; 

    if (!lastFetchSuccessful) {
        return " NO WEATHER ";
    }

    snprintf(displayBuffer, sizeof(displayBuffer), "  %c   %2d\x87""F  ", currentIcon, currentTemp);
    
    return String(displayBuffer);
}