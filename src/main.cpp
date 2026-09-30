/*
  PROJECT:      ESP32 Network Clock
  AUTHOR:       Nathaniel Kessel
  DEVICE:       ESP32-S3
  DATE:         2026-09-28
  VERSION:      pre-release
*/

////////////////////////////////////////////////////////////
// INCLUDES ////////////////////////////////////////////////
////////////////////////////////////////////////////////////

// Arduino Headers /////////////////////////////////////////
#include <Arduino.h>                                      // Basic arduino functions and classes
#include <WiFi.h>                                         // Arduino WiFi header/resources
#include <SPI.h>                                          // Arduino SPI header, used for the button panel and 74HC595 display
#include <Wire.h>                                         // I2C header for communitaction with the RTC

// ESP32 Headers ///////////////////////////////////////////
#include <esp_timer.h>                                    // Used for encoder debounce in setupEncoderTimer

// C++ Libraries ///////////////////////////////////////////
#include <string>                                         // Used for routing keyboard output

// My Headers //////////////////////////////////////////////

// Display Specific Headers
#include "display_font.h"                                 // Character segment mappings, display fonts, and final display buffer construction
#include "selection_util.h"                               // Provides the flashing cursor used in the alarm configuration menu
#include "time_util.h"                                    // Time formatting for display, handles 24hr/12hr time and padding

// Features
#include "features/stopwatch.h"                           // Stopwatch class for the stopwatch feature
#include "features/alarm.h"                               // Alarm class for the alarm feature
#include "features/timezone.h"                            // Timezone class and data for the timezone configuration menu
#include "features/notification.h"                        // Notification class for flashing notification pop ups triggered by alarms and timers
#include "features/timer.h"                               // Configurable timer class
#include "features/clock.h"                               // Clock class for the clock page's logic 
#include "features/keyboard.h"                            // Text input class, primarily used for networking SSID/Password inputs
#include "features/networking.h"                          // Networking class for setting up WiFi
#include "features/weather.h"

// Settings
#include "settings/brightness.h"                          // Brightness settings menu class
#include "settings/set_time.h"                            // Manual time configuration class

// Classes
TimeUtil timeUtil;
Stopwatch stopwatchTool;
Alarm alarmTool;
TimeZoneSetting tzTool;
Notification notifTool;
Timer timerTool;
Clock clockTool;
Brightness brightnessTool;
KeyboardInput keyboardTool(32);
Networking networkTool;
SetTime setTimeTool;
Weather weatherTool;
////////////////////////////////////////////////////////////
// DEBUG ///////////////////////////////////////////////////
////////////////////////////////////////////////////////////
bool pmrEnable = true;                                    // Enable the per minute report

////////////////////////////////////////////////////////////
// STATES //////////////////////////////////////////////////
////////////////////////////////////////////////////////////
enum SystemState {
  CLOCK_CLEAN,                                            // Clock screen without the navigation context
  NAV_MODE,                                               // Main menu navigation
  TZ_SELECT,                                              // Time zone configuration menu
  STOPWATCH,                                              // Simple stopwatch
  ALARM,                                                  // Configurable alarm with day-specific repeats, 3 alarms
  MODE_TIMER,                                             // Configurable timer, up to three timers
  NOTIFICATION,                                           // Notification pop ups
  SETTINGS,                                               // Settings submenu, contains TZ_SELECT, BRIGHTNESS, and NETWORK_MENU
  BRIGHTNESS,                                             // Brightness configuration, 8 levels + auto
  KEYBOARD_ENTRY,                                         // Keyboard used for network config
  NETWORK_MENU,                                           // Network scan and config menu
  MANUAL_TIME,                                            // Manual time config screen
  WEATHER_SCREEN,                                         // Weather screen
  WEATHER_MENU                                            // Weather configuration
};
SystemState currentState = CLOCK_CLEAN;                   // Have the clock start at the CLOCK_CLEAN page
SystemState lastState = currentState;

unsigned long menuTimeout = 0;

////////////////////////////////////////////////////////////
// GLOBALS /////////////////////////////////////////////////
////////////////////////////////////////////////////////////

// Display /////////////////////////////////////////////////
uint64_t toDisplayWords[12];                              // Initializes the array of 64-bit integers containing the segments and decimal point, mux bits, and status LED bits
unsigned long lastUpdate = 0;                             // Time since last screen update

// Clock ///////////////////////////////////////////////////
bool hour24;                                              // Boolean for handling if the clock is in 24-hour (true) or 12-hour mode (false)

// Notifications ///////////////////////////////////////////
int activeNotification = -1;                              // Set no current notifications

// Indicator Light Status //////////////////////////////////
int WiFiLight =  0;                                       // Default WiFi indicator to off
int alarmLight = 0;                                       // Default alarm light to off
int timerLight = 0;                                       // Default timer light to off

// Encoder /////////////////////////////////////////////////
volatile long encoderRawCount = 0;                        // Init the number of pulses from the PEC11R
long lastEncoderRead = 0;                                 // Since last encoder read timer
bool lastEncState = false;                                // Init the previous encoder movement
volatile bool encoderMoved = false;                       // Init tracking if the encoder moved recently
int timeLastPressed = 0;                                  // Track time since the last encoder press for debounce
int encoderDebug_timeLastPressed = 0;                     // Debug for encoder press

