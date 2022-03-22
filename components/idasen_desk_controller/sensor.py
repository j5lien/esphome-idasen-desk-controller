import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor
from esphome.core import coroutine
from . import IdasenDeskControllerComponent, CONF_IDASEN_DESK_CONTROLLER_ID

DEPENDENCIES = ['esp32', 'idasen_desk_controller']

CONF_HEIGHT = 'desk_height'
UNIT_HEIGHT = 'cm'
ICON_HEIGHT = 'mdi:arrow-up-down'

TYPES = {
    CONF_HEIGHT: 'set_desk_height_sensor',
}

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(CONF_IDASEN_DESK_CONTROLLER_ID): cv.use_id(IdasenDeskControllerComponent),
    cv.Optional(CONF_HEIGHT):
        sensor.sensor_schema(unit_of_measurement=UNIT_HEIGHT, icon=ICON_HEIGHT, accuracy_decimals=0),
})

@coroutine
def setup_conf(config, key, hub, func_name):
    if key in config:
        conf = config[key]
        var = yield sensor.new_sensor(conf)
        func = getattr(hub, func_name)
        cg.add(func(var))


def to_code(config):
    hub = yield cg.get_variable(config[CONF_IDASEN_DESK_CONTROLLER_ID])
    for key, func_name in TYPES.items():
        yield setup_conf(config, key, hub, func_name)
