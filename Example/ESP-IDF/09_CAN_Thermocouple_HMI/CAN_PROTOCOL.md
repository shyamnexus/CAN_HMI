# CAN Bus Protocol Documentation

## Overview

This document describes the CAN bus protocol used for transmitting K-type thermocouple temperature data from the ESP32-S3 HMI to other nodes on the CAN network.

## CAN Bus Configuration

- **Bitrate**: 500 kbps (configurable: 125/250/500/1000 kbps)
- **Frame Type**: Standard CAN 2.0A (11-bit identifier)
- **Transmission Rate**: 500ms per complete cycle
- **Bus Mode**: Normal operation with acknowledgment

## Message Types

### 1. Temperature Data Messages

**Message IDs**: `0x100` to `0x10F` (16 messages)

Each thermocouple channel transmits on a dedicated message ID:
- Channel 0: ID `0x100`
- Channel 1: ID `0x101`
- ...
- Channel 15: ID `0x10F`

**Data Length Code (DLC)**: 8 bytes

**Data Format**:

| Byte | Field | Type | Description |
|------|-------|------|-------------|
| 0 | Channel | uint8_t | Channel number (0-15) |
| 1 | Valid Flag | uint8_t | 0=Invalid, 1=Valid reading |
| 2 | Reserved | uint8_t | Reserved for future use |
| 3 | Reserved | uint8_t | Reserved for future use |
| 4-7 | Temperature | float32 | Temperature in Celsius (IEEE 754) |

**Example** (Channel 5, 25.5°C):
```
ID: 0x105
Data: 05 01 00 00 00 00 CC 41
      ^  ^  ^^ ^^ ^^^^^^^^
      |  |  |  |  └─ Temperature: 25.5°C (IEEE 754 little-endian)
      |  |  |  └─ Reserved
      |  |  └─ Reserved
      |  └─ Valid: 1 (valid reading)
      └─ Channel: 5
```

**Temperature Encoding**:
- Format: IEEE 754 single-precision float (32-bit)
- Byte order: Little-endian
- Range: -40°C to +400°C (typical for K-type)
- Resolution: 0.1°C

**Valid Flag Values**:
- `0x00`: Invalid reading (sensor error, disconnected, etc.)
- `0x01`: Valid reading

### 2. System Status Message

**Message ID**: `0x110`

**Data Length Code (DLC)**: 8 bytes

**Data Format**:

| Byte | Field | Type | Description |
|------|-------|------|-------------|
| 0 | CAN State | uint8_t | 0=Stopped, 1=Running |
| 1 | Messages to TX | uint8_t | Pending TX queue count |
| 2 | Messages to RX | uint8_t | Pending RX queue count |
| 3 | TX Error Counter | uint8_t | Transmission error count |
| 4 | RX Error Counter | uint8_t | Reception error count |
| 5 | TX Failed Count | uint8_t | Failed transmission count |
| 6 | RX Missed Count | uint8_t | Missed reception count |
| 7 | Bus Error Count | uint8_t | Bus error count |

**Example** (Normal operation):
```
ID: 0x110
Data: 01 00 00 00 00 00 00 00
      ^  ^  ^  ^  ^  ^  ^  ^
      |  |  |  |  |  |  |  └─ Bus Errors: 0
      |  |  |  |  |  |  └─ RX Missed: 0
      |  |  |  |  |  └─ TX Failed: 0
      |  |  |  |  └─ RX Errors: 0
      |  |  |  └─ TX Errors: 0
      |  |  └─ RX Queue: 0
      |  └─ TX Queue: 0
      └─ State: 1 (Running)
```

### 3. Heartbeat Message (Future)

**Message ID**: `0x111`

Reserved for periodic heartbeat/keepalive messages.

## Transmission Schedule

The system transmits messages in the following sequence every 500ms:

```
Time    Message
------  --------
0ms     Channel 0 (0x100)
10ms    Channel 1 (0x101)
20ms    Channel 2 (0x102)
30ms    Channel 3 (0x103)
...
150ms   Channel 15 (0x10F)
160ms   System Status (0x110)
```

Total transmission time per cycle: ~170ms
Idle time: ~330ms

## Error Handling

### Bus Errors

When bus errors occur:
1. Error counter increments in status message
2. System attempts retransmission
3. If persistent, CAN driver may enter error passive mode
4. System status flag indicates error state

