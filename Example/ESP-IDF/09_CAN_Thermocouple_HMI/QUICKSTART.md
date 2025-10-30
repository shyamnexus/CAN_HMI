# Quick Start Guide

Get your CAN Thermocouple HMI up and running in minutes!

## Prerequisites Checklist

- [ ] ESP-IDF v5.0+ installed
- [ ] Python 3.8+ installed
- [ ] USB cable for programming
- [ ] Waveshare ESP32-S3-Touch-LCD-4.3 board
- [ ] (Optional) CAN bus analyzer or receiver for testing

## 5-Minute Setup

### Step 1: Environment Setup (1 min)

Open terminal and source ESP-IDF:

```bash
# Linux/Mac
. $HOME/esp/esp-idf/export.sh

# Windows
%USERPROFILE%\esp\esp-idf\export.bat
```

### Step 2: Navigate to Project (30 sec)

```bash
cd Example/ESP-IDF/09_CAN_Thermocouple_HMI
```

### Step 3: Build Project (2 min)

```bash
idf.py build
```

Expected output:
```
Project build complete. To flash, run:
 idf.py -p (PORT) flash
```

### Step 4: Flash to Board (1 min)

Connect your board via USB and run:

```bash
# Linux
idf.py -p /dev/ttyUSB0 flash

# Mac
idf.py -p /dev/cu.usbserial-* flash

# Windows
idf.py -p COM3 flash
```

### Step 5: Monitor Output (30 sec)

```bash
idf.py -p /dev/ttyUSB0 monitor
```

To exit monitor: `Ctrl+]`

## What You Should See

### 1. Serial Output

```
I (xxx) MAIN: ===========================================
I (xxx) MAIN:  CAN Thermocouple HMI Demo
I (xxx) MAIN:  Waveshare ESP32-S3-Touch-LCD-4.3
I (xxx) MAIN: ===========================================
I (xxx) MAIN: Initializing LCD and LVGL...
I (xxx) MAIN: LCD and LVGL initialized successfully
I (xxx) MAIN: Initializing thermocouple interface...
I (xxx) THERMOCOUPLE: Thermocouple interface initialized (simulated mode)
I (xxx) MAIN: Initializing CAN interface...
I (xxx) CAN_TX: TWAI driver installed
I (xxx) CAN_TX: TWAI driver started
I (xxx) MAIN: System initialized successfully!
```

### 2. LCD Display

You should see:
- Blue title bar: "K-Type Thermocouple CAN HMI"
- Green CAN status LED (top right)
- 16 channel panels in a 4×4 grid
- Each showing temperature readings updating in real-time

### 3. CAN Bus (if analyzer connected)

- Message IDs: 0x100 - 0x110
- Messages appearing every 500ms
- Temperature data visible in hex format

## Common Issues & Fixes

### Issue: "Port doesn't exist"

**Solution**: Find correct port:
```bash
# Linux
ls /dev/ttyUSB*

# Mac
ls /dev/cu.*

# Windows - use Device Manager
```

### Issue: "Permission denied" (Linux)

**Solution**: Add user to dialout group:
```bash
sudo usermod -a -G dialout $USER
# Log out and log back in
```

### Issue: Build fails with "lvgl not found"

**Solution**: Update component dependencies:
```bash
idf.py reconfigure
idf.py build
```

### Issue: Display is blank

**Solution**: 
1. Check PSRAM is enabled in menuconfig
2. Try different avoid-tearing mode:
```bash
idf.py menuconfig
# Navigate to: Example Configuration → Display → Select Avoid Tearing Mode
# Try Mode 1 or Mode 2
```

### Issue: CAN messages not transmitting

**Solution**:
1. Check if your board has CAN transceiver properly powered
2. Verify GPIO pins are correct (GPIO19/20)
3. Check termination resistors (120Ω) on CAN bus
4. Use CAN analyzer in listen-only mode for testing

## Testing CAN Output (Without Full Bus)

### Using CAN USB Adapter

1. Connect CAN analyzer to CANH/CANL pins
2. Set analyzer to 500 kbps
3. Listen for messages on IDs 0x100-0x110

### Using Another ESP32

Flash the receive example:
```bash
cd ../07_TWAIreceive
idf.py -p /dev/ttyUSB1 flash monitor
```

Connect CANH to CANH, CANL to CANL between boards.

## Next Steps

### Customize Configuration

```bash
idf.py menuconfig
```

Navigate to:
- **Example Configuration → CAN Configuration**
  - Change bitrate (125/250/500/1000 kbps)
  - Modify GPIO pins if needed

- **Example Configuration → Thermocouple Configuration**
  - Enable/disable simulation mode
  - Adjust update rates

### View CAN Messages

Install python-can:
```bash
pip install python-can
```

Run example receiver:
```python
import can
import struct

bus = can.interface.Bus(channel='can0', bustype='socketcan', bitrate=500000)

while True:
    msg = bus.recv()
    if 0x100 <= msg.arbitration_id <= 0x10F:
        channel = msg.data[0]
        temp = struct.unpack('<f', msg.data[4:8])[0]
        print(f"Channel {channel}: {temp:.1f}°C")
```

### Integrate Real Sensors

See `README.md` section "Using Real Thermocouple Hardware" for details on integrating:
- MAX31855/MAX31856 (SPI)
- MCP9600 (I2C)
- Other K-type amplifiers

## Learning Resources

- [Full README](README.md) - Complete documentation
- [CAN Protocol](CAN_PROTOCOL.md) - Message format details
- [ESP-IDF CAN Guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/twai.html)
- [LVGL Documentation](https://docs.lvgl.io/)
- [Waveshare Wiki](https://www.waveshare.com/wiki/ESP32-S3-Touch-LCD-4.3)

## Need Help?

1. Check `README.md` Troubleshooting section
2. Enable verbose logging:
```bash
idf.py menuconfig
# Component config → Log output → Default log verbosity → Verbose
```
3. Check ESP-IDF forum: https://esp32.com/
4. Verify hardware with simpler examples (01_I2C_Test, 06_TWAItransmit)

## Success Checklist

- [ ] Display shows 16 temperature channels
- [ ] Temperatures updating in real-time
- [ ] CAN LED is green
- [ ] Serial monitor shows no errors
- [ ] CAN messages visible on bus analyzer
- [ ] Temperature colors change based on values

If all checked, congratulations! Your CAN HMI is working perfectly! 🎉

---

**Build Time**: ~2 minutes  
**Flash Time**: ~1 minute  
**Total Setup**: ~5 minutes  
**Difficulty**: ⭐⭐☆☆☆ (Beginner-friendly)
