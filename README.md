[![Support me on Ko-fi](https://ko-fi.com/img/githubbutton_sm.svg)](https://ko-fi.com/Fabiancrg)

| Supported Targets | ESP32-C6 | ESP32-H2 |
| ----------------- |  -------- | -------- |

# Zigbee Air Quality Sensor - Aeris

[![License: GPL v3](https://img.shields.io/badge/Software-GPLv3-blue.svg)](./LICENSE)
[![License: CC BY-NC-SA 4.0](https://img.shields.io/badge/Hardware-CC%20BY--NC--SA%204.0-green.svg)](./LICENSE-hardware)

This project implements a Zigbee Router that reads air quality sensors and exposes them as standard Zigbee sensor endpoints with **real-time RGB LED visual indicators**.

## Key Features

✅ **7 Zigbee Endpoints**: Temperature, Humidity, Pressure, VOC Index, NOx Index, CO2, LED Config, Status LED  
✅ **4 High-Precision Sensors**: SHT45, LPS22HB, SGP41, SCD40  
✅ **4 RGB Status LEDs**: Independent visual feedback for CO2, VOC, NOx, and Humidity  
✅ **Configurable Thresholds**: Adjust warning/danger levels via Zigbee2MQTT  
✅ **Zigbee Router**: Can accept other devices (max 10 children)  
✅ **OTA Support**: Firmware updates over Zigbee  
✅ **Factory Reset**: Boot button for easy network reset

## Hardware Requirements

- ESP32-C6 development board (or compatible)
- Air quality sensors:
  - **SHT45 Temperature and Humidity sensor** via I2C
  - **LPS22HB Pressure sensor** via I2C
  - **SGP41 VOC and NOx sensor** via I2C
  - **SCD40 CO2 sensor** via I2C
- **5× SK6812/WS2812B RGB LEDs** in daisy-chain configuration (air quality visual indicators)
  - Single GPIO20 with SN74AHCT1G125 buffer for all LEDs
  - LED chain: CO2 → VOC → NOx → Humidity → Status
- I2C connection (SDA/SCL pins configurable)
- **Power supply**: 5V USB, 500mA minimum recommended

## Features

The Zigbee air quality sensor exposes the following sensor endpoints:

### Endpoint 1: Temperature and Humidity Sensor
- **Temperature Measurement Cluster (0x0402)**: Temperature in °C
- **Relative Humidity Measurement Cluster (0x0405)**: Humidity in %
- **Sensor**: Sensirion SHT45 (I2C)
- **Temperature Range**: -40°C to +125°C
- **Temperature Accuracy**: ±0.1°C (typical, 15-40°C)
- **Humidity Range**: 0-100% RH
- **Humidity Accuracy**: ±1.0% RH (typical, 25-75% RH)
- **Measurement Time**: 8.2ms (high precision mode)
- **Conversion**: Temperature: T = -45 + 175×(S/65535), Humidity: RH = -6 + 125×(S/65535)
- **Calibration Offsets**: Custom attributes (0xF010, 0xF011) for temperature/humidity adjustment
- **Sensor Refresh Interval**: Configurable update rate (10-3600s, default 30s) via attribute 0xF012

### Endpoint 2: Pressure Sensor
- **Pressure Measurement Cluster (0x0403)**: Atmospheric pressure in hPa
- **Sensor**: STMicroelectronics LPS22HB (I2C)
- **Update Rate**: On-demand (one-shot mode for power savings)
- **Range**: 260-1260 hPa
- **Accuracy**: ±0.025 hPa (typical)
- **Measurement Time**: ~15ms per reading

### Endpoint 3: VOC Index Sensor
- **Analog Input Cluster (0x000C)**: VOC Index (1-500)
- **Sensor**: Sensirion SGP41 (I2C)
- **Description**: Volatile Organic Compounds air quality index
- **Raw signals**: VOC raw values available
- **Update Rate**: 1 Hz enforced (minimum 1-second interval per datasheet)
- **Note**: Requires temperature/humidity compensation from SHT45

### Endpoint 4: NOx Index Sensor
- **Analog Input Cluster (0x000C)**: NOx Index (1-500)
- **Sensor**: Sensirion SGP41 (I2C)
- **Description**: Nitrogen Oxides air quality index
- **Raw signals**: NOx raw values available
- **Update Rate**: 1 Hz enforced (minimum 1-second interval per datasheet)
- **Note**: Requires temperature/humidity compensation from SHT45

### Endpoint 5: CO2 Sensor
- **CO2 Concentration Cluster (0x040D)**: Carbon dioxide in ppm
- **Sensor**: Sensirion SCD40 (I2C)
- **Range**: 400-2000 ppm (optimized for indoor air quality)
- **Accuracy**: ±(40 ppm + 5% of reading)
- **Update Rate**: 5 seconds (automatic periodic measurement)
- **Pressure Compensation**: Automatic compensation using LPS22HB pressure data
- **Calibration**: Automatic self-calibration (ASC) disabled for stable baseline in controlled environments
- **Additional**: Built-in temperature and humidity sensor (bonus)

### Endpoint 6: LED Configuration
- **On/Off Cluster (0x0006)**: Master switch to enable/disable all RGB status LEDs
- **Custom Attributes (0xF000-0xF00C)**: Configurable air quality thresholds and individual LED control
- **Bitmask Control (0xF00C)**: Individual enable/disable for each of the 4 LEDs
- **Thresholds**: Adjustable orange/red warning levels for VOC, NOx, CO2, Humidity

### Endpoint 7: Status LED
- **On/Off Cluster (0x0006)**: Enable/disable Zigbee status LED
- **Automatic Status Indication**: Shows Zigbee network connection status
  - 🟢 **Green**: Successfully connected to coordinator
  - 🟡 **Blinking Green/Orange**: Joining network
  - 🟠 **Orange**: Not joined to network
  - 🔴 **Red**: Error during initialization
- **GPIO**: GPIO8 (built-in LED on ESP32-C6 Supermini)

### RGB LED Air Quality Indicators

The device includes **4 separate RGB LEDs (SK6812)** that provide real-time visual feedback for air quality:

### LED Indicators
- **CO2 LED** (GPIO1): Shows carbon dioxide level status
- **VOC LED** (GPIO18): Shows volatile organic compounds status
- **NOx LED** (GPIO15): Shows nitrogen oxides status
- **Humidity LED** (GPIO14): Shows humidity level status

### Color Coding
Each LED independently displays:
- 🟢 **Green**: Parameter is in good/healthy range
- 🟠 **Orange**: Parameter is not ideal (warning level)
- 🔴 **Red**: Parameter is poor/unhealthy (danger level)

### Features
- **Independent operation**: Each LED shows only its corresponding sensor
- **At-a-glance status**: Quickly identify which parameter needs attention
- **Configurable thresholds**: Adjust warning/danger levels via Zigbee2MQTT
- **Brightness control**: Adjustable LED brightness (1-255) via Level Control cluster
- **Settings persistence**: All LED settings, thresholds, and calibrations stored in NVS (survive reboots)
- **Dual control**:
  - **Master switch**: Enable/disable all sensor LEDs at once (On/Off cluster)
  - **Individual control**: Enable/disable each LED separately via bitmask (0xF00C)
  - **Status LED control**: Independent on/off control for Zigbee status LED (endpoint 10)
- **Low power**: LEDs only update when color changes

### LED Control

**Master On/Off Switch:**
- Turn all LEDs on/off regardless of individual settings
- Located in On/Off cluster (standard Zigbee)

**Individual LED Bitmask (0xF00C):**
- Control each LED independently using a single 8-bit value
- Each bit represents one LED (1=enabled, 0=disabled)
- Bit assignments:
  - Bit 0 (0x01): CO2 LED
  - Bit 1 (0x02): VOC LED
  - Bit 2 (0x04): NOx LED
  - Bit 4 (0x10): Humidity LED
- Default: `0x17` (all LEDs enabled)

**Examples:**
```
0x17 (23)  - All LEDs enabled
0x03 (3)   - Only CO2 and VOC enabled
0x04 (4)   - Only NOx enabled
0x05 (5)   - CO2 and NOx enabled
0x00 (0)   - All LEDs disabled (individual level)
```

### Default Thresholds (all configurable)
- **VOC Index**: Orange ≥150, Red ≥250
- **NOx Index**: Orange ≥150, Red ≥250
- **CO2**: Orange ≥1000 ppm, Red ≥1500 ppm
- **Humidity**: Orange <30% or >70%, Red <20% or >80%

See [LED Configuration Guide](LED_CONFIGURATION.md) for detailed setup and usage.

## I2C and UART Configuration

### I2C Bus (Temperature, Humidity, Pressure, VOC, CO2)

The device communicates with most sensors via I2C:

- **SDA Pin**: GPIO 6 (configurable in `aeris_driver.h`)
- **SCL Pin**: GPIO 7 (configurable in `aeris_driver.h`)
- **Frequency**: 100 kHz
- **Pull-ups**: Internal pull-ups enabled

**I2C Addresses:**
- SHT45 (Temp/Humidity): 0x44 (fixed address)
- LPS22HB (Pressure): 0x5C (default, SA0=0) or 0x5D (SA0=1)
- SGP41 (VOC/NOx): 0x59 (fixed address)
- SCD40 (CO2): 0x62 (fixed address)

## Project Structure

```
Aeris_zb/
├── main/
│   ├── esp_zb_aeris.c         # Main Zigbee application
│   ├── esp_zb_aeris.h         # Zigbee configuration header
│   ├── aeris_driver.c         # Air quality sensor driver implementation
│   ├── aeris_driver.h         # Sensor driver header
│   ├── led_indicator.c        # RGB LED driver (6 LEDs via RMT)
│   ├── led_indicator.h        # LED driver header
│   ├── settings.c             # NVS settings persistence
│   ├── settings.h             # Settings header
│   ├── esp_zb_ota.c           # OTA update support
│   ├── esp_zb_ota.h           # OTA header
│   ├── board.h                # Board pin definitions (GPIO mapping)
│   ├── CMakeLists.txt         # Component build configuration
│   └── idf_component.yml      # Component dependencies
├── CMakeLists.txt             # Project CMakeLists
├── sdkconfig                  # ESP-IDF configuration
├── README.md                  # This file
├── LED_CONFIGURATION.md   # LED setup and Zigbee2MQTT integration
├── LED_IMPLEMENTATION.md  # LED technical implementation details
└── ENDPOINT_MAP.md        # Complete Zigbee endpoi
```

## Building and Flashing

1. Set up ESP-IDF v5.5.1 or later
2. Configure the project:
   ```bash
   idf.py set-target esp32c6
   idf.py menuconfig
   ```
3. Build the project:
   ```bash
   idf.py build
   ```
4. Flash to device:
   ```bash
   idf.py -p COMx flash monitor
   ```

## Configuration

### Zigbee Configuration

The device is configured as a Zigbee Router:
- **Endpoints**: 1-7 (sensor data + LED configuration + status LED)
  - Endpoints 1-5: Sensor data (Temperature, Humidity, Pressure, VOC, NOx, CO2)
  - Endpoint 6: Air quality LED configuration and control
  - Endpoint 7: Zigbee status LED control
- **Profile**: Home Automation (0x0104)
- **Device IDs**: Various sensor types + On/Off output for LED control
- **Channel Mask**: All channels
- **Max Children**: 10 (router can accept other devices)

### LED Configuration

**Air Quality LEDs (4 LEDs)** - Configure via Zigbee2MQTT endpoint 6:
- **Master On/Off**: Single switch to enable/disable all LEDs (On/Off cluster)
- **Individual Control**: Bitmask attribute (0xF00C) for per-LED enable/disable
  - Set to `0x17` (23) for all LEDs
  - Set to `0x03` (3) for CO2 + VOC only
  - Set to `0x00` (0) to disable all (at individual level)
- **Thresholds**: 8 configurable attributes (VOC, NOx, CO2, Humidity orange/red levels)
- **GPIO Pins**: Configurable in `main/board.h` (defaults: GPIO1, 18, 15, 14)

**Control Logic:**
- Master OFF → All LEDs off (ignores bitmask)
- Master ON + bit set → LED shows sensor status color
- Master ON + bit clear → LED off

**Zigbee Status LED (1 LED)** - Configure via Zigbee2MQTT endpoint 7:
- **On/Off Control**: Enable/disable status LED
- **Automatic Status Indication**:
  - 🟢 **Green**: Successfully connected to Zigbee coordinator
  - 🟡 **Blinking Green/Orange**: Actively joining network (searching for coordinator)
  -  **Orange**: Not joined to any network or left the network (idle)
  - 🔴 **Red**: Error during initialization or join failed
- **GPIO Pin**: GPIO23 (configurable in `main/board.h`)
- **Blink Pattern**: 500ms interval during network join process

See [LED Configuration Guide](LED_CONFIGURATION.md) for detailed threshold settings.

### Sensor Configuration

Configure sensor I2C addresses and settings in `aeris_driver.c`:
- Default I2C pins: SDA=GPIO6, SCL=GPIO7
- Adjust pins in `aeris_driver.h` if needed

### Joining the Network

On first boot, the device will automatically enter network steering mode. Once joined, the device will save the network credentials and automatically rejoin on subsequent boots.

## Usage

### From Zigbee Coordinator (e.g., Zigbee2MQTT, Home Assistant)

1. Put your coordinator in pairing mode
2. Power on the ESP32-C6 device
3. Wait for the device to join (check logs - status LED will turn green when connected)
4. The air quality sensor will appear with the following entities:
   - Temperature (°C)
   - Humidity (%)
   - Pressure (hPa)
   - VOC Index (1-500)
   - NOx Index (1-500)
   - CO2 (ppm)
   - **Air Quality LED Master Switch** (On/Off - controls all 4 air quality LEDs)
   - **LED Enable Mask** (0-15 - individual LED control)
   - **LED Thresholds** (configurable warning/danger levels)
   - **Status LED Switch** (On/Off - controls Zigbee status LED)

### LED Control Examples

**Via Zigbee2MQTT:**

```javascript
// Turn all sensor LEDs on (master switch)
await publish('zigbee2mqtt/aeris/set', {state: 'ON'});

// Turn all sensor LEDs off (master switch)
await publish('zigbee2mqtt/aeris/set', {state: 'OFF'});

// Set LED brightness (1-255, default 128)
await publish('zigbee2mqtt/aeris/set', {brightness: 128});
await publish('zigbee2mqtt/aeris/set', {brightness: 255});  // Max brightness
await publish('zigbee2mqtt/aeris/set', {brightness: 64});   // Dim

// Enable all 4 LEDs individually (bitmask)
await publish('zigbee2mqtt/aeris/set', {led_enable_mask: 15});  // 0x0F

// Enable only CO2 and VOC LEDs
await publish('zigbee2mqtt/aeris/set', {led_enable_mask: 3});   // 0x03

// Enable only NOx LED
await publish('zigbee2mqtt/aeris/set', {led_enable_mask: 4});   // 0x04

// Enable CO2, VOC, and Humidity LEDs
await publish('zigbee2mqtt/aeris/set', {led_enable_mask: 11});  // 0x0B

// Disable all LEDs via bitmask (master can still be ON)
await publish('zigbee2mqtt/aeris/set', {led_enable_mask: 0});

// Status LED control (endpoint 10)
await publish('zigbee2mqtt/aeris_status/set', {state: 'ON'});   // Enable status LED
await publish('zigbee2mqtt/aeris_status/set', {state: 'OFF'});  // Disable status LED

// Calibration offsets (stored in NVS)
await publish('zigbee2mqtt/aeris/set', {temperature_offset: 5});   // +0.5°C
await publish('zigbee2mqtt/aeris/set', {temperature_offset: -10}); // -1.0°C
await publish('zigbee2mqtt/aeris/set', {humidity_offset: 20});     // +2.0% RH

// Sensor refresh interval (10-3600 seconds, default 30)
await publish('zigbee2mqtt/aeris/set', {sensor_refresh_interval: 60}); // 1 minute
```

### Visual LED Status

**Air Quality LEDs (4 RGB LEDs)** provide real-time visual feedback:
- **CO2 LED**: Green/Orange/Red based on CO2 levels
- **VOC LED**: Green/Orange/Red based on VOC Index
- **NOx LED**: Green/Orange/Red based on NOx Index
- **Humidity LED**: Green/Orange/Red based on humidity range

**Status LED (1 RGB LED)** shows Zigbee network state:
- 🟢 **Green**: Connected to coordinator
- 🟡 **Blinking Green/Orange**: Joining network (searching)
- 🟠 **Orange**: Not joined or left network (idle)
- 🔴 **Red**: Join failed or error state (will retry)

**At a glance**, you can see:
- All green (air quality) = Excellent air quality
- Mixed colors = Some parameters need attention
- Status LED solid green = Device is online and connected
- Status LED blinking = Device is trying to join network
- Red LEDs = Immediate action needed (ventilate, increase/decrease humidity, etc.)

Configure thresholds in Zigbee2MQTT to match your preferences. See [LED Configuration Guide](LED_CONFIGURATION.md).

### Sensor Reading Updates

- Sensors are polled periodically (configurable interval)
- Values are reported to the Zigbee coordinator when they change
- All endpoints support binding and reporting configuration

## Supported Sensors

This project provides a framework for air quality sensors. You'll need to implement the actual sensor drivers:

### Temperature/Humidity
- **SHT45** (implemented): Sensirion high-precision sensor
  - I2C interface (address 0x44)
  - Temperature: -40 to +125°C, ±0.1°C accuracy
  - Humidity: 0-100% RH, ±1.0% RH accuracy
  - High repeatability mode: 8.2ms measurement time
  - CRC-8 checksum for data integrity
  - Serial number readout
  - Soft reset command
  - Low power: 0.4 µA idle
- **Alternative**: SHT4x series, BME280, DHT22

### Pressure
- **LPS22HB** (implemented): STMicroelectronics MEMS pressure sensor
  - I2C interface (address 0x5C or 0x5D)
  - Measures 260-1260 hPa
  - ±0.025 hPa accuracy
  - Internal temperature sensor (bonus)
  - Low power: 3 µA @ 1Hz
  - Configurable output rate (1-75 Hz)
- **Alternative**: BMP280, BME280 (Bosch sensors)

### VOC and NOx Index
- **SGP41** (implemented): Sensirion VOC and NOx sensor
  - I2C interface (address 0x59)
  - Dual gas sensing: VOC + NOx
  - Separate Zigbee endpoints for VOC (endpoint 6) and NOx (endpoint 7)
  - Raw signal output (requires algorithm)
  - Temperature/humidity compensation
  - Built-in heater for measurement
  - Low power: 2.6 mA average @ 1Hz
  - Serial number readout
  - Self-test capability
- **Note**: Full Gas Index Algorithm integration recommended for production
- **Alternative**: SGP40 (VOC only), BME680 (combo sensor)

### CO2
- **SCD40** (implemented): Sensirion NDIR CO2 sensor
  - I2C interface (address 0x62)
  - Range: 400-2000 ppm (indoor air quality)
  - Accuracy: ±(40 ppm + 5%)
  - Automatic periodic measurement every 5 seconds
  - **Ambient pressure compensation** from LPS22HB sensor
  - Built-in temperature and humidity sensor
  - Automatic self-calibration (ASC)
  - CRC-8 data validation
  - Serial number readout
  - Low power: 15 mA average @ 1 measurement/5s
- **Alternative**: SCD41 (extended range 400-5000 ppm)

## Implementation Notes

The `aeris_driver.c` file contains:

1. **SHT45 implementation** (complete):
   - I2C initialization and device detection
   - Soft reset on startup
   - Serial number readout and verification
   - High repeatability measurement mode
   - CRC-8 validation on all data
   - Temperature and humidity conversion
   - Humidity clamping to 0-100% range

2. **LPS22HB implementation** (complete):
   - I2C initialization and device detection
   - WHO_AM_I verification (0xB1)
   - 25 Hz output data rate
   - Block data update enabled
   - Pressure reading in hPa
   - Temperature reading included

4. **SGP41 implementation** (complete):
   - I2C initialization and device detection
   - Serial number readout and verification
   - Self-test execution
   - Raw VOC and NOx signal measurement
   - Separate VOC and NOx index reporting (endpoints 6 and 7)
   - Temperature/humidity compensation support
   - CRC8 validation on all data
   - Placeholder index calculation (needs Gas Index Algorithm)

5. **SCD40 implementation** (complete):
   - I2C initialization and device detection
   - Serial number readout and verification
   - Automatic periodic measurement mode
   - CO2, temperature, and humidity reading
   - Ambient pressure compensation using LPS22HB data
   - CRC-8 validation on all data
   - Data ready status checking
   - 5-second measurement interval

## Sensor Wiring Diagrams

### SHT45 Wiring

```
SHT45 Pin → ESP32-C6
VDD       → 3.3V
GND       → GND
SCL       → GPIO 7
SDA       → GPIO 6
```

**Note**: SHT45 operates at 3.3V. No level shifters needed with ESP32-C6.

### LPS22HB Wiring

```
LPS22HB Pin → ESP32-C6
VDD         → 3.3V
GND         → GND
SCL         → GPIO 7 (I2C SCL)
SDA         → GPIO 6 (I2C SDA)
SA0         → GND (for address 0x5C) or 3.3V (for 0x5D)
```

**Note**: LPS22HB operates at 1.7-3.6V. Use 3.3V supply.

### SGP41 Wiring

```
SGP41 Pin → ESP32-C6
VDD       → 3.3V
GND       → GND
SCL       → GPIO 7 (I2C SCL)
SDA       → GPIO 6 (I2C SDA)
```

**Note**: SGP41 operates at 1.7-3.6V. Use 3.3V supply. No external pull-ups needed on most breakout boards.

### SCD40 Wiring

```
SCD40 Pin → ESP32-C6
VDD       → 3.3V
GND       → GND
SCL       → GPIO 7 (I2C SCL)
SDA       → GPIO 6 (I2C SDA)
```

**Note**: SCD40 operates at 2.4-5.5V. Use 3.3V supply. Automatic periodic measurement every 5 seconds. Benefits from ambient pressure compensation provided by LPS22HB sensor.

### SK6812 RGB LED Wiring (5 LEDs)

The device uses 5 separate RGB LEDs for visual air quality feedback:

```
LED Purpose    GPIO    Connection
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
CO2 LED        1       Data line
VOC LED        18      Data line
NOx LED        15      Data line
Humidity LED   14      Data line
Status LED     8       Data line (Zigbee status)

For each LED:
  Data → GPIO pin (see table above)
  VCC  → 5V (SK6812 specification)
  GND  → GND
```

**Notes**: 
- SK6812 LEDs use WS2812 protocol via RMT peripheral
- Each LED operates independently showing its sensor's status
- All 5 sensor LEDs can be enabled/disabled together via Zigbee (endpoint 9)
- Status LED controlled separately via endpoint 10
- Brightness adjustable 1-255 via Level Control cluster
- GPIO pins are configurable in `main/board.h`
- Total LED current (all 6 at max brightness): ~360mA

**Color Codes**:
- 🟢 Green = Good air quality for that parameter
- 🟠 Orange = Warning level (not ideal)
- 🔴 Red = Danger level (poor air quality)

See [LED Configuration Guide](LED_CONFIGURATION.md) for threshold configuration via Zigbee2MQTT.

### Important: Gas Index Algorithm

The SGP41 currently uses a **simplified placeholder** for VOC/NOx index calculation. For production use, integrate Sensirion's **Gas Index Algorithm**:

- Download from: https://github.com/Sensirion/gas-index-algorithm
- Provides proper VOC Index (1-500) and NOx Index (1-500)
- Requires learning period (~10 minutes for VOC, ~12 hours for NOx)
- Handles baseline tracking and auto-calibration

## Troubleshooting

### Sensors not responding
- **I2C sensors**:
  - Verify I2C wiring (SDA/SCL connections)
  - Check I2C pull-up resistors (usually 4.7kΩ)
  - Verify sensor power supply (3.3V for SHT45, LPS22HB, SGP41, SCD40)
  - Use `i2cdetect` to scan for sensor addresses
  - Monitor I2C traffic in logs
  - **SHT45**: Read serial number to verify communication
  - **SHT45**: Check CRC errors in logs
  - **LPS22HB**: Check WHO_AM_I register (should be 0xB1)
  - **LPS22HB**: Verify address 0x5C or 0x5D (depends on SA0 pin)
  - **SGP41**: Check serial number readout
  - **SGP41**: Verify self-test passes (result 0xD400)
  - **SGP41**: Ensure temperature/humidity data is available for compensation
  - **SCD40**: Check serial number readout
  - **SCD40**: Verify data ready status (measurements every 5 seconds)
  - **SCD40**: Check CRC errors in logs

**Power Note**: 
- **5 LEDs** (4 air quality + 1 status) at full brightness: ~300mA
- **ESP32-C6 + sensors**: ~50-80mA
- **Total**: ~350-380mA
- **Recommended power supply**: 500mA minimum (USB 5V)

### Build errors
- Make sure ESP-IDF v5.5.1+ is installed
- Run `idf.py fullclean` and rebuild
- Check that all sensor libraries are in `idf_component.yml`

---

## 🙌 Credits

- Zigbee2MQTT ecosystem: [Koenkk/zigbee2mqtt](https://github.com/Koenkk/zigbee2mqtt) and [zigbee-herdsman-converters](https://github.com/Koenkk/zigbee-herdsman-converters)
- Base Zigbee framework: Espressif ESP-IDF examples

## 🛡️ License

This repository uses multiple licenses depending on content type:

- **Code** (`*.c`, `*.h`, `*.yaml`) — [GNU GPLv3](./LICENSE)  
- **PCB files** (`*.zip`, `*.json`) — [CC BY-NC-SA 4.0](./LICENSE-hardware)

---

## ⚠️ Disclaimer

This project is provided as-is, without any warranty or guarantee of fitness for a particular purpose. Use at your own risk. The authors and contributors are not responsible for any damage, malfunction, or loss resulting from the use, modification, or installation of this hardware or software.

- This is an open-source, community-driven project intended for educational and experimental use.
- The hardware and firmware are not certified for commercial or safety-critical applications.
- Always follow proper electrical safety procedures when working with sensors and embedded systems.
- Modifying or installing this project may void your equipment warranty.
- Ensure compliance with local laws and regulations regarding wireless devices.

By using this project, you acknowledge and accept these terms.




