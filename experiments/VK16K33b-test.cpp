#include <Arduino.h>
#include <Wire.h>

int I2C_SDA = 4;
int I2C_SCL = 5;
const uint8_t I2C_Address = 0x70; // Scan agrees 10.26

void setup() {
  Wire.begin(I2C_SDA, I2C_SCL, 400000);
  Serial.begin(115200);
  // Oscillator
  Wire.beginTransmission(I2C_Address);
    Wire.write(0x21);
    Wire.endTransmission();
  // Display On
  Wire.beginTransmission(I2C_Address);
    Wire.write(0x81);
    Wire.endTransmission();
  // Brightness
  Wire.beginTransmission(I2C_Address);
    Wire.write(0xEF);
    Wire.endTransmission();
}

void loop() {

}