// Buttons /////////////////////////////////////////////////
bool homeButtonPressed =  false;
bool modButtonPressed  =  false;
bool aButtonPressed    =  false;
bool bButtonPressed    =  false;
bool cButtonPressed    =  false;
bool dButtonPressed    =  false;
bool eButtonPressed    =  false;

// Menu ////////////////////////////////////////////////////
volatile int menuIndex = 0;                               // Menu index tracking
int keyboardMode = 0;                                     // Track if you are in a keyboard input

// Weather /////////////////////////////////////////////////
String tempWeatherApi = "";
String tempWeatherLat = "";

// Brightness //////////////////////////////////////////////
uint8_t originalBrightness;                               // Tracks what the current brightness level is
uint8_t originalBrightnessIndex;                          // Tracks where in the brightness menu it is   

////////////////////////////////////////////////////////////
// PIN DEFINITIONS /////////////////////////////////////////
////////////////////////////////////////////////////////////
constexpr int PIN_COPI          =       13;               // SPI for the display
constexpr int PIN_LATCH         =       6;                // SPI RCLK for the display
constexpr int PIN_OE            =       4;                // 75HC595 display output enable, PWM control for brightness
constexpr int PIN_SCK           =       12;               // SPI clock for the display

// Peripherals /////////////////////////////////////////////
constexpr int PIN_LIGHT         =       7;                // Phototransistor
constexpr int PIN_MFP           =       8;                // External RTC 
constexpr int PIN_SCL           =       5;                // External RTC
constexpr int PIN_SDA           =       38;               // External RTC

// Encoder /////////////////////////////////////////////////
constexpr int PIN_ENCODER_PUSH  =       15;               // Encoder push button
constexpr int PIN_ENCODER_A     =       9;                // Encoder's pin A for rotation tracking
constexpr int PIN_ENCODER_B     =       10;               // Encoder's pin B for rotation tracking

// Buttons /////////////////////////////////////////////////
constexpr int PIN_COPI_BUTTON   =       14;               // SPI for the button panel
constexpr int PIN_LATCH_BUTTON  =       1;                // SPI RCLK for the button panel
constexpr int PIN_SCK_BUTTON    =       11;               // SPI clock for the button panel

////////////////////////////////////////////////////////////
// SPI  ////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
SPIClass buttonSPI(HSPI);                                 // second SPI instance for the buttons with the ESP32's HSPI bus

SPISettings srSettings(4000000, MSBFIRST, SPI_MODE0);     // Set the display's SPI settings
SPISettings btSettings(4000000, MSBFIRST, SPI_MODE2);     // Set the button panel's SPI settings 

////////////////////////////////////////////////////////////
// ENCODER LOGIC ///////////////////////////////////////////
////////////////////////////////////////////////////////////

// Lookup table ////////////////////////////////////////////
static int8_t DRAM_ATTR encoder_states[] = {              // Quadrature encoder lookup table, stored in DRAM
  0, -1,  1,  0,
  1,  0,  0, -1,
 -1,  0,  0,  1,
  0,  1, -1,  0
};

// Interrupt Handler ///////////////////////////////////////
void IRAM_ATTR onTimer(void* arg) {                       // Store in internal RAM
  static uint8_t old_AB = 0;                              // init old_AB at 0 to signal no change at boot
  old_AB <<= 2;                                           // Move last check's bits over
  old_AB |= (digitalRead(PIN_ENCODER_A) << 1) | digitalRead(PIN_ENCODER_B);     
                                                          // Read the pin states and combine them to get the index for the lookup table
  old_AB &= 0x0f;                                         // Ensure max of 4 bits

  int change = encoder_states[old_AB];                    // Looks at what old_AB means, interpret if there was a change
  if (change != 0) {
    encoderRawCount += change;                            // Increment encoderRawCount by the value stored in encoder_states
    encoderMoved = true;                                  // Signal that the encoder changed so other functions can tell
  }
}

// Interrupt Timer /////////////////////////////////////////
void setupEncoderTimer() {                                                      
  const esp_timer_create_args_t periodic_timer_args = {
    .callback = &onTimer,                                 // Call the onTimer function every time the timer ticks
    .name = "encoder_timer"
  };
  esp_timer_handle_t encoder_timer;
  esp_timer_create(&periodic_timer_args, &encoder_timer); // Store the handle and timer parameters in memory
  esp_timer_start_periodic(encoder_timer, 1000);          // Start the timer, running forever, triggering once per millisecond
}

////////////////////////////////////////////////////////////
// BUTTON LOGIC ////////////////////////////////////////////
////////////////////////////////////////////////////////////

// Button States Check /////////////////////////////////////
uint8_t pullButtonStates() {                              // Pull all of the buttons current status from the 74HC165 
  digitalWrite(PIN_LATCH_BUTTON, LOW);                    // Injest button states to the register
  delayMicroseconds(1);
  digitalWrite(PIN_LATCH_BUTTON, HIGH); 
  delayMicroseconds(1);
  
  buttonSPI.beginTransaction(btSettings);                 // Read register data
  uint8_t buttonStates = buttonSPI.transfer(0);
  buttonSPI.endTransaction();
  return buttonStates; 
}

