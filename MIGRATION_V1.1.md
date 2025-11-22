# Migration Guide v1.0 → v1.1

## Tổng quan thay đổi

Version 1.1 giải quyết các vấn đề pin mapping conflict và chuẩn hóa MQTT schema.

### Thay đổi chính

1. **DFPlayer Mini chuyển từ UNO → ESP8266** (giải quyết conflict chân)
2. **MQTT topics chuẩn hóa** theo `mqtt-schema.json`
3. **Flame/Sound sensors dùng interrupts** (D2 INT0, D3 INT1)
4. **Pin mapping tối ưu** trên UNO

---

## Pin Mapping Changes

### Arduino UNO R3 (NEW)

| Pin | Old Function | New Function | Notes |
|-----|--------------|--------------|-------|
| D0 | UART RX (ESP) | UART RX (ESP) | ✓ No change |
| D1 | UART TX (ESP) | UART TX (ESP) | ✓ No change |
| D2 | DHT11 | **Flame INT** (INT0) | 🔄 Changed |
| D3 | JSN Echo | **Sound INT** (INT1) | 🔄 Changed |
| D4 | Relay Pump | Relay Pump | ✓ No change |
| D5 | Servo Door | Relay AuxFan | 🔄 Changed |
| D6 | - | Relay LED 12V | 🔄 New |
| D7 | Relay Fan | Relay Fan | ✓ No change |
| D8 | Relay Light | JSN Trig | 🔄 Changed |
| D9 | Servo Window | Servo Window (MG996R) | ✓ No change |
| D10 | DFPlayer RX | Servo Door (MG90) | 🔄 Changed |
| D11 | DFPlayer TX | WS2812 (2 rings) | 🔄 Changed |
| D12 | JSN Trig | JSN Echo | 🔄 Changed |
| D13 | Relay AuxFan | **DHT11** | 🔄 Changed |
| A0 | Flame analog | Flame analog | ✓ No change |
| A1 | Sound analog | Sound analog | ✓ No change |
| A2 | Soil | Soil | ✓ No change |
| A3 | MQ-3 | MQ-3 | ✓ No change |
| A4 | I²C SDA | I²C SDA (BH1750, LCD) | ✓ No change |
| A5 | I²C SCL | I²C SCL (BH1750, LCD) | ✓ No change |

**Rationale**:
- D2/D3 dành cho **hardware interrupts** (fire/sound emergency)
- D10/D11 giải phóng (không cần DFPlayer SoftwareSerial)
- D11 dùng cho WS2812 (NeoPixel library ổn định)
- DHT11 chuyển D13 (digital read OK, tránh conflict LED on-board bằng cách disconnect jumper nếu cần)

### ESP8266 (NEW)

| Pin | Old Function | New Function | Notes |
|-----|--------------|--------------|-------|
| GPIO1 (TX0) | UART to UNO | **DFPlayer TX** | 🔄 Changed |
| GPIO3 (RX0) | UART to UNO | **DFPlayer RX** | 🔄 Changed |
| GPIO2 (D4) | LED Ring 1 | **DHT11** (optional backup) | 🔄 Changed |
| GPIO5 (D1) | LED Ring 2 | LED Ring 2 | ✓ Keep |
| GPIO12 (D6) | GPS RX | **UART to UNO RX** (SoftwareSerial) | 🔄 New |
| GPIO14 (D5) | GPS TX | **UART to UNO TX** (SoftwareSerial) | 🔄 New |
| GPIO4 (D2) | - | LED Ring 1 | 🔄 New |
| GPIO13 (D7) | - | GPS RX | 🔄 New |
| GPIO15 (D8) | - | GPS TX | 🔄 New |

**Rationale**:
- **UART0 (GPIO1/3) dành cho DFPlayer** (phần cứng UART ổn định)
- **UNO↔ESP8266 dùng SoftwareSerial** (GPIO12/14) - throughput đủ cho command/status
- GPS chuyển GPIO13/15 (SoftwareSerial thứ 2)

