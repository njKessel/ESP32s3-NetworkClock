#include "keyboard.h"
#include <algorithm>

KeyboardInput::KeyboardInput(int maxLen) {
    maxLength = maxLen;
    reset();
}

void KeyboardInput::reset() {
    currentString = "";
    forceVisible = false;
    isLower = false;
    cursorPosition = 0;
    charIndex = 1; 
}

void KeyboardInput::syncCharIndex() {
    if (cursorPosition < currentString.length()) {
        char c = currentString[cursorPosition];
        const std::string& set = isLower ? lowerSet : upperSet;
        size_t pos = set.find(c);
        
        if (pos == std::string::npos) {
            const std::string& otherSet = isLower ? upperSet : lowerSet;
            pos = otherSet.find(c);
        }
        charIndex = (pos != std::string::npos) ? pos : 1;
    } else {
        charIndex = 1;
    }
}

void KeyboardInput::onKnobTurn(int direction) {
    charIndex += direction;
    int setSize = upperSet.length();
    
    if (charIndex < 0) charIndex = setSize - 1;
    if (charIndex >= setSize) charIndex = 0;
    
    if (cursorPosition < currentString.length()) {
        currentString[cursorPosition] = isLower ? lowerSet[charIndex] : upperSet[charIndex];
    }
}

void KeyboardInput::onModButton() {
    isLower = !isLower;
    if (cursorPosition < currentString.length()) {
        char c = currentString[cursorPosition];
        if (isLower && c >= 'A' && c <= 'Z') {
            currentString[cursorPosition] = c + 32;
        } else if (!isLower && c >= 'a' && c <= 'z') {
            currentString[cursorPosition] = c - 32;
        }
    }
}

void KeyboardInput::onAButton() {
    forceVisible = !forceVisible;
}

void KeyboardInput::onMoveLeft() {
    if (cursorPosition > 0) {
        cursorPosition--;
        syncCharIndex();
    }
}

void KeyboardInput::onMoveRight() {
    if (cursorPosition == currentString.length()) {
        if (currentString.length() < maxLength) {
            currentString += (isLower ? lowerSet[charIndex] : upperSet[charIndex]);
            cursorPosition++;
            charIndex = 1; 
        }
    } else {
        cursorPosition++;
        syncCharIndex();
    }
}

std::string KeyboardInput::getDisplayString() {
    std::string rawText = "";
    
    for (int i = 0; i <= currentString.length(); i++) {
        if (i == cursorPosition) {
            rawText += "[";
            if (i < currentString.length()) {
                rawText += (forceVisible ? currentString[i] : currentString[i]);
            } else {
                rawText += (isLower ? lowerSet[charIndex] : upperSet[charIndex]);
            }
            rawText += "]";
        } else if (i < currentString.length()) {
            rawText += (forceVisible ? currentString[i] : '*');
        }
    }
    
    size_t bracketPos = rawText.find('[');
    std::string windowedText = "";
    
    if (bracketPos < 4) {
        windowedText = std::string(4 - bracketPos, ' ') + rawText;
    } else {
        windowedText = rawText.substr(bracketPos - 4);
    }
    
    if (windowedText.length() < 12) {
        windowedText.append(12 - windowedText.length(), ' ');
    } else if (windowedText.length() > 12) {
        windowedText = windowedText.substr(0, 12);
    }
    
    return windowedText;
}

std::string KeyboardInput::getEnteredString() {
    std::string finalStr = currentString;
    if (cursorPosition == currentString.length()) {
        char provisional = isLower ? lowerSet[charIndex] : upperSet[charIndex];
        if (provisional != ' ') {
            finalStr += provisional;
        }
    }
    return finalStr;
}