#include "weather.h"
#include "secrets.h"
#include <WiFi.h>
#include <stdio.h>
#include <ArduinoJson.h> 
#include <Preferences.h>

volatile int Weather::currentTemp = 0;
volatile char Weather::currentIcon = '\x01'; 
volatile bool Weather::lastFetchSuccessful = false;
volatile unsigned long Weather::lastFetchTime = 0;

Weather::Weather() {}

void Weather::saveApiKey(String api) {
    Preferences prefs;
    prefs.begin("weather", false);
    prefs.putString("api", api);
    prefs.end();
    lastFetchTime = 0; 
}

void Weather::saveLat(String lat) {
    Preferences prefs;
    prefs.begin("weather", false);
    prefs.putString("lat", lat);
    prefs.end();
    lastFetchTime = 0; 
}

void Weather::saveLon(String lon) {
    Preferences prefs;
    prefs.begin("weather", false);
    prefs.putString("lon", lon);
    prefs.end();
    lastFetchTime = 0; 
}

void Weather::begin() {
    xTaskCreatePinnedToCore(Weather::weatherTask, "WeatherTask", 8192, NULL, 1, NULL, 0);
}

void Weather::weatherTask(void *pvParameters) {
    static bool firstFetch = true; 
    Preferences taskPrefs;

    while (true) {
        if (WiFi.status() == WL_CONNECTED) {
            unsigned long now = millis();
            
            if (firstFetch || (now - lastFetchTime >= 3600000)) {
                firstFetch = false;
                lastFetchTime = millis();

                String apiToUse = "";
                String latToUse = "";
                String lonToUse = "";

                #ifdef OWM_USE_SECRETS
                if (String(OWM_USE_SECRETS) == "yes") {
                    apiToUse = String(OWM_API_KEY);
                    latToUse = String(OWM_LAT);
                    lonToUse = String(OWM_LON);
                } else 
                #endif
                {
                    taskPrefs.begin("weather", true);
                    apiToUse = taskPrefs.getString("api", "");
                    latToUse = taskPrefs.getString("lat", "39.1031");
                    lonToUse = taskPrefs.getString("lon", "-84.5120");
                    taskPrefs.end();
                }

                if (apiToUse.length() > 0) {
                    HTTPClient http;
                    String url = "http://api.openweathermap.org/data/2.5/weather?lat=" + latToUse + "&lon=" + lonToUse + "&appid=" + apiToUse + "&units=imperial";
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

                            if (weatherID >= 200 && weatherID < 300) currentIcon = '\x05';
                            else if (weatherID >= 300 && weatherID < 600) currentIcon = '\x04';
                            else if (weatherID >= 600 && weatherID < 700) currentIcon = '\x06';
                            else if (weatherID >= 700 && weatherID < 800) currentIcon = '\x07';
                            else if (weatherID == 800) currentIcon = '\x01';
                            else if (weatherID == 801 || weatherID == 802) currentIcon = '\x02';
                            else currentIcon = '\x03';

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
                } else {
                    Serial.println("DBG 0B5 WTAPI: Skipped fetch. No API Key configured.");
                }
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}

String Weather::getDisplayString() {
    char displayBuffer[24]; 
    if (!lastFetchSuccessful) return " NO WEATHER ";
    snprintf(displayBuffer, sizeof(displayBuffer), " %c     %2d\x08""F ", currentIcon, currentTemp);
    return String(displayBuffer);
}