---

## MQTT Topics Changes

**OLD topics** (v1.0):
```
gh/sensor/temp_c
gh/sensor/hum_pct
gh/cmd/relay/fan
gh/status/relay/fan
```

**NEW topics** (v1.1):
```
greenhouse/data/temperature
greenhouse/data/humidity
greenhouse/control/fan
greenhouse/status/fan
```

### Mapping Table

| Old Topic | New Topic | Notes |
|-----------|-----------|-------|
| `gh/sensor/temp_c` | `greenhouse/data/temperature` | String số, đơn vị °C |
| `gh/sensor/hum_pct` | `greenhouse/data/humidity` | String số, đơn vị %RH |
| `gh/sensor/soil_pct` | `greenhouse/data/soilMoisture` | String số, đơn vị % |
| `gh/sensor/light_lux` | `greenhouse/data/lightIntensity` | String integer, đơn vị lux |
| `gh/sensor/flame_do` | `greenhouse/event/flame` | Event-based: `DETECTED\|NORMAL` |
| `gh/sensor/sound_do` | `greenhouse/event/sound` | Event-based: `LOUD\|NORMAL` |
| `gh/cmd/relay/fan` | `greenhouse/control/fan` | `ON\|OFF` |
| `gh/cmd/relay/pump` | `greenhouse/control/pump` | `ON\|OFF` |
| `gh/cmd/servo/window` | `greenhouse/control/window1` | `OPEN\|CLOSE` |
| `gh/cmd/servo/door` | `greenhouse/control/mainDoor` | `OPEN\|CLOSE` |
| `gh/status/relay/fan` | `greenhouse/status/fan` | `ON\|OFF`, retained |
| `gh/cmd/mode` | `greenhouse/control/mode` | `AUTO\|MANUAL` |
| `gh/state/mode` | `greenhouse/status/mode` | `AUTO\|MANUAL`, retained |
| `gh/cv/detections` | `greenhouse/cam/detections` | JSON object |

**See `mqtt-schema.json` for complete reference.**

---

## Code Changes Required

### 1. Arduino UNO Firmware

**File**: `firmware-uno/greenhouse_uno.ino`

#### Remove

```cpp
// OLD: DFPlayer
#include <SoftwareSerial.h>
SoftwareSerial dfSerial(10, 11);  // REMOVE
```

#### Add

```cpp
// NEW: Interrupts for emergency detection
volatile bool flameDetected = false;
volatile bool soundLoud = false;

void setup() {
  // ...existing setup...

  // Attach interrupts
  pinMode(2, INPUT_PULLUP);  // Flame INT
  pinMode(3, INPUT_PULLUP);  // Sound INT
  attachInterrupt(digitalPinToInterrupt(2), flameISR, FALLING);
  attachInterrupt(digitalPinToInterrupt(3), soundISR, FALLING);

  // DHT11 on D13
  dht.begin(13);  // Update DHT pin
}

void flameISR() {
  flameDetected = true;
}

void soundISR() {
  soundLoud = true;
}

void loop() {
  // Check ISR flags
  if (flameDetected) {
    flameDetected = false;
    sendEvent("FLAME", "DETECTED");
    emergencySafety();  // Turn off pump, turn on fans
  }

  if (soundLoud) {
    soundLoud = false;
    sendEvent("SOUND", "LOUD");
  }

  // ...rest of loop...
}
```

#### Update UART Messages

```cpp
// NEW: Use standardized topic names
void sendSensorData() {
  StaticJsonDocument<512> doc;
  doc["type"] = "sensor";
  doc["data"]["temperature"] = String(sensors.temp_c, 1);
  doc["data"]["humidity"] = String(sensors.hum_pct, 1);
  doc["data"]["soilMoisture"] = String(sensors.soil_pct, 1);
  // ... etc

  serializeJson(doc, Serial);
  Serial.println();
}
```

