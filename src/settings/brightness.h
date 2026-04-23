#ifndef BRIGHTNESS_H
#define BRIGHTNESS_H

#include <Arduino.h>

struct brightnessOptions { const char* level; const uint8_t value; }; // SET UP BRIGHTNESS TABLE

const brightnessOptions MENU_DATA[] = {
    {"  LEVEL 1   ", 32  },
    {"  LEVEL 2   ", 64  },
    {"  LEVEL 3   ", 96  },
    {"  LEVEL 4   ", 128 },
    {"  LEVEL 5   ", 160 },
    {"  LEVEL 6   ", 192 },
    {"  LEVEL 7   ", 224 },
    {"  LEVEL 8   ", 255 },
    {"  AUTO      ", 255 } // PLACEHOLDER FOR ALR IMPLEMENTATION
};

class Brightness {
    private:
        int currentIndex;

        int getCount() {
            return sizeof(MENU_DATA) / sizeof(MENU_DATA[0]);
        }

    public:
        Brightness() {
            currentIndex = 7;
        }

        void onKnobTurn(int direction) {
            currentIndex += direction;
            if (currentIndex < 0) currentIndex = getCount() - 1;
            if (currentIndex >= getCount()) currentIndex = 0;
        }

        void cancel(uint8_t originalIndex) {
            currentIndex = originalIndex;
        }

        void onButtonPress() {

        }
        
        String getDisplayString() {
            return String(MENU_DATA[currentIndex].level);
        }

        uint8_t getSelectedBrightness() {
            return MENU_DATA[currentIndex].value;
        }

        uint8_t getSelectedIndex() {
            return currentIndex;
        }
};

#endif