// Check a Button //////////////////////////////////////////
bool checkButton(uint8_t buttonStates, int bitIndex) {
  return (buttonStates & (1 << bitIndex)) != 0;           
}

// Display Data Refresh ////////////////////////////////////
bool buttonDetect(bool buttonPressed, unsigned long now) {// Software debounced button detection
  if (buttonPressed && (now - timeLastPressed > 250)) {
    return true; 
  }
  return false;
}

////////////////////////////////////////////////////////////
// Display Handling ////////////////////////////////////////
////////////////////////////////////////////////////////////

// Display Data Refresh ////////////////////////////////////
static void latchPulse() {                                // Refresh the data in the registers                             
  digitalWrite(PIN_LATCH, HIGH);                          // Output current data to display
  delayMicroseconds(1);
  digitalWrite(PIN_LATCH, LOW);                           // Lock new data from being sent to the display
  delayMicroseconds(1);
}

// Display Data ransfer ///////////////////////////////////
void spiWrite64(uint64_t data) {  
  uint32_t high = (uint32_t)(data >> 32);                 // Break the MSB component off of the data
  uint32_t low = (uint32_t)(data & 0xFFFFFFFF);           // Break the LSB component off of the data

  digitalWrite(PIN_LATCH, LOW);                           // Hide new data from the display
  SPI.beginTransaction(srSettings);
  SPI.transfer32(high);                                   // Send MSB data
  SPI.transfer32(low);                                    // Send LSB data
  SPI.endTransaction();
  asm volatile("nop;nop;nop;nop");                        // Very short delay to allow shift registers to ingest
  latchPulse();                                           // Refresh data thats being sent to the display
}

// Display Render //////////////////////////////////////////
void renderDisplay(uint64_t* currentBuffer) {
  for (int i = 0; i < 12; i++) {                          // Loop through all of the characters (multiplex)
    spiWrite64(currentBuffer[i]);                         // Write current characters data (its anode and its char's segments)
    delayMicroseconds(180);                               // Multiplexing speed (refresh rate)
    spiWrite64(0);  
    delayMicroseconds(20);                                // Blanking interval
  }
}

