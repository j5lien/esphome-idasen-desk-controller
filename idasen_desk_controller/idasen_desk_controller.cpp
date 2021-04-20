#include "idasen_desk_controller.h"
#include "esphome/core/log.h"
#include "esphome/core/helpers.h"
#include <string>

namespace esphome {
namespace idasen_desk_controller {

static BLEUUID outputServiceUUID("99fa0020-338a-1024-8a49-009c0215f78a");
static BLEUUID outputCharacteristicUUID("99fa0021-338a-1024-8a49-009c0215f78a");
static BLEUUID inputServiceUUID("99fa0030-338a-1024-8a49-009c0215f78a");
static BLEUUID inputCharacteristicUUID("99fa0031-338a-1024-8a49-009c0215f78a");
static BLEUUID controlServiceUUID("99fa0001-338a-1024-8a49-009c0215f78a");
static BLEUUID controlCharacteristicUUID("99fa0002-338a-1024-8a49-009c0215f78a");

static float deskMaxHeight = 65.0;

static const char *TAG = "idasen_desk_controller";

static float transform_height_to_position(float height) { return height / deskMaxHeight; }
static float transform_position_to_height(float position) { return position * deskMaxHeight; }

double speed_0 = 0;

static void writeUInt16(BLERemoteCharacteristic *charcteristic, unsigned short value) {
  uint8_t data[2];
  data[0] = value;
  data[1] = value >> 8;
  charcteristic->writeValue(data, 2);
}

static void deskHeightUpdateNotificationCallback(BLERemoteCharacteristic *pBLERemoteCharacteristic, uint8_t *pData,
                                                 size_t length, bool isNotify) {
  IdasenDeskControllerComponent::instance->update_desk_data(pData);
};

/**
 * Scan for BLE servers and find the first one that advertises the service we are looking for.
 */
class FindDeskDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
 private:
  std::string ble_address_;
  IdasenDeskControllerComponent *desk_;

 public:
  FindDeskDeviceCallbacks(std::string arg_address, IdasenDeskControllerComponent *arg_desk) {
    this->ble_address_ = arg_address;
    this->desk_ = arg_desk;
  }

  /**
   * Called for each advertising BLE server.
   */
  void onResult(BLEAdvertisedDevice advertised_device) {
    ESP_LOGCONFIG(TAG, "BLE Device found: %s", advertised_device.toString().c_str());
    if (advertised_device.getAddress().equals(BLEAddress(this->ble_address_.c_str()))) {
      this->desk_->set_ble_device(new BLEAdvertisedDevice(advertised_device));
    }
  }
};

IdasenDeskControllerComponent *IdasenDeskControllerComponent::instance;

void IdasenDeskControllerComponent::set_mac_address(uint64_t address) {
  char buffer[24];
  auto *mac = reinterpret_cast<uint8_t *>(&address);
  snprintf(buffer, sizeof(buffer), "%02X:%02X:%02X:%02X:%02X:%02X", mac[5], mac[4], mac[3], mac[2], mac[1], mac[0]);
  this->ble_address_ = std::string(buffer);
}

void IdasenDeskControllerComponent::set_ble_device(BLEAdvertisedDevice *device) {
  BLEDevice::getScan()->stop();
  this->ble_device_ = device;

  this->set_timeout(200, [this]() { this->connect(); });
}

void IdasenDeskControllerComponent::setup() {
  ESP_LOGCONFIG(TAG, "Setting up Idasen Desk Controller...");
  IdasenDeskControllerComponent::instance = this;
  this->set_connection_(false);

  BLEDevice::init(TAG);
  BLEScan *pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new FindDeskDeviceCallbacks(this->ble_address_, this));
  pBLEScan->setInterval(1349);
  pBLEScan->setWindow(449);
  pBLEScan->setActiveScan(true);

  this->p_client_ = BLEDevice::createClient();
  this->p_client_->setClientCallbacks(this);

  this->set_interval("update_desk", 200, [this]() { this->update_desk_(); });

  // Delay bluetooth scanning from a second
  this->set_timeout(1000, [this]() { this->scan_(); });
}

void IdasenDeskControllerComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "Idasen Desk Controller:");
  ESP_LOGCONFIG(TAG, "  Mac address: %s", this->ble_address_.c_str());
  ESP_LOGCONFIG(TAG, "  Bluetooth callback: %s", this->bluetooth_callback_ ? "true" : "false");
  LOG_SENSOR("  ", "Desk height", this->desk_height_sensor_);
  LOG_BINARY_SENSOR("  ", "Desk moving", this->desk_moving_binary_sensor_);
  LOG_BINARY_SENSOR("  ", "Desk connection", this->desk_connection_binary_sensor_);
  LOG_COVER("  ", "Desk", this);
}

