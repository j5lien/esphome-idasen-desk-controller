import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import binary_sensor
from esphome.const import ESP_PLATFORM_ESP32, CONF_DEVICE_CLASS, DEVICE_CLASS_CONNECTIVITY, DEVICE_CLASS_MOVING
from . import IdasenDeskControllerComponent, CONF_IDASEN_DESK_CONTROLLER_ID

ESP_PLATFORMS = [ESP_PLATFORM_ESP32]
DEPENDENCIES = ['idasen_desk_controller']

CONF_TYPE = 'type'
CONF_CONNECTION = 'CONNECTION'
CONF_MOVING = 'MOVING'
CONF_TYPE_OPTIONS = {
    CONF_CONNECTION: CONF_CONNECTION,
    CONF_MOVING: CONF_MOVING,
}

CONFIG_SCHEMA = binary_sensor.BINARY_SENSOR_SCHEMA.extend({
    cv.GenerateID(CONF_IDASEN_DESK_CONTROLLER_ID): cv.use_id(IdasenDeskControllerComponent),
    cv.Required(CONF_TYPE): cv.enum(CONF_TYPE_OPTIONS, upper=True),
})


def to_code(config):
    hub = yield cg.get_variable(config[CONF_IDASEN_DESK_CONTROLLER_ID])
    if config[CONF_TYPE] == CONF_CONNECTION:
        config[CONF_DEVICE_CLASS] = DEVICE_CLASS_CONNECTIVITY
        var = yield binary_sensor.new_binary_sensor(config)
        cg.add(hub.set_desk_connection_binary_sensor(var))
    else:
        config[CONF_DEVICE_CLASS] = DEVICE_CLASS_MOVING
        var = yield binary_sensor.new_binary_sensor(config)
        cg.add(hub.set_desk_moving_binary_sensor(var))
