#pragma once

#include "esphome/core/component.h"
#include "esphome/core/hal.h"
#include "esphome/components/output/float_output.h"
#include "esphome/components/light/light_output.h"
#include "esphome/core/application.h"

namespace esphome {
namespace rgb_status_led {

/**
 * @brief Status states for the RGB LED with priority ordering
 * 
 * States with higher numerical values have higher priority.
 * The component will always show the highest priority active state.
 */
enum class StatusState {
  NONE = 0,           ///< No specific state (fallback)
  OK = 1,             ///< Everything is normal (lowest priority)
  USER = 2,           ///< User is manually controlling the LED
  WIFI_CONNECTED = 3, ///< WiFi is connected but API is not
  API_CONNECTED = 4,  ///< Home Assistant API is connected
  BOOT = 5,           ///< Device is booting (first 10 seconds)
  WARNING = 6,        ///< System warnings (slow blink)
  ERROR = 7,          ///< System errors (fast blink)
  OTA_PROGRESS = 8,   ///< OTA in progress (blink)
  OTA_BEGIN = 9,      ///< OTA started (solid)
  OTA_ERROR = 10      ///< OTA error (highest priority)
};

/**
 * @brief Priority modes for status vs user control
 */
enum class PriorityMode {
  STATUS_PRIORITY = 0,  ///< Status indications take priority over user control
  USER_PRIORITY = 1     ///< User control takes priority over status indications
};

/**
 * @brief RGB Status LED Component
 * 
 * This component provides intelligent RGB LED status indication by combining:
 * - Native ESPHome application state monitoring (errors, warnings)
 * - Connection state tracking (WiFi, API)
 * - OTA progress indication
 * - Boot phase detection
 * - User control with priority management
 * 
 * Priority order (highest to lowest): OTA_ERROR > OTA_BEGIN > OTA_PROGRESS > ERROR > WARNING > BOOT > API_CONNECTED > WIFI_CONNECTED > USER > OK
 */
class RGBStatusLED : public light::LightOutput, public Component {
 public:
  RGBStatusLED();

  // Component lifecycle
  void setup() override;
  void dump_config() override;
  void loop() override;
  float get_setup_priority() const override;
  float get_loop_priority() const override;

  // Light output interface
  light::LightTraits get_traits() override;
  void write_state(light::LightState *state) override;

  /**
   * @brief Event trigger methods (callable from YAML automations)
   * 
   * These methods can be called from ESPHome automations to trigger
   * specific status states. They provide a clean interface between
   * YAML configurations and the C++ implementation.
   */
  
  /// @brief Set WiFi connection status
  void set_wifi_connected(bool connected) { 
    wifi_connected_ = connected; 
    ESP_LOGD(TAG, "WiFi %s", connected ? "connected" : "disconnected");
  }
  
  /// @brief Set Home Assistant API connection status
  void set_api_connected(bool connected) { 
    api_connected_ = connected; 
    ESP_LOGD(TAG, "API %s", connected ? "connected" : "disconnected");
  }
  
  /// @brief Mark OTA update as started
  void set_ota_begin() { 
    ota_active_ = true; 
    ota_progress_time_ = millis(); 
    ESP_LOGD(TAG, "OTA update started");
  }
  
  /// @brief Update OTA progress timestamp (for blinking effect)
  void set_ota_progress() { 
    ota_progress_time_ = millis(); 
    ESP_LOGVV(TAG, "OTA progress update");
  }
  
  /// @brief Mark OTA update as completed successfully
  void set_ota_end() { 
    ota_active_ = false; 
    ESP_LOGD(TAG, "OTA update completed");
  }
  
  /// @brief Mark OTA update as failed
  void set_ota_error() { 
    ota_active_ = false; 
    ESP_LOGD(TAG, "OTA update error");
  }

  // Output configuration
  void set_red_output(output::FloatOutput *output) { red_output_ = output; }
  void set_green_output(output::FloatOutput *output) { green_output_ = output; }
  void set_blue_output(output::FloatOutput *output) { blue_output_ = output; }

  // Color configuration
  void set_error_color(float r, float g, float b) { error_color_ = {r, g, b}; }
  void set_warning_color(float r, float g, float b) { warning_color_ = {r, g, b}; }
  void set_ok_color(float r, float g, float b) { ok_color_ = {r, g, b}; }
  void set_boot_color(float r, float g, float b) { boot_color_ = {r, g, b}; }
  void set_wifi_color(float r, float g, float b) { wifi_color_ = {r, g, b}; }
  void set_api_color(float r, float g, float b) { api_color_ = {r, g, b}; }
  void set_ota_color(float r, float g, float b) { ota_color_ = {r, g, b}; }