void IdasenDeskControllerComponent::scan_() {
  if (this->ble_device_ != nullptr) {
    return;
  }

  ESP_LOGCONFIG(TAG, "Start scanning devices...");
  BLEDevice::getScan()->start(5, false);
  this->set_timeout(10000, [this]() { this->scan_(); });
}

void IdasenDeskControllerComponent::connect() {
  if (this->p_client_->isConnected()) {
    return;
  }

  ESP_LOGCONFIG(TAG, "Connecting client to device %s", this->ble_device_->getAddress().toString().c_str());
  this->p_client_->connect(this->ble_device_);

  if (false == this->p_client_->isConnected()) {
    ESP_LOGCONFIG(TAG, "Fail to connect to client");
    this->set_timeout(5000, [this]() { this->connect(); });
    return;
  }

  if (this->m_input_char_ == nullptr) {
    ESP_LOGCONFIG(TAG, "Retrieve input remote characteristic.");
    this->m_input_char_ = this->p_client_->getService(inputServiceUUID)->getCharacteristic(inputCharacteristicUUID);
  }

  if (this->m_output_char_ == nullptr) {
    ESP_LOGCONFIG(TAG, "Retrieve output remote characteristic.");
    this->m_output_char_ = this->p_client_->getService(outputServiceUUID)->getCharacteristic(outputCharacteristicUUID);

    // Register bluetooth callback
    if (this->bluetooth_callback_ && this->m_output_char_->canNotify()) {
      ESP_LOGCONFIG(TAG, "Register notification callback on output characteristic.");
      this->m_output_char_->registerForNotify(deskHeightUpdateNotificationCallback, true);
    }
  }

  if (this->m_control_char_ == nullptr) {
    ESP_LOGCONFIG(TAG, "Retrieve control remote characteristic.");
    this->m_control_char_ =
        this->p_client_->getService(controlServiceUUID)->getCharacteristic(controlCharacteristicUUID);
  }

  ESP_LOGCONFIG(TAG, "Success connecting client to device");

  // Delay data sync from 5s
  this->set_timeout(5000, [this]() { this->update_desk_data(); });
}

void IdasenDeskControllerComponent::onConnect(BLEClient *p_client) { this->set_connection_(true); }

void IdasenDeskControllerComponent::onDisconnect(BLEClient *p_client) {
  this->set_connection_(false);
  this->set_timeout(2000, [this]() { this->connect(); });
}

cover::CoverTraits IdasenDeskControllerComponent::get_traits() {
  auto traits = cover::CoverTraits();
  traits.set_is_assumed_state(false);
  traits.set_supports_position(true);
  traits.set_supports_tilt(false);
  return traits;
}

void IdasenDeskControllerComponent::set_connection_(bool connected) {
  if (this->desk_connection_binary_sensor_) {
    this->desk_connection_binary_sensor_->publish_state(connected);
  }
}

void IdasenDeskControllerComponent::update_desk_data(uint8_t *pData, bool allow_publishing_cover_state) {
  float height;
  float speed;
  bool callback_data = pData != nullptr;

  unsigned short height_mm;
  if (callback_data) {
    // Data from the callback
    height_mm = (*(uint16_t *) pData) / 10;
    speed = (*(uint16_t *) (pData + 2)) / 100;
  } else {
    std::string value = this->m_output_char_->readValue();

    height_mm = (*(uint16_t *) (value.data())) / 10;
    speed = (*(uint16_t *) (value.data() + 2)) / 100;
  }

  height = (float) height_mm / 10;

  if (speed == 0) {
    speed_0++;
  }
  else if (speed > 0) {
    speed_0 = 0;
  }

  ESP_LOGD(TAG, "Desk bluetooth data: height %.1f - speed %.1f", height, speed);
  ESP_LOGD(TAG, "Speed 0.0 Counter: %.1f", speed_0);

  if (speed_0 >= 5) {
    ESP_LOGD(TAG, "0.0 Counter Limit reached - desk stopped");
    this->stop_move_();
    return;
  }

  // set height sensor
  if (this->desk_height_sensor_ != nullptr) {
    if (this->get_heigth_() != height) {
      this->desk_height_sensor_->publish_state(height);
    }
  }

  // set moving state
  bool moving = speed > 0;
  bool moving_updated = false;
  if (this->desk_moving_binary_sensor_ != nullptr) {
    if (!this->desk_moving_binary_sensor_->has_state() || this->desk_moving_binary_sensor_->state != moving) {
      this->desk_moving_binary_sensor_->publish_state(moving);
      moving_updated = true;
    }
  }

  if (!allow_publishing_cover_state) {
    return;
  }

  // Publish cover state
  float position = transform_height_to_position(height);
  bool position_updated = position != this->position;
  bool operation_updated = this->current_operation_ != this->current_operation;

  // No updated needed when nothing has changed
  if (!position_updated && !operation_updated) {
    return;
  }

  // Only position has been updated when moving
  if (!moving_updated && !operation_updated && moving) {
    return;
  }

  this->publish_cover_state_(position);
}