### 2. ESP8266 Firmware

**File**: `firmware-esp8266/greenhouse_esp8266.ino`

#### Add DFPlayer

```cpp
#include <DFRobotDFPlayerMini.h>

DFRobotDFPlayerMini dfPlayer;

void setup() {
  // UART0 for DFPlayer
  Serial.begin(9600);  // DFPlayer baudrate

  if (!dfPlayer.begin(Serial)) {
    publishError("DFPlayer init failed");
  }

  dfPlayer.volume(25);  // 0-30

  // SoftwareSerial for UNO
  SoftwareSerial unoSerial(12, 14);  // RX, TX (GPIO12, GPIO14)
  unoSerial.begin(115200);

  // GPS on GPIO13/15
  SoftwareSerial gpsSerial(13, 15);
  gpsSerial.begin(9600);

  // ... rest of setup ...
}
```

#### Update MQTT Topics

```cpp
// NEW: Subscribe to standardized topics
void connectMqtt() {
  // ... connect ...

  mqtt.subscribe("greenhouse/control/#");
  mqtt.subscribe("greenhouse/set/thresholds/#");
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String topicStr = String(topic);
  String payloadStr = String((char*)payload).substring(0, length);

  // NEW: Handle greenhouse/control/* topics
  if (topicStr.startsWith("greenhouse/control/")) {
    String device = topicStr.substring(19);  // After "greenhouse/control/"
    handleControl(device, payloadStr);
  }
}

void handleControl(String device, String cmd) {
  if (device == "fan") {
    sendToUno("fan", cmd == "ON" ? 1 : 0);
    mqtt.publish("greenhouse/status/fan", cmd.c_str(), true);  // Retained
  }
  // ... similar for other devices ...
}
```

#### Add DFPlayer Control

```cpp
void handleDFPlayerCommand(String cmd) {
  if (cmd == "PLAY_FIRE") {
    dfPlayer.play(8);  // Track 8: Fire alert
  } else if (cmd == "PLAY_SOIL_LOW") {
    dfPlayer.play(7);  // Track 7: Soil low
  }
  // ... etc
}
```

### 3. PC-Vision

**File**: `pc-vision/vision.py`

```python
# NEW: Publish to standardized topics
def publish_detections(result):
    payload = {
        "ts": int(time.time()),
        "plant_count": result['num_plants'],
        "unhealthy_count": result['diseased_count'],
        "bboxes": result['boxes'],
        "model_ver": "yolov8n-plants-v1.0"
    }

    mqtt_client.publish("greenhouse/cam/detections", json.dumps(payload))

    if result['diseased']:
        health_payload = {
            "plant_id": result['diseased_plant_id'],
            "suspected": result['disease_label']
        }
        mqtt_client.publish("greenhouse/cam/health", json.dumps(health_payload))
    else:
        mqtt_client.publish("greenhouse/cam/health", "OK")
```

### 4. App (PyQt6)

**File**: `app-kivy/main.py` (hoặc file chính của app)

```python
# NEW: Subscribe to standardized topics
def setup_mqtt(self):
    self.mqtt_client.subscribe("greenhouse/data/#")
    self.mqtt_client.subscribe("greenhouse/status/#")
    self.mqtt_client.subscribe("greenhouse/event/#")
    self.mqtt_client.subscribe("greenhouse/cam/#")

def on_mqtt_message(self, client, userdata, message):
    topic = message.topic
    payload = message.payload.decode()

    # NEW: Handle greenhouse/data/* topics
    if topic.startswith("greenhouse/data/"):
        sensor = topic[16:]  # After "greenhouse/data/"
        self.update_sensor_display(sensor, payload)

    # NEW: Handle greenhouse/event/* topics
    elif topic.startswith("greenhouse/event/"):
        event = topic[17:]
        if event == "flame" and payload == "DETECTED":
            self.show_fire_alert()
        elif event == "sound" and payload == "LOUD":
            self.show_sound_alert()
```

