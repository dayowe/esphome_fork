#pragma once

#include "esphome/core/component.h"
#include "esphome/components/i2c/i2c.h"

namespace esphome {
namespace m5stack_hbridge {

enum hbridge_direction_t {
  HBRIDGE_FORWARD,
  HBRIDGE_BACKWARD,
  HBRIDGE_STOP
};

class M5StackHBridge : public Component, public i2c::I2CDevice {
 public:
  void setup() override;
  void loop() override;
  void set_direction(hbridge_direction_t direction);
  void set_speed(uint8_t speed);
  void set_i2c_pins(uint8_t sda, uint8_t scl);
  void set_i2c_address(uint8_t address);
  void set_i2c_speed(uint32_t speed);

 private:
  uint8_t sda_;
  uint8_t scl_;
  uint8_t i2c_address_;
  uint32_t i2c_speed_;
};

}  // namespace m5stack_hbridge
}  // namespace esphome