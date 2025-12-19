# Fan Control System - Aeris Lite

## Overview

The Aeris Lite includes an intelligent fan control system to improve sensor accuracy by managing airflow across the sensors. This is especially important because some sensors (SGP41, SCD40) generate heat during operation which can affect temperature and humidity readings.

## Hardware Setup

### Components Required

- **Fan**: 5V 4-wire PWM fan (25mm, 30mm, or 40mm)
  - Recommended: Dual ball bearing type for reliability
  - Example: Sunon MF25100V1-1000U-A99 or similar
- **MOSFET**: IRLML6344TRPBF (N-channel, SOT-23)
- **Diode**: N5819HW1-7-F (1A Schottky, SOD-123)
- **Resistor**: 10kΩ (0603/0805 SMD)

### Circuit Diagram

```
              5V Supply
                 │
              [Fan Motor]
                 │
         ┌──[N5819HW1-7-F]──┐  Flyback protection diode
         │   (Schottky)      │  (Cathode to 5V, Anode to Drain)
         │                   │
    [Drain]                  │
         │                   │
   IRLML6344TRPBF            │
         │                   │
    [Gate]────[10kΩ]─────────┘  Pull-down resistor
         │                   
GPIO5 (3.3V)                   Power control
         │                   
    [Source]                 
         │                   
        GND                  

Fan 4-Wire Connections:
  Red/Yellow → 5V (through MOSFET)
  Black      → GND
  Blue       → GPIO6 (PWM speed control)
  Yellow     → GPIO7 (TACH feedback)
```

### GPIO Pin Assignments

| Function | GPIO | Direction | Description |
|----------|------|-----------|-------------|
| Fan Power | GPIO5 | Output | N-MOSFET gate (ON/OFF control) |
| Fan PWM | GPIO6 | Output | PWM signal (25kHz, 0-100% duty) |
| Fan TACH | GPIO7 | Input | Tachometer pulses (RPM monitoring) |

## Features

