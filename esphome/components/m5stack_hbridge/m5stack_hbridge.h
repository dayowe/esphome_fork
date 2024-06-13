#ifndef __M5STACK_HBRIDGE_H
#define __M5STACK_HBRIDGE_H

#include "esphome/core/component.h"
#include "esphome/components/i2c/i2c.h"
#include "M5UnitHbridge.h"  // Include the M5UnitHbridge library

namespace esphome {
namespace m5stack_hbridge {

class M5StackHBridge : public Component, public i2c::I2CDevice {
 public:
  void setup() override {
    // Initialize the H-Bridge
    if (!driver.begin(&Wire, address_, sda_, scl_, speed_)) {
      ESP_LOGE("m5stack_hbridge", "H-Bridge not found");
    }
  }

  void loop() override {
    // Loop function for periodic tasks
  }

  void set_direction(hbridge_direction_t direction) {
    driver.setDriverDirection(direction);
  }

  void set_speed(uint8_t speed) {
    driver.setDriverSpeed8Bits(speed);
  }

  void set_i2c_pins(uint8_t sda, uint8_t scl) {
    sda_ = sda;
    scl_ = scl;
  }

  void set_i2c_address(uint8_t address) {
    address_ = address;
  }

  void set_i2c_speed(uint32_t speed) {
    speed_ = speed;
  }

 private:
  M5UnitHbridge driver;
  uint8_t sda_;
  uint8_t scl_;
  uint8_t address_ = HBRIDGE_I2C_ADDR;
  uint32_t speed_ = 100000L;
};

}  // namespace m5stack_hbridge
}  // namespace esphome

#endif  // __M5STACK_HBRIDGE_H
