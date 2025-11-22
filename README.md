# 🌿 IoT Greenhouse Control System

Hệ thống nhà kính thông minh tích hợp đầy đủ với Arduino UNO R3, ESP8266, ESP32-CAM, Computer Vision AI (YOLO + Hugging Face), và ứng dụng điều khiển PyQt6.

## 📋 Tổng quan

Dự án này xây dựng hệ thống IoT hoàn chỉnh cho nhà kính thông minh với các tính năng:

- **Giám sát môi trường**: Nhiệt độ, độ ẩm, ánh sáng, độ ẩm đất, chất lượng không khí (MQ-3), phát hiện lửa, âm thanh
- **Điều khiển tự động**: 2 chế độ TỰ ĐỘNG/THỦ CÔNG với logic thông minh
- **Computer Vision AI**: Đếm cây (YOLO) + phát hiện bệnh lá (Hugging Face models)
- **Camera streaming**: ESP32-CAM với HTTP WebServer
- **PlantTalk**: DFPlayer Mini với thông báo âm thanh + LCD1602
- **LED effects**: 2 vòng LED WS2812B với hiệu ứng động
- **GPS tracking**: Vị trí nhà kính với NEO-6
- **App điều khiển**: Giao diện PyQt6 chuyên nghiệp với dark/light theme

## 🏗️ Kiến trúc hệ thống

```
┌─────────────────────────────────────────────────────────────────┐
│                    MQTT Broker (HiveMQ)                          │
│                  (broker.hivemq.com:1883)                        │
└────────────────────────┬────────────────────────────────────────┘
                         │
            ┌────────────┼────────────┬──────────────┐
            │            │            │              │
    ┌───────▼─────┐  ┌──▼──────┐  ┌─▼───────┐  ┌──▼──────────┐
    │   ESP8266   │  │PC-Vision│  │  App    │  │ ESP32-CAM   │
    │(MQTT Bridge)│  │ (YOLO+  │  │ (PyQt6) │  │(HTTP Server)│
    │  + WS2812   │  │   HF)   │  │         │  │             │
    │  + GPS      │  └─────────┘  └─────────┘  └─────────────┘
    └──────┬──────┘                                   │
           │ UART 57600                               │ HTTP
           │ (JSON)                                   │ (GET /stream)
    ┌──────▼──────┐                                   │
    │ Arduino UNO │◄──────────────────────────────────┘
    │   R3        │
    │ - DHT11     │
    │ - BH1750    │
    │ - JSN-SR04T │
    │ - MQ-3      │
    │ - Flame/Snd │
    │ - 4 Relays  │
    │ - 2 Servos  │
    │ - DFPlayer  │
    │ - LCD1602   │
    └─────────────┘
```

## 📦 Phần cứng

### Danh sách linh kiện

| Linh kiện | Số lượng | Ghi chú |
|-----------|----------|---------|
| Arduino UNO R3 | 1 | Board chính |
| ESP8266 (NodeMCU/WeMos) | 1 | MQTT bridge + LED |
| ESP32-CAM (AI-Thinker) | 1 | Camera streaming |
| DHT11 | 1 | Nhiệt độ + độ ẩm |
| BH1750 | 1 | Cảm biến ánh sáng (I²C) |
| JSN-SR04T | 1 | Siêu âm chống nước |
| MQ-3 | 1 | Cảm biến alcohol |
| Flame sensor (analog) | 1 | Phát hiện lửa |
| Sound sensor (analog) | 1 | Phát hiện âm thanh |
| Soil moisture sensor | 1 | Độ ẩm đất |
| GPS NEO-6 | 1 | Định vị |
| Relay 4 kênh (12V) | 1 | Pump, Fan×2, Light |
| Servo MG996R (180°) | 1 | Cửa sổ |
| Servo MG90 (180°) | 1 | Cửa chính |
| DFPlayer Mini | 1 | MP3 player |
| LCD1602 I²C | 1 | Màn hình hiển thị |
| LED Ring WS2812B (8 LEDs) | 2 | Hiệu ứng ánh sáng |

### Sơ đồ chân (Pinout)

Xem chi tiết trong:
- [`docs/pinout-uno.md`](docs/pinout-uno.md)
- [`docs/pinout-esp8266.md`](docs/pinout-esp8266.md)
- [`fritzing/`](fritzing/) - Sơ đồ Fritzing đầy đủ

