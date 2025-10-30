# System Architecture

## Overview

The CAN Thermocouple HMI system is designed with a modular architecture that separates concerns and enables easy maintenance and extension.

## System Block Diagram

```
┌─────────────────────────────────────────────────────────────────┐
│                    ESP32-S3-Touch-LCD-4.3                       │
│                                                                 │
│  ┌──────────────┐    ┌──────────────┐    ┌──────────────┐    │
│  │              │    │              │    │              │    │
│  │ Thermocouple │───▶│ HMI Display  │    │ CAN Transmit │    │
│  │   Module     │    │   Module     │    │   Module     │    │
│  │              │    │              │    │              │    │
│  └──────┬───────┘    └──────┬───────┘    └──────┬───────┘    │
│         │                   │                   │             │
│         │            ┌──────▼──────┐           │             │
│         │            │    LVGL     │           │             │
│         │            │  Graphics   │           │             │
│         │            └──────┬──────┘           │             │
│         │                   │                   │             │
│  ┌──────▼───────────────────▼───────────────────▼───────┐    │
│  │                                                       │    │
│  │              Main Application                         │    │
│  │                                                       │    │
│  └───────────────────────────────────────────────────────┘    │
│         │                   │                   │             │
│         │                   │                   │             │
│  ┌──────▼──────┐    ┌──────▼──────┐    ┌──────▼──────┐      │
│  │   ADC/SPI   │    │  RGB LCD +  │    │    TWAI     │      │
│  │  Interface  │    │   Touch     │    │  (CAN Bus)  │      │
│  └─────────────┘    └─────────────┘    └──────┬──────┘      │
│                                                │             │
└────────────────────────────────────────────────┼─────────────┘
                                                 │
                                          ┌──────▼──────┐
                                          │  CAN Bus    │
                                          │  Network    │
                                          └─────────────┘
```

## Module Description

### 1. Thermocouple Module (`thermocouple.c/h`)

**Responsibility**: Interface with thermocouple sensors

**Key Functions**:
- `thermocouple_init()` - Initialize sensor interface
- `thermocouple_read_channel()` - Read single channel
- `thermocouple_read_all()` - Read all 16 channels
- `thermocouple_get_temperature()` - Get cached reading

**Current Implementation**:
- Simulated data generation for demo
- Realistic temperature variations
- Per-channel validation

**Extension Points**:
- SPI interface for MAX31855/MAX31856
- I2C interface for MCP9600
- ADC with cold junction compensation

### 2. HMI Display Module (`hmi_display.c/h`)

**Responsibility**: User interface and visualization

**Key Functions**:
- `hmi_display_init()` - Create LVGL UI
- `hmi_display_update_channel()` - Update single channel
- `hmi_display_update_all()` - Update all displays
- `hmi_display_update_can_status()` - Update CAN indicator

**Features**:
- 4×4 grid layout for 16 channels
- Color-coded temperature display
- CAN status indicator
- Touch-ready interface

**UI Components**:
```
┌─────────────────────────────────────────────────┐
│  K-Type Thermocouple CAN HMI          ●  CAN   │ ← Title Bar
├─────────────────────────────────────────────────┤
│  ┌──────┐  ┌──────┐  ┌──────┐  ┌──────┐      │
│  │ CH 0 │  │ CH 1 │  │ CH 2 │  │ CH 3 │      │
│  │ 25°C │  │ 30°C │  │ 35°C │  │ 40°C │      │
│  │Normal│  │Normal│  │Normal│  │Normal│      │
│  └──────┘  └──────┘  └──────┘  └──────┘      │
│                                                 │
│  ┌──────┐  ┌──────┐  ┌──────┐  ┌──────┐      │
│  │ CH 4 │  │ CH 5 │  │ CH 6 │  │ CH 7 │      │
│  │ 45°C │  │ 50°C │  │ 55°C │  │ 60°C │      │
│  │Normal│  │Normal│  │Normal│  │Normal│      │
│  └──────┘  └──────┘  └──────┘  └──────┘      │
│                                                 │
│  [... 8 more channels ...]                     │
│                                                 │
└─────────────────────────────────────────────────┘
```

