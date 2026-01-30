# ESPHome RGB Status LED Component

A powerful ESPHome external component that provides intelligent RGB LED status indication by combining native ESPHome state monitoring with connection tracking and OTA progress indication.

## 🚀 Features

- **Native ESPHome Integration**: Hooks into ESPHome's application state for automatic error/warning detection
- **ESPHome Compatible Timing**: Matches internal status_led blink patterns exactly (60% error duty, 17% warning duty)
- **Connection Status Tracking**: Monitors WiFi and Home Assistant API connection status
- **OTA Progress Indication**: Shows visual feedback during Over-The-Air updates
- **Boot Phase Detection**: Indicates device startup status
- **Priority Management**: Intelligent priority system balancing status indications with user control
- **Full Customization**: Configurable colors, timing, and behavior
- **Power Saving Mode**: Optional OK state disabling for battery-powered devices
- **Clean Interface**: Simple YAML configuration with comprehensive automation support

## 📋 Status States & Priority

The component follows a strict priority system (highest to lowest):

| Priority | State | Color | Effect | Description |
|----------|-------|-------|--------|-------------|
| 10 | **OTA Error** | 🔴 Red | Fast Blink | Critical OTA failure |
| 9 | **OTA Begin** | 🔵 Blue | Solid | OTA update started |
| 8 | **OTA Progress** | 🔵 Blue | Blink | OTA in progress |
| 7 | **Error** | 🔴 Red | Fast Blink | System errors (configuration, hardware) |
| 6 | **Warning** | 🟠 Orange | Slow Blink | System warnings (sensor failures, etc.) |
| 5 | **Boot** | 🔴 Red | Solid | Device booting (first 10s) |
| 4 | **API Connected** | 🟢 Green | Solid | Home Assistant API connected |
| 3 | **WiFi Connected** | ⚪ White | Solid | WiFi connected (no API) |
| 2 | **User Control** | 🎨 Custom | User-defined | Manual user control |
| 1 | **OK** | 🟢 Green | Solid | Everything normal |

## 🛠️ Installation

### 1. Clone the Component

```bash
# In your ESPHome configuration directory
mkdir components
cd components
git clone https://github.com/your-username/esphome-rgb-status-led.git rgb_status_led
```

### 2. Configure ESPHome

Add the external component to your YAML configuration:

```yaml
# Use local external components
external_components:
  - source: components

# RGB Status LED configuration
light:
  - platform: rgb_status_led
    id: system_status_led
    name: "System Status LED"
    red: out_led_r
    green: out_led_g
    blue: out_led_b
    
    # Custom colors (optional - defaults shown)
    error_color:
      red: 100%
      green: 0%
      blue: 0%
    warning_color:
      red: 100%
      green: 50%
      blue: 0%
    ok_color:
      red: 0%
      green: 100%
      blue: 10%
    boot_color:
      red: 100%
      green: 0%
      blue: 0%
    wifi_color:
      red: 70%
      green: 70%
      blue: 70%
    api_color:
      red: 0%
      green: 100%
      blue: 10%
    ota_color:
      red: 0%
      green: 0%
      blue: 100%
    
    # Timing configuration
    error_blink_speed: 250ms
    warning_blink_speed: 1500ms
    brightness: 50%
    
    # Priority mode
    priority_mode: "status"  # or "user"

# RGB LED outputs
output:
  - platform: ledc
    id: out_led_r
    pin: GPIO4
    channel: 0
  - platform: ledc
    id: out_led_g
    pin: GPIO19
    channel: 1
  - platform: ledc
    id: out_led_b
    pin: GPIO18
    channel: 2
```

### 3. Add Event Automations

```yaml
# WiFi connection events
wifi:
  on_connect: 
    then:
      - lambda: 'id(system_status_led).set_wifi_connected(true);'
  on_disconnect: 
    then:
      - lambda: 'id(system_status_led).set_wifi_connected(false);'

# Home Assistant API events
api:
  on_client_connected:
    then:
      - lambda: 'id(system_status_led).set_api_connected(true);'
  on_client_disconnected:
    then:
      - lambda: 'id(system_status_led).set_api_connected(false);'

# OTA events
ota:
  on_begin:
    then:
      - lambda: 'id(system_status_led).set_ota_begin();'
  on_progress:
    then:
      - lambda: 'id(system_status_led).set_ota_progress();'
  on_end:
    then:
      - lambda: 'id(system_status_led).set_ota_end();'
  on_error:
    then:
      - lambda: 'id(system_status_led).set_ota_error();'
```

## ⚙️ Configuration Options

### Color Configuration

Each status state can be customized with RGB values:

```yaml
error_color:
  red: 100%    # 0.0 to 1.0 or 0% to 100%
  green: 0%
  blue: 0%
```

### Behavior Configuration

| Option | Default | Description |
|--------|---------|-------------|
| `error_blink_speed` | 250ms | Blink period for error state |
| `warning_blink_speed` | 1500ms | Blink period for warning state |
| `brightness` | 50% | Global brightness multiplier |
| `priority_mode` | "status" | "status" or "user" priority mode |
| `ok_state_enabled` | true | Show OK state (true) or turn LED off when OK (false) |

