#ifndef NETWORKING_H
#define NETWORKING_H

#include <Arduino.h>
#include <WiFi.h>
#include <Preferences.h>
#include <vector>
#include <string>
#include "selection_util.h"

enum NetworkMenuState {
    NET_INIT,           
    NET_SCANNING,       
    NET_SELECT_SSID,    
    NET_CONNECTING,     
    NET_CONNECTED,      
    NET_FAILED          
};

class Networking {
    private:
        Preferences prefs;
        
        std::string savedSSID;
        std::string savedPassword;
        
        std::vector<std::string> scannedNetworks;
        int listIndex;           
        int networkCount;        
        
        NetworkMenuState menuState;
        selectionUtility selector;
        
        unsigned long connectionStartTime;
        unsigned long connectedTime; 

    public:
        Networking();

        void begin();                           // Initialize saved creds
        void saveCredentials();                 
        void factoryReset();                    // Reset creds

        void onKnobTurn(int direction);         // Move through menu
        void onButtonPress();
        void reset();                           // Reset menu state 

        std::string getDisplayString();         
        NetworkMenuState getMenuState();        
        
        void setTargetSSID(std::string ssid);   // Set SSID
        void setTargetPassword(std::string pass);
                                                // Set password
        
        void startScan();                       // Scan for networks
        void connectToTarget();                 // Connect to network
        
        bool isConnected();
        std::string getCurrentSSID();
        bool isCustomSelected();                
};

#endif