### 3. CAN Transmit Module (`can_transmit.c/h`)

**Responsibility**: CAN bus communication

**Key Functions**:
- `can_init()` - Initialize TWAI driver
- `can_transmit_temperature()` - Send single channel
- `can_transmit_all_temperatures()` - Send all channels
- `can_transmit_status()` - Send system status
- `can_start_transmission_task()` - Start background task

**Features**:
- Automatic periodic transmission
- Error monitoring and reporting
- Configurable bitrate and IDs
- Message queuing and retry

### 4. Main Application (`main.c`)

**Responsibility**: System coordination and initialization

**Initialization Sequence**:
1. NVS Flash
2. LCD and LVGL
3. Thermocouple interface
4. CAN interface
5. HMI display
6. Background tasks

**Task Management**:
- HMI update task (2 Hz)
- CAN transmission task (2 Hz)
- LVGL timer task (automatic)

### 5. LVGL Port Layer (`lvgl_port.c/h`)

**Responsibility**: LVGL integration with ESP-IDF

**Features**:
- Display driver integration
- Touch input handling
- Task scheduling
- Mutex protection
- Memory management

### 6. Waveshare LCD Port (`waveshare_rgb_lcd_port.c/h`)

**Responsibility**: Hardware-specific LCD initialization

**Features**:
- RGB LCD configuration
- GT911 touch controller setup
- Backlight control
- I2C initialization

## Data Flow

### Temperature Reading Flow

```
┌─────────────┐
│  Timer      │
│  Interrupt  │
└──────┬──────┘
       │
       ▼
┌──────────────────┐
│ Thermocouple     │
│ Read All         │
│ Channels         │
└──────┬───────────┘
       │
       ├───────────────┐
       │               │
       ▼               ▼
┌──────────────┐  ┌──────────────┐
│ HMI Update   │  │ CAN Transmit │
│ Task         │  │ Task         │
└──────┬───────┘  └──────┬───────┘
       │                 │
       ▼                 ▼
┌──────────────┐  ┌──────────────┐
│ LVGL         │  │ CAN Bus      │
│ Display      │  │ Network      │
└──────────────┘  └──────────────┘
```

### CAN Transmission Flow

```
┌─────────────────┐
│ CAN TX Task     │
│ (500ms period)  │
└────────┬────────┘
         │
         ▼
┌─────────────────────┐
│ Read All Channels   │
└────────┬────────────┘
         │
         ▼
┌─────────────────────┐
│ For Each Channel:   │
│ - Pack data         │
│ - Queue CAN message │
└────────┬────────────┘
         │
         ▼
┌─────────────────────┐
│ Transmit Status     │
└────────┬────────────┘
         │
         ▼
┌─────────────────────┐
│ Check Alerts        │
│ Handle Errors       │
└─────────────────────┘
```

## Threading Model

```
┌─────────────────────────────────────────────────┐
│                   ESP32-S3                      │
│                                                 │
│  Core 0                      Core 1            │
│  ┌─────────────────┐         ┌──────────────┐ │
│  │                 │         │              │ │
│  │ LVGL Timer Task │         │ HMI Update   │ │
│  │ (Priority 2-4)  │         │ Task         │ │
│  │                 │         │ (Priority 4) │ │
│  └─────────────────┘         └──────────────┘ │
│                                                 │
│  ┌─────────────────┐         ┌──────────────┐ │
│  │                 │         │              │ │
│  │ System Tasks    │         │ CAN TX Task  │ │
│  │ (FreeRTOS)      │         │ (Priority 5) │ │
│  │                 │         │              │ │
│  └─────────────────┘         └──────────────┘ │
│                                                 │
└─────────────────────────────────────────────────┘
```

**Task Priorities**:
- Highest: CAN TX Task (5) - Time-critical
- Medium: HMI Update (4) - User experience
- Low: LVGL Timer (2-4) - Graphics rendering

**Synchronization**:
- LVGL mutex for display access
- FreeRTOS queues for inter-task communication
- No shared global state (data passed via function calls)

## Memory Layout

