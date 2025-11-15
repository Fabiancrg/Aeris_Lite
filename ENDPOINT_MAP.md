# Aeris_zb Endpoint Map

## Complete Endpoint Structure

```
┌─────────────────────────────────────────────────────────────────┐
│                     Aeris Air Quality Sensor                     │
│                      (Zigbee Router Device)                      │
└─────────────────────────────────────────────────────────────────┘

Endpoint 1: Temperature & Humidity
├─ Temperature Measurement (0x0402)
│  └─ Measured Value (int16, 0.01°C units)
├─ Relative Humidity Measurement (0x0405)
│  └─ Measured Value (uint16, 0.01% units)
└─ Basic, Identify clusters

Endpoint 2: Pressure
├─ Pressure Measurement (0x0403)
│  └─ Measured Value (int16, 0.1 hPa units)
└─ Basic, Identify clusters

Endpoint 3: PM1.0
├─ Analog Input (0x000C)
│  ├─ Present Value (float, µg/m³)
│  └─ Description: "PM1.0 µg/m³"
└─ Basic, Identify clusters

Endpoint 4: PM2.5
├─ Analog Input (0x000C)
│  ├─ Present Value (float, µg/m³)
│  └─ Description: "PM2.5 µg/m³"
└─ Basic, Identify clusters

Endpoint 5: PM10
├─ Analog Input (0x000C)
│  ├─ Present Value (float, µg/m³)
│  └─ Description: "PM10 µg/m³"
└─ Basic, Identify clusters

Endpoint 6: VOC Index
├─ Analog Input (0x000C)
│  ├─ Present Value (float, 1-500 index)
│  └─ Description: "VOC Index"
└─ Basic, Identify clusters

Endpoint 7: CO2
├─ Carbon Dioxide Measurement (0x040D)
│  ├─ Measured Value (float, ppm)
│  ├─ Min Measured Value: 400
│  └─ Max Measured Value: 5000
└─ Basic, Identify clusters

Endpoint 8: LED Configuration
├─ On/Off (0x0006)
│  └─ OnOff (bool) - Enable/disable LED
├─ Analog Output (0x000D) - Container for custom attributes
│  ├─ 0xF000: VOC Orange Threshold (uint16)
│  ├─ 0xF001: VOC Red Threshold (uint16)
│  ├─ 0xF002: CO2 Orange Threshold (uint16)
│  ├─ 0xF003: CO2 Red Threshold (uint16)
│  ├─ 0xF004: Humidity Orange Low (uint16)
│  ├─ 0xF005: Humidity Orange High (uint16)
│  ├─ 0xF006: Humidity Red Low (uint16)
│  ├─ 0xF007: Humidity Red High (uint16)
│  ├─ 0xF008: PM2.5 Orange Threshold (uint16)
│  └─ 0xF009: PM2.5 Red Threshold (uint16)
└─ Basic, Identify clusters
```

## Sensor Hardware

```
┌─────────────────────────────────────────────────────────────────┐
│                          I2C Bus (GPIO6/7)                       │
│                           100 kHz, 3.3V                          │
└─────────────────────────────────────────────────────────────────┘
    │
    ├─ SHT45 (0x44)      → Temperature & Humidity
    ├─ LPS22HB (0x5D)    → Pressure
    ├─ SGP41 (0x59)      → VOC Raw Signals → VOC Index
    └─ SCD40 (0x62)      → CO2

┌─────────────────────────────────────────────────────────────────┐
│                       UART (GPIO20, 9600)                        │
└─────────────────────────────────────────────────────────────────┘
    │
    └─ PMSA003A          → PM1.0, PM2.5, PM10

┌─────────────────────────────────────────────────────────────────┐
│                     RMT (4 channels, 10MHz)                      │
└─────────────────────────────────────────────────────────────────┘
    │
    ├─ SK6812 RGB LED (GPIO21)  → CO2 Level Indicator
    ├─ SK6812 RGB LED (GPIO4)   → VOC Index Indicator
    ├─ SK6812 RGB LED (GPIO5)   → PM2.5 Level Indicator
    └─ SK6812 RGB LED (GPIO10)  → Humidity Level Indicator
```

## Update Cycle

