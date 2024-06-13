#include "m5stack_hbridge.h"
#include "esphome/core/log.h"

namespace esphome {
namespace m5stack_hbridge {

static const char *TAG = "m5stack_hbridge";

void M5StackHBridge::setup() {
  ESP_LOGD(TAG, "Setting up M5Stack HBridge...");
  // Initialize I2C communication, set default direction and speed
}

void M5StackHBridge::loop() {
  // Implement loop logic if necessary
}

void M5StackHBridge::set_direction(hbridge_direction_t direction) {
  ESP_LOGD(TAG, "Setting direction: %d", direction);
  
  // Send direction command to I2C register 0x00
  uint8_t data[2] = {0x00, static_cast<uint8_t>(direction)};
  auto status = this->write_bytes(data, 2);
  if (!status) {
    ESP_LOGE(TAG, "Failed to set direction via I2C");
  }
}

void M5StackHBridge::set_speed(uint8_t speed) {
  ESP_LOGD(TAG, "Setting speed: %d", speed);
  
  // Send speed command to I2C register 0x01
  uint8_t data[3] = {0x00, 0x00, speed}; // 16 bits PWM duty cycle
  auto status = this->write_bytes(data, 3);
  if (!status) {
    ESP_LOGE(TAG, "Failed to set speed via I2C");
  }
}

void M5StackHBridge::set_i2c_pins(uint8_t sda, uint8_t scl) {
  sda_ = sda;
  scl_ = scl;
  ESP_LOGD(TAG, "Setting I2C pins: SDA=%d, SCL=%d", sda, scl);
}

void M5StackHBridge::set_i2c_address(uint8_t address) {
  i2c_address_ = address;
  ESP_LOGD(TAG, "Setting I2C address: 0x%02X", address);
}

void M5StackHBridge::set_i2c_speed(uint32_t speed) {
  i2c_speed_ = speed;
  ESP_LOGD(TAG, "Setting I2C speed: %d", speed);
}

}  // namespace m5stack_hbridge
}  // namespace esphome
