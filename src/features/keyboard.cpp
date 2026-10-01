//////////////////////////////////////////////////////////////
// INCLUDES  /////////////////////////////////////////////////
//////////////////////////////////////////////////////////////
#include "Arduino.h"                                        // Base arduino functions
#include "keyboard.h"                                       // Keyboard header
#include <algorithm>                                        // Used for string search functions

//////////////////////////////////////////////////////////////
// INITIALIZATION ////////////////////////////////////////////
//////////////////////////////////////////////////////////////
KeyboardInput::KeyboardInput(int maxLen) {
    maxLength = maxLen;
    reset();
}

void KeyboardInput::reset() {
    currentString = "";
    forceVisible = false;
    isLower = false;
    cursorPosition = 0;
    charIndex = 1;                                          // Default to index 1 (usually 'A' or 'a', skipping ' ')
}

//////////////////////////////////////////////////////////////
// CHARACTER MAP SYNC ////////////////////////////////////////
//////////////////////////////////////////////////////////////
void KeyboardInput::syncCharIndex() {
    if (cursorPosition < currentString.length()) {          // If cursor moves over an existing character
        char c = currentString[cursorPosition];
        const std::string& set = isLower ? lowerSet : upperSet;
        size_t pos = set.find(c);
        
        if (pos == std::string::npos) {                     // If char isn't in current case set, check the other
            const std::string& otherSet = isLower ? upperSet : lowerSet;
            pos = otherSet.find(c);
        }
        charIndex = (pos != std::string::npos) ? pos : 1;   // Snap the encoder rotation to match the existing char
    } else {
        charIndex = 1;                                      // If at the end of the string, reset back to 'A'/'a'
    }
}

//////////////////////////////////////////////////////////////
// INPUTS ////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////

// Encoder Rotation //////////////////////////////////////////
void KeyboardInput::onKnobTurn(int direction) {
    charIndex += direction;
    int setSize = upperSet.length();
    
    if (charIndex < 0) charIndex = setSize - 1;             // Wrap around character sets
    if (charIndex >= setSize) charIndex = 0;
    
    if (cursorPosition < currentString.length()) {          // Live-update the character if editing an existing slot
        currentString[cursorPosition] = isLower ? lowerSet[charIndex] : upperSet[charIndex];
    }
}

// Case Toggle Button ////////////////////////////////////////
void KeyboardInput::onModButton() {
    Serial.println("E3 KEYBD: Toggle case");
    isLower = !isLower;
    if (cursorPosition < currentString.length()) {
        char c = currentString[cursorPosition];
                                                            // Standard ASCII math to flip case on the fly
        if (isLower && c >= 'A' && c <= 'Z') {
            currentString[cursorPosition] = c + 32;
        } else if (!isLower && c >= 'a' && c <= 'z') {
            currentString[cursorPosition] = c - 32;
        }
    }
}

// Visibility Toggle /////////////////////////////////////////
void KeyboardInput::onAButton() {
    Serial.println("E2 KEYBD: Toggle input visibility");
    forceVisible = !forceVisible;                           // Used to reveal passwords instead of showing '*'
}

// Cursor Navigation /////////////////////////////////////////
void KeyboardInput::onMoveLeft() {
    if (cursorPosition > 0) {
        cursorPosition--;
        syncCharIndex();                                    // Keep encoder mapped to the newly highlighted char
    }
}

void KeyboardInput::onMoveRight() {
    if (cursorPosition == currentString.length()) {         // If at the end, lock in the character and advance
        if (currentString.length() < maxLength) {
            currentString += (isLower ? lowerSet[charIndex] : upperSet[charIndex]);
            cursorPosition++;
            charIndex = 1; 
        }
    } else {                                                // If editing the middle of a string, just move right
        cursorPosition++;
        syncCharIndex();
    }
}

//////////////////////////////////////////////////////////////
// DISPLAY FORMATTING ////////////////////////////////////////
//////////////////////////////////////////////////////////////
std::string KeyboardInput::getDisplayString() {
    std::string rawText = "";
    
    for (int i = 0; i <= currentString.length(); i++) {
        if (i == cursorPosition) {                          // Wrap the active cursor slot in brackets
            rawText += "[";
            if (i < currentString.length()) {
                rawText += (forceVisible ? currentString[i] : currentString[i]);
            } else {
                rawText += (isLower ? lowerSet[charIndex] : upperSet[charIndex]);
            }
            rawText += "]";
        } else if (i < currentString.length()) {            // Mask inactive characters if visibility is toggled off
            rawText += (forceVisible ? currentString[i] : '*');
        }
    }
    
                                                            // Viewport math to fit scrolling text on a 12-char display
    size_t bracketPos = rawText.find('[');
    std::string windowedText = "";
    
    if (bracketPos < 4) {                                   // Pad the left side if near the start of the string
        windowedText = std::string(4 - bracketPos, ' ') + rawText;
    } else {                                                // Keep the active brackets centered in the viewport
        windowedText = rawText.substr(bracketPos - 4);
    }
    
                                                            // Hard-cap the physical display width
    if (windowedText.length() < 12) {
        windowedText.append(12 - windowedText.length(), ' ');
    } else if (windowedText.length() > 12) {
        windowedText = windowedText.substr(0, 12);
    }
    
    return windowedText;
}

// Final Submission //////////////////////////////////////////
std::string KeyboardInput::getEnteredString() {
    std::string finalStr = currentString;
    if (cursorPosition == currentString.length()) {         // Capture the provisional character if they hit submit 
        char provisional = isLower ? lowerSet[charIndex] : upperSet[charIndex];
        if (provisional != ' ') {                           // Don't append trailing blanks
            finalStr += provisional;
        }
    }
    return finalStr;
}