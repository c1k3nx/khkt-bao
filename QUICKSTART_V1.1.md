# Quick Start Guide v1.1

## ⚡ TL;DR

```bash
# 1. Clone repo
git clone <repo-url>
cd khkt-bao

# 2. Review architecture
cat ARCHITECTURE_V1.1.md

# 3. Wire hardware according to pin mapping
# See: docs/pinout-uno.md, docs/pinout-esp8266.md

# 4. Flash firmware (update Wi-Fi credentials first!)
# - Arduino IDE: firmware-uno/greenhouse_uno.ino
# - Arduino IDE: firmware-esp8266/greenhouse_esp8266.ino
# - Arduino IDE: firmware-esp32cam/greenhouse_esp32cam.ino

# 5. Run PC-Vision
cd pc-vision
pip install -r requirements.txt
cp .env.example .env
# Edit .env: set BROKER, CAM_URL, model paths
python vision.py

# 6. Run App
cd app-kivy
pip install -r requirements.txt
python app.py  # or main Python file
```

---

## 📋 Prerequisites

### Hardware

- [x] Arduino UNO R3
- [x] ESP8266 (NodeMCU recommended)
- [x] ESP32-CAM (AI-Thinker)
- [x] All sensors & actuators (see hardware list in README)
- [x] Breadboard + jumper wires
- [x] **Level shifters** or resistor dividers (5V↔3.3V)
- [x] Power supplies: 5V/2A (UNO), 5V/1A (ESP8266), 5V/2A (ESP32-CAM)

### Software

- [x] Arduino IDE 1.8.x or 2.x
- [x] Python 3.8+ (for PC-Vision & App)
- [x] pip (Python package manager)
- [x] MQTT broker access (default: broker.hivemq.com)

### Network

- [x] Wi-Fi 2.4GHz (ESP8266/ESP32-CAM)
- [x] Same LAN for all devices (or internet if using cloud MQTT)

---

## 🔌 Hardware Setup (30 min)

### Step 1: Wire Arduino UNO

Follow **exact pin mapping** from [`docs/pinout-uno.md`](docs/pinout-uno.md):

**Critical connections**:
- **D0/D1 (UART)**: To ESP8266 with **level shifter**
  ```
  UNO D1 (TX) ──┬── 1kΩ ──┬── ESP8266 GPIO3 (RX)
                │         │
                └─ 2kΩ ───┴── GND

  UNO D0 (RX) ────────────── ESP8266 GPIO1 (TX)  # Direct, no shifter
  ```

- **D2 (INT0)**: Flame sensor DO (digital output)
- **D3 (INT1)**: Sound sensor DO (digital output)
- **A0-A3**: Analog sensors (Flame AO, Sound AO, Soil, MQ-3)
- **A4/A5**: I²C (BH1750, LCD1602)

**Checklist**:
- [ ] All GND connected (UNO, ESP8266, sensors, actuators)
- [ ] 5V power stable (multimeter test: 4.8-5.2V)
- [ ] Level shifter for UNO TX → ESP8266 RX
- [ ] Servo power from **separate 5V** (not UNO 5V pin!)
- [ ] Relay VCC from external 5V (if optoisolated)

### Step 2: Wire ESP8266

Follow [`docs/pinout-esp8266.md`](docs/pinout-esp8266.md):

**Key connections**:
- **GPIO1/3 (UART0)**: DFPlayer Mini
  ```
  ESP8266 GPIO1 (TX) ──────── DFPlayer RX (via 1kΩ resistor)
  ESP8266 GPIO3 (RX) ──────── DFPlayer TX
  ```

- **GPIO12/14**: SoftwareSerial to UNO
- **GPIO13/15**: GPS NEO-6 (SoftwareSerial)
- **GPIO4/5**: WS2812B LED rings

**Checklist**:
- [ ] DFPlayer VCC = 5V, GND common
- [ ] GPS VCC = 3.3V (module has regulator) or 5V
- [ ] WS2812B VCC = 5V **separate**, GND common
- [ ] NodeMCU powered via USB or VIN (5V)

### Step 3: Wire ESP32-CAM

**Simple**:
- [ ] 5V + GND via FTDI or USB adapter
- [ ] GPIO0 to GND for flashing (then disconnect)

---

## 💾 Software Setup (20 min)

### Step 1: Arduino Libraries

Install via **Library Manager** (Sketch → Include Library → Manage Libraries):

- [x] DHT sensor library (Adafruit)
- [x] Adafruit BH1750
- [x] LiquidCrystal I2C
- [x] ArduinoJson (v6.x)
- [x] Servo (built-in)
- [x] Adafruit NeoPixel (for WS2812)

For ESP8266:
- [x] ESP8266WiFi (built-in after board install)
- [x] PubSubClient (MQTT)
- [x] ArduinoJson
- [x] TinyGPSPlus
- [x] DFRobotDFPlayerMini

