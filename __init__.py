"""
ESPHome RGB Status LED Component

This component provides intelligent RGB LED status indication using event-driven
configuration similar to ESPHome's internal status_led but with RGB capabilities.

Events are configured declaratively in YAML and automatically trigger based on
system state changes, WiFi/API connections, and OTA operations.

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
EventConfig = rgb_status_led_ns.struct("EventConfig")

# Configuration keys for different events
CONF_ERROR = "error"
CONF_WARNING = "warning"
CONF_OK = "ok"
CONF_BOOT = "boot"
CONF_WIFI_CONNECTED = "wifi_connected"
CONF_API_CONNECTED = "api_connected"
CONF_API_DISCONNECTED = "api_disconnected"
CONF_OTA_BEGIN = "ota_begin"
CONF_OTA_PROGRESS = "ota_progress"
CONF_OTA_END = "ota_end"
CONF_OTA_ERROR = "ota_error"

# Event configuration keys
CONF_ENABLED = "enabled"
CONF_COLOR = "color"
CONF_BRIGHTNESS = "brightness"
CONF_EFFECT = "effect"

# Global configuration keys
CONF_ERROR_BLINK_SPEED = "error_blink_speed"
CONF_WARNING_BLINK_SPEED = "warning_blink_speed"
CONF_BRIGHTNESS = "brightness"
CONF_PRIORITY_MODE = "priority_mode"
CONF_OK_STATE_ENABLED = "ok_state_enabled"

# Schema for RGB color configuration
ColorSchema = cv.Schema({
    cv.Required(CONF_RED): cv.percentage,
    cv.Required(CONF_GREEN): cv.percentage,
    cv.Required(CONF_BLUE): cv.percentage,
})

# Schema for individual event configuration
EventConfigSchema = cv.Schema({
    cv.Optional(CONF_ENABLED, default=True): cv.boolean,
    cv.Optional(CONF_COLOR, default={CONF_RED: 1.0, CONF_GREEN: 1.0, CONF_BLUE: 1.0}): ColorSchema,
    cv.Optional(CONF_BRIGHTNESS, default=1.0): cv.percentage,
    cv.Optional(CONF_EFFECT, default="none"): cv.string,
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
        
        # Event configurations with ESPHome-compatible defaults
        cv.Optional(CONF_ERROR, default={
            CONF_ENABLED: True,
            CONF_COLOR: {CONF_RED: 1.0, CONF_GREEN: 0.0, CONF_BLUE: 0.0},  # Red
            CONF_BRIGHTNESS: 1.0,
            CONF_EFFECT: "blink"  # Fast blink
        }): EventConfigSchema,
        
        cv.Optional(CONF_WARNING, default={
            CONF_ENABLED: True,
            CONF_COLOR: {CONF_RED: 1.0, CONF_GREEN: 0.5, CONF_BLUE: 0.0},  # Orange
            CONF_BRIGHTNESS: 1.0,
            CONF_EFFECT: "blink"  # Slow blink
        }): EventConfigSchema,
        
        cv.Optional(CONF_OK, default={
            CONF_ENABLED: True,
            CONF_COLOR: {CONF_RED: 0.0, CONF_GREEN: 1.0, CONF_BLUE: 0.1},  # Green
            CONF_BRIGHTNESS: 1.0,
            CONF_EFFECT: "none"  # Solid
        }): EventConfigSchema,
        
        cv.Optional(CONF_BOOT, default={
            CONF_ENABLED: True,
            CONF_COLOR: {CONF_RED: 1.0, CONF_GREEN: 0.0, CONF_BLUE: 0.0},  # Red
            CONF_BRIGHTNESS: 1.0,
            CONF_EFFECT: "none"  # Solid
        }): EventConfigSchema,
        
        cv.Optional(CONF_WIFI_CONNECTED, default={
            CONF_ENABLED: True,
            CONF_COLOR: {CONF_RED: 0.7, CONF_GREEN: 0.7, CONF_BLUE: 0.7},  # White
            CONF_BRIGHTNESS: 1.0,
            CONF_EFFECT: "none"  # Solid
        }): EventConfigSchema,
        
        cv.Optional(CONF_API_CONNECTED, default={
            CONF_ENABLED: True,
            CONF_COLOR: {CONF_RED: 0.0, CONF_GREEN: 1.0, CONF_BLUE: 0.1},  # Green
            CONF_BRIGHTNESS: 1.0,
            CONF_EFFECT: "none"  # Solid
        }): EventConfigSchema,
        
        cv.Optional(CONF_API_DISCONNECTED, default={
            CONF_ENABLED: True,
            CONF_COLOR: {CONF_RED: 1.0, CONF_GREEN: 1.0, CONF_BLUE: 0.0},  # Yellow
            CONF_BRIGHTNESS: 1.0,
            CONF_EFFECT: "none"  # Solid
        }): EventConfigSchema,
        
        cv.Optional(CONF_OTA_BEGIN, default={
            CONF_ENABLED: True,
            CONF_COLOR: {CONF_RED: 0.0, CONF_GREEN: 0.0, CONF_BLUE: 1.0},  # Blue
            CONF_BRIGHTNESS: 1.0,
            CONF_EFFECT: "none"  # Solid
        }): EventConfigSchema,
        
        cv.Optional(CONF_OTA_PROGRESS, default={
            CONF_ENABLED: True,
            CONF_COLOR: {CONF_RED: 0.0, CONF_GREEN: 0.0, CONF_BLUE: 1.0},  # Blue
            CONF_BRIGHTNESS: 1.0,
            CONF_EFFECT: "blink"  # Blink
        }): EventConfigSchema,
        
        cv.Optional(CONF_OTA_END, default={
            CONF_ENABLED: True,
            CONF_COLOR: {CONF_RED: 0.0, CONF_GREEN: 1.0, CONF_BLUE: 0.1},  # Green
            CONF_BRIGHTNESS: 1.0,
            CONF_EFFECT: "none"  # Solid
        }): EventConfigSchema,
        
        cv.Optional(CONF_OTA_ERROR, default={
            CONF_ENABLED: True,
            CONF_COLOR: {CONF_RED: 1.0, CONF_GREEN: 0.0, CONF_BLUE: 0.0},  # Red
            CONF_BRIGHTNESS: 1.0,
            CONF_EFFECT: "blink"  # Fast blink
        }): EventConfigSchema,
        
        # Global timing configurations
        cv.Optional(CONF_ERROR_BLINK_SPEED, default="250ms"): cv.positive_time_period,
        cv.Optional(CONF_WARNING_BLINK_SPEED, default="1500ms"): cv.positive_time_period,
        cv.Optional(CONF_BRIGHTNESS, default=0.5): cv.percentage,
        
        # Priority mode: "status" (default) or "user"
        cv.Optional(CONF_PRIORITY_MODE, default="status"): cv.enum(["status", "user"]),
        
        # OK state configuration
        cv.Optional(CONF_OK_STATE_ENABLED, default=True): cv.boolean,
    }
).extend(cv.COMPONENT_SCHEMA)


@coroutine_with_priority(CoroPriority.STATUS)
async def to_code(config):
    """
    Generate C++ code from the YAML configuration.
    
    This function is called by ESPHome during configuration validation
    and code generation. It sets up the component with all the
    specified event configurations and connects it to the RGB outputs.
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
    
    # Helper function to create EventConfig
    def create_event_config(event_config):
        color = event_config[CONF_COLOR]
        return cg.StructInitializer(
            EventConfig,
            ("enabled", event_config[CONF_ENABLED]),
            ("color", cg.StructInitializer(
                rgb_status_led_ns.struct("RGBColor"),
                ("r", color[CONF_RED]),
                ("g", color[CONF_GREEN]),
                ("b", color[CONF_BLUE])
            )),
            ("brightness", event_config[CONF_BRIGHTNESS]),
            ("effect", event_config[CONF_EFFECT])
        )
    
    # Configure event states
    cg.add(var.set_error_config(create_event_config(config[CONF_ERROR])))
    cg.add(var.set_warning_config(create_event_config(config[CONF_WARNING])))
    cg.add(var.set_ok_config(create_event_config(config[CONF_OK])))
    cg.add(var.set_boot_config(create_event_config(config[CONF_BOOT])))
    cg.add(var.set_wifi_connected_config(create_event_config(config[CONF_WIFI_CONNECTED])))
    cg.add(var.set_api_connected_config(create_event_config(config[CONF_API_CONNECTED])))
    cg.add(var.set_api_disconnected_config(create_event_config(config[CONF_API_DISCONNECTED])))
    cg.add(var.set_ota_begin_config(create_event_config(config[CONF_OTA_BEGIN])))
    cg.add(var.set_ota_progress_config(create_event_config(config[CONF_OTA_PROGRESS])))
    cg.add(var.set_ota_end_config(create_event_config(config[CONF_OTA_END])))
    cg.add(var.set_ota_error_config(create_event_config(config[CONF_OTA_ERROR])))
    
    # Configure global timing and behavior
    cg.add(var.set_error_blink_speed(config[CONF_ERROR_BLINK_SPEED]))
    cg.add(var.set_warning_blink_speed(config[CONF_WARNING_BLINK_SPEED]))
    cg.add(var.set_brightness(config[CONF_BRIGHTNESS]))
    cg.add(var.set_priority_mode(config[CONF_PRIORITY_MODE]))
    cg.add(var.set_ok_state_enabled(config[CONF_OK_STATE_ENABLED]))
    
    # Enable the component in the build
    cg.add_define("USE_RGB_STATUS_LED")