### 1. PWM Speed Control
- **Frequency**: 25kHz (standard for PC fans)
- **Resolution**: 8-bit (0-255 duty cycle)
- **Speed Range**: 0-100%
- **Minimum Speed**: 20% (fans typically don't start below this)

### 2. RPM Monitoring
- Real-time RPM measurement via tachometer
- 2 pulses per revolution (standard)
- Blocking measurement (~1 second)
- Health monitoring and fault detection

### 3. Operating Modes
- **OFF** (0%): Fan completely stopped
- **LOW** (30%): Quiet operation, minimal airflow
- **MEDIUM** (60%): Balanced airflow and noise
- **HIGH** (100%): Maximum airflow
- **AUTO**: Adaptive speed based on sensor readings

### 4. Adaptive Control
Automatically adjusts fan speed based on:
- **Temperature**: Higher temps → faster fan
- **VOC Index**: Poor air quality → faster fan
- **Thresholds**:
  - T > 35°C or VOC > 300 → HIGH (100%)
  - T > 30°C or VOC > 200 → MEDIUM (60%)
  - T > 25°C or VOC > 150 → LOW (30%)
  - Otherwise → OFF (0%)

### 5. Health Monitoring
- Detects fan stall/failure (RPM < 100)
- Verifies fan starts when commanded
- Can trigger alerts or fallback behavior

## Software API

### Initialization

```c
#include "fan_control.h"

// Initialize fan control (call once at startup)
esp_err_t ret = fan_init();
if (ret != ESP_OK) {
    ESP_LOGE("APP", "Fan init failed");
}
```

### Basic Control

```c
// Set fan speed (0-100%)
fan_set_speed(50);  // 50% speed

// Power control
fan_set_power(true);   // Turn on
fan_set_power(false);  // Turn off

// Predefined modes
fan_set_mode(FAN_MODE_LOW);     // 30%
fan_set_mode(FAN_MODE_MEDIUM);  // 60%
fan_set_mode(FAN_MODE_HIGH);    // 100%
fan_set_mode(FAN_MODE_OFF);     // 0%
```

### RPM Monitoring

```c
// Get current RPM (blocks for ~1 second)
uint32_t rpm = fan_get_rpm();
ESP_LOGI("APP", "Fan RPM: %lu", rpm);

// Quick check (uses last reading)
bool running = fan_is_running();
if (!running) {
    ESP_LOGW("APP", "Fan not running!");
}

// Get full status
fan_status_t status;
fan_get_status(&status);
ESP_LOGI("APP", "Enabled: %d, Speed: %d%%, RPM: %lu, Running: %d, Fault: %d",
         status.enabled, status.speed_percent, status.rpm, 
         status.running, status.fault);
```

### Advanced: Adaptive Control

```c
// In your main measurement loop:
void sensor_measurement_task(void *pvParameters) {
    while (1) {
        float temp = read_temperature();
        uint16_t voc = read_voc_index();
        
        // Automatically adjust fan speed
        fan_adaptive_control(temp, voc);
        
        vTaskDelay(pdMS_TO_TICKS(30000));  // Every 30s
    }
}
```

### Health Check Example

```c
// Set speed and verify fan starts
esp_err_t ret = fan_control_with_check(60);
if (ret == ESP_FAIL) {
    ESP_LOGE("APP", "Fan failed to start - check hardware!");
    // Could trigger alarm, LED indicator, etc.
}
```

## Integration with Aeris Driver

The fan control is automatically initialized in `aeris_driver_init()` and starts at low speed (30%) by default. This provides continuous gentle airflow for accurate sensor readings while keeping noise low.

To enable adaptive control, modify your measurement loop:

```c
aeris_sensor_state_t state;
aeris_get_sensor_data(&state);

// Adaptive fan control based on readings
fan_adaptive_control(state.temperature_c, state.voc_index);
```

## Zigbee Integration (Future Enhancement)

The fan control can be exposed via Zigbee custom attributes:

```c
/* Proposed custom attributes for fan control */
#define ATTR_FAN_ENABLED      0xF030  // bool - Fan power ON/OFF
#define ATTR_FAN_SPEED        0xF031  // uint8 - Speed percentage (0-100)
#define ATTR_FAN_MODE         0xF032  // uint8 - Mode (0=OFF, 1=LOW, 2=MED, 3=HIGH, 4=AUTO)
#define ATTR_FAN_RPM          0xF033  // uint32 - Current RPM (read-only)
#define ATTR_FAN_STATUS       0xF034  // uint8 - Status (0=Off, 1=Running, 2=Fault)
```

This would allow control via Home Assistant/Zigbee2MQTT:
- Enable/disable fan remotely
- Adjust speed percentage
- Select operating mode
- Monitor RPM and health status

## Troubleshooting

### Fan Doesn't Start
1. Check 5V power supply (adequate current)
2. Verify GPIO connections (GPIO5, GPIO6, GPIO7)
3. Check MOSFET soldering (IRLML6344TRPBF)
4. Verify diode polarity (N5819HW1-7-F)
5. Try higher speed: `fan_set_speed(50)` or more
6. Check fan minimum starting speed (usually 20-30%)

### Fan Runs But No RPM Reading
1. Verify TACH wire connected to GPIO7
2. Check pull-up enabled (automatic in code)
3. Confirm fan has 4 wires (TACH output)
3-wire fans don't have TACH
4. Try different fan (some cheap fans have fake TACH)

### Fan Too Noisy
1. Reduce speed: `fan_set_speed(30)` or lower
2. Check fan quality (dual ball bearing are quieter long-term)
3. Verify fan is mounted securely (vibration causes noise)
4. Consider larger fan at lower RPM (40mm vs 25mm)

### Sensors Still Show Self-Heating
1. Increase fan speed: `fan_set_speed(60)`
2. Check airflow path (ensure air reaches sensors)
3. Verify fan is drawing air across sensors
4. Use temperature offset calibration (existing feature)
5. Consider enclosure ventilation holes

### Fan Runs at Full Speed Always
1. Check PWM wire connected to GPIO6 (not 5V)
2. Verify fan supports PWM control (4-wire)
3. Check PWM signal with oscilloscope (25kHz)
4. Try different GPIO if interference suspected

## Power Consumption

| Mode | Fan Speed | Typical Current | Power |
|------|-----------|-----------------|-------|
| OFF | 0% | 0mA | 0W |
| LOW | 30% | 40-60mA | 0.2-0.3W |
| MEDIUM | 60% | 80-100mA | 0.4-0.5W |
| HIGH | 100% | 120-150mA | 0.6-0.75W |

**Note**: Power consumption varies by fan model. Larger fans draw more current.

## PCB Layout Recommendations

1. **Trace widths**: 
   - 5V power to fan: 20-30 mil (0.5-0.76mm)
   - GPIO signals: 10 mil (0.25mm) minimum
   
2. **Component placement**:
   - Place MOSFET and diode close to fan connector
   - Keep PWM trace short to reduce EMI
   
3. **Ground plane**: 
   - Connect MOSFET source directly to ground plane
   - Use thermal relief for easy soldering
   
4. **Thermal considerations**:
   - No heatsink needed for IRLML6344TRPBF (<1mW dissipation)
   - Keep away from hot components if possible

## Performance Impact

### With Fan (Recommended)
- ✅ Temperature accuracy: ±0.2°C
- ✅ Humidity accuracy: ±1%RH
- ✅ Fast response time (<30s)
- ✅ Representative sampling
- ⚠️ Additional power: ~0.2-0.5W
- ⚠️ Slight noise (dependent on speed)

### Without Fan
- ⚠️ Temperature error: +2-4°C (self-heating)
- ⚠️ Humidity affected by temperature error
- ⚠️ Slow response time (>2 min)
- ⚠️ Stagnant air (non-representative)
- ✅ Lower power consumption
- ✅ Silent operation

## Conclusion

The fan control system significantly improves sensor accuracy for the Aeris Lite by managing self-heating effects and ensuring representative air sampling. The adaptive control automatically balances performance, accuracy, and noise based on environmental conditions.
