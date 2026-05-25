import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import CONF_ID

# Named cc1101_smartrc to avoid collision with the native ESPHome cc1101 component
# added in ESPHome 2026.x. Uses the SmartRC-CC1101-Driver-Lib directly via Arduino
# SPI, which avoids the strict chip-ID validation in the native component that
# rejects common CC1101 clone modules.

DEPENDENCIES = ["spi"]

cc1101_smartrc_ns = cg.esphome_ns.namespace("cc1101_smartrc")
CC1101Component = cc1101_smartrc_ns.class_("CC1101", cg.Component)

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
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(
        config[CONF_ID],
        config["sck_pin"],
        config["miso_pin"],
        config["mosi_pin"],
        config["csn_pin"],
        config["gdo0_pin"],
        config["bandwidth"],
        config["frequency"],
    )
    await cg.register_component(var, config)
    cg.add_library("SPI", None)
    cg.add_library("lsatan/SmartRC-CC1101-Driver-Lib", None)
