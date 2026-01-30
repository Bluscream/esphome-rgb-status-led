#pragma once

#include "esphome/core/component.h"
#include "esphome/core/hal.h"
#include "esphome/components/output/float_output.h"
#include "esphome/components/light/light_output.h"
#include "esphome/core/application.h"
#include <string>

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
 * @brief Event configuration structure for different states
 */
struct EventConfig {
  bool enabled{true};                    ///< Whether this event is enabled
  RGBColor color{0.0f, 0.0f, 0.0f};     ///< Color for this event
  float brightness{1.0f};                ///< Brightness override (0.0-1.0, 1.0 = use global)
  std::string effect{"none"};            ///< Effect to apply ("none", "blink", "pulse", etc.)
  
  EventConfig() = default;
  EventConfig(bool en, const RGBColor &col, float bright = 1.0f, const std::string &eff = "none")
    : enabled(en), color(col), brightness(bright), effect(eff) {}
};

/**
 * @brief RGB Status LED Component
 * 
 * This component provides intelligent RGB LED status indication using event-driven
 * configuration similar to ESPHome's internal status_led but with RGB capabilities.
 * 
 * Events are configured declaratively in YAML and automatically trigger based on
 * system state changes, WiFi/API connections, and OTA operations.
 * 
 * Default behavior matches ESPHome's internal status_led with RGB colors:
 * - Error: Red fast blink (60% duty, 250ms period)
 * - Warning: Orange slow blink (17% duty, 1500ms period)  
 * - OK: Green solid (or off if disabled)
 * - Boot: Red solid (first 10 seconds)
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

  // Event configuration methods
  void set_error_config(const EventConfig &config) { error_config_ = config; }
  void set_warning_config(const EventConfig &config) { warning_config_ = config; }
  void set_ok_config(const EventConfig &config) { ok_config_ = config; }
  void set_boot_config(const EventConfig &config) { boot_config_ = config; }
  void set_wifi_connected_config(const EventConfig &config) { wifi_connected_config_ = config; }
  void set_api_connected_config(const EventConfig &config) { api_connected_config_ = config; }
  void set_api_disconnected_config(const EventConfig &config) { api_disconnected_config_ = config; }
  void set_ota_begin_config(const EventConfig &config) { ota_begin_config_ = config; }
  void set_ota_progress_config(const EventConfig &config) { ota_progress_config_ = config; }
  void set_ota_end_config(const EventConfig &config) { ota_end_config_ = config; }
  void set_ota_error_config(const EventConfig &config) { ota_error_config_ = config; }

  // Output configuration
  void set_red_output(output::FloatOutput *output) { red_output_ = output; }
  void set_green_output(output::FloatOutput *output) { green_output_ = output; }
  void set_blue_output(output::FloatOutput *output) { blue_output_ = output; }

  // Global configuration
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
  
  // Event configurations with ESPHome-compatible defaults
  EventConfig error_config_{true, {1.0f, 0.0f, 0.0f}, 1.0f, "blink"};        ///< Red fast blink
  EventConfig warning_config_{true, {1.0f, 0.5f, 0.0f}, 1.0f, "blink"};      ///< Orange slow blink
  EventConfig ok_config_{true, {0.0f, 1.0f, 0.1f}, 1.0f, "none"};           ///< Green solid
  EventConfig boot_config_{true, {1.0f, 0.0f, 0.0f}, 1.0f, "none"};          ///< Red solid
  EventConfig wifi_connected_config_{true, {0.7f, 0.7f, 0.7f}, 1.0f, "none"}; ///< White solid
  EventConfig api_connected_config_{true, {0.0f, 1.0f, 0.1f}, 1.0f, "none"};   ///< Green solid
  EventConfig api_disconnected_config_{true, {1.0f, 1.0f, 0.0f}, 1.0f, "none"}; ///< Yellow solid
  EventConfig ota_begin_config_{true, {0.0f, 0.0f, 1.0f}, 1.0f, "none"};      ///< Blue solid
  EventConfig ota_progress_config_{true, {0.0f, 0.0f, 1.0f}, 1.0f, "blink"};   ///< Blue blink
  EventConfig ota_end_config_{true, {0.0f, 1.0f, 0.1f}, 1.0f, "none"};        ///< Green solid
  EventConfig ota_error_config_{true, {1.0f, 0.0f, 0.0f}, 1.0f, "blink"};      ///< Red fast blink

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
  void apply_effect_(const EventConfig &config);                   ///< Apply effect based on configuration
  
  // Effect methods
  void apply_none_effect_(const EventConfig &config);             ///< Solid color effect
  void apply_blink_effect_(const EventConfig &config, uint32_t period, uint32_t on_time); ///< Blink effect
  void apply_pulse_effect_(const EventConfig &config);            ///< Pulse effect
  
  // Blink effect management
  bool is_blink_on_{false};            ///< Current blink state (on/off)
  uint32_t last_blink_toggle_{0};      ///< Timestamp of last blink toggle
};

}  // namespace rgb_status_led
}  // namespace esphome
