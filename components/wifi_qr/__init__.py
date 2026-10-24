"""Wi-Fi QR: bind two Home Assistant text helpers to the Tessera plugin."""
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import smart_display, text_sensor
from esphome.const import CONF_ID

# The core component every Tessera screen has; the plugin API lives in it.
DEPENDENCIES = ["smart_display", "text_sensor"]

CONF_SSID = "ssid"
CONF_PASSWORD = "password"

wifi_qr_ns = cg.esphome_ns.namespace("wifi_qr")
WifiQr = wifi_qr_ns.class_("WifiQr", cg.Component)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(WifiQr),
        cv.Required(CONF_SSID): cv.use_id(text_sensor.TextSensor),
        cv.Required(CONF_PASSWORD): cv.use_id(text_sensor.TextSensor),
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await smart_display.register_plugin(var, __file__)
    cg.add(var.set_ssid(await cg.get_variable(config[CONF_SSID])))
    cg.add(var.set_password(await cg.get_variable(config[CONF_PASSWORD])))
