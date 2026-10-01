#ifndef NOTIFICATION_H
#define NOTIFICATION_H

//////////////////////////////////////////////////////////////
// INCLUDES  /////////////////////////////////////////////////
//////////////////////////////////////////////////////////////

#include <Arduino.h>                                        // Base arduino library

class Notification {
    private:
        unsigned long long lastFlash;
        bool flashState;
    public:
        Notification() {
            lastFlash = 0;
            flashState = false;
        }

        String getNotificationDisplay(const char* notificationName) {
            if (millis() - lastFlash > 200) {
                flashState = !flashState;                   // Flashing alert
                lastFlash = millis();
            }

            char notifBuffer[25];
            if (flashState) {                               // With flash
                snprintf(notifBuffer, sizeof(notifBuffer), "##%s#########", notificationName);
                return notifBuffer;
            } else {                                        // Without flash
                snprintf(notifBuffer, sizeof(notifBuffer), "  %s", notificationName);
                return notifBuffer;
            }
        }
};

#endif