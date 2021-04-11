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

static short deskMaxHeight = 65;

static const char *TAG = "idasen_desk_controller";

static void writeUInt16(BLERemoteCharacteristic *charcteristic, unsigned short value) {
  uint8_t data[2];
  data[0] = value;
  data[1] = value >> 8;
  charcteristic->writeValue(data, 2);
}

static void deskHeightUpdateNotificationCallback(BLERemoteCharacteristic *pBLERemoteCharacteristic, uint8_t *pData,
                                                 size_t length, bool isNotify) {
  unsigned short height = (*(uint16_t *) pData);
  float speed = (*(uint16_t *) (pData + 2));
  IdasenDeskControllerComponent::instance->set_height(height / 100);
  IdasenDeskControllerComponent::instance->set_speed(speed / 100);
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

  this->set_interval("update_desk", 150, [this]() { this->update_desk_(); });
  this->set_interval("update_height_when_moving", 1000, [this]() { this->update_height_when_moving_(); });
  this->set_timeout(1000, [this]() { this->scan_(); });
}

void IdasenDeskControllerComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "Idasen Desk Controller:");
  ESP_LOGCONFIG(TAG, "  Mac address: %s", this->ble_address_.c_str());
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
    if (this->m_output_char_->canNotify()) {
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

  this->set_timeout(5000, [this]() {
    this->update_moving_state_(false);
    this->set_height(this->get_height_());
  });
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

void IdasenDeskControllerComponent::control(const cover::CoverCall &call) {
  if (call.get_position().has_value()) {
    if (true == this->move_) {
      this->stop_move_();
    }

    float pos = *call.get_position();
    this->height_target_ = (unsigned short) (pos * deskMaxHeight);
    unsigned short height = this->get_height_();
    ESP_LOGD(TAG, "Cover control - START - position %f - target %i - current %i", pos, this->height_target_, height);

    if (this->height_target_ == height) {
      return;
    }

    this->move_up_ = this->height_target_ > height;
    this->start_move_torwards_();
  }

  if (call.get_stop()) {
    ESP_LOGD(TAG, "Cover control - STOP");
    this->stop_move_();
  }
}

void IdasenDeskControllerComponent::update_desk_() {
  if (false == this->move_) {
    return;
  }

  unsigned short height = this->get_height_();
  if ((this->move_up_ && height >= this->height_target_) ||
      (false == this->move_up_ and height <= this->height_target_)) {
    ESP_LOGD(TAG, "Update Desk - target reached");
    this->stop_move_();
    return;
  }

  ESP_LOGD(TAG, "Update Desk - Move from %i to %i", height, this->height_target_);
  this->move_torwards_();
}

void IdasenDeskControllerComponent::update_height_when_moving_() {
  if (false == this->move_) {
    return;
  }

  this->set_height(this->get_height_());
}

void IdasenDeskControllerComponent::set_height(unsigned short height) {
  if (this->desk_height_sensor_ != nullptr) {
    if (!this->desk_height_sensor_->has_state() || this->desk_height_sensor_->get_raw_state() != height) {
      this->desk_height_sensor_->publish_state(height);
    }
  }

  float position = (float) height / (float) deskMaxHeight;
  if (position != this->position) {
    this->position = position;
    this->publish_state();
  }
}

void IdasenDeskControllerComponent::set_speed(short speed) { this->update_moving_state_(speed > 0); }

void IdasenDeskControllerComponent::update_moving_state_(bool moving) {
  if (this->desk_moving_binary_sensor_ != nullptr) {
    if (!this->desk_moving_binary_sensor_->has_state() || this->desk_moving_binary_sensor_->state != moving) {
      this->desk_moving_binary_sensor_->publish_state(moving);
    }
  }
}

void IdasenDeskControllerComponent::set_connection_(bool connected) {
  if (this->desk_connection_binary_sensor_) {
    this->desk_connection_binary_sensor_->publish_state(connected);
  }
}

unsigned short IdasenDeskControllerComponent::get_height_() { return this->m_output_char_->readUInt16() / 100; }

void IdasenDeskControllerComponent::start_move_torwards_() {
  this->move_ = true;
  writeUInt16(this->m_control_char_, 0xFE);
  writeUInt16(this->m_control_char_, 0xFF);
  this->update_moving_state_(true);
}

void IdasenDeskControllerComponent::move_torwards_() {
  writeUInt16(this->m_input_char_, (unsigned short) (this->height_target_ * 100));
}

void IdasenDeskControllerComponent::stop_move_() {
  this->move_ = false;
  writeUInt16(this->m_control_char_, 0xFF);
  writeUInt16(this->m_input_char_, 0x8001);
  this->update_moving_state_(false);
  this->set_height(this->get_height_());
}
}  // namespace idasen_desk_controller
}  // namespace esphome