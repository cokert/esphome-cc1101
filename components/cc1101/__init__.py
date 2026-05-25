import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import remote_transmitter
from esphome.const import CONF_ID

DEPENDENCIES = ["remote_transmitter", "spi"]

cc1101_ns = cg.esphome_ns.namespace("cc1101")
CC1101Component = cc1101_ns.class_("CC1101", cg.Component)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(CC1101Component),
        cv.Required("sck_pin"): cv.int_,
        cv.Required("miso_pin"): cv.int_,
        cv.Required("mosi_pin"): cv.int_,
        cv.Required("csn_pin"): cv.int_,
        cv.Required("gdo0_pin"): cv.int_,
        cv.Required("bandwidth"): cv.float_,
        cv.Required("frequency"): cv.float_,
        cv.Required("transmitter_id"): cv.use_id(
            remote_transmitter.RemoteTransmitterComponent
        ),
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    transmitter = await cg.get_variable(config["transmitter_id"])
    var = cg.new_Pvariable(
        config[CONF_ID],
        config["sck_pin"],
        config["miso_pin"],
        config["mosi_pin"],
        config["csn_pin"],
        config["gdo0_pin"],
        config["bandwidth"],
        config["frequency"],
        transmitter,
    )
    await cg.register_component(var, config)
    cg.add_library("SPI", None)
    cg.add_library("SmartRC-CC1101-Driver-Lib", None)
