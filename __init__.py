"""
ESPHome RGB Status LED Component

This component provides intelligent RGB LED status indication by combining:
- Native ESPHome application state monitoring (errors, warnings)
- Connection state tracking (WiFi, API)
- OTA progress indication
- Boot phase detection
- User control with priority management

Author: Bluscream
License: MIT
"""

import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import light, output
from esphome.const import CONF_ID, CONF_OUTPUT, CONF_RED, CONF_GREEN, CONF_BLUE
from esphome.core import CoroPriority, coroutine_with_priority

# Component metadata
CODEOWNERS = ["@esphome/core"]
AUTO_LOAD = ["light"]

# Namespace for the component
rgb_status_led_ns = cg.esphome_ns.namespace("rgb_status_led")
RGBStatusLED = rgb_status_led_ns.class_("RGBStatusLED", light::LightOutput, cg.Component)

# Configuration keys for different status colors
CONF_ERROR_COLOR = "error_color"
CONF_WARNING_COLOR = "warning_color"
CONF_OK_COLOR = "ok_color"
CONF_BOOT_COLOR = "boot_color"
CONF_WIFI_COLOR = "wifi_color"
CONF_API_COLOR = "api_color"
CONF_OTA_COLOR = "ota_color"

# Timing and behavior configuration keys
CONF_ERROR_BLINK_SPEED = "error_blink_speed"
CONF_WARNING_BLINK_SPEED = "warning_blink_speed"
CONF_BRIGHTNESS = "brightness"
CONF_PRIORITY_MODE = "priority_mode"
CONF_OK_STATE_ENABLED = "ok_state_enabled"

# Schema for RGB color configuration
# Each color requires red, green, and blue components as percentages (0.0 to 1.0)
ColorSchema = cv.Schema({
    cv.Required(CONF_RED): cv.percentage,
    cv.Required(CONF_GREEN): cv.percentage,
    cv.Required(CONF_BLUE): cv.percentage,
})

# Main configuration schema for the RGB Status LED component
CONFIG_SCHEMA = light.RGB_LIGHT_SCHEMA.extend(
    {
        # Component ID for code generation
        cv.GenerateID(): cv.declare_id(RGBStatusLED),
        
        # Required RGB output connections
        cv.Required(CONF_RED): cv.use_id(output.FloatOutput),
        cv.Required(CONF_GREEN): cv.use_id(output.FloatOutput),
        cv.Required(CONF_BLUE): cv.use_id(output.FloatOutput),
        
        # Color configurations with sensible defaults
        # Each color can be customized to match user preferences
        cv.Optional(CONF_ERROR_COLOR, default={CONF_RED: 1.0, CONF_GREEN: 0.0, CONF_BLUE: 0.0}): ColorSchema,
        cv.Optional(CONF_WARNING_COLOR, default={CONF_RED: 1.0, CONF_GREEN: 0.5, CONF_BLUE: 0.0}): ColorSchema,
        cv.Optional(CONF_OK_COLOR, default={CONF_RED: 0.0, CONF_GREEN: 1.0, CONF_BLUE: 0.1}): ColorSchema,
        cv.Optional(CONF_BOOT_COLOR, default={CONF_RED: 1.0, CONF_GREEN: 0.0, CONF_BLUE: 0.0}): ColorSchema,
        cv.Optional(CONF_WIFI_COLOR, default={CONF_RED: 0.7, CONF_GREEN: 0.7, CONF_BLUE: 0.7}): ColorSchema,
        cv.Optional(CONF_API_COLOR, default={CONF_RED: 0.0, CONF_GREEN: 1.0, CONF_BLUE: 0.1}): ColorSchema,
        cv.Optional(CONF_OTA_COLOR, default={CONF_RED: 0.0, CONF_GREEN: 0.0, CONF_BLUE: 1.0}): ColorSchema,
        
        # Timing configurations for blink effects
        cv.Optional(CONF_ERROR_BLINK_SPEED, default="250ms"): cv.positive_time_period,
        cv.Optional(CONF_WARNING_BLINK_SPEED, default="1500ms"): cv.positive_time_period,
        cv.Optional(CONF_BRIGHTNESS, default=0.5): cv.percentage,
        
        # Priority mode: "status" (default) or "user"
        # "status": Status indications take priority over user control
        # "user": User control takes priority over status indications
        cv.Optional(CONF_PRIORITY_MODE, default="status"): cv.enum(["status", "user"]),
        
        # OK state configuration
        # "true": Show OK state with configured color
        # "false": Turn LED off when everything is OK (power saving)
        cv.Optional(CONF_OK_STATE_ENABLED, default=True): cv.boolean,
    }
).extend(cv.COMPONENT_SCHEMA)


@coroutine_with_priority(CoroPriority.STATUS)
async def to_code(config):
    """
    Generate C++ code from the YAML configuration.
    
    This function is called by ESPHome during configuration validation
    and code generation. It sets up the component with all the
    specified parameters and connects it to the RGB outputs.
    """
    # Get the RGB output components
    red = await cg.get_variable(config[CONF_RED])
    green = await cg.get_variable(config[CONF_GREEN])
    blue = await cg.get_variable(config[CONF_BLUE])
    
    # Create the component instance
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await light.register_light(var, config)
    
    # Connect RGB outputs
    cg.add(var.set_red_output(red))
    cg.add(var.set_green_output(green))
    cg.add(var.set_blue_output(blue))
    
    # Configure colors for different states
    error_color = config[CONF_ERROR_COLOR]
    cg.add(var.set_error_color(error_color[CONF_RED], error_color[CONF_GREEN], error_color[CONF_BLUE]))
    
    warning_color = config[CONF_WARNING_COLOR]
    cg.add(var.set_warning_color(warning_color[CONF_RED], warning_color[CONF_GREEN], warning_color[CONF_BLUE]))
    
    ok_color = config[CONF_OK_COLOR]
    cg.add(var.set_ok_color(ok_color[CONF_RED], ok_color[CONF_GREEN], ok_color[CONF_BLUE]))
    
    boot_color = config[CONF_BOOT_COLOR]
    cg.add(var.set_boot_color(boot_color[CONF_RED], boot_color[CONF_GREEN], boot_color[CONF_BLUE]))
    
    wifi_color = config[CONF_WIFI_COLOR]
    cg.add(var.set_wifi_color(wifi_color[CONF_RED], wifi_color[CONF_GREEN], wifi_color[CONF_BLUE]))
    
    api_color = config[CONF_API_COLOR]
    cg.add(var.set_api_color(api_color[CONF_RED], api_color[CONF_GREEN], api_color[CONF_BLUE]))
    
    ota_color = config[CONF_OTA_COLOR]
    cg.add(var.set_ota_color(ota_color[CONF_RED], ota_color[CONF_GREEN], ota_color[CONF_BLUE]))
    
    # Configure timing and behavior
    cg.add(var.set_error_blink_speed(config[CONF_ERROR_BLINK_SPEED]))
    cg.add(var.set_warning_blink_speed(config[CONF_WARNING_BLINK_SPEED]))
    cg.add(var.set_brightness(config[CONF_BRIGHTNESS]))
    cg.add(var.set_priority_mode(config[CONF_PRIORITY_MODE]))
    cg.add(var.set_ok_state_enabled(config[CONF_OK_STATE_ENABLED]))
    
    # Enable the component in the build
    cg.add_define("USE_RGB_STATUS_LED")
