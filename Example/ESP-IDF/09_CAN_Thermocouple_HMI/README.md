# CAN Thermocouple HMI Demo

A comprehensive demonstration project for the **Waveshare ESP32-S3-Touch-LCD-4.3** board that implements a professional CAN HMI system for displaying and transmitting 16-channel K-type thermocouple data.

![Project Status](https://img.shields.io/badge/status-demo-green)
![ESP-IDF](https://img.shields.io/badge/ESP--IDF-v5.x-blue)
![License](https://img.shields.io/badge/license-Apache--2.0-blue)

## Features

- **16 Channel Thermocouple Interface**
  - K-type thermocouple support (simulated data for demo)
  - Real-time temperature monitoring
  - Individual channel status tracking
  - Extensible for real hardware (MAX31855, MAX31856, MCP9600)

- **Professional HMI Display**
  - 800x480 RGB LCD with touch support
  - Modern dark-themed UI using LVGL
  - Real-time temperature visualization
  - Color-coded temperature status (Normal/Warning/Danger)
  - CAN bus status indicator
  - 4x4 grid layout for all 16 channels

- **CAN Bus Transmission**
  - 500 kbps CAN bus (configurable)
  - Individual channel temperature messages
  - System status messages
  - Automatic periodic transmission
  - Error monitoring and reporting

## Hardware Requirements

- **Board**: Waveshare ESP32-S3-Touch-LCD-4.3
- **MCU**: ESP32-S3-WROOM-1-N16R8 (16MB Flash, 8MB PSRAM)
- **Display**: 4.3" RGB LCD (800x480)
- **Touch**: Capacitive touch (GT911)
- **CAN**: Built-in CAN transceiver

For detailed hardware specifications, visit:
[Waveshare ESP32-S3-Touch-LCD-4.3 Wiki](https://www.waveshare.com/wiki/ESP32-S3-Touch-LCD-4.3)

## Software Requirements

- ESP-IDF v5.0 or later
- Python 3.8 or later (for ESP-IDF tools)

## Project Structure

```
09_CAN_Thermocouple_HMI/
├── CMakeLists.txt              # Main project CMake configuration
├── README.md                   # This file
├── sdkconfig.defaults          # Default configuration
└── main/
    ├── CMakeLists.txt          # Main component CMake
    ├── idf_component.yml       # Component dependencies
    ├── Kconfig.projbuild       # Configuration menu
    ├── main.c                  # Main application
    ├── thermocouple.h/c        # Thermocouple interface module
    ├── can_transmit.h/c        # CAN transmission module
    ├── hmi_display.h/c         # HMI display module
    ├── lvgl_port.h/c           # LVGL port layer
    └── waveshare_rgb_lcd_port.h/c  # LCD hardware port
```

## Building and Flashing

### 1. Set up ESP-IDF Environment

```bash
# Source ESP-IDF environment
. $HOME/esp/esp-idf/export.sh

# Or if you have ESP-IDF in a different location
. /path/to/esp-idf/export.sh
```

### 2. Navigate to Project Directory

```bash
cd Example/ESP-IDF/09_CAN_Thermocouple_HMI
```

### 3. Configure Project (Optional)

```bash
idf.py menuconfig
```

Key configuration options:
- **Example Configuration → CAN Configuration**
  - CAN TX GPIO: GPIO20 (default)
  - CAN RX GPIO: GPIO19 (default)
  - CAN Bitrate: 500 kbps (125/250/500/1000 available)
  - CAN Base ID: 0x100 (default)

- **Example Configuration → Thermocouple Configuration**
  - Use Simulated Data: Yes (for demo)
  - Number of Channels: 16

- **Example Configuration → Display**
  - LVGL buffer configuration
  - Anti-tearing settings

### 4. Build the Project

```bash
idf.py build
```

### 5. Flash to Device

```bash
idf.py -p /dev/ttyUSB0 flash monitor
```

Replace `/dev/ttyUSB0` with your serial port (e.g., `COM3` on Windows).

## Usage

### On Startup

1. The system initializes all components:
   - LCD display and touch controller
   - LVGL graphics library
   - Thermocouple interface (simulated mode)
   - CAN bus interface

2. The HMI displays:
   - Title bar with "K-Type Thermocouple CAN HMI"
   - CAN status indicator (green LED when active)
   - 16 channel panels in 4x4 grid

3. Each channel panel shows:
   - Channel number (CH 0 - CH 15)
   - Current temperature reading
   - Status text (Normal/High Temp/Out of Range/Invalid)
   - Color-coded temperature display

### Temperature Display Color Coding

- **Green**: Normal temperature (0-100°C)
- **Orange**: High temperature warning (100-150°C)
- **Red**: Out of range (<0°C or >150°C)
- **Gray**: Invalid/No data

### CAN Bus Operation

The system automatically transmits temperature data over CAN bus:

**Temperature Messages** (500ms interval)
- Message IDs: 0x100 - 0x10F (one per channel)
- Data Length: 8 bytes
- Format:
  ```
  Byte 0: Channel number (0-15)
  Byte 1: Valid flag (1=valid, 0=invalid)
  Byte 2-3: Reserved
  Byte 4-7: Temperature (IEEE 754 float, little-endian)
  ```

**Status Message** (500ms interval)
- Message ID: 0x110
- Data Length: 8 bytes
- Format:
  ```
  Byte 0: CAN state (1=running)
  Byte 1: Messages to TX
  Byte 2: Messages to RX
  Byte 3: TX error counter
  Byte 4: RX error counter
  Byte 5: TX failed count
  Byte 6: RX missed count
  Byte 7: Bus error count
  ```

## Customization

### Using Real Thermocouple Hardware

To integrate real K-type thermocouple sensors:

1. **For MAX31855/MAX31856 (SPI-based)**:
   - Modify `thermocouple.c` to add SPI initialization
   - Implement SPI read functions for MAX31855/MAX31856
   - Update `thermocouple_read_channel()` function

2. **For MCP9600 (I2C-based)**:
   - Modify `thermocouple.c` to add I2C communication
   - Implement I2C read functions for MCP9600
   - Update `thermocouple_read_channel()` function

3. **Configuration**:
   - Set `CONFIG_EXAMPLE_USE_SIMULATED_DATA=n` in menuconfig
   - Define GPIO pins for your sensor interfaces
   - Add sensor-specific initialization code

### Changing CAN Message Format

Edit `can_transmit.c`:
- Modify `can_transmit_temperature()` for custom data packing
- Change message IDs in `can_transmit.h`
- Update transmission rate with `CAN_TX_RATE_MS`

### Customizing HMI Appearance

Edit `hmi_display.c`:
- Change color scheme (COLOR_* defines)
- Modify panel layout and size
- Add additional UI elements (graphs, logs, etc.)
- Customize LVGL styles

## Troubleshooting

### Display Issues

**Problem**: Screen is blank or not displaying correctly
- **Solution**: Check PSRAM is enabled in menuconfig
- Verify LCD connections on the board
- Try different avoid-tearing modes in menuconfig

### CAN Bus Issues

**Problem**: CAN messages not transmitting
- **Solution**: Verify CAN transceiver is enabled (I2C initialization)
- Check CAN TX/RX GPIO pins (GPIO19, GPIO20)
- Ensure CAN bus has proper termination (120Ω resistors)
- Verify bitrate matches other CAN nodes

**Problem**: CAN error alerts
- **Solution**: Check bus termination
- Verify no other devices are using the same message IDs
- Reduce transmission rate if bus is congested

### Build Issues

**Problem**: LVGL component not found
- **Solution**: Run `idf.py reconfigure` to fetch dependencies
- Manually install LVGL component manager package

**Problem**: Compilation errors
- **Solution**: Ensure ESP-IDF version is v5.0 or later
- Clean build: `idf.py fullclean` then `idf.py build`

## Performance Notes

- **Update Rates**:
  - HMI refresh: 500ms (2 Hz)
  - CAN transmission: 500ms (2 Hz)
  - Simulated data: Updates on each read

- **Memory Usage**:
  - LVGL buffers: ~384 KB (PSRAM)
  - Application heap: ~100 KB
  - Minimum free heap: >500 KB

## Future Enhancements

- [ ] Add data logging to SD card
- [ ] Implement temperature trending/graphing
- [ ] Add alarm/threshold configuration
- [ ] Support for CAN receive mode
- [ ] WiFi connectivity for remote monitoring
- [ ] Cold junction compensation for real thermocouples
- [ ] Multi-language support
- [ ] Touchscreen calibration interface

## References

- [ESP32-S3 Technical Reference Manual](https://www.espressif.com/sites/default/files/documentation/esp32-s3_technical_reference_manual_en.pdf)
- [ESP-IDF Programming Guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/)
- [LVGL Documentation](https://docs.lvgl.io/)
- [Waveshare ESP32-S3-Touch-LCD-4.3 Wiki](https://www.waveshare.com/wiki/ESP32-S3-Touch-LCD-4.3)
- [K-Type Thermocouple Specifications](https://www.omega.com/en-us/resources/thermocouple-types)

## License

This project is licensed under the Apache License 2.0 - see the LICENSE file for details.

## Contributing

Contributions are welcome! Please feel free to submit pull requests or open issues for bugs and feature requests.

## Acknowledgments

- Waveshare for the excellent ESP32-S3-Touch-LCD-4.3 hardware
- Espressif for ESP-IDF framework
- LVGL team for the graphics library
- ESP32 community for examples and support

---

**Note**: This is a demonstration project using simulated thermocouple data. For production use with real sensors, additional hardware interfacing and safety considerations are required.