## 🚀 Bắt đầu

**📖 ĐỌC TRƯỚC KHI BẮT ĐẦU:**
- [V1.4_WIRING_GUIDE.md](V1.4_WIRING_GUIDE.md) - Hướng dẫn nối mạch chi tiết
- [V1.4_USAGE_GUIDE.md](V1.4_USAGE_GUIDE.md) - Hướng dẫn sử dụng từ A-Z

### 1. Flash Firmware

#### Arduino UNO R3 (v1.4)

```bash
# Mở file trong Arduino IDE
open firmware-uno/greenhouse_uno.ino

# Cài đặt thư viện cần thiết (via Library Manager):
# - Adafruit BH1750 (light sensor)
# - LiquidCrystal I2C
# - ArduinoJson (v6.x)
# - Servo (built-in)
# - Adafruit NeoPixel
# - NewPing (ultrasonic)

# ⚠️ CRITICAL: DISCONNECT D0/D1 pins trước khi upload!
# Chọn board: Arduino UNO
# Chọn port COM tương ứng
# Upload
# ⚠️ RECONNECT D0/D1 pins sau khi upload xong!
```

#### ESP8266 (v1.4)

```bash
# Mở file trong Arduino IDE
open firmware-esp8266/greenhouse_esp8266.ino

# Cài đặt ESP8266 board support:
# File → Preferences → Additional Boards Manager URLs:
# http://arduino.esp8266.com/stable/package_esp8266com_index.json

# Cài đặt thư viện:
# - ESP8266WiFi (built-in)
# - PubSubClient
# - ArduinoJson (v6.x)
# - NeoPixelBus (thay Adafruit_NeoPixel cho ESP8266)
# - DHT sensor library (DHT11 on GPIO4)
# - TinyGPSPlus
# - LittleFS (built-in)

# **QUAN TRỌNG**: Sửa Wi-Fi credentials
# const char* WIFI_SSID = "YourWiFiSSID";
# const char* WIFI_PASS = "YourWiFiPassword";

# ⚠️ CRITICAL: DISCONNECT GPIO1/GPIO3 (TX/RX) trước khi upload!
# Chọn board: NodeMCU 1.0 (ESP-12E Module)
# Upload Speed: 115200
# Flash Size: 4MB (FS:2MB OTA:~1019KB)
# Upload
# ⚠️ RECONNECT GPIO1/GPIO3 sau khi upload xong!
```

#### ESP32-CAM

```bash
# Mở file trong Arduino IDE
open firmware-esp32cam/greenhouse_esp32cam.ino

# Cài đặt ESP32 board support:
# https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json

# **QUAN TRỌNG**: Sửa Wi-Fi credentials
# const char* WIFI_SSID = "YourWiFiSSID";
# const char* WIFI_PASS = "YourWiFiPassword";

# Chọn board: AI Thinker ESP32-CAM
# Partition Scheme: Huge APP (3MB No OTA)
# PSRAM: Enabled

# Để vào chế độ upload:
# - Nối GPIO0 xuống GND
# - Nhấn RST
# - Upload
# - Ngắt GPIO0, nhấn RST lại
```

### 2. Cài đặt PC-Vision

```bash
cd pc-vision

# Tạo virtual environment (khuyến nghị)
python -m venv venv
source venv/bin/activate  # Linux/Mac
# hoặc: venv\Scripts\activate  # Windows

# Cài đặt dependencies
pip install -r requirements.txt

# Cấu hình
cp .env.example .env
nano .env  # Sửa các tham số (MQTT broker, camera URL, model paths)

# Chạy
python vision.py
```

### 3. Chạy App điều khiển

```bash
cd app-kivy  # (Lưu ý: app thực tế dùng PyQt6, không phải Kivy)

# Cài đặt dependencies
pip install PyQt6 paho-mqtt opencv-python

# Chạy app
python app.py  # (hoặc tên file app chính)
```

## 📡 Giao thức MQTT

Hệ thống sử dụng MQTT làm giao thức chính cho giao tiếp giữa các node.

### Topics chính

