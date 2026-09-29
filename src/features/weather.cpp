#include "weather.h"
#include "secrets.h"
#include <WiFi.h>
#include <stdio.h>
#include <ArduinoJson.h> 

extern volatile bool globalWiFiConnected;

volatile int Weather::currentTemp = 0;
volatile char Weather::currentIcon = '\x01'; // Safe default
volatile bool Weather::lastFetchSuccessful = false;
volatile unsigned long Weather::lastFetchTime = 0;

Weather::Weather() {}

void Weather::begin() {
    xTaskCreatePinnedToCore(
        Weather::weatherTask,   
        "WeatherTask",          
        8192,                   
        NULL,                   
        1,                      
        NULL,                   
        0                       
    );
}

void Weather::weatherTask(void *pvParameters) {
    static bool firstFetch = true; 

    while (true) {
        if (WiFi.status() == WL_CONNECTED) {
            unsigned long now = millis();
            
            if (firstFetch || (now - lastFetchTime >= 3600000)) {
                firstFetch = false;
                lastFetchTime = millis();

                HTTPClient http;
                String url = "http://api.openweathermap.org/data/2.5/weather?lat=" + String(OWM_LAT) + "&lon=" + String(OWM_LON) + "&appid=" + String(OWM_API_KEY) + "&units=imperial";
                Serial.println("DBG XXX WTAPI: Made an API call");

                http.begin(url);
                int httpCode = http.GET();

                if (httpCode == 200) {
                    String payload = http.getString();
                    
                    JsonDocument doc;
                    DeserializationError error = deserializeJson(doc, payload);

                    if (!error) {
                        currentTemp = (int)round(doc["main"]["temp"].as<float>());
                        int weatherID = doc["weather"][0]["id"].as<int>();

                        if (weatherID >= 200 && weatherID < 300) currentIcon = '\x05';      // Thunderstorm
                        else if (weatherID >= 300 && weatherID < 600) currentIcon = '\x04'; // Rain
                        else if (weatherID >= 600 && weatherID < 700) currentIcon = '\x06'; // Snow
                        else if (weatherID >= 700 && weatherID < 800) currentIcon = '\x07'; // Fog/Mist
                        else if (weatherID == 800) currentIcon = '\x01';                    // Sunny/Clear
                        else if (weatherID == 801 || weatherID == 802) currentIcon = '\x02';// Partly Cloudy
                        else currentIcon = '\x03';                                          // Cloudy

                        lastFetchSuccessful = true;
                        Serial.println("DBG 0BF WTAPI: Weather sync successful");
                    } else {
                        lastFetchSuccessful = false;
                        Serial.println("DBG 0B3 WTAPI: JSON Parse Failed");
                    }
                } else {
                    lastFetchSuccessful = false;
                    Serial.print("DBG 0B4 WTAPI: HTTP Error Code: ");
                    Serial.println(httpCode);
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

    snprintf(displayBuffer, sizeof(displayBuffer), " %c TDR %2d\x08""F ", currentIcon, currentTemp);
    
    return String(displayBuffer);
}