```
Every 30 seconds:
  ┌────────────────────────────────────────┐
  │ 1. Read all sensors                    │
  │    - SHT45: Temp, Humidity             │
  │    - LPS22HB: Pressure                 │
  │    - PMSA003A: PM1.0, PM2.5, PM10      │
  │    - SGP41: VOC Index                  │
  │    - SCD40: CO2                        │
  └────────────────────────────────────────┘
              ↓
  ┌────────────────────────────────────────┐
  │ 2. Update Zigbee attributes            │
  │    - Endpoints 1-7 sensor values       │
  └────────────────────────────────────────┘
              ↓
  ┌────────────────────────────────────────┐
  │ 3. Evaluate each LED independently     │
  │    - CO2 LED: Check CO2 vs thresholds  │
  │    - VOC LED: Check VOC vs thresholds  │
  │    - Hum LED: Check Hum vs thresholds  │
  │    - PM2.5 LED: Check PM vs thresholds │
  └────────────────────────────────────────┘
              ↓
  ┌────────────────────────────────────────┐
  │ 4. Update each LED color (if changed)  │
  │    - Each LED: Green/Orange/Red/Off    │
  └────────────────────────────────────────┘
```

## LED Color Logic (Per LED)

```
VOC LED:
  ┌─────────────────────────────────────────────────────────┐
  │ IF VOC >= voc_red_threshold           → RED             │
  │ ELSE IF VOC >= voc_orange_threshold   → ORANGE          │
  │ ELSE                                  → GREEN           │
  └─────────────────────────────────────────────────────────┘

CO2 LED:
  ┌─────────────────────────────────────────────────────────┐
  │ IF CO2 >= co2_red_threshold           → RED             │
  │ ELSE IF CO2 >= co2_orange_threshold   → ORANGE          │
  │ ELSE                                  → GREEN           │
  └─────────────────────────────────────────────────────────┘

Humidity LED (special case - range check):
  ┌─────────────────────────────────────────────────────────┐
  │ IF humidity <= hum_red_low OR >= hum_red_high → RED     │
  │ ELSE IF humidity <= hum_orange_low OR >= hum_orange_high│
  │                                           → ORANGE       │
  │ ELSE                                      → GREEN        │
  └─────────────────────────────────────────────────────────┘

PM2.5 LED:
  ┌─────────────────────────────────────────────────────────┐
  │ IF PM2.5 >= pm25_red_threshold        → RED             │
  │ ELSE IF PM2.5 >= pm25_orange_threshold → ORANGE         │
  │ ELSE                                  → GREEN           │
  └─────────────────────────────────────────────────────────┘

Global enable/disable:
  ┌─────────────────────────────────────────────────────────┐
  │ IF LED disabled via Zigbee     → ALL LEDs = OFF         │
  └─────────────────────────────────────────────────────────┘
```

## Zigbee2MQTT Device Structure

```json
{
  "ieee_address": "0x...",
  "friendly_name": "Aeris Air Quality Sensor",
  "model": "aeris-z",
  "vendor": "ESPRESSIF",
  "definition": {
    "endpoints": {
      "1": ["temperature", "humidity"],
      "2": ["pressure"],
      "3": ["pm1_0"],
      "4": ["pm2_5"],
      "5": ["pm10"],
      "6": ["voc_index"],
      "7": ["co2"],
      "8": ["led_config", "thresholds"]
    }
  }
}
```

## Pin Assignment Summary

| GPIO | Function | Device | Protocol |
|------|----------|--------|----------|
| 3 | Antenna SEL0 | FM8625H | Digital Out |
| 4 | RMT Data | SK6812 VOC LED | RMT |
| 5 | RMT Data | SK6812 PM2.5 LED | RMT |
| 6 | I2C SDA | Sensors | I2C |
| 7 | I2C SCL | Sensors | I2C |
| 9 | Boot Button | Factory Reset | Digital In |
| 10 | RMT Data | SK6812 Humidity LED | RMT |
| 14 | Antenna SEL1 | FM8625H | Digital Out |
| 20 | UART RX | PMSA003A | UART |
| 21 | RMT Data | SK6812 CO2 LED | RMT |

## Power Budget (Estimated)

| Component | Current | Notes |
|-----------|---------|-------|
| ESP32-C6 | ~80mA | Active, Zigbee router |
| SHT45 | <0.5mA | Measurement mode |
| LPS22HB | <0.1mA | Low power mode |
| SGP41 | ~3mA | Active measurement |
| SCD40 | ~20mA | Active measurement |
| PMSA003A | ~100mA | Fan + laser active |
| SK6812 LEDs (×4) | ~240mA | Max brightness, all colors, all LEDs |
| **Total** | **~445mA** | Peak during measurement |

USB 5V supply required (min 1A recommended for margin).
