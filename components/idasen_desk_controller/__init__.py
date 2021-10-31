import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import cover, ble_client
from esphome.const import CONF_ID

DEPENDENCIES = ['esp32', 'ble_client']

AUTO_LOAD = ['cover']
MULTI_CONF = True

CONF_IDASEN_DESK_CONTROLLER_ID = 'idasen_desk_controller_id'
CONF_ONLY_UP_DOWN_COMMAND = 'only_up_down_command'

idasen_desk_controller_ns = cg.esphome_ns.namespace('idasen_desk_controller')

IdasenDeskControllerComponent = idasen_desk_controller_ns.class_(
    'IdasenDeskControllerComponent', cg.Component, cover.Cover, ble_client.BLEClientNode)

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(IdasenDeskControllerComponent),
    cv.Optional(CONF_ONLY_UP_DOWN_COMMAND, False): cv.boolean,
}).extend(ble_client.BLE_CLIENT_SCHEMA)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await ble_client.register_ble_node(var, config)

    cg.add(var.use_only_up_down_command(config[CONF_ONLY_UP_DOWN_COMMAND]))
