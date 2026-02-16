# UART Protocol (UNO ↔ ESP8266) - v1.4 HARDWARE SERIAL

## Connection Specifications

| Parameter | Value |
|-----------|-------|
| **Version** | **v1.4 COMPREHENSIVE OVERHAUL** |
| **Connection** | **Hardware Serial D0/D1 (UNO) ↔ GPIO1/3 (ESP8266)** |
| Baud Rate | **115200 bps** (v1.4: increased from 57600) |
| Data Bits | 8 |
| Parity | None |
| Stop Bits | 1 |
| Flow Control | None (8N1) |
| Format | JSON, newline-delimited (`\n`) |
| Voltage | **UNO D1/TX (5V) → ESP8266 GPIO3/RX (3.3V)**: NEEDS level shifter! |
| | **ESP8266 GPIO1/TX (3.3V) → UNO D0/RX (5V)**: OK, no shifter needed |

## ⚠️ CRITICAL v1.4 UPLOAD WARNING

**Hardware Serial uses D0/D1 (UNO) and GPIO1/3 (ESP8266) - same pins used for USB programming!**

**MUST DISCONNECT** these pins during firmware upload:
- **UNO**: Disconnect D0 and D1 from level shifter
- **ESP8266**: Disconnect GPIO1 (TX) and GPIO3 (RX) from level shifter

**RECONNECT** after upload completes.

See [V1.4_USAGE_GUIDE.md](../V1.4_USAGE_GUIDE.md) for detailed upload procedure.

## Level Shifter Circuit (v1.4)

```
UNO D1 (TX, 5V) ──┬── 1kΩ ──┬── ESP8266 GPIO3 (RX, 3.3V)
                  │         │
                  └─ 2kΩ ───┴── GND

Output voltage: 5V × (2kΩ / 3kΩ) = 3.33V ✓
```

**Alternative**: Use bidirectional level shifter module (e.g., TXS0108E) for more reliable connection.

See [V1.4_WIRING_GUIDE.md](../V1.4_WIRING_GUIDE.md) for complete wiring instructions.

## Message Types

### 1. Sensor Data (UNO → ESP8266)

**Direction**: UNO → ESP8266

**Frequency**: Every 3 seconds

**Format**:
```json
{
  "type": "sensor",
  "ts": 1732251234,
  "payload": {
    "temp_c": 30.4,
    "hum_pct": 68.5,
    "light_lux": 120.0,
    "soil_pct": 41.2,
    "mq3": 412,
    "flame": 83,
    "sound": 612,
    "distance_cm": 128.5
  }
}
```

**Fields**:
- `type`: Always `"sensor"`
- `ts`: Timestamp (Unix epoch seconds, `millis() / 1000`)
- `payload`: Object containing all sensor readings
  - `temp_c`: Temperature (°C), float, -999 if error
  - `hum_pct`: Humidity (%), float, -999 if error
  - `light_lux`: Light intensity (lux), float
  - `soil_pct`: Soil moisture (%), float, 0-100
  - `mq3`: MQ-3 alcohol sensor, integer, 0-1023 ADC
  - `flame`: Flame sensor, integer, 0-1023 ADC
  - `sound`: Sound sensor, integer, 0-1023 ADC
  - `distance_cm`: Ultrasonic distance (cm), float, -1 if error

---

### 2. Command (ESP8266 → UNO)

**Direction**: ESP8266 → UNO

**Trigger**: When MQTT command received

**Format**:
```json
{
  "type": "set",
  "ts": 1732251234,
  "req_id": 101,
  "payload": {
    "pump": 1,
    "fan_main": 0,
    "fan_aux": 1,
    "led12v": 2,
    "window": 1,
    "door": 0
  }
}
```

**Fields**:
- `type`: Always `"set"`
- `ts`: Timestamp
- `req_id`: Request ID (integer), used for ACK matching
- `payload`: Object with actuator commands
  - **Relays** (`pump`, `fan_main`, `fan_aux`, `led12v`):
    - `0` = OFF
    - `1` = ON
    - `2` = BLINK (for led12v, handled by ESP8266 LED rings)
  - **Servos** (`window`, `door`):
    - `0` = CLOSE
    - `1` = OPEN

**Behavior**:
- UNO executes commands immediately
- UNO sends ACK back (see below)

---

### 3. Acknowledgement (UNO → ESP8266)

**Direction**: UNO → ESP8266

**Trigger**: After executing `set` command

**Format**:
```json
{
  "type": "ack",
  "ts": 1732251234,
  "req_id": 101,
  "ok": true
}
```

**Fields**:
- `type`: Always `"ack"`
- `ts`: Timestamp
- `req_id`: Matches the `req_id` from command
- `ok`: Boolean, `true` if success, `false` if error

---

### 4. Event (UNO → ESP8266)

**Direction**: UNO → ESP8266

**Trigger**: Emergency events (fire, loud sound, etc.)

**Format**:
```json
{
  "type": "event",
  "ts": 1732251234,
  "event": "FIRE",
  "level": 830
}
```