```
gh/sensor/temp_c          → Nhiệt độ (°C)
gh/sensor/hum_pct         → Độ ẩm không khí (%)
gh/sensor/soil_pct        → Độ ẩm đất (%)
gh/sensor/light_lux       → Ánh sáng (lux)
gh/sensor/tank_pct        → Mực nước bồn (%)
gh/sensor/flame_do        → Lửa (0/1)
gh/sensor/sound_do        → Âm thanh (0/1)

gh/cmd/relay/<name>       ← Điều khiển relay (ON/OFF)
gh/cmd/servo/<name>       ← Điều khiển servo (OPEN/CLOSE)
gh/cmd/led/power          ← LED power (ON/OFF)
gh/cmd/led/color          ← LED color (#RRGGBB)

gh/status/relay/<name>    → Trạng thái relay (retain)
gh/status/servo/<name>    → Trạng thái servo (retain)
gh/status/led/power       → Trạng thái LED (retain)
gh/status/led/color       → Màu LED (retain)

gh/cv/detections          → Kết quả computer vision (JSON)

gh/sys/heartbeat/<node>   → Heartbeat (mỗi 30s)
gh/sys/error/<node>       → Lỗi hệ thống
```

Xem chi tiết: [`docs/mqtt-topics.md`](docs/mqtt-topics.md)

## 🔌 Giao thức UART (UNO ↔ ESP8266)

- **Baud rate**: 115200 (v1.4 Hardware Serial)
- **Connection**: Hardware Serial D0/D1 (UNO) ↔ GPIO1/3 (ESP8266)
- **Format**: JSON, newline-delimited
- **Timeout**: 3s
- **Retry**: 3 lần
- **⚠️ IMPORTANT**: Disconnect D0/D1 during UNO upload!

Ví dụ:

```json
// UNO → ESP8266 (sensor data)
{"type":"sensor","ts":1732251234,"payload":{
  "temp_c":30.4,"hum_pct":68.5,"light_lux":120,"soil_pct":41.2,
  "mq3":412,"flame":83,"sound":612,"distance_cm":128
}}

// ESP8266 → UNO (command)
{"type":"set","ts":1732251234,"req_id":101,"payload":{
  "pump":1,"fan_main":1,"fan_aux":0,"led12v":0,
  "window":1,"door":0
}}

// UNO → ESP8266 (ACK)
{"type":"ack","ts":1732251234,"req_id":101,"ok":true}
```

Xem chi tiết: [`docs/uart-protocol.md`](docs/uart-protocol.md)

## 🤖 Logic tự động

Chế độ **TỰ ĐỘNG** (AUTO) được thực thi trên **App**, với các quy tắc:

| Điều kiện | Hành động |
|-----------|-----------|
| `soil_pct < soil_low` | ✅ Pump ON → tưới nước |
| `temp_c > temp_high` | ✅ Fan ON + Window OPEN |
| `hum_pct > hum_high` | ✅ Aux Fan ON |
| `light_lux < light_low` | ✅ Light 12V ON + LED rings COLOR |
| `flame > threshold` | 🚨 Pump ON + All Fans ON + LED rings ALERT (red blink) |
| `sound > threshold` | 🔊 DFPlayer phát cảnh báo |
| `cv.diseased == true` | 🌱 DFPlayer nhắc chăm sóc cây + highlight UI |

**Fallback an toàn** (UNO):
- Nếu mất kết nối UART/MQTT > 3s:
  - Phát hiện lửa → Bật quạt, tắt bơm
  - Nhiệt độ cao → Bật quạt, mở cửa sổ
  - Còn lại → Giữ trạng thái an toàn (tắt đèn, giữ quạt nếu cần)

Xem chi tiết: [`docs/logic-auto.md`](docs/logic-auto.md)

## 🎵 DFPlayer Tracks

Danh sách tracks gợi ý (thư mục `/01` trên thẻ TF):

1. "Bật chế độ thủ công"
2. "Tắt chế độ thủ công, chuyển tự động"
3. "Đang mở quạt chính"
4. "Đang tắt quạt chính"
5. "Đang mở cửa sổ thông gió"
6. "Nhiệt độ cao, mở cửa sổ và bật quạt"
7. "Độ ẩm thấp, bơm đang chạy"
8. "Phát hiện lửa, kích hoạt dập"
9. "Âm thanh lớn, vui lòng giảm tiếng ồn"
10. "Cây số {id} có dấu hiệu bệnh, cần chăm sóc"

## 🔧 Cấu hình

### Ngưỡng mặc định (có thể chỉnh trong App)

