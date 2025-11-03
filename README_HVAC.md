# Zigbee Air Quality Sensor - Aeris

This project implements a Zigbee End Device that reads air quality sensors and exposes them as standard Zigbee sensor endpoints.

## Hardware Requirements

- ESP32-C6 development board (or compatible)
- Air quality sensors:
  - Temperature and Humidity sensor (e.g., SHT4x, BME280)
  - Pressure sensor (e.g., BMP280, BME280)  
  - Particulate Matter sensor (e.g., PMS5003, SPS30)
  - VOC Index sensor (e.g., SGP40, BME680)
  - CO2 sensor (e.g., SCD40, SCD41)
- I2C connection (SDA/SCL pins configurable)

## Features

The Zigbee air quality sensor exposes the following sensor endpoints:

### Endpoint 1: Temperature and Humidity Sensor
- **Temperature Measurement Cluster (0x0402)**: Temperature in °C
- **Relative Humidity Measurement Cluster (0x0405)**: Humidity in %

### Endpoint 2: Pressure Sensor
- **Pressure Measurement Cluster (0x0403)**: Atmospheric pressure in hPa

### Endpoint 3: Particulate Matter (PM) Sensor
- **PM2.5 Measurement**: Fine particulate matter (µg/m³)
- **PM10 Measurement**: Coarse particulate matter (µg/m³)
- **PM1.0 Measurement**: Ultra-fine particulate matter (µg/m³)

### Endpoint 4: VOC Index Sensor
- **VOC Index** (1-500): Volatile Organic Compounds air quality index

### Endpoint 5: CO2 Sensor
- **CO2 Concentration Cluster (0x040D)**: Carbon dioxide in ppm

## I2C Configuration

The device communicates with sensors via I2C:

- **SDA Pin**: GPIO 6 (configurable in `aeris_driver.h`)
- **SCL Pin**: GPIO 7 (configurable in `aeris_driver.h`)
- **Frequency**: 100 kHz
- **Pull-ups**: Internal pull-ups enabled

## Project Structure

```
Aeris_zb/
├── main/
│   ├── esp_zb_aeris.c         # Main Zigbee application
│   ├── esp_zb_aeris.h         # Zigbee configuration header
│   ├── aeris_driver.c         # Air quality sensor driver implementation
│   ├── aeris_driver.h         # Sensor driver header
│   ├── esp_zb_ota.c           # OTA update support
│   ├── esp_zb_ota.h           # OTA header
│   ├── board.h                # Board pin definitions
│   ├── CMakeLists.txt         # Component build configuration
│   └── idf_component.yml      # Component dependencies
├── CMakeLists.txt             # Project CMakeLists
├── sdkconfig                  # ESP-IDF configuration
└── README.md                  # This file
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

The device is configured as a Zigbee End Device (ZED):
- **Endpoints**: 1-5 (one per sensor type)
- **Profile**: Home Automation (0x0104)
- **Device IDs**: Various sensor types (Temperature, Humidity, Pressure, etc.)
- **Channel Mask**: All channels

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
3. Wait for the device to join (check logs)
4. The air quality sensor will appear with the following entities:
   - Temperature (°C)
   - Humidity (%)
   - Pressure (hPa)
   - PM1.0, PM2.5, PM10 (µg/m³)
   - VOC Index (1-500)
   - CO2 (ppm)

### Sensor Reading Updates

- Sensors are polled periodically (configurable interval)
- Values are reported to the Zigbee coordinator when they change
- All endpoints support binding and reporting configuration

## Supported Sensors

This project provides a framework for air quality sensors. You'll need to implement the actual sensor drivers:

### Temperature/Humidity
- **SHT4x** (recommended): High accuracy, low power
- **BME280**: Temperature, humidity, and pressure in one chip
- **DHT22**: Budget option

### Pressure
- **BMP280**: Barometric pressure sensor
- **BME280**: Combined temp/humidity/pressure

### Particulate Matter
- **PMS5003**: Laser-based PM sensor
- **SPS30**: Sensirion PM sensor (I2C)

### VOC Index
- **SGP40**: VOC sensor with built-in algorithm
- **BME680**: VOC + temperature/humidity/pressure

### CO2
- **SCD40**: NDIR CO2 sensor (400-2000 ppm)
- **SCD41**: Extended range (400-5000 ppm)

## Implementation Notes

The `aeris_driver.c` file contains stub implementations. You need to:

1. Add sensor-specific libraries to `idf_component.yml`
2. Implement sensor initialization in `aeris_driver_init()`
3. Implement actual I2C read functions for each sensor
4. Update the periodic read task to poll all sensors

## Removed Features

This project was adapted from an HVAC controller. The following features were removed:

- ❌ HVAC thermostat control
- ❌ Fan speed control
- ❌ Eco/Night/Swing modes
- ❌ UART communication with HVAC unit

All HVAC control logic has been replaced with air quality sensor reading functionality.

## Troubleshooting

### Device won't join network
- Check that Zigbee coordinator is in pairing mode
- Verify ESP32-C6 is powered correctly
- Check serial logs for error messages
- Try factory reset (hold button during boot)

### Sensors not responding
- Verify I2C wiring (SDA/SCL connections)
- Check I2C pull-up resistors (usually 4.7kΩ)
- Verify sensor power supply (usually 3.3V)
- Use `i2cdetect` to scan for sensor addresses
- Monitor I2C traffic in logs

### Values not updating
- Check that sensors are initialized correctly
- Verify periodic read task is running
- Check log output for sensor read errors
- Ensure reporting is configured on Zigbee coordinator

### Build errors
- Make sure ESP-IDF v5.5.1+ is installed
- Run `idf.py fullclean` and rebuild
- Check that all sensor libraries are in `idf_component.yml`

## License

This code is based on Espressif's Zigbee examples and inherits their licensing.

## Credits

- Base Zigbee framework: Espressif ESP-IDF examples
- Air quality sensor concept: Community contributions
- Original ESPHome component: See `CodeToReusePartially/acw02_esphome/`

## Future Enhancements

- [ ] Add button for factory reset
- [ ] Implement command retry logic
- [ ] Add OTA firmware update support
- [ ] Support horizontal swing control
- [ ] Add night mode support
- [ ] Implement filter status monitoring
- [ ] Add clean mode support