---

## Migration Checklist

### Hardware

- [ ] **Rewire UNO pins** theo pin mapping mới
- [ ] **Move DFPlayer** từ UNO sang ESP8266
  - [ ] Connect DFPlayer RX → ESP8266 TX0 (GPIO1)
  - [ ] Connect DFPlayer TX → ESP8266 RX0 (GPIO3) qua 1kΩ resistor
- [ ] **Add interrupt wiring**
  - [ ] Flame sensor DO → UNO D2
  - [ ] Sound sensor DO → UNO D3
- [ ] **Move DHT11** từ D2 → D13
- [ ] **Move JSN-SR04T** Trig từ D12 → D8, Echo từ D3 → D12
- [ ] **Move Servos** Door từ D5 → D10
- [ ] **Move WS2812** từ D2 → D11
- [ ] **Test all connections** với multimeter

### Software

- [ ] **Update firmware UNO**
  - [ ] Remove DFPlayer code
  - [ ] Add interrupt handlers
  - [ ] Update pin definitions
  - [ ] Update UART message format
- [ ] **Update firmware ESP8266**
  - [ ] Add DFPlayer initialization
  - [ ] Update MQTT topic subscriptions
  - [ ] Update MQTT topic publications
  - [ ] Add SoftwareSerial for UNO
- [ ] **Update PC-Vision**
  - [ ] Update MQTT topics
  - [ ] Validate payload against schema
- [ ] **Update App**
  - [ ] Update MQTT subscriptions
  - [ ] Update MQTT publications
  - [ ] Update UI labels (if needed)

### Testing

- [ ] **Test UART UNO↔ESP8266**
  - [ ] Send command from ESP → UNO
  - [ ] Receive sensor data UNO → ESP
- [ ] **Test DFPlayer on ESP8266**
  - [ ] Play test track
  - [ ] Verify audio output
- [ ] **Test interrupts**
  - [ ] Trigger flame sensor
  - [ ] Trigger sound sensor
- [ ] **Test MQTT**
  - [ ] Publish sensor data
  - [ ] Subscribe to control commands
  - [ ] Verify retained status messages
- [ ] **Test full flow**
  - [ ] AUTO mode triggers actuators
  - [ ] MANUAL mode controls devices
  - [ ] Vision detections appear in app

---

## Rollback Plan

Nếu có vấn đề, quay lại v1.0:

1. Git checkout v1.0 firmware:
   ```bash
   git checkout v1.0 firmware-uno/ firmware-esp8266/
   ```

2. Restore old wiring theo `docs/pinout-uno.md` v1.0

3. App/PC-Vision có thể dùng "topic adapter" tạm:
   ```python
   # Adapter for backward compatibility
   def subscribe_v1_topics(client):
       client.message_callback_add("gh/#", lambda c, u, m:
           forward_to_v1_1(m.topic.replace("gh/", "greenhouse/"), m.payload)
       )
   ```

---

## Benefits of v1.1

✅ **Ổn định hơn**: DFPlayer dùng hardware UART (ESP8266)
✅ **Responsive hơn**: Fire/Sound dùng interrupts (không miss event)
✅ **Chuẩn hóa**: MQTT schema rõ ràng, validate được
✅ **Mở rộng dễ**: Pin mapping linh hoạt hơn cho thêm sensors

---

## Support

Nếu gặp vấn đề khi migrate:

1. Kiểm tra `mqtt-schema.json` để ensure topic names đúng
2. Xem logs từ Serial Monitor (UNO 115200, ESP8266 9600)
3. Dùng MQTT Explorer để debug topic/payload
4. Open issue trên GitHub với logs chi tiết

---

**Migration date**: 2025-11-23
**Estimated time**: 2-3 hours (hardware + software)
