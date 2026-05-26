import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import spi
from esphome.const import CONF_ID

# Named cc1101_smartrc to avoid collision with the native ESPHome cc1101 component
# added in ESPHome 2026.x. That component does strict chip-ID validation rejecting
# common CC1101 clones. This component drives the radio directly via ESPHome's SPI
# bus — no external Arduino libraries needed.

DEPENDENCIES = ["spi"]

cc1101_smartrc_ns = cg.esphome_ns.namespace("cc1101_smartrc")
CC1101Component = cc1101_smartrc_ns.class_("CC1101", cg.Component, spi.SPIDevice)

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(CC1101Component),
            cv.Required("bandwidth"): cv.float_,
            cv.Required("frequency"): cv.float_,
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
    .extend(spi.spi_device_schema(cs_pin_required=True))
)


async def to_code(config):
    var = cg.new_Pvariable(
        config[CONF_ID],
        config["bandwidth"],
        config["frequency"],
    )
    await cg.register_component(var, config)
    await spi.register_spi_device(var, config)
