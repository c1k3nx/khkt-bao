# MQTT Topics Reference

## Broker Configuration

| Parameter | Value |
|-----------|-------|
| Broker | `broker.hivemq.com` (public) hoặc broker riêng |
| Port | `1883` (non-TLS) hoặc `8883` (TLS) |
| Protocol | MQTT v3.1.1 / v5 |
| Authentication | None (public broker) |

## Topic Hierarchy

```
gh/                           # Root prefix (greenhouse)
├── sensor/                   # Sensor data (published by ESP8266)
│   ├── temp_c
│   ├── hum_pct
│   ├── soil_pct
│   ├── light_lux
│   ├── tank_pct
│   ├── flame_do
│   ├── sound_do
│   └── gps
├── status/                   # Device status (published by ESP8266, retained)
│   ├── relay/
│   │   ├── fan
│   │   ├── auxfan
│   │   ├── pump
│   │   └── light12v
│   ├── servo/
│   │   ├── window
│   │   └── door
│   └── led/
│       ├── power
│       └── color
├── cmd/                      # Commands (subscribed by ESP8266, not retained)
│   ├── mode
│   ├── relay/
│   │   ├── fan
│   │   ├── auxfan
│   │   ├── pump
│   │   └── light12v
│   ├── servo/
│   │   ├── window
│   │   └── door
│   ├── led/
│   │   ├── power
│   │   └── color
│   └── dfp/
│       ├── volume
│       ├── play
│       └── stop
├── cv/                       # Computer Vision (published by PC-Vision)
│   └── detections
├── server/                   # Server/App commands (subscribed by ESP8266)
│   ├── mode
│   ├── planttalk
│   ├── alerts
│   └── lcd
└── sys/                      # System monitoring
    ├── heartbeat/
    │   ├── esp8266
    │   ├── uno
    │   └── pc-vision
    └── error/
        ├── esp8266
        ├── uno
        └── pc-vision
```

## 📡 Sensor Topics (Published by ESP8266)

### Temperature

**Topic**: `gh/sensor/temp_c`

**QoS**: 0

**Retain**: No

**Payload**: String (float)

**Unit**: °C

**Example**:
```
30.5
```

---

### Humidity

**Topic**: `gh/sensor/hum_pct`

**QoS**: 0

**Retain**: No

**Payload**: String (float)

**Unit**: %

**Example**:
```
68.2
```

---

### Soil Moisture

**Topic**: `gh/sensor/soil_pct`

**QoS**: 0

**Retain**: No

**Payload**: String (float)

**Unit**: % (0=dry, 100=wet)

**Example**:
```
45.0
```

---

### Light Intensity

**Topic**: `gh/sensor/light_lux`

**QoS**: 0

**Retain**: No

**Payload**: String (float)

**Unit**: lux

**Example**:
```
320.5
```

---

### Water Tank Level

**Topic**: `gh/sensor/tank_pct`

**QoS**: 0

**Retain**: No

**Payload**: String (integer)

**Unit**: % (0=empty, 100=full)

**Example**:
```
75
```

---

### Flame Detection

**Topic**: `gh/sensor/flame_do`

**QoS**: 0 (hoặc 1 cho critical alert)

**Retain**: No

**Payload**: String ("0" hoặc "1")

**Value**: 0=no fire, 1=fire detected

**Example**:
```
1
```

---

### Sound Detection

**Topic**: `gh/sensor/sound_do`

**QoS**: 0

**Retain**: No

**Payload**: String ("0" hoặc "1")

**Value**: 0=quiet, 1=loud sound

**Example**:
```
0
```

---

### GPS Location

**Topic**: `gh/sensor/gps`

**QoS**: 0

**Retain**: No

**Payload**: JSON

**Example**:
```json
{
  "lat": 21.028511,
  "lng": 105.804817
}
```

---

## 📊 Status Topics (Published by ESP8266, Retained)

### Relay Status

**Topics**:
- `gh/status/relay/fan`
- `gh/status/relay/auxfan`
- `gh/status/relay/pump`
- `gh/status/relay/light12v`

**QoS**: 1

**Retain**: **Yes** (last known state)

**Payload**: String

**Values**: `"ON"` | `"OFF"`

**Example**:
```
ON
```

---

### Servo Status

**Topics**:
- `gh/status/servo/window`
- `gh/status/servo/door`

**QoS**: 1

**Retain**: **Yes**

**Payload**: String

**Values**: `"OPEN"` | `"CLOSE"`

**Example**:
```
OPEN
```

---

### LED Status

**Topics**:
- `gh/status/led/power`
- `gh/status/led/color`

**QoS**: 1

**Retain**: **Yes**

**Payload**: String

**Values**:
- `power`: `"ON"` | `"OFF"`
- `color`: `"#RRGGBB"` (hex color)

**Example**:
```
# Power
ON

# Color
#00FF00
```

---

## 🎛️ Command Topics (Subscribed by ESP8266)

### Mode Change

**Topic**: `gh/cmd/mode`

**QoS**: 1

**Retain**: No

**Payload**: String

