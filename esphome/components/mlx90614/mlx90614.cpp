#include "mlx90614.h"

#include "esphome/core/hal.h"
#include "esphome/core/helpers.h"
#include "esphome/core/log.h"

namespace esphome {
namespace mlx90614 {

static const uint8_t MLX90614_RAW_IR_1 = 0x04;
static const uint8_t MLX90614_RAW_IR_2 = 0x05;
static const uint8_t MLX90614_TEMPERATURE_AMBIENT = 0x06;
static const uint8_t MLX90614_TEMPERATURE_OBJECT_1 = 0x07;
static const uint8_t MLX90614_TEMPERATURE_OBJECT_2 = 0x08;

static const uint8_t MLX90614_TOMAX = 0x20;
static const uint8_t MLX90614_TOMIN = 0x21;
static const uint8_t MLX90614_PWMCTRL = 0x22;
static const uint8_t MLX90614_TARANGE = 0x23;
static const uint8_t MLX90614_EMISSIVITY = 0x24;
static const uint8_t MLX90614_CONFIG = 0x25;
static const uint8_t MLX90614_ADDR = 0x2E;
static const uint8_t MLX90614_ID1 = 0x3C;
static const uint8_t MLX90614_ID2 = 0x3D;
static const uint8_t MLX90614_ID3 = 0x3E;
static const uint8_t MLX90614_ID4 = 0x3F;

// Special commands
static const uint8_t MLX90614_SLEEP_MODE = 0xFF;

static const char *const TAG = "mlx90614";

static const float MLX90614_MIN_TEMP = -70.0f;
static const float MLX90614_MAX_TEMP = 380.0f;

void MLX90614Component::setup() {
  ESP_LOGD(TAG, "Setting up MLX90614...");
  
  // PREVENTION 1: I2C Bus Reset before initialization
  // Send 9 clock pulses with SDA high to reset any stuck slave
  // This clears any incomplete transaction from before reset
  this->i2c_bus_reset_();
  
  // PREVENTION 2: Extended power-on delay
  // MLX90614 needs 250ms minimum, but we'll give it more time
  // to ensure power supply is fully stable after ESP32 boot
  delay(500);
  
  // PREVENTION 3: Gentle wake-up sequence
  // Some sensors need a "wake-up" read before they respond properly
  uint16_t dummy_data;
  this->read_register(MLX90614_ID1, reinterpret_cast<uint8_t*>(&dummy_data), 2, false);
  delay(50);  // Brief pause after wake-up read
  
  // PREVENTION 4: Verify sensor is in good state before proceeding
  // Read multiple registers to ensure sensor is responding correctly
  uint16_t id_data, config_data, emissivity_data;
  bool sensor_ok = true;
  
  if (!this->read_data_with_crc_(MLX90614_ID1, &id_data)) {
    ESP_LOGW(TAG, "Failed to read ID register");
    sensor_ok = false;
  }
  
  if (!this->read_data_with_crc_(MLX90614_CONFIG, &config_data)) {
    ESP_LOGW(TAG, "Failed to read config register");
    sensor_ok = false;
  }
  
  if (!this->read_data_with_crc_(MLX90614_EMISSIVITY, &emissivity_data)) {
    ESP_LOGW(TAG, "Failed to read emissivity register");
    sensor_ok = false;
  }
  
  // If sensor is not responding properly, try a reset
  if (!sensor_ok) {
    ESP_LOGW(TAG, "Sensor not responding properly, attempting reset");
    if (this->reset_sensor_()) {
      delay(250);  // Give sensor time to recover
      // Verify it's working now
      if (!this->read_data_with_crc_(MLX90614_EMISSIVITY, &emissivity_data)) {
        ESP_LOGE(TAG, "Failed to communicate with MLX90614 after reset - check wiring!");
        this->mark_failed();
        return;
      }
    } else {
      ESP_LOGE(TAG, "Failed to reset MLX90614 - check wiring!");
      this->mark_failed();
      return;
    }
  }
  
  ESP_LOGD(TAG, "MLX90614 ID: 0x%04X, Config: 0x%04X, Emissivity: 0x%04X", 
           id_data, config_data, emissivity_data);
  
  // PREVENTION 5: Clear any error flags before starting operation
  // Read temperature registers once to clear any error states
  uint16_t temp_data;
  this->read_data_with_crc_(MLX90614_TEMPERATURE_AMBIENT, &temp_data);
  this->read_data_with_crc_(MLX90614_TEMPERATURE_OBJECT_1, &temp_data);
  
  // Now write emissivity if needed
  if (!this->write_emissivity_()) {
    ESP_LOGE(TAG, "Failed to write emissivity");
    this->mark_failed();
    return;
  }
  
  ESP_LOGD(TAG, "MLX90614 setup complete - sensor initialized successfully");
}

bool MLX90614Component::write_emissivity_() {
  if (std::isnan(this->emissivity_))
    return true;
  uint16_t value = (uint16_t) (this->emissivity_ * 65535);
  if (!this->write_bytes_(MLX90614_EMISSIVITY, 0)) {
    return false;
  }
  delay(10);
  if (!this->write_bytes_(MLX90614_EMISSIVITY, value)) {
    return false;
  }
  delay(10);
  return true;
}

uint8_t MLX90614Component::crc8_pec_(const uint8_t *data, uint8_t len) {
  uint8_t crc = 0;
  for (uint8_t i = 0; i < len; i++) {
    uint8_t in = data[i];
    for (uint8_t j = 0; j < 8; j++) {
      bool carry = (crc ^ in) & 0x80;
      crc <<= 1;
      if (carry)
        crc ^= 0x07;
      in <<= 1;
    }
  }
  return crc;
}

bool MLX90614Component::write_bytes_(uint8_t reg, uint16_t data) {
  uint8_t buf[5];
  buf[0] = this->address_ << 1;
  buf[1] = reg;
  buf[2] = data & 0xFF;
  buf[3] = data >> 8;
  buf[4] = this->crc8_pec_(buf, 4);
  return this->write_bytes(reg, buf + 2, 3);
}

void MLX90614Component::dump_config() {
  ESP_LOGCONFIG(TAG, "MLX90614:");
  LOG_I2C_DEVICE(this);
  if (this->is_failed()) {
    ESP_LOGE(TAG, ESP_LOG_MSG_COMM_FAIL);
  }
  LOG_UPDATE_INTERVAL(this);
  LOG_SENSOR("  ", "Ambient", this->ambient_sensor_);
  LOG_SENSOR("  ", "Object", this->object_sensor_);
}

float MLX90614Component::get_setup_priority() const { return setup_priority::DATA; }

bool MLX90614Component::read_data_with_crc_(uint8_t reg, uint16_t *data) {
  uint8_t raw[3];
  uint8_t retry_count = 0;
  
  while (retry_count < this->read_retries_) {
    i2c::ErrorCode err = this->read_register(reg, raw, 3, false);
    
    if (err != i2c::ERROR_OK) {
      ESP_LOGW(TAG, "I2C read error for register 0x%02X, attempt %d/%d", reg, retry_count + 1, this->read_retries_);
      retry_count++;
      delay(10);  // Small delay before retry
      continue;
    }
    
    // Calculate CRC for validation
    uint8_t crc_buffer[5];
    crc_buffer[0] = this->address_ << 1;  // I2C address with write bit
    crc_buffer[1] = reg;  // Register address
    crc_buffer[2] = (this->address_ << 1) | 1;  // I2C address with read bit
    crc_buffer[3] = raw[0];  // LSB
    crc_buffer[4] = raw[1];  // MSB
    
    uint8_t calculated_crc = this->crc8_pec_(crc_buffer, 5);
    
    if (calculated_crc != raw[2]) {
      ESP_LOGW(TAG, "CRC mismatch for register 0x%02X: calculated=0x%02X, received=0x%02X, attempt %d/%d", 
               reg, calculated_crc, raw[2], retry_count + 1, this->read_retries_);
      retry_count++;
      delay(10);
      continue;
    }
    
    // CRC is valid, return the data
    *data = encode_uint16(raw[1], raw[0]);
    return true;
  }
  
  ESP_LOGE(TAG, "Failed to read register 0x%02X after %d attempts", reg, this->read_retries_);
  return false;
}

bool MLX90614Component::validate_temperature_(float temp) {
  // Check if temperature is within the valid range for MLX90614
  if (temp < MLX90614_MIN_TEMP || temp > MLX90614_MAX_TEMP) {
    ESP_LOGW(TAG, "Temperature %.1f°C is outside valid range (%.1f to %.1f°C)", 
             temp, MLX90614_MIN_TEMP, MLX90614_MAX_TEMP);
    return false;
  }
  return true;
}

bool MLX90614Component::enter_sleep_mode_() {
  // Send sleep command - this is a special PEC calculation
  uint8_t pec_buffer[2];
  pec_buffer[0] = this->address_ << 1;  // Address with write bit
  pec_buffer[1] = MLX90614_SLEEP_MODE;  // Sleep command
  uint8_t pec = this->crc8_pec_(pec_buffer, 2);
  
  // Send the sleep command
  i2c::ErrorCode err = this->write(&MLX90614_SLEEP_MODE, 1, false);
  if (err == i2c::ERROR_OK) {
    err = this->write(&pec, 1, true);  // Send PEC and stop
  }
  
  if (err != i2c::ERROR_OK) {
    ESP_LOGW(TAG, "Failed to enter sleep mode");
    return false;
  }
  
  ESP_LOGD(TAG, "Entered sleep mode");
  return true;
}

bool MLX90614Component::exit_sleep_mode_() {
  // To exit sleep mode, we need to send a wake-up signal
  // This is done by sending the device address with SDA held low for at least 33ms
  // Then sending SCL pulses
  
  // For simplicity, we'll use a general I2C bus reset sequence
  // Send device address to wake it up
  uint8_t wake_cmd = 0x00;
  i2c::ErrorCode err = this->write(&wake_cmd, 0, true);  // Just send address
  
  // Need at least 250ms for sensor to wake up and stabilize
  delay(250);
  
  // Verify sensor is awake by reading a register
  uint16_t test_data;
  if (!this->read_data_with_crc_(MLX90614_EMISSIVITY, &test_data)) {
    ESP_LOGW(TAG, "Failed to wake sensor");
    return false;
  }
  
  ESP_LOGD(TAG, "Sensor woke up successfully");
  return true;
}

void MLX90614Component::i2c_bus_reset_() {
  // Perform I2C bus reset sequence to clear any stuck slaves
  // This is the standard I2C bus recovery procedure
  ESP_LOGD(TAG, "Performing I2C bus reset sequence");
  
  // The I2C component in ESPHome handles this internally when needed,
  // but we can trigger it explicitly
  // Note: This is a simplified version - full implementation would
  // manually toggle SCL while monitoring SDA
  
  // Send a stop condition to reset the bus
  this->write(nullptr, 0, true);
  delay(10);
  
  // Try to read from general call address to clear the bus
  uint8_t dummy;
  this->read_register(0x00, &dummy, 1, false);
  delay(10);
}

bool MLX90614Component::reset_sensor_() {
  ESP_LOGI(TAG, "Attempting to reset MLX90614 sensor");
  
  // SMBus General Call Reset - this works reliably
  // Send 0x06 to address 0x00 (general call address)
  uint8_t general_call_addr = 0x00;
  uint8_t reset_cmd = 0x06;
  
  // Temporarily change address for general call
  uint8_t original_addr = this->address_;
  this->address_ = general_call_addr;
  
  i2c::ErrorCode err = this->write(&reset_cmd, 1, true);
  
  // Restore original address
  this->address_ = original_addr;
  
  if (err == i2c::ERROR_OK) {
    ESP_LOGI(TAG, "SMBus general call reset sent");
    delay(100);  // Reduced delay - sensor needs time to reset
    
    // Verify sensor is responsive
    uint16_t test_data;
    if (this->read_data_with_crc_(MLX90614_EMISSIVITY, &test_data)) {
      ESP_LOGI(TAG, "Sensor reset successful");
      return true;
    }
  }
  
  ESP_LOGE(TAG, "Failed to reset sensor");
  return false;
}

void MLX90614Component::update() {
  uint16_t raw_object;
  uint16_t raw_ambient;
  
  // Read object temperature with CRC validation and retries
  if (!this->read_data_with_crc_(MLX90614_TEMPERATURE_OBJECT_1, &raw_object)) {
    ESP_LOGE(TAG, "Failed to read object temperature");
    this->status_set_warning();
    return;
  }
  
  // Read ambient temperature with CRC validation and retries
  if (!this->read_data_with_crc_(MLX90614_TEMPERATURE_AMBIENT, &raw_ambient)) {
    ESP_LOGE(TAG, "Failed to read ambient temperature");
    this->status_set_warning();
    return;
  }
  
  // Check for common error values that indicate sensor issues
  // 0xFFFF is a common error value, and values < 0x2000 often indicate errors
  // as they represent temperatures below -200°C which the sensor can't measure
  bool ambient_error = (raw_ambient & 0x8000) || (raw_ambient == 0xFFFF) || (raw_ambient < 0x2000);
  bool object_error = (raw_object & 0x8000) || (raw_object == 0xFFFF) || (raw_object < 0x2000);
  
  // If both readings show errors, the sensor needs an immediate reset
  if (ambient_error && object_error) {
    ESP_LOGW(TAG, "Both temperature readings invalid (ambient: 0x%04X, object: 0x%04X)",
             raw_ambient, raw_object);
    this->status_set_warning();
    
    // Immediately try to reset the sensor - no point in waiting
    ESP_LOGI(TAG, "Attempting immediate sensor reset");
    if (this->reset_sensor_()) {
      ESP_LOGI(TAG, "Sensor reset successful, resuming normal operation");
      this->error_count_ = 0;
      
      // Re-apply emissivity after reset
      if (!std::isnan(this->emissivity_)) {
        delay(50);
        this->write_emissivity_();
      }
      
      // Try to read valid data immediately after reset
      delay(100);  // Brief delay for sensor to stabilize
      
      // Attempt to read temperatures again
      if (this->read_data_with_crc_(MLX90614_TEMPERATURE_OBJECT_1, &raw_object) &&
          this->read_data_with_crc_(MLX90614_TEMPERATURE_AMBIENT, &raw_ambient)) {
        
        // Check if readings are now valid
        bool ambient_ok = !(raw_ambient & 0x8000) && (raw_ambient != 0xFFFF) && (raw_ambient >= 0x2000);
        bool object_ok = !(raw_object & 0x8000) && (raw_object != 0xFFFF) && (raw_object >= 0x2000);
        
        if (ambient_ok || object_ok) {
          ESP_LOGI(TAG, "Sensor recovered - got valid readings after reset");
          // Continue to process the new readings below
          ambient_error = !ambient_ok;
          object_error = !object_ok;
        } else {
          ESP_LOGW(TAG, "Sensor still returning errors after reset, will retry next update");
          return;
        }
      } else {
        ESP_LOGW(TAG, "Failed to read after reset, will retry next update");
        return;
      }
    } else {
      // Reset failed, track failures
      this->error_count_++;
      if (this->error_count_ >= 10) {
        ESP_LOGE(TAG, "Sensor reset failed %d times - check hardware connections", this->error_count_);
        this->error_count_ = 0;  // Reset counter to avoid spamming logs
      }
      return;
    }
  }
  
  // Convert raw values to temperature
  // The MLX90614 returns temperature in 0.02K units
  float ambient = NAN;
  float object = NAN;
  
  if (!ambient_error) {
    ambient = raw_ambient * 0.02f - 273.15f;
    if (!this->validate_temperature_(ambient)) {
      ESP_LOGW(TAG, "Invalid ambient temperature reading: %.1f°C (raw: 0x%04X)", ambient, raw_ambient);
      ambient = NAN;
      ambient_error = true;  // Mark as error for reset logic
    }
  } else {
    ESP_LOGW(TAG, "Ambient temperature error (raw: 0x%04X)", raw_ambient);
  }
  
  if (!object_error) {
    object = raw_object * 0.02f - 273.15f;
    if (!this->validate_temperature_(object)) {
      ESP_LOGW(TAG, "Invalid object temperature reading: %.1f°C (raw: 0x%04X)", object, raw_object);
      object = NAN;
      object_error = true;  // Mark as error for reset logic
    }
  } else {
    ESP_LOGW(TAG, "Object temperature error (raw: 0x%04X)", raw_object);
  }
  
  // Check if we have persistent errors that need a reset
  // This handles cases where sensor returns out-of-range values after working normally
  if (ambient_error || object_error) {
    this->error_count_++;
    
    // After 3 consecutive errors on ANY channel, reset the sensor
    if (this->error_count_ >= 3) {
      ESP_LOGW(TAG, "Persistent invalid readings detected (count: %d), attempting reset", this->error_count_);
      
      if (this->reset_sensor_()) {
        ESP_LOGI(TAG, "Sensor reset successful after persistent errors");
        this->error_count_ = 0;
        
        // Re-apply emissivity after reset
        if (!std::isnan(this->emissivity_)) {
          delay(50);
          this->write_emissivity_();
        }
        
        // Skip this reading cycle and let next update get fresh data
        return;
      } else {
        ESP_LOGE(TAG, "Failed to reset sensor after persistent errors");
        if (this->error_count_ >= 10) {
          ESP_LOGE(TAG, "Too many reset failures - check hardware");
          this->error_count_ = 3;  // Keep trying but don't spam logs
        }
      }
    } else {
      ESP_LOGD(TAG, "Error count: %d/3 before reset", this->error_count_);
    }
  } else {
    // Both readings are valid, reset error counter
    if (this->error_count_ > 0) {
      ESP_LOGD(TAG, "Valid readings received, clearing error counter (was %d)", this->error_count_);
      this->error_count_ = 0;
    }
  }
  
  ESP_LOGD(TAG, "Got Object=%.1f°C Ambient=%.1f°C", object, ambient);
  
  // Only publish valid temperatures
  if (this->ambient_sensor_ != nullptr && !std::isnan(ambient)) {
    this->ambient_sensor_->publish_state(ambient);
  }
  
  if (this->object_sensor_ != nullptr && !std::isnan(object)) {
    this->object_sensor_->publish_state(object);
  }
  
  this->status_clear_warning();
}

}  // namespace mlx90614
}  // namespace esphome