```
┌──────────────────────────────────────┐
│         ESP32-S3 Memory Map          │
├──────────────────────────────────────┤
│  Flash (16 MB)                       │
│  ├─ Bootloader                       │
│  ├─ Partition table                  │
│  ├─ Application (this project)       │
│  └─ NVS / OTA / File system          │
├──────────────────────────────────────┤
│  PSRAM (8 MB)                        │
│  ├─ LVGL frame buffers (~384 KB)    │
│  ├─ LVGL draw buffers                │
│  └─ Large data structures            │
├──────────────────────────────────────┤
│  Internal SRAM (512 KB)              │
│  ├─ Task stacks                      │
│  ├─ Heap                             │
│  ├─ BSS/Data sections                │
│  └─ DMA buffers                      │
└──────────────────────────────────────┘
```

**Memory Usage Estimates**:
- Application code: ~500 KB (Flash)
- LVGL buffers: ~384 KB (PSRAM)
- Task stacks: ~40 KB (SRAM)
- Heap: ~150 KB (SRAM + PSRAM)
- Free heap at runtime: >500 KB

## Configuration System

```
┌─────────────────────────────────────┐
│      Kconfig.projbuild              │
│  (Compile-time configuration)       │
└──────────┬──────────────────────────┘
           │
           ▼
┌─────────────────────────────────────┐
│     sdkconfig.defaults              │
│   (Default settings)                │
└──────────┬──────────────────────────┘
           │
           ▼
┌─────────────────────────────────────┐
│      Build System                   │
│  (CMake + ESP-IDF)                  │
└──────────┬──────────────────────────┘
           │
           ▼
┌─────────────────────────────────────┐
│   Compiled Application              │
│  (CONFIG_* macros)                  │
└─────────────────────────────────────┘
```

**Key Configuration Options**:
- CAN: Bitrate, GPIO pins, message IDs
- Thermocouple: Simulation mode, channel count
- Display: Buffer sizes, anti-tearing mode
- LVGL: Task priority, update rates

## Extension Points

### Adding New Sensors

1. Modify `thermocouple.c`:
   - Add hardware initialization
   - Implement read functions
   - Update data validation

2. Update `Kconfig.projbuild`:
   - Add sensor-specific options
   - GPIO pin configuration

### Adding New Display Elements

1. Modify `hmi_display.c`:
   - Create new LVGL objects
   - Add update functions
   - Integrate with existing layout

2. Extend API in `hmi_display.h`

### Adding CAN Receive

1. Create new module `can_receive.c/h`
2. Add RX task and message parsing
3. Integrate with HMI or data logging

### Adding Data Logging

1. Create `data_logger.c/h` module
2. Interface with SD card (see example 03)
3. Log temperature data with timestamps
4. Add UI controls for log management

## Performance Characteristics

**Response Times**:
- Sensor read: <1ms (simulated) or 10-100ms (real SPI/I2C)
- Display update: <50ms (bounded by LVGL)
- CAN transmission: <1ms per message
- Touch response: <100ms

**Update Rates**:
- Temperature reading: 2 Hz
- Display refresh: 2 Hz
- CAN transmission: 2 Hz (full cycle)
- LVGL render: Up to 60 Hz (if needed)

**Latency**:
- Sensor → Display: <500ms
- Sensor → CAN: <500ms
- Touch → Response: <100ms

## Error Handling Strategy

**Sensor Errors**:
- Mark channel as invalid
- Display error status
- Continue with other channels
- Auto-recovery on next read

**CAN Errors**:
- Log error to console
- Update status message
- Attempt retry
- Enter error passive if persistent

**Display Errors**:
- Graceful degradation
- Fallback to simpler rendering
- Log but don't crash

**System Errors**:
- Watchdog timer protection
- Panic handler for critical errors
- Restart if unrecoverable

## Security Considerations

**Current Status**: Development/Demo system

**Future Production Requirements**:
- CAN message authentication
- Encrypted configuration storage
- Secure boot
- Access control for settings
- Audit logging

---

This architecture provides a solid foundation that is:
- **Modular**: Easy to modify individual components
- **Extensible**: Simple to add new features
- **Maintainable**: Clear separation of concerns
- **Performant**: Efficient use of dual-core ESP32-S3
- **Robust**: Error handling at all levels