**Fields**:
- `type`: Always `"event"`
- `ts`: Timestamp
- `event`: Event name (string)
  - `"FIRE"`: Fire detected
  - `"SOUND"`: Loud sound detected
- `level`: Sensor reading (integer, ADC value)

**Behavior**:
- ESP8266 publishes alert to MQTT
- App can trigger DFPlayer warning

---

### 5. Debug Message (UNO → ESP8266) - NEW in v1.4 Phase 2

**Direction**: UNO → ESP8266

**Trigger**: Debug logging from UNO (replaces Serial.print debug)

**Format**:
```json
{
  "type": "debug",
  "source": "UNO",
  "msg": "BH1750 init OK",
  "ts": 12345
}
```

**Fields**:
- `type`: Always `"debug"`
- `source`: Always `"UNO"` (identifies sender)
- `msg`: Debug message string
- `ts`: Timestamp (millis())

**Behavior**:
- ESP8266 forwards to MQTT topic `greenhouse/sys/debug`
- Used for remote debugging since Hardware Serial is occupied

**v1.4 Note**: Hardware Serial now used for UNO communication, so traditional `Serial.println()` debug is not available. Use MQTT debug instead.

---

### 6. Environment Data (ESP8266 → UNO) - NEW in v1.3/v1.4

**Direction**: ESP8266 → UNO

**Trigger**: After ESP8266 reads DHT11 and GPS (every 2-3 seconds)

**Format**:
```json
{
  "type": "env",
  "temp": "25.3",
  "hum": "65.2",
  "gps_lat": "10.762622",
  "gps_lng": "106.660172",
  "gps_valid": true
}
```

**Fields**:
- `type`: Always `"env"`
- `temp`: Temperature (°C) as string
- `hum`: Humidity (%) as string
- `gps_lat`: GPS latitude as string (if valid)
- `gps_lng`: GPS longitude as string (if valid)
- `gps_valid`: Boolean, true if GPS has fix

**Behavior**:
- UNO receives and displays on LCD multi-screen rotation
- Enables UNO to show ALL sensor data even though DHT11 is on ESP8266

**v1.2/v1.4 Note**: DHT11 moved from UNO to ESP8266 GPIO4 to resolve pin conflict on UNO D13.

---

## Timing & Error Handling

### Timeouts

| Scenario | Timeout | Action |
|----------|---------|--------|
| ESP8266 no sensor data | 3 seconds | Publish `gh/sys/error/esp8266` with code `"uart_timeout"` |
| UNO no ACK for command | 2 seconds | Retry up to 3 times, then enter safe state |

### Retry Logic (UNO)

When ESP8266 sends a command:

1. UNO receives command
2. UNO executes command
3. UNO sends ACK
4. If ESP8266 doesn't receive ACK within 2s:
   - Retry command (up to 3 times)
   - If still no ACK: Log error, assume connection lost

### Fallback Safe State (UNO)

If UART connection lost (no data from ESP8266 for >3s):

1. UNO enters **fallback mode**
2. Check emergency conditions:
   - **Fire detected** (`flame > 800`):
     - Turn OFF pump (prevent damage)
     - Turn ON fans (ventilate)
     - Display "FIRE DETECTED" on LCD
   - **High temperature** (`temp > 40°C`):
     - Turn ON fans
     - Open window (servo)
     - Display "TEMP HIGH" on LCD
3. Otherwise: Turn off all actuators (safe state)
4. Display "UART LOST - FALLBACK" on LCD
5. Continue monitoring sensors
6. When UART reconnects: Resume normal operation

### Error Detection

| Error | Detection | Code behavior |
|-------|-----------|---------------|
| JSON parse error | `deserializeJson()` returns error | Increment error count, discard packet |
| Invalid message type | `type` field unknown | Discard packet |
| Missing required field | Field not present in JSON | Use default value or discard |
| Buffer overflow | Message too long (>512 bytes) | Discard, wait for newline |

---

## Code Examples

### UNO: Send Sensor Data

```cpp
void sendSensorData() {
  StaticJsonDocument<512> doc;
  doc["type"] = "sensor";
  doc["ts"] = millis() / 1000;

  JsonObject payload = doc.createNestedObject("payload");
  payload["temp_c"] = sensors.temp_c;
  payload["hum_pct"] = sensors.hum_pct;
  // ... other sensors

  serializeJson(doc, Serial);
  Serial.println();  // Newline delimiter
}
```

### UNO: Receive Command

```cpp
void handleUartCommand() {
  String line = Serial.readStringUntil('\n');
  line.trim();

  StaticJsonDocument<512> doc;
  DeserializationError error = deserializeJson(doc, line);

  if (error) {
    uartErrorCount++;
    return;
  }

  const char* type = doc["type"];
  if (strcmp(type, "set") == 0) {
    handleSetCommand(doc);
  }
}
```

### UNO: Send ACK

```cpp
void sendAck(long req_id, bool ok) {
  StaticJsonDocument<128> doc;
  doc["type"] = "ack";
  doc["ts"] = millis() / 1000;
  doc["req_id"] = req_id;
  doc["ok"] = ok;

  serializeJson(doc, Serial);
  Serial.println();
}
```