```json
{
  "temp_high": 33.0,
  "hum_high": 85.0,
  "soil_target": 45.0,
  "soil_low": 30.0,
  "light_low": 150,
  "flame_high": 800,
  "sound_high": 700,
  "mq3_high": 500
}
```

### MQTT Broker

Mặc định: `broker.hivemq.com:1883` (public, không authentication)

Để dùng broker riêng:
- Sửa trong ESP8266 firmware
- Sửa trong PC-Vision `.env`
- Cấu hình trong App

## 📸 Camera Endpoints (ESP32-CAM)

```
GET /stream                           → MJPEG stream (multipart)
GET /capture                          → Single JPEG image
GET /led?duty=0..255                  → Control flash LED brightness
GET /control?framesize=SVGA&quality=12 → Set camera params
GET /status                           → JSON system status
```

Ví dụ:
```bash
# Live stream
http://192.168.1.77/stream

# Capture single image
curl http://192.168.1.77/capture -o image.jpg

# Set flash LED to 50%
curl "http://192.168.1.77/led?duty=128"

# Change resolution to XGA
curl "http://192.168.1.77/control?framesize=XGA&quality=10"
```

## 🧠 Computer Vision

### YOLO (Plant Detection)

- Model: YOLOv8/v11 (Ultralytics)
- Weights: `yolov8n.pt` (pretrained) hoặc custom trained
- Output: Số lượng cây + bounding boxes

**Training custom model:**

```bash
# Chuẩn bị dataset (YOLO format)
# train/images/*.jpg
# train/labels/*.txt

# Train
yolo train data=plants.yaml model=yolov8n.pt epochs=100 imgsz=640

# Sử dụng
# YOLO_WEIGHTS=runs/detect/train/weights/best.pt
```

### Disease Detection (Hugging Face)

- Model: `linkanjarad/mobilenet_v2_1.0_224-plant-disease-identification`
- Hoặc các models khác trên Hugging Face Hub:
  - `nateraw/vit-base-beans` (bean diseases)
  - `fxmarty/resnet-tiny-beans` (lightweight)

**Thay đổi model:**

Sửa trong `pc-vision/.env`:
```
DISEASE_MODEL=your-huggingface-username/model-name
```

## 🧪 Kiểm thử

### Test PC-Vision

```bash
cd pc-vision
python test_vision.py
```

Các test bao gồm:
- ✅ Image fetch từ ESP32-CAM
- ✅ YOLO plant detection
- ✅ Disease detection
- ✅ MQTT publish

### Test Hardware

1. **Test relay**: Nghe click của relay khi bật/tắt
2. **Test servo**: Kiểm tra góc quay (0° và 90°)
3. **Test DFPlayer**: Phát track thử nghiệm
4. **Test LED rings**: Kiểm tra màu sắc và hiệu ứng
5. **Test sensors**: Xem giá trị trong Serial Monitor (57600 baud)

## 📁 Cấu trúc dự án

```
khkt-bao/
├── firmware-uno/
│   └── greenhouse_uno.ino          # Arduino UNO firmware
├── firmware-esp8266/
│   └── greenhouse_esp8266.ino      # ESP8266 firmware
├── firmware-esp32cam/
│   └── greenhouse_esp32cam.ino     # ESP32-CAM firmware
├── pc-vision/
│   ├── vision.py                   # Main vision script
│   ├── test_vision.py              # Test suite
│   ├── requirements.txt            # Python dependencies
│   └── .env.example                # Config template
├── app-kivy/                       # PyQt6 App
│   └── app.py                      # Main app (tên thực tế có thể khác)
├── docs/
│   ├── pinout-uno.md               # Arduino UNO pinout
│   ├── pinout-esp8266.md           # ESP8266 pinout
│   ├── mqtt-topics.md              # MQTT topics reference
│   ├── uart-protocol.md            # UART protocol spec
│   └── logic-auto.md               # Auto control logic
├── fritzing/
│   ├── greenhouse-full.fzz         # Sơ đồ tổng thể
│   ├── uno-detail.fzz              # Chi tiết UNO
│   ├── esp8266-detail.fzz          # Chi tiết ESP8266
│   └── esp32cam-detail.fzz         # Chi tiết ESP32-CAM
└── README.md                       # This file
```

## ⚠️ Lưu ý quan trọng

### Nguồn điện