### Sensor Errors

When thermocouple reading fails:
1. Valid flag set to `0x00` in temperature message
2. Temperature field may contain last valid reading or 0.0
3. Individual channel status tracked independently

## Integration Examples

### Python CAN Receiver

```python
import can
import struct

# Initialize CAN bus
bus = can.interface.Bus(channel='can0', bustype='socketcan', bitrate=500000)

while True:
    msg = bus.recv()
    
    # Check if temperature message
    if 0x100 <= msg.arbitration_id <= 0x10F:
        channel = msg.data[0]
        valid = msg.data[1]
        temp = struct.unpack('<f', msg.data[4:8])[0]
        
        if valid:
            print(f"Channel {channel}: {temp:.1f}°C")
        else:
            print(f"Channel {channel}: Invalid reading")
    
    # Check if status message
    elif msg.arbitration_id == 0x110:
        state = "Running" if msg.data[0] else "Stopped"
        print(f"CAN Status: {state}")
```

### Arduino CAN Receiver

```cpp
#include <mcp_can.h>

MCP_CAN CAN(10); // CS pin

void setup() {
    Serial.begin(115200);
    CAN.begin(MCP_ANY, CAN_500KBPS, MCP_8MHZ);
    CAN.setMode(MCP_NORMAL);
}

void loop() {
    long unsigned int rxId;
    unsigned char len = 0;
    unsigned char rxBuf[8];
    
    if (CAN.checkReceive() == CAN_MSGAVAIL) {
        CAN.readMsgBuf(&rxId, &len, rxBuf);
        
        // Temperature message
        if (rxId >= 0x100 && rxId <= 0x10F) {
            uint8_t channel = rxBuf[0];
            uint8_t valid = rxBuf[1];
            float temp;
            memcpy(&temp, &rxBuf[4], 4);
            
            if (valid) {
                Serial.print("CH");
                Serial.print(channel);
                Serial.print(": ");
                Serial.print(temp, 1);
                Serial.println("°C");
            }
        }
    }
}
```

## Message Filtering

To receive only temperature messages:
- **Standard Mask**: `0x7F0` (filter on upper bits)
- **Standard Filter**: `0x100`
- Result: Receives messages `0x100` to `0x10F`

To receive all system messages:
- **Standard Mask**: `0x700` (filter on upper 3 bits)
- **Standard Filter**: `0x100`
- Result: Receives messages `0x100` to `0x1FF`

## Bus Loading Calculation

- Message size: ~130 bits (including CAN overhead)
- Bitrate: 500 kbps
- Time per message: ~0.26ms
- 17 messages per cycle: ~4.42ms
- Cycle time: 500ms
- **Bus utilization: <1%**

This low utilization allows many additional nodes and messages on the bus.

## Electrical Specifications

### CAN Bus Physical Layer

- **Standard**: ISO 11898-2 (high-speed CAN)
- **Voltage Levels**:
  - Dominant: CANH ~3.5V, CANL ~1.5V (differential ~2V)
  - Recessive: CANH ~2.5V, CANL ~2.5V (differential ~0V)
- **Termination**: 120Ω resistors at each end of bus
- **Maximum Bus Length**: 
  - @ 500 kbps: ~100 meters
  - @ 125 kbps: ~500 meters
- **Maximum Nodes**: 30 (limited by electrical loading)

### Waveshare Board Pinout

- **CAN_TX**: GPIO20
- **CAN_RX**: GPIO19
- **Transceiver**: Built-in, enabled via I2C

## Compliance

This protocol follows:
- CAN 2.0A specification (11-bit identifiers)
- ISO 11898-1 (data link layer)
- ISO 11898-2 (physical layer for high-speed CAN)

## Version History

- **v1.0** (2025-10-30): Initial protocol definition
  - 16 channel temperature messages
  - System status message
  - 500ms transmission cycle

## Future Protocol Extensions

Potential enhancements for future versions:
1. Extended CAN (29-bit) support for more devices
2. Multi-frame messages for additional data
3. Configuration messages for remote setup
4. Data logging control messages
5. Alarm/threshold messages
6. Timestamp synchronization

---

For implementation details, see the source code in `can_transmit.c` and `can_transmit.h`.