### ESP8266: Send Command

```cpp
void sendCommandToUno(String device, int value) {
  StaticJsonDocument<256> doc;
  doc["type"] = "set";
  doc["ts"] = millis() / 1000;
  doc["req_id"] = random(1000, 9999);

  JsonObject payload = doc.createNestedObject("payload");
  payload[device] = value;

  serializeJson(doc, Serial);
  Serial.println();
}
```

### ESP8266: Parse Sensor Data

```cpp
void handleSensorData(JsonDocument& doc) {
  JsonObject payload = doc["payload"];

  sensorData.temp_c = payload["temp_c"] | -999.0;
  sensorData.hum_pct = payload["hum_pct"] | -999.0;
  // ... other sensors

  // Publish to MQTT
  publishSensorData();
}
```

---

## Protocol Flow Diagrams

### Normal Operation

```
UNO                           ESP8266
 │                               │
 ├── sensor data (every 3s) ────→│
 │                               ├── publish to MQTT
 │                               │
 │                    ┌── MQTT command received
 │         ←── set command ──────┤
 ├── execute                     │
 ├── ack ────────────────────────→│
 │                               ├── update status
 │                               │
 ├── sensor data ───────────────→│
 │                               │
```

### Emergency Event

```
UNO                           ESP8266
 │                               │
 ├── (flame > 800)               │
 ├── event "FIRE" ───────────────→│
 │                               ├── publish gh/sys/error
 │                               ├── trigger alert
 │                               │
 ├── enter safe mode             │
 │   (fans ON, pump OFF)         │
 │                               │
```

### Connection Loss & Recovery

```
UNO                           ESP8266
 │                               │
 ├── sensor data ───────────────→│
 │                               │
 │                          ⚠️ ESP8266 crash/reboot
 │                               │
 ├── (no data for 3s)            │
 ├── enter fallback mode         │
 ├── display "UART LOST"         │
 │                               │
 │                          ✓ ESP8266 back online
 │         ←── set command ──────┤
 ├── ack ────────────────────────→│
 ├── exit fallback mode          │
 ├── resume normal operation     │
 │                               │
```

---

## JSON Size Limits

| Message Type | Typical Size | Max Size |
|--------------|--------------|----------|
| Sensor data | ~200 bytes | 512 bytes |
| Command | ~100 bytes | 256 bytes |
| ACK | ~60 bytes | 128 bytes |
| Event | ~80 bytes | 128 bytes |

**Buffer Configuration**:
- UNO: `StaticJsonDocument<512>` for sensor, `<256>` for commands
- ESP8266: `StaticJsonDocument<512>` for all

---

## Testing UART Communication

### Test 1: Loopback (v1.4)

⚠️ **Cannot test loopback directly** - Hardware Serial is used for USB programming. Test with ESP8266 connected.

### Test 2: Send from UNO, Monitor on ESP8266 (v1.4)

UNO code:
```cpp
void loop() {
  Serial.println("{\"type\":\"sensor\",\"ts\":123,\"data\":{\"light\":\"120\",\"soil\":\"45\"}}");
  delay(3000);
}
```

ESP8266 Serial Monitor (115200 baud): Should see JSON messages.

**Note**: Disconnect D0/D1 to upload UNO code, then reconnect.

### Test 3: Send from ESP8266, Monitor on UNO (v1.4)

ESP8266 code:
```cpp
void loop() {
  Serial.println("{\"type\":\"control\",\"device\":\"pump\",\"action\":\"ON\"}");
  delay(5000);
}
```

UNO Serial Monitor (115200 baud): Should see JSON commands.

**Note**: Disconnect GPIO1/GPIO3 to upload ESP8266 code, then reconnect.

### Test 4: Full Bidirectional

Upload both firmwares, monitor MQTT topics to verify data flow.

---

## Troubleshooting (v1.4)

### No data received

1. Check baud rate: **115200** on both sides (v1.4 changed from 57600)
2. Check wiring: D1 → GPIO3, GPIO1 → D0 (through level shifter)
3. Check GND common between UNO and ESP8266
4. Check level shifter voltage (should be ~3.3V at ESP8266 side)
5. **Verify D0/D1 and GPIO1/GPIO3 are connected** (not still disconnected from upload)

### Garbage characters

1. Baud rate mismatch
2. Bad wiring (noise, loose connection)
3. Missing GND common
4. Level shifter voltage wrong

### JSON parse errors

1. Check newline delimiter (`\n`)
2. Check JSON syntax (use online validator)
3. Check buffer size (may be truncated)
4. Check special characters (use `\"` for quotes inside strings)

### High latency

1. Reduce sensor publish interval (>3s)
2. Reduce JSON payload size (remove unnecessary fields)
3. Check Serial buffer (may be full)

### ACK timeout

1. UNO not sending ACK (check code)
2. ESP8266 not waiting long enough (increase timeout)
3. UART RX buffer overflow (process faster)

---

**Version**: v1.4 COMPREHENSIVE OVERHAUL
**Last updated**: 2025-11-22
