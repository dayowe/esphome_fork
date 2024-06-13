#ifndef HBRIDGEV11_I2C_H
#define HBRIDGEV11_I2C_H

#include "esphome.h"
#include "Wire.h"

class HBridgeV11I2C : public Component, public PollingComponent {
 public:
  HBridgeV11I2C() : PollingComponent(15000) {}  // Polling every 15 seconds

  void setup() override {
    // Initialize I2C
    Wire.begin();
  }

  void update() override {
    // This function will be called every 'update_interval' milliseconds.
    // Add code here to periodically read/write from/to the I2C device.
  }

  void set_motor_direction(uint8_t direction, uint8_t duty_cycle) {
    // Example: write the direction and duty cycle to the driver config register
    Wire.beginTransmission(0x20);  // Replace with your I2C address
    Wire.write(0x00);  // Driver config register
    Wire.write(direction);
    Wire.write(duty_cycle);  // 8-bit PWM duty cycle
    Wire.write(0x00);  // Rest of the data
    Wire.endTransmission();
  }

  void stop_motor() {
    // Stop the motor
    Wire.beginTransmission(0x20);  // Replace with your I2C address
    Wire.write(0x00);  // Driver config register
    Wire.write(0x00);  // Stop direction
    Wire.write(0x00);  // 0% duty cycle
    Wire.write(0x00);  // Rest of the data
    Wire.endTransmission();
  }
};

#endif // HBRIDGEV11_I2C_H
