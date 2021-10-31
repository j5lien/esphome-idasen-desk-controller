import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import cover
from . import IdasenDeskControllerComponent, CONF_IDASEN_DESK_CONTROLLER_ID

DEPENDENCIES = ['esp32', 'idasen_desk_controller']

CONFIG_SCHEMA = cover.COVER_SCHEMA.extend(({
    cv.GenerateID(CONF_IDASEN_DESK_CONTROLLER_ID): cv.use_id(IdasenDeskControllerComponent),
}))

def to_code(config):
    hub = yield cg.get_variable(config[CONF_IDASEN_DESK_CONTROLLER_ID])
    yield cover.register_cover(hub, config)