For ESP32:
- [x] esp_camera (built-in)
- [x] WiFi (built-in)

### Step 2: Flash Firmware

#### Arduino UNO

1. Open `firmware-uno/greenhouse_uno.ino`
2. **No config needed** (pin defs already correct)
3. Select: Tools → Board → Arduino UNO
4. Select: Tools → Port → (your COM port)
5. Click **Upload**
6. Open Serial Monitor (115200 baud) → Should see sensor JSON every 2s

#### ESP8266

1. Open `firmware-esp8266/greenhouse_esp8266.ino`
2. **CRITICAL**: Edit Wi-Fi credentials
   ```cpp
   const char* WIFI_SSID = "YourWiFiSSID";
   const char* WIFI_PASS = "YourWiFiPassword";
   ```
3. Edit MQTT broker (if not using HiveMQ public):
   ```cpp
   const char* MQTT_BROKER = "broker.hivemq.com";
   const int MQTT_PORT = 1883;
   ```
4. Select: Tools → Board → NodeMCU 1.0 (ESP-12E Module)
5. Select: Tools → Upload Speed → 115200
6. Click **Upload**
7. Serial Monitor (115200 baud) → Should see Wi-Fi connect, MQTT connect

#### ESP32-CAM

1. Open `firmware-esp32cam/greenhouse_esp32cam.ino`
2. Edit Wi-Fi credentials (same as ESP8266)
3. Select: Tools → Board → AI Thinker ESP32-CAM
4. Select: Tools → Partition Scheme → Huge APP (3MB No OTA)
5. **Connect GPIO0 to GND**, press RST button
6. Click **Upload**
7. **Disconnect GPIO0**, press RST button
8. Serial Monitor (115200 baud) → Should see IP address: `http://192.168.x.x/stream`

### Step 3: PC-Vision

```bash
cd pc-vision

# Create virtual env (recommended)
python -m venv venv
source venv/bin/activate  # Linux/Mac
# venv\Scripts\activate   # Windows

# Install dependencies
pip install -r requirements.txt

# Configure
cp .env.example .env
nano .env
```

**Edit `.env`**:
```env
MQTT_BROKER=broker.hivemq.com
MQTT_PORT=1883
CAM_URL=http://192.168.1.77  # Your ESP32-CAM IP
YOLO_WEIGHTS=yolov8n.pt      # Or custom trained model
DISEASE_MODEL=linkanjarad/mobilenet_v2_1.0_224-plant-disease-identification
INTERVAL_S=5.0
```

**Run**:
```bash
python vision.py
```

Should see:
```
[INFO] Vision processor starting...
[INFO] MQTT connected
[INFO] Image fetched: (800, 600, 3)
[INFO] Detected 5 plants
[INFO] Disease detection: healthy (0.98)
```

### Step 4: App

```bash
cd app-kivy

# Install dependencies
pip install -r requirements.txt

# Run (file name may vary)
python app.py
```

**First run**:
1. Configure MQTT broker (default: broker.hivemq.com)
2. Set camera URL: `http://192.168.1.77:81/stream`
3. Click "Save & Reconnect"
4. Should see:
   - Sensor data updating
   - Camera stream (if ESP32-CAM reachable)
   - Status: MQTT Connected

---

## ✅ Verification (10 min)

### Test 1: Sensor Data Flow

**Expected**: UNO → ESP8266 → MQTT → App