**Values**: `"AUTO"` | `"MANUAL"`

**Example**:
```
AUTO
```

**Behavior**:
- ESP8266 forwards to App logic
- Publishes to `gh/state/mode` (retained)

---

### Relay Commands

**Topics**:
- `gh/cmd/relay/fan`
- `gh/cmd/relay/auxfan`
- `gh/cmd/relay/pump`
- `gh/cmd/relay/light12v`

**QoS**: 1

**Retain**: No

**Payload**: String

**Values**: `"ON"` | `"OFF"` | `"BLINK"`

**Example**:
```
ON
```

**Behavior**:
- ESP8266 forwards command to UNO via UART
- Publishes status to `gh/status/relay/<name>`

---

### Servo Commands

**Topics**:
- `gh/cmd/servo/window`
- `gh/cmd/servo/door`

**QoS**: 1

**Retain**: No

**Payload**: String

**Values**: `"OPEN"` | `"CLOSE"`

**Example**:
```
OPEN
```

**Behavior**:
- ESP8266 forwards command to UNO
- Publishes status to `gh/status/servo/<name>`

---

### LED Commands

**Topics**:
- `gh/cmd/led/power`
- `gh/cmd/led/color`

**QoS**: 1

**Retain**: No

**Payload**: String

**Values**:
- `power`: `"ON"` | `"OFF"`
- `color`: `"#RRGGBB"`

**Example**:
```
# Power
OFF

# Color
#FF5733
```

**Behavior**:
- ESP8266 controls WS2812B rings directly
- Publishes status to `gh/status/led/<name>`

---

### DFPlayer Commands

**Topics**:
- `gh/cmd/dfp/volume`
- `gh/cmd/dfp/play`
- `gh/cmd/dfp/stop`

**QoS**: 1

**Retain**: No

**Payload**:
- `volume`: String (integer 0-30)
- `play`: JSON `{"track": 1, "vol": 25}`
- `stop`: String `"STOP"`

**Example**:
```json
# Play
{"track": 5, "vol": 20}

# Volume
25

# Stop
STOP
```

**Note**: Hiện tại forward đến UNO chưa implement đầy đủ trong firmware minimal.

---

## 🖥️ Server/App Topics (Subscribed by ESP8266)

### Mode (from App)

**Topic**: `gh/server/mode`

**QoS**: 1

**Retain**: No

**Payload**: String

**Values**: `"AUTO"` | `"MANUAL"`

**Same as** `gh/cmd/mode`

---

### PlantTalk Enable

**Topic**: `gh/server/planttalk`

**QoS**: 1

**Retain**: No

**Payload**: String

**Values**: `"ON"` | `"OFF"`

**Behavior**:
- Enable/disable DFPlayer automatic announcements

---

### Alerts Config

**Topic**: `gh/server/alerts`

**QoS**: 1

**Retain**: No

**Payload**: JSON

**Example**:
```json
{
  "fire": true,
  "sound": false,
  "tankLow": true,
  "soilLow": true,
  "tempHigh": true
}
```

---

### LCD Display

**Topic**: `gh/server/lcd`

**QoS**: 1

**Retain**: No

**Payload**: JSON

**Example**:
```json
{
  "l1": "T:30.5C H:68%",
  "l2": "SOIL45% LUX320"
}
```

**Note**: ESP8266 có thể forward đến UNO nếu cần.

---

## 🤖 Computer Vision Topics (Published by PC-Vision)

### Detections

**Topic**: `gh/cv/detections`

**QoS**: 0 (hoặc 1 nếu muốn đảm bảo)

**Retain**: No

**Payload**: JSON

**Schema**:
```json
{
  "ts": 1732251234,
  "num_plants": 12,
  "diseased": true,
  "disease_label": "leaf_blight",
  "disease_score": 0.92,
  "boxes": [
    [x, y, w, h, "plant_id_1"],
    [x, y, w, h, "plant_id_2"]
  ],
  "cam_ip": "192.168.1.77"
}
```

**Fields**:
- `ts`: Unix timestamp (seconds)
- `num_plants`: Number of plants detected (integer)
- `diseased`: Boolean, true if disease detected
- `disease_label`: Disease name (string)
- `disease_score`: Confidence score (0.0-1.0)
- `boxes`: Array of bounding boxes `[x, y, width, height, plant_id]`
- `cam_ip`: ESP32-CAM IP address

**Example**:
```json
{
  "ts": 1732251234,
  "num_plants": 5,
  "diseased": false,
  "disease_label": "healthy",
  "disease_score": 0.98,
  "boxes": [
    [10, 20, 80, 120, "plant_1"],
    [150, 30, 75, 110, "plant_2"]
  ],
  "cam_ip": "192.168.1.77"
}
```

---

## 🔧 System Topics

### Heartbeat

**Topics**:
- `gh/sys/heartbeat/esp8266`
- `gh/sys/heartbeat/uno` (optional, chưa implement)
- `gh/sys/heartbeat/pc-vision` (optional, chưa implement)

**QoS**: 0

**Retain**: No