### OK State Configuration

The `ok_state_enabled` option provides power-saving functionality:

- **`true`** (default): LED shows configured `ok_color` when everything is normal
- **`false`**: LED turns OFF when everything is normal (only lights up for important states)

**Example for power saving:**
```yaml
light:
  - platform: rgb_status_led
    # ... other configuration ...
    ok_state_enabled: false  # LED off when everything is OK
```

**With `ok_state_enabled: false`, the LED behavior becomes:**
- Boot: Red solid (first 10 seconds)
- WiFi connected: White solid  
- API connected: Green solid
- Errors: Red fast blink
- Warnings: Orange slow blink
- OTA: Blue solid/blinking
- **Everything OK: LED OFF** (power saving)

### Priority Modes

- **"status"** (default): Status indications take priority over user control
- **"user"**: User control takes priority over status indications

## 🎯 Use Cases

### Basic Status Monitoring
Perfect for devices where you want to see the system status at a glance:
- Boot status
- WiFi connectivity
- Home Assistant connection
- System errors and warnings

### Development & Debugging
Essential during development:
- Visual feedback during OTA updates
- Error indication for configuration issues
- Warning detection for sensor problems

### Production Monitoring
Ideal for deployed devices:
- Quick visual health check
- Remote status indication
- User-friendly feedback

## 🔧 Advanced Usage

### Custom State Triggers

You can manually trigger states from your automations:

```yaml
# Show WiFi connected state
- lambda: 'id(system_status_led).set_wifi_connected(true);'

# Start OTA indication
- lambda: 'id(system_status_led).set_ota_begin();'

# Mark OTA complete
- lambda: 'id(system_status_led).set_ota_end();'
```

### Integration with Other Components

Works seamlessly with other ESPHome components:

```yaml
# Button to toggle LED
button:
  - platform: gpio
    name: "Toggle LED"
    pin: GPIO27
    on_click:
      then:
        - light.toggle: system_status_led

# Switch for alarm indication
switch:
  - platform: template
    name: "Alarm"
    turn_on_action:
      - light.turn_on:
          id: system_status_led
          red: 100%
          green: 0%
          blue: 0%
          brightness: 100%
```

## 🐛 Troubleshooting

### LED Not Working
1. Check GPIO pin assignments
2. Verify output component configuration
3. Ensure proper power supply for RGB LED
4. Check component compilation logs

### Wrong Colors
1. Verify RGB color values (0.0-1.0 or 0%-100%)
2. Check LED RGB order (common vs GRB)
3. Test with basic color configuration

### Status Not Updating
1. Verify automation callbacks are configured
2. Check log output for state changes
3. Ensure priority mode matches expectations

## 🤝 Contributing

Contributions are welcome! Please feel free to submit issues and pull requests.

### Development Setup

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Test thoroughly
5. Submit a pull request

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## 🙏 Acknowledgments

- ESPHome team for the excellent framework
- Community members for feedback and suggestions
- Original status_led component for inspiration

## 📚 Additional Resources

- [ESPHome Documentation](https://esphome.io/)
- [ESPHome Components Guide](https://esphome.io/components/)
- [ESPHome External Components](https://esphome.io/components/external_components/)

## 🔧 Technical Details

### ESPHome Compatibility

This component is designed to be fully compatible with ESPHome's internal status_led implementation:

#### **Status Flag Detection**
```cpp
// Uses the same flags as internal status_led
uint32_t app_state = App.get_app_state();
if ((app_state & STATUS_LED_ERROR) != 0u) { ... }    // Bit 4 (0x10)
if ((app_state & STATUS_LED_WARNING) != 0u) { ... }  // Bit 3 (0x08)
```

#### **Blink Timing**
```cpp
// Error: 250ms period, 150ms on (60% duty cycle) - matches ESPHome exactly
millis() % 250u < 150u

// Warning: 1500ms period, 250ms on (17% duty cycle) - matches ESPHome exactly  
millis() % 1500u < 250u
```

#### **Priority System**
- **ERROR** (bit 4) takes precedence over **WARNING** (bit 3)
- Additional states (OTA, WiFi, API, Boot) have higher priority
- Maintains backward compatibility with existing ESPHome status behavior

### Component Architecture

```
RGBStatusLED (LightOutput + Component)
├── State Management
│   ├── determine_status_state_() - Priority-based state selection
│   ├── apply_state_() - Visual effects and timing
│   └── should_show_status_() - User vs status priority
├── Event Integration  
│   ├── set_wifi_connected() - WiFi event handler
│   ├── set_api_connected() - API event handler
│   └── set_ota_*() - OTA event handlers
└── Output Control
    ├── set_rgb_output_() - Hardware abstraction
    └── Color management with brightness scaling
```

### Memory Footprint

- **RAM Usage**: ~200 bytes (state tracking + color buffers)
- **Flash Usage**: ~8KB (compiled component)
- **CPU Overhead**: Minimal (state checks only in loop)
- **Compatible with**: ESP32, ESP32-S2, ESP32-S3, ESP32-C3, ESP8266