void IdasenDeskControllerComponent::publish_cover_state_(float position) {
  this->position = position;
  this->current_operation = this->current_operation_;
  this->set_timeout("update_cover_state", 1, [this]() { this->publish_state(false); });
}

float IdasenDeskControllerComponent::get_heigth_() {
  if (this->desk_height_sensor_ == nullptr || !this->desk_height_sensor_->has_state()) {
    return 0;
  }

  return this->desk_height_sensor_->get_raw_state();
}

void IdasenDeskControllerComponent::update_desk_() {
  // Was stopped
  if (this->current_operation_ == cover::COVER_OPERATION_IDLE) {
    return;
  }

  if (!this->bluetooth_callback_) {
    this->update_desk_data(nullptr, false);
  }

  // Retrieve current desk height
  float height = this->get_heigth_();

  // Check if target has been reached
  if (this->is_at_target_(height)) {
    ESP_LOGD(TAG, "Update Desk - target reached");
    this->stop_move_();
    return;
  }

  ESP_LOGD(TAG, "Update Desk - Move from %.1f to %.1f", height, this->height_target_);
  this->move_torwards_();
}

void IdasenDeskControllerComponent::control(const cover::CoverCall &call) {
  if (call.get_position().has_value()) {
    speed_0 = 0;
    if (this->current_operation_ != cover::COVER_OPERATION_IDLE) {
      this->stop_move_();
    }

    float position_target = *call.get_position();
    this->height_target_ = transform_position_to_height(position_target);
    this->update_desk_data(nullptr, false);
    float height = this->get_heigth_();
    ESP_LOGD(TAG, "Cover control - START - position %.1f - target %.1f - current %.1f", position_target,
             this->height_target_, height);

    if (this->height_target_ == height) {
      return;
    }

    if (this->height_target_ > height) {
      this->current_operation_ = cover::COVER_OPERATION_OPENING;
    } else {
      this->current_operation_ = cover::COVER_OPERATION_CLOSING;
    }

    // Prevent from potential stop moving update @see IdasenDeskControllerComponent::stop_move_()
    this->cancel_timeout("stop_moving_update");

    // Instead publish cover data
    this->publish_cover_state_(transform_height_to_position(height));

    this->start_move_torwards_();
    return;
  }

  if (call.get_stop()) {
    ESP_LOGD(TAG, "Cover control - STOP");
    this->stop_move_();
  }
}

void IdasenDeskControllerComponent::start_move_torwards_() {
  writeUInt16(this->m_control_char_, 0xFE);
  writeUInt16(this->m_control_char_, 0xFF);
}

void IdasenDeskControllerComponent::move_torwards_() {
  writeUInt16(this->m_input_char_, (unsigned short) (this->height_target_ * 100));
}

void IdasenDeskControllerComponent::stop_move_() {
  speed_0 = 0;
  writeUInt16(this->m_control_char_, 0xFF);
  writeUInt16(this->m_input_char_, 0x8001);
  this->current_operation_ = cover::COVER_OPERATION_IDLE;
  this->set_timeout("stop_moving_update", 200, [this]() { this->update_desk_data(); });
}

bool IdasenDeskControllerComponent::is_at_target_(float height) const {
  switch (this->current_operation_) {
    case cover::COVER_OPERATION_OPENING:
      return height >= this->height_target_;
    case cover::COVER_OPERATION_CLOSING:
      return height <= this->height_target_;
    case cover::COVER_OPERATION_IDLE:
    default:
      return true;
  }
}
}  // namespace idasen_desk_controller
}  // namespace esphome
