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

        void begin();                           
        void saveCredentials();                 
        void factoryReset();                    

        void onKnobTurn(int direction);
        void onButtonPress();
        void reset();                           

        std::string getDisplayString();         
        NetworkMenuState getMenuState();        
        
        void setTargetSSID(std::string ssid);
        void setTargetPassword(std::string pass);
        
        void startScan();                       
        void connectToTarget();                 
        
        bool isConnected();
        std::string getCurrentSSID();
        bool isCustomSelected();                
};

#endif