**Interval**: 30s

**Payload**: JSON

**Example**:
```json
{
  "ts": 1732251234,
  "uptime_s": 3600,
  "heap": 25600
}
```

**Fields**:
- `ts`: Unix timestamp
- `uptime_s`: Uptime in seconds
- `heap`: Free heap memory (bytes)

---

### Error Reports

**Topics**:
- `gh/sys/error/esp8266`
- `gh/sys/error/uno`
- `gh/sys/error/pc-vision`

**QoS**: 1

**Retain**: No

**Payload**: JSON

**Example**:
```json
{
  "ts": 1732251234,
  "code": "uart_timeout",
  "msg": "No data from UNO for 3s"
}
```

**Common error codes**:
- `uart_timeout`: UART communication timeout
- `uart_parse_error`: JSON parse failed
- `mqtt_connect_fail`: MQTT connection failed
- `sensor_read_error`: Sensor reading failed
- `wifi_disconnect`: Wi-Fi disconnected

---

## 📋 QoS & Retain Policy

| Topic Type | QoS | Retain | Reason |
|------------|-----|--------|--------|
| Sensor data | 0 | No | High frequency, OK to lose |
| Status | 1 | **Yes** | Last known state needed |
| Commands | 1 | No | Execute once, not persistent |
| CV detections | 0 | No | Frequent updates |
| Heartbeat | 0 | No | Periodic, OK to lose |
| Errors | 1 | No | Important but not retained |

## 🔐 Security Recommendations

### For Production

1. **Use private broker** (Mosquitto, HiveMQ, etc.)
2. **Enable authentication**:
   ```
   username: greenhouse
   password: <strong-password>
   ```
3. **Use TLS** (port 8883):
   ```
   CA certificate: ca.crt
   Client certificate: client.crt (optional)
   Client key: client.key (optional)
   ```
4. **ACL (Access Control List)**:
   ```
   # esp8266
   user esp8266
   topic readwrite gh/sensor/#
   topic readwrite gh/status/#
   topic read gh/cmd/#

   # app
   user app
   topic read gh/sensor/#
   topic read gh/status/#
   topic write gh/cmd/#

   # pc-vision
   user pc-vision
   topic write gh/cv/#
   ```

5. **Firewall**: Restrict MQTT port to local network

## 📊 MQTT Explorer Tools

### CLI Tools

```bash
# Subscribe to all topics
mosquitto_sub -h broker.hivemq.com -t "gh/#" -v

# Publish test command
mosquitto_pub -h broker.hivemq.com -t "gh/cmd/relay/fan" -m "ON"

# Monitor specific sensor
mosquitto_sub -h broker.hivemq.com -t "gh/sensor/temp_c"
```

### GUI Tools

- **MQTT Explorer** (Windows/Mac/Linux): https://mqtt-explorer.com/
- **MQTT.fx** (Java-based)
- **HiveMQ Websocket Client**: http://www.hivemq.com/demos/websocket-client/

## 🧪 Testing Topics

### Test Sensor Publishing

```python
import paho.mqtt.publish as publish

publish.single(
    "gh/sensor/temp_c",
    payload="25.5",
    hostname="broker.hivemq.com"
)
```

### Test Command Subscription

```python
import paho.mqtt.client as mqtt

def on_message(client, userdata, message):
    print(f"{message.topic}: {message.payload.decode()}")

client = mqtt.Client()
client.on_message = on_message
client.connect("broker.hivemq.com", 1883)
client.subscribe("gh/cmd/#")
client.loop_forever()
```

### Test Retained Messages

```bash
# Publish retained
mosquitto_pub -h broker.hivemq.com -t "gh/status/relay/fan" -m "ON" -r

# Subscribe (will receive retained message immediately)
mosquitto_sub -h broker.hivemq.com -t "gh/status/relay/fan"
```

## 📖 Reference

- [MQTT v3.1.1 Specification](https://docs.oasis-open.org/mqtt/mqtt/v3.1.1/mqtt-v3.1.1.html)
- [MQTT v5.0 Specification](https://docs.oasis-open.org/mqtt/mqtt/v5.0/mqtt-v5.0.html)
- [Eclipse Paho Python](https://www.eclipse.org/paho/index.php?page=clients/python/index.php)
- [PubSubClient (Arduino)](https://pubsubclient.knolleary.net/)

## 🆘 Troubleshooting

### Messages not received

1. Check QoS level (subscriber QoS must be ≥ publisher QoS)
2. Check subscription wildcard: `gh/#` vs `gh/sensor/+` vs specific topic
3. Check retain flag (old retained messages may appear)
4. Check MQTT connection active

### High latency

1. Use QoS 0 for non-critical messages
2. Reduce message frequency
3. Use smaller payloads (compress JSON)
4. Check network latency: `ping broker.hivemq.com`

### Connection drops

1. Increase keepalive interval (60-120s)
2. Check Wi-Fi signal strength
3. Use QoS 1 with clean session = false for persistence
4. Implement reconnect with exponential backoff

---

**Last updated**: 2025-01-22
