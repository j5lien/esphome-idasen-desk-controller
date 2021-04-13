This component creates a bluetooth bridge for an Ikea Idasen desk that uses a Linak controller with [ESPHome](https://esphome.io) and an [ESP32 device](https://esphome.io/devices/esp32.html).

| [Cover integration](https://www.home-assistant.io/integrations/cover/) | [Linak Desk Card](https://github.com/IhorSyerkov/linak-desk-card)                                                              |
| ---------------------------------------------------------------------- | ------------------------------------------------------------------------------------------------------------------------------ |
| ![Home Assistant Desk Controller](ha-desk-controller.png)              | <img src="https://user-images.githubusercontent.com/9998984/107797805-a3a6c800-6d5b-11eb-863a-56ae0343995c.png" width="300" /> |

The desk is controlled using the [cover integration](https://www.home-assistant.io/integrations/cover/) or [Linak Desk Card](https://github.com/IhorSyerkov/linak-desk-card) which is available in [HACS](https://hacs.xyz) in Home assistant.

## Installation

Copy the `idasen_desk_controller` directory into your ESPHome `custom_components` directory (creating it if it does not exist).
For the first connection you will need to press the pairing button on the desk.

## Dependencies

This component requires an [ESP32 device](https://esphome.io/devices/esp32.html).

## Configuration


```yaml
idasen_desk_controller:
    # Desk controller bluetooth mac address
    # -----------
    # Required
    mac_address: "00:00:00:00:00:00"
    # Use bluetooth callback to update sensors and cover entities
    # Deactivate this option when data are not correctly updated when using the cover entity
    # -----------
    # Optionnal (default true)
    bluetooth_callback: true

cover:
  - platform: idasen_desk_controller
    name: "Desk"

sensor:
  - platform: idasen_desk_controller
    desk_height:
      # Height in cm
      name: "Desk Height"

binary_sensor:
  # Desk bluetooth connection
  - platform: idasen_desk_controller
    name: "Desk Connection"
    type: CONNECTION
  # Desk moving status
  - platform: idasen_desk_controller
    name: "Desk Moving"
    type: MOVING
```

## Troubleshooting

### Wifi deconnexion

If you experience Wifi deconnexion, try to activate the [wifi fast connect](https://esphome.io/components/wifi.html) option.
```yaml
wifi:
  ssid: ...
  password: ...
  fast_connect: true
```

You can also try to set the [power save mode](https://esphome.io/components/wifi.html?highlight=wifi#power-save-mode) option to `none`.
```yaml
wifi:
  ssid: ...
  password: ...
  fast_connect: true
  power_save_mode: none
```

### Sensors not updating

When height sensor, moving binary sensor or cover don't correctly update after moving the desk using the cover entity, it means the bluetooth callback doesn't correctly work.
You can deactivate the bluetooth callback option to not rely on it :

```yaml
idasen_desk_controller:
    mac_address: "00:00:00:00:00:00"
    bluetooth_callback: false
```

## References

* https://github.com/TheRealMazur/LinakDeskEsp32Controller
