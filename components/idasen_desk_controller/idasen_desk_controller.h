#pragma once

#include "esphome/core/component.h"
#include "esphome/components/cover/cover.h"
#include "esphome/components/ble_client/ble_client.h"
#include "esphome/components/esp32_ble_tracker/esp32_ble_tracker.h"

#ifdef ARDUINO_ARCH_ESP32
// https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/bluetooth/esp_gattc.html
#include <esp_gattc_api.h>

namespace esphome {
namespace idasen_desk_controller {

namespace espbt = esphome::esp32_ble_tracker;

using namespace ble_client;

espbt::ESPBTUUID uuid128_from_string(std::string value);

class IdasenDeskControllerComponent : public Component, public cover::Cover, public BLEClientNode {
 public:
  IdasenDeskControllerComponent() : Component(){};

  float get_setup_priority() const override { return setup_priority::LATE; }
  void setup() override;
  void dump_config() override;

  void loop() override;

  void use_only_up_down_command(bool use_only_up_down_command) { use_only_up_down_command_ = use_only_up_down_command; }

  void gattc_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if,
                           esp_ble_gattc_cb_param_t *param) override;

  cover::CoverTraits get_traits() override;
  void control(const cover::CoverCall &call) override;

 private:
  bool use_only_up_down_command_ = false;

  espbt::ESPBTUUID output_service_uuid_ = uuid128_from_string("99fa0020-338a-1024-8a49-009c0215f78a");
  espbt::ESPBTUUID output_char_uuid_ = uuid128_from_string("99fa0021-338a-1024-8a49-009c0215f78a");
  uint16_t output_handle_;

  espbt::ESPBTUUID input_service_uuid_ = uuid128_from_string("99fa0030-338a-1024-8a49-009c0215f78a");
  espbt::ESPBTUUID input_char_uuid_ = uuid128_from_string("99fa0031-338a-1024-8a49-009c0215f78a");
  uint16_t input_handle_;

  espbt::ESPBTUUID control_service_uuid_ = uuid128_from_string("99fa0001-338a-1024-8a49-009c0215f78a");
  espbt::ESPBTUUID control_char_uuid_ = uuid128_from_string("99fa0002-338a-1024-8a49-009c0215f78a");
  uint16_t control_handle_;

  bool controlled_ = false;
  float position_target_;

  bool notify_disable_ = true;
  int not_moving_loop_ = 0;

  void write_value_(uint16_t handle, unsigned short value);
  void read_value_(uint16_t handle);
  void publish_cover_state_(uint8_t *value, uint16_t value_len);
  void move_desk_();
  bool is_at_target_() const;
  void start_move_torwards_();
  void move_torwards_();
  void stop_move_();
};
}  // namespace idasen_desk_controller
}  // namespace esphome

#endif