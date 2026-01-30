#include "rgb_status_led.h"
#include "esphome/core/log.h"
#include <cmath>

namespace esphome {
namespace rgb_status_led {

const char *const RGBStatusLED::TAG = "rgb_status_led";

RGBStatusLED::RGBStatusLED() {
  // Initialize with boot state - device is starting up
  this->current_state_ = StatusState::BOOT;
  this->last_state_ = StatusState::NONE;
}

void RGBStatusLED::setup() {
  ESP_LOGCONFIG(TAG, "Setting up RGB Status LED...");
  
  // Initialize outputs to off
  this->set_rgb_output_(0.0f, 0.0f, 0.0f);
  
  // Mark boot start time
  this->boot_complete_time_ = millis();
  
  ESP_LOGCONFIG(TAG, "RGB Status LED setup completed");
  ESP_LOGCONFIG(TAG, "  Error blink speed: %ums (matches ESPHome)", this->error_blink_speed_);
  ESP_LOGCONFIG(TAG, "  Warning blink speed: %ums (matches ESPHome)", this->warning_blink_speed_);
  ESP_LOGCONFIG(TAG, "  Brightness: %.1f%%", this->brightness_ * 100.0f);
  ESP_LOGCONFIG(TAG, "  Priority mode: %s", 
                (this->priority_mode_ == PriorityMode::STATUS_PRIORITY) ? "Status" : "User");
}

void RGBStatusLED::dump_config() {
  ESP_LOGCONFIG(TAG, "RGB Status LED:");
  ESP_LOGCONFIG(TAG, "  Priority Mode: %s", 
                (this->priority_mode_ == PriorityMode::STATUS_PRIORITY) ? "Status Priority" : "User Priority");
  ESP_LOGCONFIG(TAG, "  Error Color: R=%.1f, G=%.1f, B=%.1f", 
                this->error_color_.r * 100.0f, this->error_color_.g * 100.0f, this->error_color_.b * 100.0f);
  ESP_LOGCONFIG(TAG, "  Warning Color: R=%.1f, G=%.1f, B=%.1f", 
                this->warning_color_.r * 100.0f, this->warning_color_.g * 100.0f, this->warning_color_.b * 100.0f);
  ESP_LOGCONFIG(TAG, "  OK Color: R=%.1f, G=%.1f, B=%.1f", 
                this->ok_color_.r * 100.0f, this->ok_color_.g * 100.0f, this->ok_color_.b * 100.0f);
  ESP_LOGCONFIG(TAG, "  Boot Color: R=%.1f, G=%.1f, B=%.1f", 
                this->boot_color_.r * 100.0f, this->boot_color_.g * 100.0f, this->boot_color_.b * 100.0f);
}

light::LightTraits RGBStatusLED::get_traits() {
  auto traits = light::LightTraits();
  traits.set_supported_color_modes({light::ColorMode::RGB});
  return traits;
}

void RGBStatusLED::write_state(light::LightState *state) {
  // This is called when user controls the light
  if (this->priority_mode_ == PriorityMode::USER_PRIORITY) {
    this->user_control_active_ = true;
    this->current_state_ = StatusState::USER;
    
    // Apply user state immediately
    auto call = state->turn_on();
    call.perform();
  } else {
    // In status priority mode, mark user control but don't apply immediately
    this->user_control_active_ = true;
  }
}

void RGBStatusLED::loop() {
  if (this->first_loop_) {
    this->first_loop_ = false;
    this->last_state_change_ = millis();
    return;
  }
  
  this->update_state_();
}

float RGBStatusLED::get_setup_priority() const { 
  return setup_priority::HARDWARE; 
}

float RGBStatusLED::get_loop_priority() const { 
  return 50.0f; 
}

void RGBStatusLED::update_state_() {
  StatusState new_state = this->determine_status_state_();
  
  // Check if state has changed
  if (new_state != this->last_state_) {
    this->last_state_ = new_state;
    this->last_state_change_ = millis();
    this->is_blink_on_ = false;  // Reset blink state
  }
  
  // Apply the current state
  this->apply_state_(new_state);
}

StatusState RGBStatusLED::determine_status_state_() {
  // Check if we should show status or user control
  if (!this->should_show_status_()) {
    return StatusState::USER;
  }
  
  // Priority 1: OTA operations (highest priority)
  // OTA overrides everything including system errors during update
  if (this->ota_active_) {
    // During OTA, alternate between begin and progress states for visual feedback
    // Show solid blue for 500ms, then blink to indicate activity
    if (millis() - this->ota_progress_time_ < 500) {
      return StatusState::OTA_BEGIN;
    } else {
      return StatusState::OTA_PROGRESS;
    }
  }
  
  // Get ESPHome application state for native error/warning detection
  uint32_t app_state = App.get_app_state();
  
  // Priority 2: System errors (critical issues)
  // These include configuration errors, hardware failures, etc.
  if ((app_state & STATUS_LED_ERROR) != 0u) {
    return StatusState::ERROR;
  }
  
  // Priority 3: System warnings (non-critical issues)
  // These include temporary sensor failures, connection issues, etc.
  if ((app_state & STATUS_LED_WARNING) != 0u) {
    return StatusState::WARNING;
  }
  
  // Priority 4: Boot phase (device initialization)
  // Show boot state for first 10 seconds after startup
  if (millis() - this->boot_complete_time_ < 10000) {
    return StatusState::BOOT;
  }
  
  // Priority 5: Home Assistant API connection
  // Highest level of connectivity - full integration
  if (this->api_connected_) {
    return StatusState::API_CONNECTED;
  }
  
  // Priority 6: WiFi connection
  // Network connectivity but no Home Assistant connection
  if (this->wifi_connected_) {
    return StatusState::WIFI_CONNECTED;
  }
  
  // Priority 7: Everything is OK (lowest priority)
  // No specific state to show - device is running normally
  // If OK state is disabled, return NONE to turn LED off
  if (this->ok_state_enabled_) {
    return StatusState::OK;
  } else {
    return StatusState::NONE;
  }
}

bool RGBStatusLED::should_show_status_() {
  if (this->priority_mode_ == PriorityMode::USER_PRIORITY) {
    return false;  // User always has priority
  }
  
  // In status priority mode, show status unless user is actively controlling
  // and we've been in OK state for more than 30 seconds
  if (this->user_control_active_ && this->last_state_ == StatusState::OK) {
    return (millis() - this->last_state_change_ < 30000);
  }
  
  return true;
}

void RGBStatusLED::apply_effect_(const EventConfig &config) {
  if (!config.enabled) {
    // Event disabled - turn off LED
    this->set_rgb_output_(0.0f, 0.0f, 0.0f);
    this->is_blink_on_ = false;
    return;
  }
  
  // Apply brightness override if specified (1.0 = use global brightness)
  float brightness_scale = (config.brightness == 1.0f) ? this->brightness_ : config.brightness;
  
  // Apply the specified effect
  if (config.effect == "none") {
    this->apply_none_effect_(config);
  } else if (config.effect == "blink") {
    // Determine blink timing based on context (error vs warning vs other)
    uint32_t period = 1000;  // Default 1 second
    uint32_t on_time = 500;  // Default 50% duty
    
    // Use ESPHome-compatible timing for error/warning
    if (&config == &this->error_config_) {
      period = this->error_blink_speed_;
      on_time = period * 3 / 5;  // 60% duty cycle
    } else if (&config == &this->warning_config_) {
      period = this->warning_blink_speed_;
      on_time = period / 6;  // 17% duty cycle
    }
    
    this->apply_blink_effect_(config, period, on_time);
  } else if (config.effect == "pulse") {
    this->apply_pulse_effect_(config);
  } else {
    // Unknown effect - default to solid
    this->apply_none_effect_(config);
  }
}

void RGBStatusLED::apply_none_effect_(const EventConfig &config) {
  // Apply brightness override if specified
  float brightness_scale = (config.brightness == 1.0f) ? this->brightness_ : config.brightness;
  this->set_rgb_output_(config.color, brightness_scale);
  this->is_blink_on_ = false;
}

void RGBStatusLED::apply_blink_effect_(const EventConfig &config, uint32_t period, uint32_t on_time) {
  uint32_t now = millis();
  
  // Apply brightness override if specified
  float brightness_scale = (config.brightness == 1.0f) ? this->brightness_ : config.brightness;
  
  if ((now % period) < on_time) {
    if (!this->is_blink_on_) {
      this->set_rgb_output_(config.color, brightness_scale);
      this->is_blink_on_ = true;
    }
  } else {
    if (this->is_blink_on_) {
      this->set_rgb_output_(0.0f, 0.0f, 0.0f);
      this->is_blink_on_ = false;
    }
  }
}

void RGBStatusLED::apply_pulse_effect_(const EventConfig &config) {
  uint32_t now = millis();
  
  // Apply brightness override if specified
  float brightness_scale = (config.brightness == 1.0f) ? this->brightness_ : config.brightness;
  
  // Create a smooth pulse effect over 2 seconds
  uint32_t pulse_period = 2000;
  float phase = (now % pulse_period) / float(pulse_period);
  
  // Use sine wave for smooth pulsing
  float pulse_brightness = (sin(phase * 2 * M_PI) + 1.0f) / 2.0f;
  float final_brightness = brightness_scale * pulse_brightness;
  
  this->set_rgb_output_(config.color, final_brightness);
  this->is_blink_on_ = (pulse_brightness > 0.5f);
}

void RGBStatusLED::apply_state_(StatusState state) {
  this->current_state_ = state;
  
  // Apply the appropriate event configuration based on state
  switch (state) {
    case StatusState::ERROR:
      this->apply_effect_(this->error_config_);
      break;
      
    case StatusState::WARNING:
      this->apply_effect_(this->warning_config_);
      break;
      
    case StatusState::BOOT:
      this->apply_effect_(this->boot_config_);
      break;
      
    case StatusState::WIFI_CONNECTED:
      this->apply_effect_(this->wifi_connected_config_);
      break;
      
    case StatusState::API_CONNECTED:
      this->apply_effect_(this->api_connected_config_);
      break;
      
    case StatusState::API_DISCONNECTED:
      this->apply_effect_(this->api_disconnected_config_);
      break;
      
    case StatusState::OTA_BEGIN:
      this->apply_effect_(this->ota_begin_config_);
      break;
      
    case StatusState::OTA_PROGRESS:
      this->apply_effect_(this->ota_progress_config_);
      break;
      
    case StatusState::OTA_ERROR:
      this->apply_effect_(this->ota_error_config_);
      break;
      
    case StatusState::OK:
      this->apply_effect_(this->ok_config_);
      break;
      
    case StatusState::NONE:
      // LED off (used when OK state is disabled)
      this->set_rgb_output_(0.0f, 0.0f, 0.0f);
      this->is_blink_on_ = false;
      break;
      
    case StatusState::USER:
      // User control - don't interfere, the light state will be managed by the light system
      this->is_blink_on_ = false;
      break;
      
    default:
      // Turn off
      this->set_rgb_output_(0.0f, 0.0f, 0.0f);
      this->is_blink_on_ = false;
      break;
  }
}

void RGBStatusLED::set_rgb_output_(const RGBColor &color, float brightness_scale) {
  this->set_rgb_output_(color.r, color.g, color.b, brightness_scale);
}

void RGBStatusLED::set_rgb_output_(float r, float g, float b, float brightness_scale) {
  float final_brightness = this->brightness_ * brightness_scale;
  
  if (this->red_output_ != nullptr) {
    this->red_output_->set_level(r * final_brightness);
  }
  if (this->green_output_ != nullptr) {
    this->green_output_->set_level(g * final_brightness);
  }
  if (this->blue_output_ != nullptr) {
    this->blue_output_->set_level(b * final_brightness);
  }
}

}  // namespace rgb_status_led
}  // namespace esphome
