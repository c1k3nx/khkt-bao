# 🌱 HƯỚNG DẪN SỬ DỤNG - GREENHOUSE IoT GUI

## 📋 MỤC LỤC
1. [Cài đặt](#cài-đặt)
2. [Chạy ứng dụng](#chạy-ứng-dụng)
3. [Giao diện](#giao-diện)
4. [Tính năng](#tính-năng)
5. [Troubleshooting](#troubleshooting)

---

## 🚀 CÀI ĐẶT

### Bước 1: Cài Python (nếu chưa có)

```bash
# Ubuntu/Debian
sudo apt update
sudo apt install python3 python3-pip python3-venv

# macOS (với Homebrew)
brew install python3

# Windows
# Download từ: https://www.python.org/downloads/
```

### Bước 2: Cài đặt dependencies

```bash
cd pc-vision

# Tạo virtual environment (khuyến nghị)
python3 -m venv venv
source venv/bin/activate  # Linux/macOS
# venv\Scripts\activate   # Windows

# Cài đặt packages
pip install -r requirements.txt
```

### Bước 3: (Optional) Download YOLO model

```bash
# Tải YOLOv8 nano (nhỏ gọn, nhanh)
python -c "from ultralytics import YOLO; YOLO('yolov8n.pt')"
```

---

## ▶️ CHẠY ỨNG DỤNG

### Cách 1: Sử dụng launcher script (khuyến nghị)

```bash
cd pc-vision
chmod +x run_gui.sh
./run_gui.sh
```

### Cách 2: Chạy trực tiếp

```bash
cd pc-vision
python greenhouse_gui.py
```

### Cách 3: Chạy từ virtual environment

```bash
cd pc-vision
source venv/bin/activate
python greenhouse_gui.py
```

---

## 🖥️ GIAO DIỆN

### Tab 1: 📊 DASHBOARD

Hiển thị tất cả sensor realtime:

```
┌──────────────────────────────────────────────────────┐
│ 🌡️ Temperature │ 💧 Humidity │ 🌱 Soil  │ 💡 Light  │
│     25.3°C     │    65.2%    │  45.8%   │ 12500 lux │
│   14:30:25     │   14:30:25  │ 14:30:25 │ 14:30:25  │
├──────────────────────────────────────────────────────┤
│ 💨 Gas MQ-3    │ 🔥 Flame    │ 🔊 Sound │ 💧 Tank   │
│     1.23V      │     120     │   450    │  15.2 cm  │
│   14:30:25     │   14:30:25  │ 14:30:25 │ 14:30:25  │
└──────────────────────────────────────────────────────┘
```

**Tính năng:**
- Auto-update mỗi giây
- Hiển thị giá trị + đơn vị + timestamp
- Color-coded warnings (sẽ có trong tương lai)

### Tab 2: 🎛️ CONTROL

Điều khiển tất cả thiết bị:

**Mode Control:**
```
🎯 Control Mode
┌──────────────────────────┐
│ [ AUTO Mode ]            │ ← Click để chuyển sang AUTO
│ [ MANUAL Mode ]          │ ← Click để chuyển sang MANUAL
└──────────────────────────┘
```

**Device Controls:**
```
💧 Water Pump     🌀 Fan          🌀 Aux Fan
┌──────────────┐ ┌──────────────┐ ┌──────────────┐
│ Status: ON   │ │ Status: OFF  │ │ Status: OFF  │
│ [ON]  [OFF]  │ │ [ON]  [OFF]  │ │ [ON]  [OFF]  │
└──────────────┘ └──────────────┘ └──────────────┘

💡 Grow Light     🪟 Window        🚪 Door
┌──────────────┐ ┌──────────────┐ ┌──────────────┐
│ Status: OFF  │ │ Status: CLOSE│ │ Status: CLOSE│
│ [ON]  [OFF]  │ │ [ON]  [OFF]  │ │ [ON]  [OFF]  │
└──────────────┘ └──────────────┘ └──────────────┘
```

**RGB LED Control:**
```
🌈 RGB LED Control
┌──────────────────────────────────┐
│ Color: [#00FF00] [Set] [OFF]     │
└──────────────────────────────────┘
```

### Tab 3: 📷 CAMERA

Xem live stream từ ESP32-CAM:

```
┌────────────────────────────────────────┐
│                                        │
│                                        │
│         [CAMERA FEED]                  │
│       with YOLO overlay                │
│                                        │
│                                        │
└────────────────────────────────────────┘
┌──────────────────────────────────────┐
│ [📷 Capture] ☑ YOLO Detection        │
└──────────────────────────────────────┘
```

**Tính năng:**
- Live stream 10 FPS
- YOLO plant detection (bounding boxes)
- Capture & save images
- Toggle YOLO on/off

### Tab 4: 📈 CHARTS

Xem lịch sử sensors qua graphs:

```
Temperature (°C)              Humidity (%)
┌─────────────────────┐      ┌─────────────────────┐
│   30°─┐             │      │  80%─┐              │
│       │   ╱─╲       │      │      │      ╱─╲     │
│   25°─┼──╱   ╲──    │      │  65%─┼─────╱   ╲──  │
│       │           ╲ │      │      │              │
│   20°─┘             │      │  50%─┘              │
└─────────────────────┘      └─────────────────────┘

Soil Moisture (%)            Light Intensity (lux)
┌─────────────────────┐      ┌─────────────────────┐
│   60%─┐             │      │ 15k──┐              │
│       │      ╱──╲   │      │      │  ╱─╲         │
│   45%─┼─────╱    ╲  │      │ 12k──┼─╱   ╲────╲   │
│       │            ╲│      │      │            ╲ │
│   30%─┘             │      │ 10k──┘             │
└─────────────────────┘      └─────────────────────┘
```

**Tính năng:**
- Realtime graphs
- Auto-scrolling (100 samples)
- Color-coded lines
- Grid & labels

### Tab 5: 🚨 ALERTS

Xem log của tất cả events:

```
🚨 System Alerts & Events
┌──────────────────────────────────────────────────────┐
│ [2024-11-23 14:30:45] 🔥 FIRE DETECTED! Value: 850  │
│ [2024-11-23 14:25:12] 🔊 LOUD SOUND! Value: 750     │
│ [2024-11-23 14:20:30] ℹ️  Mode changed to AUTO       │
│ [2024-11-23 14:15:00] ℹ️  Pump turned ON             │
│                                                      │
│                                                      │
└──────────────────────────────────────────────────────┘
┌──────────────────────────────────────────────────────┐
│                   [Clear Alerts]                     │
└──────────────────────────────────────────────────────┘
```

**Tính năng:**
- Color-coded alerts (red, orange, blue)
- Timestamps
- Popup notifications cho critical events
- Clear log button

---

## ✨ TÍNH NĂNG CHI TIẾT

### 1. AUTO Mode
- Hệ thống tự động điều khiển thiết bị dựa trên thresholds
- Pump ON khi soil < 30%
- Fan ON khi temp > 35°C
- All devices managed by ESP8266

### 2. MANUAL Mode
- Bạn điều khiển thủ công từ GUI
- Click ON/OFF buttons
- Changes instant via MQTT

### 3. YOLO Detection
- Detect số lượng cây
- Vẽ bounding boxes
- Realtime trên camera feed
- Toggle on/off để tiết kiệm CPU

### 4. RGB LED Control
- Nhập hex color (#RRGGBB)
- Ví dụ:
  - `#FF0000` - Red
  - `#00FF00` - Green
  - `#0000FF` - Blue
  - `#FFFF00` - Yellow
  - `#FF00FF` - Magenta
- Click "Set Color" để apply
- Click "OFF" để tắt

### 5. Alerts System
- 🔥 Fire alerts → Red popup
- 🔊 Sound alerts → Orange log
- ❌ Errors → Red popup
- ℹ️  Info → Blue log

---

## ⚙️ CẤU HÌNH

### Thay đổi MQTT Broker

Mở file `greenhouse_gui.py`, tìm class `Config`:

```python
class Config:
    # MQTT Settings
    MQTT_BROKER = "broker.hivemq.com"  # ← Thay đổi địa chỉ
    MQTT_PORT = 1883                   # ← Thay đổi port nếu cần
```

### Thay đổi Camera URL

```python
class Config:
    # ESP32-CAM Settings
    CAM_URL = "http://192.168.4.1"  # ← Thay đổi IP của camera
```

### Thay đổi Update Intervals

```python
class Config:
    # Update intervals (ms)
    CAMERA_UPDATE_MS = 100    # Camera FPS = 10
    SENSOR_UPDATE_MS = 1000   # Sensors mỗi 1s
    CHART_UPDATE_MS = 2000    # Charts mỗi 2s
```

---

## 🔧 TROUBLESHOOTING

### ❌ Camera không hiển thị

**Vấn đề:** Màn hình camera hiển thị "Camera Offline"

**Giải pháp:**
1. Kiểm tra ESP32-CAM đã bật chưa
2. Kiểm tra IP address đúng chưa:
   ```bash
   ping 192.168.4.1  # Thay IP của bạn
   ```
3. Test camera với browser:
   ```
   http://192.168.4.1/capture
   ```
4. Kiểm tra firewall
5. Thử tắt YOLO detection (check box)

### ❌ MQTT không kết nối

**Vấn đề:** Status bar hiển thị "MQTT: ✗ Disconnected"

**Giải pháp:**
1. Kiểm tra broker address:
   ```bash
   ping broker.hivemq.com
   ```
2. Test với MQTT Explorer hoặc mosquitto_sub:
   ```bash
   mosquitto_sub -h broker.hivemq.com -t "greenhouse/#"
   ```
3. Kiểm tra firewall port 1883
4. Thử broker khác (test.mosquitto.org)

### ❌ Sensor data không update

**Vấn đề:** Dashboard hiển thị "--" hoặc giá trị cũ

**Giải pháp:**
1. Kiểm tra MQTT đã kết nối chưa (xem tab Alerts)
2. Kiểm tra ESP8266 đang publish data:
   ```bash
   mosquitto_sub -h broker.hivemq.com -t "greenhouse/data/#"
   ```
3. Kiểm tra topic names trong firmware
4. Xem console logs (terminal output)

### ❌ Charts không vẽ

**Vấn đề:** Charts trống hoặc không update

**Giải pháp:**
1. Đợi ít nhất 2-3 giây cho data
2. Kiểm tra sensor data có đúng format không
3. Xem console logs cho errors
4. Restart ứng dụng

### ❌ YOLO không hoạt động

**Vấn đề:** "YOLO not available" hoặc detection không chạy

**Giải pháp:**
1. Cài ultralytics:
   ```bash
   pip install ultralytics
   ```
2. Download model:
   ```bash
   yolo task=detect mode=predict model=yolov8n.pt source='https://ultralytics.com/images/bus.jpg'
   ```
3. Kiểm tra model file tồn tại:
   ```bash
   ls -la yolov8n.pt
   ```
4. Thử tắt YOLO (uncheck box) nếu chỉ cần camera

### ❌ Ứng dụng chạy chậm

**Vấn đề:** GUI lag hoặc freeze

**Giải pháp:**
1. Tắt YOLO detection (CPU usage cao)
2. Giảm camera FPS:
   ```python
   CAMERA_UPDATE_MS = 200  # Từ 100 → 200 (5 FPS)
   ```
3. Tăng chart update interval:
   ```python
   CHART_UPDATE_MS = 5000  # Từ 2000 → 5000
   ```
4. Đóng các ứng dụng khác
5. Sử dụng YOLOv8n thay vì v8s/v8m

### ❌ Import errors

**Vấn đề:** `ModuleNotFoundError: No module named 'PyQt6'`

**Giải pháp:**
```bash
# Cài lại dependencies
pip install --upgrade -r requirements.txt

# Hoặc cài thủ công
pip install PyQt6 paho-mqtt opencv-python numpy pyqtgraph
```

---

## 📝 TIPS & TRICKS

### 1. Chạy nhanh hơn
- Tắt YOLO detection khi không cần
- Giảm FPS camera xuống 5 FPS
- Tăng chart interval lên 5s

### 2. Tiết kiệm bandwidth
- Giảm camera resolution trên ESP32-CAM
- Giảm FPS xuống 5 FPS
- Tắt YOLO

### 3. Debug
- Mở terminal để xem logs
- Check console cho errors
- Use MQTT Explorer để monitor topics

### 4. Shortcuts
- `Ctrl+C` trong terminal để stop
- Đóng window để exit gracefully
- Click refresh camera để reset stream

---

## 🎓 VIDEO HƯỚNG DẪN

*(Coming soon - sẽ cập nhật link YouTube)*

---

## 📞 HỖ TRỢ

### Gặp vấn đề?
1. Đọc [TROUBLESHOOTING](#troubleshooting)
2. Kiểm tra console logs
3. Tạo GitHub Issue: [khkt-bao/issues](https://github.com/c1k3nx/khkt-bao/issues)
4. Email: [your-email]

### Muốn đóng góp?
- Fork repo
- Tạo feature branch
- Submit Pull Request

---

## 📚 TÀI LIỆU THAM KHẢO

- [PyQt6 Documentation](https://www.riverbankcomputing.com/static/Docs/PyQt6/)
- [MQTT Protocol](https://mqtt.org/)
- [YOLOv8 Docs](https://docs.ultralytics.com/)
- [OpenCV Python](https://docs.opencv.org/4.x/d6/d00/tutorial_py_root.html)

---

**Made with ❤️ for v1.4 COMPREHENSIVE OVERHAUL**

🌱 Happy Farming! 🚀
