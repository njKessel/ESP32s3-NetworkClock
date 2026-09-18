#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <string>

class KeyboardInput {
private:
    std::string currentString;
    bool forceVisible;
    bool isLower;
    int cursorPosition;
    int charIndex;
    int maxLength;
    
    const std::string upperSet = " ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-._!?"; 
    const std::string lowerSet = " abcdefghijklmnopqrstuvwxyz0123456789-._!?"; 

    void syncCharIndex();

public:
    KeyboardInput(int maxLen = 32);
    
    void reset();
    
    void onKnobTurn(int direction);
    void onModButton();  
    void onAButton();    
    void onMoveLeft();  
    void onMoveRight();   
    
    std::string getDisplayString();
    std::string getEnteredString();
};

#endif