  // Behavior configuration
  void set_error_blink_speed(uint32_t speed) { error_blink_speed_ = speed; }
  void set_warning_blink_speed(uint32_t speed) { warning_blink_speed_ = speed; }
  void set_brightness(float brightness) { brightness_ = brightness; }
  void set_priority_mode(const std::string &mode) {
    priority_mode_ = (mode == "user") ? PriorityMode::USER_PRIORITY : PriorityMode::STATUS_PRIORITY;
  }
  void set_ok_state_enabled(bool enabled) { ok_state_enabled_ = enabled; }

 protected:
  /// @brief Tag for logging
  static const char *const TAG;

  // Hardware output components
  output::FloatOutput *red_output_{nullptr};
  output::FloatOutput *green_output_{nullptr};
  output::FloatOutput *blue_output_{nullptr};

  /**
   * @brief RGB color structure
   * 
   * Stores RGB values as floats (0.0 to 1.0) for consistency
   * with ESPHome's color system.
   */
  struct RGBColor {
    float r, g, b;
    RGBColor(float red = 0, float green = 0, float blue = 0) : r(red), g(green), b(blue) {}
  };
  
  // Color definitions with sensible defaults
  RGBColor error_color_{1.0f, 0.0f, 0.0f};     ///< Red for errors
  RGBColor warning_color_{1.0f, 0.5f, 0.0f};   ///< Orange for warnings
  RGBColor ok_color_{0.0f, 1.0f, 0.1f};        ///< Green for OK state
  RGBColor boot_color_{1.0f, 0.0f, 0.0f};      ///< Red for boot phase
  RGBColor wifi_color_{0.7f, 0.7f, 0.7f};      ///< White for WiFi connected
  RGBColor api_color_{0.0f, 1.0f, 0.1f};       ///< Green for API connected
  RGBColor ota_color_{0.0f, 0.0f, 1.0f};       ///< Blue for OTA operations

  // Timing configuration - matches ESPHome internal status_led exactly
  uint32_t error_blink_speed_{250};     ///< Error blink period in milliseconds (matches ESPHome)
  uint32_t warning_blink_speed_{1500};  ///< Warning blink period in milliseconds (matches ESPHome)
  float brightness_{0.5f};               ///< Global brightness multiplier (0.0 to 1.0)

  // Priority and behavior configuration
  PriorityMode priority_mode_{PriorityMode::STATUS_PRIORITY};
  bool ok_state_enabled_{true};  ///< Whether to show OK state or turn LED off

  // State management
  StatusState current_state_{StatusState::BOOT};  ///< Currently displayed state
  StatusState last_state_{StatusState::NONE};      ///< Previously displayed state
  bool user_control_active_{false};                 ///< Whether user is controlling the LED
  bool first_loop_{true};                           ///< First loop iteration flag
  uint32_t last_state_change_{0};                   ///< Timestamp of last state change
  uint32_t boot_complete_time_{0};                   ///< Timestamp when boot phase completes
  
  // Connection state tracking (set via automation callbacks)
  bool wifi_connected_{false};        ///< WiFi connection status
  bool api_connected_{false};         ///< Home Assistant API connection status
  bool ota_active_{false};            ///< OTA operation in progress
  uint32_t ota_progress_time_{0};     ///< Last OTA progress update timestamp

  // Core logic methods
  void update_state_();                                           ///< Main state update logic
  void set_rgb_output_(const RGBColor &color, float brightness_scale = 1.0f);  ///< Set RGB output with color
  void set_rgb_output_(float r, float g, float b, float brightness_scale = 1.0f); ///< Set RGB output with components
  StatusState determine_status_state_();                           ///< Determine current status based on all inputs
  void apply_state_(StatusState state);                           ///< Apply visual effects for a state
  bool should_show_status_();                                     ///< Check if status should override user control
  
  // Blink effect management
  bool is_blink_on_{false};            ///< Current blink state (on/off)
  uint32_t last_blink_toggle_{0};      ///< Timestamp of last blink toggle
};

}  // namespace rgb_status_led
}  // namespace esphome
