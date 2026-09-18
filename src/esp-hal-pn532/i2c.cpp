#include "pn532_hal/i2c.hpp"
#include "pn532_cxx/transaction.hpp"
#include <Arduino.h>
#include <esp_log.h>
#include <esp_err.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <algorithm>

static const char *TAG = "PN532::I2C";

namespace pn532 {

I2cTransport::I2cTransport(i2c_port_t port, gpio_num_t sda, gpio_num_t scl)
    : _port(port), _address(0x24)
{
  i2c_driver_delete(_port);

  i2c_config_t conf = {};
  conf.mode = I2C_MODE_MASTER;
  conf.sda_io_num = sda;
  conf.scl_io_num = scl;
  conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
  conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
  conf.master.clk_speed = 100000;
  conf.clk_flags = 0;

  esp_err_t ret = i2c_param_config(_port, &conf);
  if (ret != ESP_OK) {
    Serial.printf("[PN532::I2C] Failed to configure I2C: %s (0x%X)\n", esp_err_to_name(ret), ret);
  }

  ret = i2c_driver_install(_port, conf.mode, 0, 0, 0);
  if (ret != ESP_OK) {
    Serial.printf("[PN532::I2C] Failed to install I2C driver: %s (0x%X)\n", esp_err_to_name(ret), ret);
  }

  _rxBuffer.reserve(64);
}

I2cTransport::~I2cTransport() {
  i2c_driver_delete(_port);
}

void I2cTransport::swReset() {
  // PN532 I2C Wakeup sequence
  static const uint8_t WAKEUP[16] = {0x55, 0x55, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  
  i2c_cmd_handle_t cmd = i2c_cmd_link_create();
  i2c_master_start(cmd);
  i2c_master_write_byte(cmd, (_address << 1) | I2C_MASTER_WRITE, false); // Ignore NACK on wakeup
  i2c_master_write(cmd, const_cast<uint8_t *>(WAKEUP), sizeof(WAKEUP), false);
  i2c_master_stop(cmd);

  i2c_master_cmd_begin(_port, cmd, pdMS_TO_TICKS(100));
  i2c_cmd_link_delete(cmd);

  vTaskDelay(pdMS_TO_TICKS(100));
}

void I2cTransport::abort() {
  _rxBuffer.clear();
  _rxBufferPos = 0;
}

Transaction I2cTransport::begin() {
  _rxBuffer.clear();
  _rxBufferPos = 0;
  return Transaction(*this, true);
}

Status I2cTransport::writeChunk(span<const uint8_t> data) {
  i2c_cmd_handle_t cmd = i2c_cmd_link_create();
  i2c_master_start(cmd);
  i2c_master_write_byte(cmd, (_address << 1) | I2C_MASTER_WRITE, true);
  i2c_master_write(cmd, const_cast<uint8_t *>(data.data()), data.size(), true);
  i2c_master_stop(cmd);

  esp_err_t ret = i2c_master_cmd_begin(_port, cmd, pdMS_TO_TICKS(200));
  i2c_cmd_link_delete(cmd);

  if (ret != ESP_OK) {
    Serial.printf("[PN532::I2C] Write failed to address 0x%02X: %s (0x%X)\n", _address, esp_err_to_name(ret), ret);
    return Status::TRANSPORT_ERROR;
  }

  return Status::SUCCESS;
}

bool I2cTransport::waitReady(uint32_t timeout_ms) {
  uint32_t start = xTaskGetTickCount();
  uint8_t status = 0;

  while ((xTaskGetTickCount() - start) < pdMS_TO_TICKS(timeout_ms)) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (_address << 1) | I2C_MASTER_READ, true);
    i2c_master_read_byte(cmd, &status, I2C_MASTER_LAST_NACK);
    i2c_master_stop(cmd);

    esp_err_t ret = i2c_master_cmd_begin(_port, cmd, pdMS_TO_TICKS(20));
    i2c_cmd_link_delete(cmd);

    if (ret == ESP_OK && (status & 0x01)) {
      return true;
    }

    vTaskDelay(pdMS_TO_TICKS(5));
  }
  return false;
}

Status I2cTransport::prepareRead() {
  _rxBuffer.clear();
  _rxBufferPos = 0;

  // Read response frame into buffer in a single atomic I2C operation
  uint8_t temp[64];

  i2c_cmd_handle_t cmd = i2c_cmd_link_create();
  i2c_master_start(cmd);
  i2c_master_write_byte(cmd, (_address << 1) | I2C_MASTER_READ, true);
  i2c_master_read(cmd, temp, sizeof(temp) - 1, I2C_MASTER_ACK);
  i2c_master_read_byte(cmd, temp + sizeof(temp) - 1, I2C_MASTER_LAST_NACK);
  i2c_master_stop(cmd);

  esp_err_t ret = i2c_master_cmd_begin(_port, cmd, pdMS_TO_TICKS(200));
  i2c_cmd_link_delete(cmd);

  if (ret != ESP_OK) {
    Serial.printf("[PN532::I2C] prepareRead failed: %s (0x%X)\n", esp_err_to_name(ret), ret);
    return Status::TRANSPORT_ERROR;
  }

  // Byte 0 is Ready Status byte (0x01 = RDY)
  if ((temp[0] & 0x01) == 0) {
    return Status::TIMEOUT;
  }

  // Copy frame data (skipping temp[0] status byte) into _rxBuffer
  _rxBuffer.assign(temp + 1, temp + sizeof(temp));
  _rxBufferPos = 0;

  return Status::SUCCESS;
}

Status I2cTransport::readChunk(span<uint8_t> buffer) {
  if (_rxBufferPos + buffer.size() > _rxBuffer.size()) {
    Serial.printf("[PN532::I2C] readChunk overflow: req %zu, avail %zu\n",
                  buffer.size(), _rxBuffer.size() - _rxBufferPos);
    return Status::TRANSPORT_ERROR;
  }

  std::copy_n(_rxBuffer.begin() + _rxBufferPos, buffer.size(), buffer.begin());
  _rxBufferPos += buffer.size();

  return Status::SUCCESS;
}

void I2cTransport::endTransaction() {
}

} // namespace pn532
