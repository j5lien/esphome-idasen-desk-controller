#pragma once

#include "esphome/core/component.h"
#include "esphome/components/cover/cover.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/binary_sensor/binary_sensor.h"
#include "BLEDevice.h"

namespace esphome {
namespace idasen_desk_controller {
class IdasenDeskControllerComponent : public Component, public cover::Cover, public BLEClientCallbacks {
 public:
  IdasenDeskControllerComponent() : Component(){};

  void set_mac_address(uint64_t address);
  void use_bluetooth_callback(bool bluetooth_callback) { bluetooth_callback_ = bluetooth_callback; };
  void set_ble_device(BLEAdvertisedDevice *device);

  void set_desk_height_sensor(sensor::Sensor *desk_height_sensor) { desk_height_sensor_ = desk_height_sensor; };
  void set_desk_moving_binary_sensor(binary_sensor::BinarySensor *desk_moving_binary_sensor) {
    desk_moving_binary_sensor_ = desk_moving_binary_sensor;
  }
  void set_desk_connection_binary_sensor(binary_sensor::BinarySensor *desk_connection_binary_sensor) {
    desk_connection_binary_sensor_ = desk_connection_binary_sensor;
  }

  float get_setup_priority() const override { return setup_priority::LATE; }
  void setup() override;
  void dump_config() override;

  void onConnect(BLEClient *p_client);
  void onDisconnect(BLEClient *p_client);
  void connect();

  void update_desk_data(uint8_t *pData = nullptr, bool allow_publishing_cover_state = true);

  cover::CoverTraits get_traits() override;
  void control(const cover::CoverCall &call) override;

  static IdasenDeskControllerComponent *instance;

 private:
  sensor::Sensor *desk_height_sensor_ = nullptr;
  binary_sensor::BinarySensor *desk_moving_binary_sensor_ = nullptr;
  binary_sensor::BinarySensor *desk_connection_binary_sensor_ = nullptr;

  std::string ble_address_;
  bool bluetooth_callback_;
  BLEAdvertisedDevice *ble_device_ = nullptr;
  BLEClient *p_client_ = nullptr;
  BLERemoteCharacteristic *m_input_char_ = nullptr;
  BLERemoteCharacteristic *m_output_char_ = nullptr;
  BLERemoteCharacteristic *m_control_char_ = nullptr;

  float height_target_ = 0;
  cover::CoverOperation current_operation_{cover::COVER_OPERATION_IDLE};

  void scan_();
  void set_connection_(bool connected);

  void update_desk_();
  float get_heigth_();
  bool is_at_target_(float height) const;
  void publish_cover_state_(float position);

  void start_move_torwards_();
  void move_torwards_();
  void stop_move_();
};
}  // namespace idasen_desk_controller
}  // namespace esphome