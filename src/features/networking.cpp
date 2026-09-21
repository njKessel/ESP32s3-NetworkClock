#include "networking.h"

Networking::Networking() {
    menuState = NET_INIT;
    listIndex = 0;
    networkCount = 0;
    savedSSID = "";
    savedPassword = "";
}

void Networking::begin() {
    prefs.begin("wifi", false);
    savedSSID = prefs.getString("ssid", "").c_str();
    savedPassword = prefs.getString("pass", "").c_str();

    if (!savedSSID.empty()) {
        WiFi.begin(savedSSID.c_str(), savedPassword.c_str());
    }
}

void Networking::saveCredentials() {
    prefs.putString("ssid", savedSSID.c_str());
    prefs.putString("pass", savedPassword.c_str());
}

void Networking::factoryReset() {
    prefs.begin("wifi", false);
    prefs.clear();
    savedSSID = "";
    savedPassword = "";
    WiFi.disconnect(true, true);
}

void Networking::reset() {
    menuState = NET_INIT;
    listIndex = 0;
    scannedNetworks.clear();
}

void Networking::onKnobTurn(int direction) {
    if (menuState == NET_SELECT_SSID && !scannedNetworks.empty()) {
        listIndex += direction;
        
        int maxIndex = scannedNetworks.size() - 1;
        
        if (listIndex < 0) {
            listIndex = maxIndex;
        } else if (listIndex > maxIndex) {
            listIndex = 0;
        }
    }
}

void Networking::onButtonPress() {
    if (menuState == NET_INIT || menuState == NET_FAILED) {
        startScan();
    } else if (menuState == NET_SELECT_SSID) {
        savedSSID = scannedNetworks[listIndex];
    }
}

void Networking::startScan() {
    menuState = NET_SCANNING;
    scannedNetworks.clear();
    
    networkCount = WiFi.scanNetworks();
    
    if (networkCount > 0) {
        for (int i = 0; i < networkCount; ++i) {
            scannedNetworks.push_back(WiFi.SSID(i).c_str());
        }
    }
    
    scannedNetworks.push_back("CUSTOM");
    
    listIndex = 0;
    menuState = NET_SELECT_SSID;
}

void Networking::setTargetSSID(std::string ssid) {
    savedSSID = ssid;
}

void Networking::setTargetPassword(std::string pass) {
    savedPassword = pass;
}

void Networking::connectToTarget() {
    menuState = NET_CONNECTING;
    connectionStartTime = millis();
    
    WiFi.disconnect();
    WiFi.begin(savedSSID.c_str(), savedPassword.c_str());
}

bool Networking::isConnected() {
    return WiFi.status() == WL_CONNECTED;
}

std::string Networking::getCurrentSSID() {
    return savedSSID;
}

bool Networking::isCustomSelected() {
    if (scannedNetworks.empty()) return false;
    return scannedNetworks[listIndex] == "CUSTOM";
}

NetworkMenuState Networking::getMenuState() {
    if (menuState == NET_CONNECTING) {
        if (WiFi.status() == WL_CONNECTED) {
            if (millis() - connectionStartTime >= 1000) {
                menuState = NET_CONNECTED;
                connectedTime = millis(); 
                saveCredentials();
            }
        } else if (millis() - connectionStartTime > 10000) {
            menuState = NET_FAILED;
        }
    }
    return menuState;
}

std::string Networking::getDisplayString() {
    std::string text = "";
    std::string output = "";

    switch (menuState) {
        case NET_INIT:
            text = "FIND NET";
            break;
            
        case NET_SCANNING:
            text = "SCANNING";
            break;
            
        case NET_SELECT_SSID:
            if (!scannedNetworks.empty()) {
                text = scannedNetworks[listIndex];
                if (text.length() > 8) {
                    text = text.substr(0, 8);
                }
            } else {
                text = "NO NETS";
            }
            break;
            
        case NET_CONNECTING:
            text = "CONNECT";
            break;
            
        case NET_CONNECTED:
            text = "SUCCESS";
            break;
            
        case NET_FAILED:
            text = "FAILED";
            break;
    }

    // Pad text to perfectly center it within an 8-character window
    int padding = 8 - text.length();
    if (padding > 0) {
        int padLeft = padding / 2;
        int padRight = padding - padLeft;
        text = std::string(padLeft, ' ') + text + std::string(padRight, ' ');
    }

    output = "  " + text + "  ";

    return output;
}