**Steps**:
1. Open MQTT Explorer (http://mqtt-explorer.com/)
2. Connect to broker.hivemq.com
3. Look for topics: `greenhouse/data/*`
4. Should see temperature, humidity, soil, light updating every 2s

**Troubleshoot**:
- No topics? Check ESP8266 Serial Monitor → MQTT connected?
- No data? Check UNO Serial Monitor → sending JSON?
- Wrong values? Check sensor wiring

### Test 2: Control Flow

**Expected**: App → MQTT → ESP8266 → UNO → Actuator

**Steps**:
1. In App, switch to **MANUAL** mode
2. Click "Fan" → **ON**
3. Should see:
   - Relay click (hardware)
   - Serial Monitor: `{"type":"control","device":"fan","action":"ON"}`
   - MQTT topic: `greenhouse/status/fan` → `"ON"` (retained)
   - App UI: Fan button changes state

**Troubleshoot**:
- No relay action? Check UNO pin D7 with multimeter
- No MQTT status? Check ESP8266 MQTT connected
- No UI update? Check App subscribed to `greenhouse/status/#`

### Test 3: Emergency Interrupt

**Expected**: Hardware interrupt → Instant response

**Steps**:
1. Trigger flame sensor (lighter near sensor)
2. Should see **within 1ms**:
   - UNO: Serial `{"type":"event","event":"flame","value":"DETECTED"}`
   - ESP8266: Publish `greenhouse/event/flame` → `"DETECTED"`
   - DFPlayer: Play track 8 (fire alert)
   - App: Fire alert dialog

**Troubleshoot**:
- No interrupt? Check D2 wiring, sensor DO output
- Slow response? Check ISR attached: `attachInterrupt(digitalPinToInterrupt(2), flameISR, FALLING)`

### Test 4: Vision Detection

**Expected**: ESP32-CAM → PC-Vision → MQTT → App

**Steps**:
1. Point ESP32-CAM at plants (or test image)
2. PC-Vision should log:
   ```
   [INFO] Detected 3 plants
   [INFO] Disease detection: healthy (0.95)
   ```
3. App should show:
   - Plant count: 3
   - Health: OK

**Troubleshoot**:
- No image? Check ESP32-CAM `/capture` in browser
- No detection? Check YOLO model loaded
- No MQTT? Check PC-Vision MQTT connected

---

## 🚀 Next Steps

### Calibration

1. **Sensor calibration**:
   - Soil moisture: Dry (0%) vs wet (100%)
   - MQ-3: Clean air baseline
   - Flame: Distance threshold

2. **Threshold tuning**:
   - App → AUTO mode → Thresholds
   - Adjust: `temperatureHigh`, `soilMoistureLow`, `lightIntensityLow`

3. **Vision model**:
   - Train custom YOLO on your plants
   - Fine-tune disease model

### DFPlayer Setup

1. Format SD card: FAT32
2. Create folder: `/01/`
3. Copy MP3 files: `001.mp3`, `002.mp3`, ... `010.mp3`
   - See `docs/dfplayer-tracks.md` for TTS generation script

4. Test:
   - App → Manual mode → Play track 1
   - Should hear: "Bật chế độ thủ công"

### Production Deployment

1. **Secure MQTT**:
   - Use private broker (Mosquitto)
   - Enable TLS + authentication

2. **Persistent storage**:
   - Log sensor data to database (InfluxDB)
   - Dashboard (Grafana)

3. **Monitoring**:
   - Set up alerts (Telegram bot)
   - Watchdog service (systemd)

---

## 📚 Documentation

| Document | Purpose |
|----------|---------|
| [`README.md`](README.md) | Main overview |
| [`ARCHITECTURE_V1.1.md`](ARCHITECTURE_V1.1.md) | System design details |
| [`MIGRATION_V1.1.md`](MIGRATION_V1.1.md) | Upgrade from v1.0 |
| [`mqtt-schema.json`](mqtt-schema.json) | MQTT protocol spec |
| [`docs/pinout-uno.md`](docs/pinout-uno.md) | Arduino wiring |
| [`docs/pinout-esp8266.md`](docs/pinout-esp8266.md) | ESP8266 wiring |
| [`docs/mqtt-topics.md`](docs/mqtt-topics.md) | MQTT topics reference |
| [`docs/uart-protocol.md`](docs/uart-protocol.md) | UNO↔ESP8266 protocol |

---

## 🆘 Common Issues

### Issue: ESP8266 won't connect to Wi-Fi

**Solutions**:
- Check SSID/password (case-sensitive!)
- Wi-Fi must be 2.4GHz (ESP8266 doesn't support 5GHz)
- Try static IP instead of DHCP
- Check signal strength: `WiFi.RSSI()` → should be > -70 dBm

### Issue: UART communication errors

**Solutions**:
- Check baud rate: **115200** on both sides
- Verify level shifter: voltage at ESP8266 RX should be ~3.3V
- Check GND common between UNO & ESP8266
- Try lower baud rate: 57600 (update both firmwares)

### Issue: DFPlayer not playing

**Solutions**:
- SD card format: **FAT32** (not exFAT!)
- File naming: `01/001.mp3`, `01/002.mp3` (exact format)
- Try 16GB or smaller SD card (some modules have issues with 32GB)
- Check wiring: 1kΩ resistor on RX line

### Issue: Camera stream laggy

**Solutions**:
- Reduce framesize: `FRAMESIZE_VGA` instead of `SVGA`
- Increase JPEG quality number (lower quality, smaller size): `quality=15`
- Check Wi-Fi signal strength
- Use wired Ethernet if available (ESP32-Ethernet)

---

## ✨ Success Criteria

You know it's working when:

- ✅ Sensor values update in App every 2s
- ✅ Camera stream is smooth (>10 FPS)
- ✅ Manual controls work (relay clicks)
- ✅ AUTO mode triggers actuators based on thresholds
- ✅ Fire sensor triggers instant alert + DFPlayer
- ✅ Vision detects plants and diseases
- ✅ All MQTT topics appear in MQTT Explorer

---

**Estimated total time**: 1-2 hours (first time)

**Need help?** Check [CONTRIBUTING.md](CONTRIBUTING.md) or open an issue on GitHub.

Happy greenhоusing! 🌿🤖
