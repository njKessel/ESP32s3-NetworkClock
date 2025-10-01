#include <Arduino.h>
#include <LiquidCrystal.h>

// put function declarations here:
const int rs = 15, en = 17, d7 = 12, d6 = 11, d5 = 10, d4 = 9;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

void setup() {
  lcd.begin(16,2);
  lcd.print("test!@#$%^&12345");
}

void loop() {
  // put your main code here, to run repeatedly:
}
