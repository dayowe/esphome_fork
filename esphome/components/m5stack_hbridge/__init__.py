import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import pins
from esphome.components import i2c
from esphome.const import CONF_ID, CONF_SDA, CONF_SCL, CONF_I2C_ADDR, CONF_SPEED

DEPENDENCIES = ['i2c']

m5stack_hbridge_ns = cg.esphome_ns.namespace('m5stack_hbridge')
M5StackHBridge = m5stack_hbridge_ns.class_('M5StackHBridge', cg.Component)

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(M5StackHBridge),
    cv.Optional(CONF_SDA, default=21): pins.input_pin,
    cv.Optional(CONF_SCL, default=22): pins.input_pin,
    cv.Optional(CONF_I2C_ADDR, default=0x20): cv.i2c_address,
    cv.Optional(CONF_SPEED, default=100000): cv.positive_int,
}).extend(i2c.i2c_device_schema(0x20))  # Default I2C address

def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    yield cg.register_component(var, config)
    yield i2c.register_i2c_device(var, config)

    cg.add(var.set_i2c_pins(config[CONF_SDA], config[CONF_SCL]))
    cg.add(var.set_i2c_address(config[CONF_I2C_ADDR]))
    cg.add(var.set_i2c_speed(config[CONF_SPEED]))