- **Arduino UNO**: 5V/2A (USB hoặc DC jack)
- **ESP8266**: 3.3V, cấp từ USB (NodeMCU có voltage regulator)
- **ESP32-CAM**: 5V/2A (cần nguồn ổn định, tránh brownout)
- **Relay**: 12V riêng cho coil (optoisolated)
- **Servo**: 5V/2A riêng (tránh nhiễu)

**Mass chung**: Tất cả GND phải nối chung!

### ⚡ Chia áp UART (v1.4 Hardware Serial)

**v1.4 sử dụng Hardware Serial D0/D1 (UNO) ↔ GPIO1/3 (ESP8266)**

UNO TX/D1 (5V) → ESP8266 RX/GPIO3 (3.3V) **CẦN chia áp**:

```
UNO D1 (TX) ──┬──── 1kΩ ────┬──── ESP8266 GPIO3 (RX)
              │             │
              └─── 2kΩ ─────┴──── GND

Vout = 5V × (2kΩ / (1kΩ + 2kΩ)) = 3.33V ✓
```

ESP8266 TX/GPIO1 (3.3V) → UNO RX/D0 (5V): **Không cần** chia áp (UNO chấp nhận 3.3V)

**⚠️ CRITICAL**: Phải disconnect D0/D1 khi upload code lên UNO! (Xem V1.4_USAGE_GUIDE.md)

### Relay Active Logic

Kiểm tra relay module:
- **Active LOW**: `RELAY_ON = LOW`, `RELAY_OFF = HIGH`
- **Active HIGH**: `RELAY_ON = HIGH`, `RELAY_OFF = LOW`

Sửa trong `firmware-uno/greenhouse_uno.ino`:
```cpp
#define RELAY_ON LOW   // hoặc HIGH
#define RELAY_OFF HIGH // hoặc LOW
```

### Diode bảo vệ

Nối **diode 1N4007** ngược song song với relay coil và servo để chống dập điện cảm khi tắt.

## 🛠️ Troubleshooting

### ESP32-CAM không stream

- Kiểm tra nguồn 5V/2A ổn định
- Thử giảm framesize xuống SVGA hoặc CIF
- Disable brownout detector (đã có trong code)
- Kiểm tra Wi-Fi signal strength

### UART không kết nối

- Kiểm tra chia áp đúng chưa
- Kiểm tra GND chung
- Kiểm tra baud rate: 57600
- Xem Serial Monitor của cả UNO và ESP8266

### MQTT disconnect liên tục

- Kiểm tra Wi-Fi stability
- Thử broker khác (ví dụ: `test.mosquitto.org`)
- Tăng `keepalive` timeout trong code
- Kiểm tra QoS settings

### YOLO không detect

- Kiểm tra model weights path đúng chưa
- Thử giảm `YOLO_CONF` threshold (ví dụ 0.15)
- Kiểm tra classes: model có hỗ trợ "plant" không?
- Thử với ảnh test có sẵn trước

### DFPlayer không phát

- Kiểm tra kết nối TX/RX đúng chưa
- Kiểm tra thẻ TF format FAT32
- File MP3 phải trong `/01/001.mp3`, `/01/002.mp3`, ...
- Volume đủ lớn (>15)

## 📚 Tài liệu tham khảo

- [Arduino Reference](https://www.arduino.cc/reference/en/)
- [ESP8266 Arduino Core](https://arduino-esp8266.readthedocs.io/)
- [ESP32-CAM Guide](https://randomnerdtutorials.com/esp32-cam-video-streaming-web-server-camera-home-assistant/)
- [MQTT Protocol](https://mqtt.org/)
- [Ultralytics YOLO](https://docs.ultralytics.com/)
- [Hugging Face Transformers](https://huggingface.co/docs/transformers/)

## 👥 Đóng góp

Contributions are welcome! Please:

1. Fork repo
2. Create feature branch
3. Commit changes
4. Push and create Pull Request

## 📄 License

MIT License - Free for educational and commercial use.

## 🙏 Credits

- Arduino, ESP8266, ESP32 communities
- Ultralytics (YOLO)
- Hugging Face (Transformers)
- Eclipse Mosquitto (MQTT broker)

---

**Dự án**: IoT Greenhouse Control System
**Version**: 1.4.0 COMPREHENSIVE OVERHAUL
**Last updated**: 2025-11-22

**Made with ❤️ for smart agriculture**
