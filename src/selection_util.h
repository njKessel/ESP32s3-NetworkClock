#ifndef SELECTION_UTIL_H
#define SELECTION_UTIL_H

#include <Arduino.h>

class selectionUtility {
    private:
        unsigned long long lastFlash;
        bool flashState;

    public:
        selectionUtility() {
            lastFlash = 0;
            flashState = false;
        }

        void update() {
            if (millis() - lastFlash > 500) {
                flashState = !flashState;
                lastFlash = millis();
            }
        }

        String getBlinkText(bool selected, int value, int minWidth) {
            char buf[10];
            if (minWidth == 2) snprintf(buf, sizeof(buf), "%02d", value);
            else snprintf(buf, sizeof(buf), "%d", value);

            if (!selected) return String(buf);

            return (flashState) ? ((minWidth == 2) ? "##" : "#") : String(buf);
        }

        String getBlinkText(bool selected, String text) {
            if (!selected) return text;
            
            if (flashState) {
                String mask = "";
                for (unsigned int i = 0; i < text.length(); i++) {
                    char c = text[i];
                    mask += (c == ':' || c == '.') ? c : '#';
                }
                return mask;
            }
            return text;
        }
};

#endif