#include "m5stack_hbridge.h"
#include "esphome/core/log.h"
#include "esphome/core/hal.h"

namespace esphome {
namespace m5stack_hbridge {

static const char *TAG = "m5stack_hbridge";

void M5StackHBridge::setup() {
  ESP_LOGCONFIG(TAG, "Setting up M5Stack H-Bridge...");
  // Initialize the H-Bridge
  if (!driver.begin(&Wire, address_, sda_, scl_, speed_)) {
    ESP_LOGE(TAG, "H-Bridge not found");
  }
}

void M5StackHBridge::loop() {
  // Loop function for periodic tasks
}

void M5StackHBridge::set_direction(hbridge_direction_t direction) {
  driver.setDriverDirection(direction);
}

void M5StackHBridge::set_speed(uint8_t speed) {
  driver.setDriverSpeed8Bits(speed);
}

void M5StackHBridge::set_i2c_pins(uint8_t sda, uint8_t scl) {
  sda_ = sda;
  scl_ = scl;
}

void M5StackHBridge::set_i2c_address(uint8_t address) {
  address_ = address;
}

void M5StackHBridge::set_i2c_speed(uint32_t speed) {
  speed_ = speed;
}

// Define the CONFIG_SCHEMA for the m5stack_hbridge component
static const auto &schema = i2c::I2CDevice::create_schema(
  i2c::I2CDevice::CONFIG_SCHEMA
    .required("id", std::string("my_hbridge"))
    .optional("sda", uint8_t(21))
    .optional("scl", uint8_t(22))
    .optional("i2c_addr", uint8_t(0x20))
    .optional("speed", uint32_t(100000L))
);

}  // namespace m5stack_hbridge
}  // namespace esphome
