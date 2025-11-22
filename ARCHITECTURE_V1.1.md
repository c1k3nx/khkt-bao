# Architecture v1.1 - Optimized Design

## Overview

Version 1.1 addresses pin mapping conflicts and standardizes MQTT communication protocol. Key changes:

1. **DFPlayer moved to ESP8266** (hardware UART stability)
2. **Emergency sensors use interrupts** (fire, sound)
3. **Standardized MQTT schema** (JSON validation)
4. **Optimized pin allocation** on Arduino UNO

---

## System Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                    MQTT Broker (HiveMQ)                          │
│              Topic: greenhouse/* (standardized)                  │
└────────────────────────┬────────────────────────────────────────┘
                         │
            ┌────────────┼────────────┬──────────────┐
            │            │            │              │
    ┌───────▼─────┐  ┌──▼──────┐  ┌─▼───────┐  ┌──▼──────────┐
    │   ESP8266   │  │PC-Vision│  │  App    │  │ ESP32-CAM   │
    │             │  │ YOLO+HF │  │ PyQt6   │  │HTTP Stream  │
    │ - MQTT      │  │         │  │         │  │             │
    │ - DFPlayer  │  │ Publish │  │Subscribe│  │ /stream     │
    │ - GPS       │  │ Vision  │  │ Control │  │ /capture    │
    │ - WS2812    │  │ Results │  │ Display │  │             │
    └──────┬──────┘  └─────────┘  └─────────┘  └─────────────┘
           │                                             │
           │ SoftwareSerial 115200 (UNO↔ESP)            │
           │ (Command/Status JSON)                       │ HTTP GET
           │                                             │ (Image)
    ┌──────▼──────┐                                     │
    │ Arduino UNO │◄────────────────────────────────────┘
    │             │
    │ SENSORS:                ACTUATORS:                │
    │ - DHT11 (D13)          - 4 Relays (D4/5/6/7)     │
    │ - BH1750 (I²C)         - 2 Servos (D9/10)        │
    │ - JSN-SR04T (D8/12)    - WS2812 (D11)            │
    │ - Soil (A0)            - LCD1602 (I²C)           │
    │ - MQ-3 (A3)                                      │
    │ - Flame (A0 + D2 INT)                            │
    │ - Sound (A1 + D3 INT)                            │
    └─────────────┘
```

---

## Communication Channels

### 1. UNO ↔ ESP8266 (UART via SoftwareSerial)

**ESP8266 side**: SoftwareSerial on GPIO12 (RX), GPIO14 (TX)
**UNO side**: Hardware UART on D0 (RX), D1 (TX) - **level shifted!**

**Baudrate**: 115200 bps

**Protocol**: JSON, newline-delimited

**Message Types**:

```json
// UNO → ESP8266 (sensor data)
{
  "type": "sensor",
  "data": {
    "temperature": "26.5",
    "humidity": "68.0",
    "soilMoisture": "47.0",
    "lightIntensity": "320",
    "flameAnalog": "256",
    "soundLevel": "512",
    "gasMQ3": "0.62"
  }
}

// ESP8266 → UNO (control command)
{
  "type": "control",
  "device": "fan",
  "action": "ON"
}

// UNO → ESP8266 (event)
{
  "type": "event",
  "event": "flame",
  "value": "DETECTED"
}
```

**Error Handling**:
- Timeout: 3 seconds
- Retry: 3 attempts
- Fallback: UNO enters safe mode (fans ON, pump OFF)

### 2. ESP8266 ↔ MQTT Broker

**Topics**: See [`mqtt-schema.json`](mqtt-schema.json)

**QoS Levels**:
- Sensor data: QoS 0 (fire-and-forget)
- Control commands: QoS 1 (at-least-once)
- Status messages: QoS 1 + **retained**

**Reconnection**: Exponential backoff (2s, 4s, 8s, 16s, 30s max)

### 3. ESP32-CAM ↔ PC-Vision (HTTP)

**Endpoints**:
- `GET /stream` - MJPEG stream for live view (App)
- `GET /capture` - Single JPEG for analysis (PC-Vision)

**PC-Vision**: Polls `/capture` every 5 seconds (configurable)

### 4. App ↔ MQTT Broker

**Subscriptions**:
- `greenhouse/data/#` - All sensor readings
- `greenhouse/status/#` - Device status (retained)
- `greenhouse/event/#` - Emergency events
- `greenhouse/cam/#` - Vision results

**Publications**:
- `greenhouse/control/#` - Device control commands
- `greenhouse/set/thresholds/#` - Auto mode thresholds

---

## Pin Allocation (Optimized)

### Arduino UNO R3

| Pin | Function | Type | Notes |
|-----|----------|------|-------|
| **Digital** |
| D0 | UART RX (ESP8266) | Input | Level shifted 3.3V |
| D1 | UART TX (ESP8266) | Output | Level shifted to 3.3V |
| D2 | Flame Interrupt | Input (INT0) | Emergency detection |
| D3 | Sound Interrupt | Input (INT1) | Emergency detection |
| D4 | Relay: Pump | Output | Active LOW |
| D5 | Relay: Aux Fan | Output | Active LOW |
| D6 | Relay: LED 12V | Output | Active LOW |
| D7 | Relay: Main Fan | Output | Active LOW |
| D8 | JSN Trigger | Output | Ultrasonic sensor |
| D9 | Servo: Window (MG996R) | PWM | 180° servo |
| D10 | Servo: Door (MG90) | PWM | 180° servo |
| D11 | WS2812B Data | Output | 2 rings × 8 LEDs |
| D12 | JSN Echo | Input | Ultrasonic sensor |
| D13 | DHT11 Data | Input/Output | Temp/humidity |
| **Analog** |
| A0 | Flame Analog | Input | 0-1023 ADC |
| A1 | Sound Analog | Input | 0-1023 ADC |
| A2 | Soil Moisture | Input | 0-1023 ADC |
| A3 | MQ-3 Gas | Input | 0-1023 ADC |
| A4 | I²C SDA | Bidir | BH1750, LCD1602 |
| A5 | I²C SCL | Bidir | BH1750, LCD1602 |

**Power**: 5V/2A recommended (servo + relays)

### ESP8266 (NodeMCU)

| GPIO | NodeMCU | Function | Notes |
|------|---------|----------|-------|
| GPIO1 | TX | DFPlayer TX | UART0 hardware |
| GPIO3 | RX | DFPlayer RX | UART0 hardware (via 1kΩ) |
| GPIO2 | D4 | (Reserved) | Boot mode pin |
| GPIO4 | D2 | WS2812 Ring 1 | 8 LEDs |
| GPIO5 | D1 | WS2812 Ring 2 | 8 LEDs |
| GPIO12 | D6 | UNO UART RX | SoftwareSerial |
| GPIO14 | D5 | UNO UART TX | SoftwareSerial |
| GPIO13 | D7 | GPS RX | SoftwareSerial |
| GPIO15 | D8 | GPS TX | SoftwareSerial (pulldown!) |

**Power**: 5V/1A via USB (AMS1117 regulator)

**Note**: GPIO15 must be LOW at boot (has pulldown resistor on NodeMCU)

---

## Data Flow

### Normal Operation (AUTO Mode)

```
1. UNO reads sensors every 2s
   ↓
2. UNO sends JSON via UART to ESP8266
   ↓
3. ESP8266 publishes to MQTT: greenhouse/data/*
   ↓
4. App receives sensor data, evaluates AUTO rules
   ↓
5. App publishes control: greenhouse/control/fan → "ON"
   ↓
6. ESP8266 receives control, sends JSON to UNO
   ↓
7. UNO executes command, publishes status back
   ↓
8. ESP8266 publishes status: greenhouse/status/fan → "ON" (retained)
   ↓
9. App updates UI with confirmed status
```

### Emergency Event (Fire Detection)

```
1. Flame sensor triggers D2 interrupt (ISR)
   ↓
2. UNO immediately:
   - Sends event JSON to ESP8266
   - Enters emergency mode (pump OFF, fans ON)
   ↓
3. ESP8266:
   - Publishes greenhouse/event/flame → "DETECTED"
   - Plays DFPlayer track 8 (fire alert)
   ↓
4. App:
   - Shows fire alert dialog
   - Overrides AUTO rules (safety priority)
```

### Vision Detection Flow

```
1. PC-Vision polls ESP32-CAM /capture every 5s
   ↓
2. YOLO detects plants, Hugging Face detects disease
   ↓
3. PC-Vision publishes greenhouse/cam/detections (JSON)
   ↓
4. App receives detections:
   - Updates plant count display
   - Highlights diseased plants
   ↓
5. If diseased plant found:
   - App sends greenhouse/control/plantTalk → "ON"
   - ESP8266 plays DFPlayer track 10 (care needed)
```

---

## Safety Features

### Hardware Interrupts (Emergency Response)

**Flame Detection** (D2/INT0):
- **Trigger**: Falling edge (active LOW sensor)
- **Response**: <1ms interrupt latency
- **Action**: Pump OFF, all fans ON, send event

**Sound Detection** (D3/INT1):
- **Trigger**: Falling edge
- **Response**: <1ms
- **Action**: Send event, optional DFPlayer warning

### Watchdog & Fallback

**UNO Watchdog**:
- If no UART from ESP8266 for >3s:
  - Enter safe mode
  - Display "UART LOST" on LCD
  - Continue monitoring sensors
  - Emergency overrides still active

**ESP8266 Watchdog**:
- If no UART from UNO for >3s:
  - Publish error: `greenhouse/event/error` → "UNO_TIMEOUT"
  - Maintain last known status (retained MQTT)

**Network Watchdog**:
- If Wi-Fi lost: Retry with exponential backoff
- If MQTT lost: Retry connection, queue messages

### Safe State Defaults

When entering fallback mode:
- ✅ Main fan: **ON** (ventilation)
- ✅ Aux fan: **ON** (safety)
- ❌ Pump: **OFF** (prevent flooding)
- ❌ LED 12V: **OFF** (reduce power)
- ❌ Servos: **Hold last position**
- ✅ WS2812: **Red alert pattern**

---

## MQTT Topic Hierarchy

See [`mqtt-schema.json`](mqtt-schema.json) for complete schema.

### Quick Reference

| Category | Prefix | QoS | Retain | Publisher |
|----------|--------|-----|--------|-----------|
| Sensor data | `greenhouse/data/` | 0 | No | ESP8266 |
| Events | `greenhouse/event/` | 1 | No | ESP8266/UNO |
| Status | `greenhouse/status/` | 1 | **Yes** | ESP8266 |
| Control | `greenhouse/control/` | 1 | No | App |
| Thresholds | `greenhouse/set/thresholds/` | 1 | Yes | App |
| Vision | `greenhouse/cam/` | 0 | No | PC-Vision |

**Naming Convention**:
- Use **camelCase** for multi-word fields
- Use **full words** (not abbreviations)
- Use **string values** for sensor data (not raw JSON numbers)
- Use **enum values** for states ("ON"/"OFF", not 1/0)

---

## Performance Characteristics

### Latency

| Operation | Latency | Notes |
|-----------|---------|-------|
| Sensor read (UNO) | 2s | Polling interval |
| UART UNO→ESP | ~10ms | 115200 baud, small JSON |
| MQTT publish | 20-100ms | Depends on network |
| Control command | 50-200ms | Round-trip UNO→ESP→MQTT→App |
| Emergency interrupt | <1ms | Hardware interrupt |
| Vision detection | 5s | Configurable poll interval |

### Throughput

| Channel | Throughput | Max Message Size |
|---------|------------|------------------|
| UART UNO↔ESP | ~11 KB/s | 512 bytes/message |
| MQTT | ~50 msg/s | 256 bytes (PubSubClient) |
| HTTP stream | 15-30 FPS | Depends on network |

### Resource Usage

**Arduino UNO**:
- Flash: ~20KB / 32KB (60%)
- SRAM: ~1.2KB / 2KB (60%)
- CPU: ~30% (with sensor polling)

**ESP8266**:
- Flash: ~300KB / 4MB (7%)
- Heap: ~25KB free (runtime)
- CPU: ~40% (Wi-Fi + MQTT)

---

## Extending the System

### Adding New Sensors

1. **Update pin mapping** in `ARCHITECTURE_V1.1.md`
2. **Add to UART protocol** (UNO→ESP8266 JSON)
3. **Add to MQTT schema** (`mqtt-schema.json`)
4. **Update firmware**: `firmware-uno/`, `firmware-esp8266/`
5. **Update App** UI to display new sensor

### Adding New Actuators

1. **Allocate pin** (check conflicts in pin table)
2. **Add control topic** to `mqtt-schema.json`
3. **Update ESP8266** to subscribe to new topic
4. **Update UNO** to handle new device
5. **Update App** to add control button

### Scaling Considerations

**Multiple Greenhouses**:
- Use **topic prefix**: `site1/greenhouse/`, `site2/greenhouse/`
- Separate MQTT namespaces
- Central dashboard aggregates data

**High-Frequency Sensors** (>1Hz):
- Consider **local buffering** on ESP8266
- Publish **aggregate** (min/max/avg) instead of raw
- Use **QoS 0** for non-critical data

---

## References

- **MQTT Schema**: [`mqtt-schema.json`](mqtt-schema.json)
- **Pin Mapping**: [`docs/pinout-uno.md`](docs/pinout-uno.md), [`docs/pinout-esp8266.md`](docs/pinout-esp8266.md)
- **Migration Guide**: [`MIGRATION_V1.1.md`](MIGRATION_V1.1.md)
- **UART Protocol**: [`docs/uart-protocol.md`](docs/uart-protocol.md)

---

**Version**: 1.1.0
**Last updated**: 2025-11-23