// Brightness //////////////////////////////////////////////
void setDisplayBrightness(uint8_t brightnessLevel) {
    uint8_t hardwareDuty = 255 - brightnessLevel;

    ledcWrite(0, hardwareDuty);                           // Send current duty cycle to PWM control
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

            /////////   /////////   /////////   ///   ///   /////////
            ///         ///            ///      ///   ///   ///   ///
//////////  /////////   /////////      ///      ///   ///   /////////   ////////////////////////////////////////////////////////
                  ///   ///            ///      ///   ///   ///
            /////////   /////////      ///      /////////   ///

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void setup() {
  Serial.begin(115200);                                                         // START SERIAL MONITOR AT BAUD RATE 115200
  Serial.println("DBG 001 SETUP: Serial Monitor Online");
  Serial.println("DBG 000 SETUP: Setup begin");
  uint32_t start = millis();
  while (!Serial && (millis() - start < 3000)) {
    delay(10); 
  }
  
  

  pinMode(PIN_ENCODER_PUSH, INPUT);                                      // DEFINE ENCODER BUTTON AS INPUT
  if (digitalRead(PIN_ENCODER_PUSH) == LOW) {
    Serial.println("DBG 0F0 RESET: Factory reset");
    displayBuilder(" RESETTING  ", toDisplayWords, false);
    
    alarmTool.begin();      
    alarmTool.factoryReset();
    networkTool.factoryReset();
    
    unsigned long startReset = millis();
    while (millis() - startReset < 2000) {
      renderDisplay(toDisplayWords); 
    }
  }
  alarmTool.begin();
  timerTool.begin();
  networkTool.begin();
  pinMode(PIN_LATCH, OUTPUT);                                                   // DEFINE LATCH AS OUTPUT
  Serial.println("DBG 002 SETUP: Display latch pin.");
  pinMode(PIN_LIGHT, ANALOG);
  Serial.println("DBG 003 SETUP: Light sense pin.");
  weatherTool.begin();

  const int oeChannel = 0; 
  ledcSetup(oeChannel, 5000, 8);
  ledcAttachPin(PIN_OE, oeChannel);                                                  // DEFINE OE AS OUTPUT
  Serial.println("DBG 004 SETUP: Display Output Enable Pin (Brightness).");

  setDisplayBrightness(brightnessTool.getSelectedBrightness(analogRead(PIN_LIGHT)));
  originalBrightness = brightnessTool.getSelectedBrightness(analogRead(PIN_LIGHT));
  Serial.println("DBG 020 CALIB: Brightness initial callibration.");

  SPI.begin(PIN_SCK, -1, PIN_COPI, PIN_LATCH);                                  // INDICATE WHAT PINS ARE WHICH TO SPI FUNCTIONS
  Serial.println("DBG 007 SETUP: Display SPI configurated");
  pinMode(PIN_LATCH_BUTTON, OUTPUT);
  digitalWrite(PIN_LATCH_BUTTON, HIGH); 

  buttonSPI.begin(PIN_SCK_BUTTON, PIN_COPI_BUTTON, 16, -1);
  Serial.println("DBG 008 SETUP: Button SPI configurated");

  initFontTable();                                                              // BRING FONT TABLE INTO MEMORY
  pinMode(PIN_ENCODER_A, INPUT_PULLUP);                                         // DEFINE ENCODER ROTATION DETECTION
  pinMode(PIN_ENCODER_B, INPUT_PULLUP);               
  Serial.println("DBG 009 SETUP: Encoder rotation pins configured");

  setupEncoderTimer();                                                          // START TIMER FOR DEBOUNCE
  Serial.println("DBG 00A SETUP: Encoder SW Debounce configured");
  
  displayBuilder("  NTP SYNC  ", toDisplayWords, false);
  // timeUtil.initTime("EST5EDT");                                                          // DEFAULT TO EST TIME ZONE AND SYNC TIME
  Wire.begin(PIN_SDA, PIN_SCL);
  clockTool.begin();
  Serial.println("DBG 002 SETUP: End of setup");
}

            ///////////////   /////////   /////////   /////////
            ///   ///   ///   ///   ///      ///      ///   ///
//////////  ///   ///   ///   /////////      ///      ///   ///   //////////////////////////////////////////////////////////////
            ///   ///   ///   ///   ///      ///      ///   ///
            ///   ///   ///   ///   ///   /////////   ///   ///

void loop() {
  unsigned long now = millis();                                                 // TIMESTAMP START OF LOOP
  // Per minute report
  static unsigned long timeSinceLastPMR = 0;
  if(pmrEnable && (now - timeSinceLastPMR > 60000)){
    Serial.println("DBG 000 PMINR | PER MINUTE REPORT");
    Serial.println("DBG 000 PMINR | -----------------");
    Serial.print(  "DBG 000 PMINR | UPTIME: ");           Serial.println(millis());
    Serial.print(  "DBG 000 PMINR | BRIGHTNESS: ");       Serial.println(brightnessTool.getSelectedBrightness(analogRead(PIN_LIGHT)));
    Serial.print(  "DBG 000 PMINR | WIFI: ");             Serial.println(networkTool.isConnected());
    timeSinceLastPMR = now;
  }



  if (lastState != currentState) {
    Serial.print("DBG 030 STATE: State change to ");
    Serial.println(SystemState(currentState));
  }
  lastState = currentState;
  uint16_t lightSensorData = analogRead(PIN_LIGHT);

  bool hasMoved = encoderMoved;

  if (hasMoved) {
    encoderMoved = false;
  }
  bool buttonPressed = (digitalRead(PIN_ENCODER_PUSH) == LOW);                  // DETERMINE STATE OF ENCODER BUTTON

  if (buttonPressed && (now - encoderDebug_timeLastPressed > 250)) {
    Serial.println("DBG 010 INPUT: Encoder Button Detect");
    encoderDebug_timeLastPressed = now;
  }


  uint8_t currentButtonStates = pullButtonStates();

  bool homeButtonPressed = (checkButton(currentButtonStates, 0) == false);
  bool modButtonPressed  = (checkButton(currentButtonStates, 1) == false);
  bool aButtonPressed    = (checkButton(currentButtonStates, 2) == false);
  bool bButtonPressed    = (checkButton(currentButtonStates, 3) == false);
  bool cButtonPressed    = (checkButton(currentButtonStates, 4) == false);
  bool dButtonPressed    = (checkButton(currentButtonStates, 5) == false);
  bool eButtonPressed    = (checkButton(currentButtonStates, 6) == false);

  WiFiLight = networkTool.isConnected() ? 1 : 0;

  static bool timeInitialized = false;
  if (WiFiLight == 1 && !timeInitialized) {
      Serial.println("DBG 095 NETWK: WiFi Connected, starting NTP sync");
      timeUtil.initTime("EST5EDT");
      timeInitialized = true;
  }

  // 1. INPUTS
  if (hasMoved) {
    menuTimeout = now; 
    
    long movement = encoderRawCount / 4;
    

    if (currentState == CLOCK_CLEAN) {
        if (movement != lastEncoderRead) {
            currentState = NAV_MODE;
            menuIndex = 0;
            
            lastEncoderRead = movement; 
        }
    } 

    else if (currentState == NAV_MODE || currentState == SETTINGS || currentState == WEATHER_MENU) {
        if (movement != lastEncoderRead) {
            if (movement > lastEncoderRead) menuIndex++; else menuIndex--;
            lastEncoderRead = movement;
        }
    }

    else if (currentState == ALARM) {
      if (movement != lastEncoderRead) {
        int direction = (movement > lastEncoderRead) ? 1 : -1;
              
        alarmTool.onKnobTurn(direction);
              
        lastEncoderRead = movement;
      }
    }

    else if (currentState == TZ_SELECT) {
        if (movement != lastEncoderRead) {
            int direction = (movement > lastEncoderRead) ? 1 : -1;

            tzTool.onKnobTurn(direction);

            lastEncoderRead = movement;
        }
    }

    else if (currentState == BRIGHTNESS) {
        if (movement != lastEncoderRead) {
            int direction = (movement > lastEncoderRead) ? 1 : -1;

            brightnessTool.onKnobTurn(direction);

            lastEncoderRead = movement;
        }
    }

    else if (currentState == MODE_TIMER) {
        if (movement != lastEncoderRead) {
            int direction = (movement > lastEncoderRead) ? 1 : -1;

            timerTool.onKnobTurn(direction);

            lastEncoderRead = movement;
        }
    }

    else if (currentState == KEYBOARD_ENTRY) {
        if (movement != lastEncoderRead) {
            int direction = (movement > lastEncoderRead) ? 1 : -1;
            keyboardTool.onKnobTurn(direction);
            lastEncoderRead = movement;
        }
    }

    else if (currentState == NETWORK_MENU) {
        if (movement != lastEncoderRead) {
            int direction = (movement > lastEncoderRead) ? 1 : -1;
            networkTool.onKnobTurn(direction);
            lastEncoderRead = movement;
        }
    }

    else if (currentState == MANUAL_TIME) {
        if (movement != lastEncoderRead) {
            int direction = (movement > lastEncoderRead) ? 1 : -1;
            setTimeTool.onKnobTurn(direction);
            lastEncoderRead = movement;
        }
    }
    
    menuTimeout = now;
  }

  // 2. LOGIC (50ms gate)
  static unsigned long lastLogic = 0;                                                                 // INIT LAST LOGIC CHAGE
  static int logicRefreshSpeed = 50;

  if (currentState == STOPWATCH) {
    if (logicRefreshSpeed == 50) {Serial.println("DBG 061 REFRE: Logic refresh timing changed to 20ms");}
    logicRefreshSpeed = 20; 
  } else {
    logicRefreshSpeed = 50; 
  }

  if (now - lastLogic > logicRefreshSpeed) {                                                                         // IF ITS BEEN 50ms
    lastLogic = now;                                                                                  // TIMESTAMP LAST LOGIC

    if (brightnessTool.getSelectedIndex() == 8) {
      setDisplayBrightness(brightnessTool.getSelectedBrightness(lightSensorData));
    }
    // Timeout
    unsigned long timeoutDuration = 10000; //= (currentState == ALARM) ? 20000 : ((currentState == NAV_MODE) ? 10000 : 5000);                              // IF ON SETTINGS MENU SET TIMEOUT TO 10s, IF ON CLOCK SET TIMEOUT TO 5s, if in alarm settings 20s
    if (currentState == ALARM || currentState == MANUAL_TIME || currentState == WEATHER_MENU) {timeoutDuration = 20000;}
    else if (currentState == NAV_MODE) {timeoutDuration = 5000;}
    else if (currentState == SETTINGS) {timeoutDuration = 20000;}
    

    if (currentState != CLOCK_CLEAN && currentState != STOPWATCH && currentState != NOTIFICATION && currentState != MODE_TIMER && currentState != KEYBOARD_ENTRY && currentState != NETWORK_MENU &&(now - menuTimeout > timeoutDuration)) {                       // IF NOT ON THE CLEAN CLOCK PAGE AND ITS BEEN LONGER THAN TIMEOUT GO TO CLOCK PAGE
      if (currentState == ALARM) {
          alarmTool.save();
      }
      currentState = CLOCK_CLEAN;
      alarmTool.reset();
    }

    if (buttonDetect(homeButtonPressed, now) && (currentState != KEYBOARD_ENTRY)) {
      Serial.println("DBG 011 INPUT: Home Button Detect");
      timeLastPressed = now;
      alarmTool.reset();
      timerTool.reset();
      stopwatchTool.reset();
      clockTool.onHomeButtonPress();
      if (currentState == BRIGHTNESS) {
        setDisplayBrightness(originalBrightness);
        brightnessTool.cancel(originalBrightnessIndex);
        currentState = CLOCK_CLEAN;
      } else {
        currentState = CLOCK_CLEAN;
      }
    }

    static unsigned long lastNotifCheck = 0;
    if (now - lastNotifCheck >= 1000) {
      lastNotifCheck = now;

      if (currentState != NOTIFICATION) {
        if (alarmTool.shouldRing(0)) {
          activeNotification = 0;
          currentState = NOTIFICATION;
          Serial.println("80 NOTIF: Alarm Notification 0");
        } else if (alarmTool.shouldRing(1)) {
          activeNotification = 1;
          currentState = NOTIFICATION;
          Serial.println("81 NOTIF: Alarm Notification 1");
        } else if (alarmTool.shouldRing(2)) {
          activeNotification = 2;
          currentState = NOTIFICATION;
          Serial.println("82 NOTIF: Alarm Notification 2");
        } else if (timerTool.shouldRing(1)) {
          activeNotification = 3;
          currentState = NOTIFICATION;
          Serial.println("83 NOTIF: Timer Notification 1");
        } else if (timerTool.shouldRing(2)) {
          activeNotification = 4;
          currentState = NOTIFICATION;
          Serial.println("84 NOTIF: Timer Notification 2");
        } else if (timerTool.shouldRing(3)) {
          activeNotification = 5;
          currentState = NOTIFICATION;
          Serial.println("85 NOTIF: Timer Notification 3");
        }
      }
      lastNotifCheck = now;
    }
    // State Machine
    switch (currentState) {
      case CLOCK_CLEAN:                                                                               // IF ON CLEAN CLOCK PAGE
        displayBuilder((char*)clockTool.getClockDisplay().c_str(), toDisplayWords, false);                // RETURNS BUILT toDisplayWords WITHOUT NAV ARROWS

        if (buttonPressed && (now - timeLastPressed > 250)) {                                         // IF THE BUTTON IS PRESSED AND AFTER 250ms
          lastEncState = !lastEncState;                                                               // SWAP BUTTON STATE (TOGGLE SWITCH)
          clockTool.onButtonPress();
          timeLastPressed = now;                                                                      // TIMESTAMP BUTTON PRESS
        }
        if (buttonDetect(modButtonPressed, now)) {
            timeLastPressed = now;
            clockTool.onModButtonPress();
            menuTimeout = now;
          }
        if (buttonDetect(eButtonPressed, now)) {      // <--- ADD THIS
            timeLastPressed = now;
            menuTimeout = now;
            currentState = WEATHER_SCREEN;
        }
        break;
      case WEATHER_SCREEN:
        displayBuilder((char*)weatherTool.getDisplayString().c_str(), toDisplayWords, false);
        
        // Auto-exit after 5 seconds, or if they press the encoder button/home button
        if ((now - menuTimeout > 5000) || buttonDetect(homeButtonPressed, now) || (buttonPressed && (now - timeLastPressed > 250))) {
            timeLastPressed = now;
            currentState = CLOCK_CLEAN;
        }
        break;
      case NAV_MODE:                                                                                  // IF ON NAV CLOCK PAGE
        if (menuIndex < 0) menuIndex = 4;                                                             // IF MENU IS LESS THAN 0 CORRECT TO 1
        if (menuIndex > 4) menuIndex = 0;                                                             // IF MENU IS MORE THAN 1 CORRECT TO 0

        if (menuIndex == 0) {                                                                         // IF ON TIME PAGE
          displayBuilder((char*)clockTool.getClockDisplay().c_str(), toDisplayWords, true);                                                                     // GET toDisplayWords FOR TIME WITH NAV ARROWS
          if (buttonPressed && (now - timeLastPressed > 250)) {                                         // IF THE BUTTON IS PRESSED AND AFTER 250ms
            lastEncState = !lastEncState;                                                               // SWAP BUTTON STATE (TOGGLE SWITCH)
            clockTool.onButtonPress();
            timeLastPressed = now;                                                                      // TIMESTAMP BUTTON PRESS
          }
          if (buttonDetect(modButtonPressed, now)) {
            timeLastPressed = now;
            clockTool.onModButtonPress();
            menuTimeout = now;
          }
          if (buttonDetect(eButtonPressed, now)) {      // <--- ADD THIS
            timeLastPressed = now;
            menuTimeout = now;
            currentState = WEATHER_SCREEN;
          }
        } else if (menuIndex == 1) {
          displayBuilder(" STOPWATCH  ", toDisplayWords, true);
          if (buttonPressed && (now - timeLastPressed > 250)) {                                       // IF BUTTON IS PRESSED
            currentState = STOPWATCH;                                                                 // GO TO TIME ZONE SELECT MENU
            timeLastPressed = now;                                                                    // TIMESTAMP BUTTON PRESS
          }
        } else if (menuIndex == 2) {
          displayBuilder(" TIMER      ", toDisplayWords, true);
          if (buttonDetect(buttonPressed, now)) {
            currentState = MODE_TIMER;

            timeLastPressed = now;
            menuTimeout = now;
          }
        } else if (menuIndex == 3) {
          displayBuilder(" ALARM     ", toDisplayWords, true);
          if (buttonPressed && (now - timeLastPressed > 250)) {
            timeLastPressed = now;
            currentState = ALARM;

            alarmTool.reset();

            encoderRawCount = 0;
            lastEncoderRead = 0;
          }
        } else if (menuIndex == 4) {
          displayBuilder(" SETTINGS   ", toDisplayWords, true);
          if (buttonDetect(buttonPressed, now)) {
            timeLastPressed = now;
            menuIndex = 0;
            currentState = SETTINGS;
          }
        } 
        break;

      case TZ_SELECT:
        displayBuilder((char*)tzTool.getDisplayString().c_str(), toDisplayWords, true);

        if (buttonPressed && (now - timeLastPressed > 250)) {
          timeUtil.initTime(tzTool.getSelectedPosix());
          currentState = CLOCK_CLEAN;
          timeLastPressed = now;
        }
        break;
        
      case BRIGHTNESS: {
        displayBuilder((char*)brightnessTool.getDisplayString().c_str(), toDisplayWords, true);
        setDisplayBrightness(brightnessTool.getSelectedBrightness(lightSensorData));
        
        if (buttonDetect(buttonPressed, now)) {
          setDisplayBrightness(brightnessTool.getSelectedBrightness(lightSensorData));
          currentState = SETTINGS;

          timeLastPressed = now;
          menuTimeout = now;
        }

        break;
      }
      case STOPWATCH: { 
        displayBuilder((char*)stopwatchTool.getFormattedTime().c_str(), toDisplayWords, false);

        if (buttonPressed && (now - timeLastPressed > 250)) {
          timeLastPressed = now; 
          stopwatchTool.toggle();
        }

        if (hasMoved) {
          currentState = NAV_MODE;
          stopwatchTool.reset();
          encoderMoved = false;
        }
        
        break;
      }
      case ALARM: {
        displayBuilder((char*)alarmTool.getAlarmDisplay(hour24).c_str(), toDisplayWords, true);

        if (buttonPressed && (now - timeLastPressed > 250)) {
          timeLastPressed = now;
          alarmTool.onButtonPress();
          menuTimeout = now;
        }

        break;
      }
      case MODE_TIMER: {
        displayBuilder((char*)timerTool.getTimerDisplay().c_str(), toDisplayWords, true);

        if (buttonDetect(buttonPressed, now)) {
          timeLastPressed = now;
          timerTool.onButtonPress();
          menuTimeout = now;
        }

        if (buttonDetect(modButtonPressed, now)) {
          timeLastPressed = now;
          timerTool.onModButtonPress();
          menuTimeout = now;
        }

        break;
      }
      case NOTIFICATION: {
        const char* message = "";
        switch (activeNotification) {
          case 0:
            message = "ALARM 1";
            break;
          case 1:
            message = "ALARM 2"; 
            break;
          case 2:
            message = "ALARM 3";
            break;
          case 3:
            message = "TIMER 1";
            break;
          case 4:
            message = "TIMER 2";
            break;
          case 5:
            message = "TIMER 3";
            break;
          
          };

        displayBuilder((char*)notifTool.getNotificationDisplay(message).c_str(), toDisplayWords, false);

        if (buttonPressed && (now - timeLastPressed > 250)) {
            activeNotification = -1;
            timeLastPressed = now;
            currentState = CLOCK_CLEAN;
        }
        break;
      }
      case SETTINGS: {
        if (menuIndex < 0) menuIndex = 4;
        if (menuIndex > 4) menuIndex = 0;

        if (menuIndex == 0) {
          displayBuilder(" TIME ZONE  ", toDisplayWords, true);                                       // IF NOT ON THE TIME PAGE THEN GET toDisplayWords FOR TIME ZONE OPTION
          if (buttonPressed && (now - timeLastPressed > 250)) {                                       // IF BUTTON IS PRESSED
            currentState = TZ_SELECT;                                                                 // GO TO TIME ZONE SELECT MENU
            
            timeLastPressed = now;                                                                    // TIMESTAMP BUTTON PRESS
            menuTimeout = now;                                                                        // TIMESTAMP TIMEOUT
          }
          
        } else if (menuIndex == 1) {
          displayBuilder(" BRIGHTNESS ", toDisplayWords, true);
          
          if (buttonDetect(buttonPressed, now)) {
            originalBrightness = brightnessTool.getSelectedBrightness(lightSensorData);
            originalBrightnessIndex = brightnessTool.getSelectedIndex();
            currentState = BRIGHTNESS;

            timeLastPressed = now;
            menuTimeout = now;
          }

        } else if (menuIndex == 2) {
          displayBuilder(" NETWORK    ", toDisplayWords, true);
          if (buttonDetect(buttonPressed, now)) {
            currentState = NETWORK_MENU;
            networkTool.reset();
            timeLastPressed = now;
            menuTimeout = now;
          }

        } else if (menuIndex == 3) {
          displayBuilder(" SET TIME   ", toDisplayWords, true);
          if (buttonDetect(buttonPressed, now)) {
            currentState = MANUAL_TIME;
            setTimeTool.reset();
            timeLastPressed = now;
            menuTimeout = now;
          }
        
        } else if (menuIndex == 4) {
          displayBuilder(" WEATHER    ", toDisplayWords, true);
          if (buttonDetect(buttonPressed, now)) {
            timeLastPressed = now;
            menuIndex = 0;
            currentState = WEATHER_MENU;
            menuTimeout = now;
          }
        }
        break;
      }
      case MANUAL_TIME: {
        displayBuilder((char*)setTimeTool.getDisplayString().c_str(), toDisplayWords, true);

        if (buttonPressed && (now - timeLastPressed > 250)) {
            timeLastPressed = now;
            menuTimeout = now;
            
            if (setTimeTool.onButtonPress()) { 
                clockTool.setManualTime(
                    setTimeTool.getYear(),
                    setTimeTool.getMonth(),
                    setTimeTool.getDay(),
                    setTimeTool.getHour(),
                    setTimeTool.getMinute()
                );
                currentState = SETTINGS;
            }
        }
        
        if (buttonDetect(modButtonPressed, now)) {
            timeLastPressed = now;
            currentState = SETTINGS;
        }
        break;
      }

      case KEYBOARD_ENTRY: {
        displayBuilder((char*)keyboardTool.getDisplayString().c_str(), toDisplayWords, false);

        if (buttonDetect(homeButtonPressed, now)) {
          timeLastPressed = now;
          std::string finalInput = keyboardTool.getEnteredString();
          if (keyboardMode == 1) { 
              networkTool.setTargetSSID(finalInput);
              keyboardTool.reset();
              keyboardMode = 2; 
          } else if (keyboardMode == 2) { 
              networkTool.setTargetPassword(finalInput);
              networkTool.connectToTarget();
              currentState = NETWORK_MENU;
              keyboardMode = 0; 
          } else if (keyboardMode == 3) {
              String apiStr = String(finalInput.c_str());
              apiStr.toLowerCase();
              weatherTool.saveApiKey(apiStr);
              
              currentState = WEATHER_MENU;
              menuIndex = 0;
              keyboardMode = 0; 
          } else if (keyboardMode == 4) {
              weatherTool.saveLat(String(finalInput.c_str()));
              currentState = WEATHER_MENU;
              menuIndex = 1;
              keyboardMode = 0; 
          } else if (keyboardMode == 5) {
              weatherTool.saveLon(String(finalInput.c_str()));
              currentState = WEATHER_MENU;
              menuIndex = 2;
              keyboardMode = 0; 
          } else { 
              currentState = CLOCK_CLEAN; 
          }
        }

        if (buttonPressed && (now - timeLastPressed > 250)) {
            timeLastPressed = now;
            keyboardTool.onMoveRight();
            menuTimeout = now;
        }

        if (buttonDetect(dButtonPressed, now)) {
            timeLastPressed = now;
            keyboardTool.onMoveRight();
            menuTimeout = now;
        }

        if (buttonDetect(eButtonPressed, now)) {
            timeLastPressed = now;
            keyboardTool.onMoveLeft();
            menuTimeout = now;
        }

        if (buttonDetect(modButtonPressed, now)) {
            timeLastPressed = now;
            keyboardTool.onModButton();
            menuTimeout = now;
        }

        if (buttonDetect(aButtonPressed, now)) {
            timeLastPressed = now;
            keyboardTool.onAButton();
            menuTimeout = now;
        }
        break;
      }

      case NETWORK_MENU: {
        NetworkMenuState netState = networkTool.getMenuState();
        
        // Only show the < > navigation arrows if we are actively selecting a network
        bool showArrows = (netState == NET_SELECT_SSID);

        displayBuilder((char*)networkTool.getDisplayString().c_str(), toDisplayWords, showArrows);

        static unsigned long statusTimer = 0;
        if (netState == NET_CONNECTED || netState == NET_FAILED) {
            if (statusTimer == 0) statusTimer = now;
            if (now - statusTimer > 2000) {
                if (netState == NET_FAILED) {
                    networkTool.reset();
                }
                currentState = CLOCK_CLEAN;
            }
        } else {
            statusTimer = 0;
        }

        // Handle button inputs
        if (buttonPressed && (now - timeLastPressed > 250)) {
            timeLastPressed = now;
            menuTimeout = now;
            
            if (netState == NET_INIT || netState == NET_FAILED || netState == NET_CONNECTED) {
                networkTool.onButtonPress();
            } 
            else if (netState == NET_SELECT_SSID) {
                networkTool.onButtonPress();
                
                keyboardTool.reset();
                if (networkTool.isCustomSelected()) {
                    keyboardMode = 1;
                } else {
                    keyboardMode = 2;
                }
                currentState = KEYBOARD_ENTRY;
            }
        }
        
        if (buttonDetect(modButtonPressed, now)) {
            timeLastPressed = now;
            currentState = SETTINGS;
        }
        break;
      }

      case WEATHER_MENU: {
        if (menuIndex < 0) menuIndex = 2;
        if (menuIndex > 2) menuIndex = 0;

        if (menuIndex == 0) {
          displayBuilder(" API KEY    ", toDisplayWords, true);
          if (buttonDetect(buttonPressed, now)) {
            currentState = KEYBOARD_ENTRY;
            keyboardTool.reset();
            keyboardMode = 3;
            timeLastPressed = now;
            menuTimeout = now;
          }
        } else if (menuIndex == 1) {
          displayBuilder(" LATITUDE   ", toDisplayWords, true);
          if (buttonDetect(buttonPressed, now)) {
            currentState = KEYBOARD_ENTRY;
            keyboardTool.reset();
            keyboardMode = 4;
            timeLastPressed = now;
            menuTimeout = now;
          }
        } else if (menuIndex == 2) {
          displayBuilder(" LONGITUDE  ", toDisplayWords, true);
          if (buttonDetect(buttonPressed, now)) {
            currentState = KEYBOARD_ENTRY;
            keyboardTool.reset();
            keyboardMode = 5;
            timeLastPressed = now;
            menuTimeout = now;
          }
        }

        if (buttonDetect(modButtonPressed, now)) {
            timeLastPressed = now;
            menuIndex = 4;
            currentState = SETTINGS;
        }
        break;
      }
    }
  }
  renderDisplay(toDisplayWords);                                                                      // RENDER CURRENT SCREEN STATE
}
