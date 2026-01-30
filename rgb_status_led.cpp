#include "rgb_status_led.h"
#include "esphome/core/log.h"

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
  ESP_LOGCONFIG(TAG, "  Error blink speed: %ums", this->error_blink_speed_);
  ESP_LOGCONFIG(TAG, "  Warning blink speed: %ums", this->warning_blink_speed_);
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

void RGBStatusLED::apply_state_(StatusState state) {
  this->current_state_ = state;
  uint32_t now = millis();
  
  switch (state) {
    case StatusState::ERROR: {
      // Fast blinking
      uint32_t period = this->error_blink_speed_;
      uint32_t on_time = period * 3 / 4;  // 75% on, 25% off
      
      if ((now % period) < on_time) {
        if (!this->is_blink_on_) {
          this->set_rgb_output_(this->error_color_);
          this->is_blink_on_ = true;
        }
      } else {
        if (this->is_blink_on_) {
          this->set_rgb_output_(0.0f, 0.0f, 0.0f);
          this->is_blink_on_ = false;
        }
      }
      break;
    }
    
    case StatusState::WARNING: {
      // Slow blinking
      uint32_t period = this->warning_blink_speed_;
      uint32_t on_time = period / 4;  // 25% on, 75% off
      
      if ((now % period) < on_time) {
        if (!this->is_blink_on_) {
          this->set_rgb_output_(this->warning_color_);
          this->is_blink_on_ = true;
        }
      } else {
        if (this->is_blink_on_) {
          this->set_rgb_output_(0.0f, 0.0f, 0.0f);
          this->is_blink_on_ = false;
        }
      }
      break;
    }
    
    case StatusState::BOOT:
      // Solid boot color
      this->set_rgb_output_(this->boot_color_);
      this->is_blink_on_ = false;
      break;
      
    case StatusState::WIFI_CONNECTED:
      // Solid WiFi color (white)
      this->set_rgb_output_(this->wifi_color_);
      this->is_blink_on_ = false;
      break;
      
    case StatusState::API_CONNECTED:
      // Solid API color (green)
      this->set_rgb_output_(this->api_color_);
      this->is_blink_on_ = false;
      break;
      
    case StatusState::OTA_BEGIN:
      // Solid OTA color (blue)
      this->set_rgb_output_(this->ota_color_);
      this->is_blink_on_ = false;
      break;
      
    case StatusState::OTA_PROGRESS:
      // Blinking OTA color (blue)
      uint32_t ota_period = 1000;  // 1 second period
      if ((now % ota_period) < (ota_period / 2)) {
        if (!this->is_blink_on_) {
          this->set_rgb_output_(this->ota_color_);
          this->is_blink_on_ = true;
        }
      } else {
        if (this->is_blink_on_) {
          this->set_rgb_output_(0.0f, 0.0f, 0.0f);
          this->is_blink_on_ = false;
        }
      }
      break;
      
    case StatusState::OK:
      // Solid OK color
      this->set_rgb_output_(this->ok_color_);
      this->is_blink_on_ = false;
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
