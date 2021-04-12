import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import cover
from esphome.const import ESP_PLATFORM_ESP32, CONF_ID, CONF_MAC_ADDRESS

AUTO_LOAD = ['binary_sensor', 'cover', 'sensor']
ESP_PLATFORMS = [ESP_PLATFORM_ESP32]
MULTI_CONF = True

CONF_IDASEN_DESK_CONTROLLER_ID = 'idasen_desk_controller_id'
CONF_MAC_ADDRESS = 'mac_address'
CONF_BLUETOOTH_CALLBACK = 'bluetooth_callback'

idasen_desk_controller_ns = cg.esphome_ns.namespace('idasen_desk_controller')

IdasenDeskControllerComponent = idasen_desk_controller_ns.class_(
    'IdasenDeskControllerComponent', cg.Component, cover.Cover)

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(IdasenDeskControllerComponent),
    cv.Required(CONF_MAC_ADDRESS): cv.mac_address,
    cv.Optional(CONF_BLUETOOTH_CALLBACK, True): cv.boolean
})


def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    yield cg.register_component(var, config)

    cg.add(var.set_mac_address(config[CONF_MAC_ADDRESS].as_hex))
    cg.add(var.use_bluetooth_callback(config[CONF_BLUETOOTH_CALLBACK]))
