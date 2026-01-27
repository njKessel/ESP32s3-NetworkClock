#ifndef NOTIFICATION_H
#define NOTIFICATION_H

#include <Arduino.h>

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
                flashState = !flashState;
                lastFlash = millis();
            }

            char notifBuffer[25];
            if (flashState) {
                snprintf(notifBuffer, sizeof(notifBuffer), "##%s#########", notificationName);
                return notifBuffer;
            } else {
                snprintf(notifBuffer, sizeof(notifBuffer), "  %s", notificationName);
                return notifBuffer;
            }
        }
};

#endif