# 🌱 Greenhouse IoT - PyQt6 GUI Application

Giao diện đồ họa hoàn chỉnh để giám sát và điều khiển hệ thống Greenhouse IoT.

## ✨ Tính năng

### 📊 Dashboard
- **Realtime monitoring** tất cả sensors:
  - 🌡️ Temperature (°C)
  - 💧 Humidity (%)
  - 🌱 Soil Moisture (%)
  - 💡 Light Intensity (lux)
  - 💨 Gas MQ-3 (V)
  - 🔥 Flame sensor
  - 🔊 Sound level
  - 💧 Water tank level

### 🎛️ Control Panel
- **AUTO/MANUAL Mode** switching
- **Device controls**:
  - 💧 Water Pump (ON/OFF)
  - 🌀 Fan & Aux Fan
  - 💡 Grow Light
  - 🪟 Window servo
  - 🚪 Door servo
  - 🌈 RGB LED với color picker (#RRGGBB)

### 📷 Camera View
- **Live stream** từ ESP32-CAM
- **YOLO plant detection** overlay (realtime)
- **Capture** & save images
- Toggle YOLO detection on/off

### 📈 Charts
- **Realtime graphs** cho:
  - Temperature history
  - Humidity history
  - Soil moisture history
  - Light intensity history
- **Auto-scrolling** với 100 samples buffer

### 🚨 Alerts
- **Event log** với timestamps
- **Color-coded alerts**:
  - 🔥 Fire detection (red)
  - 🔊 Loud sound (orange)
  - ❌ Errors (red)
  - ℹ️ Info (blue)
- **Popup notifications** cho critical events
- Clear log button

## 🚀 Cài đặt

### 1. Install Python dependencies

```bash
cd pc-vision

# Cài đặt tất cả dependencies
pip install -r requirements.txt

# HOẶC cài đặt minimal (không có YOLO/AI)
pip install PyQt6 paho-mqtt opencv-python numpy pyqtgraph requests
```

### 2. (Optional) Download YOLO model

```bash
# YOLOv8 nano - lightweight
python -c "from ultralytics import YOLO; YOLO('yolov8n.pt')"

# HOẶC YOLOv8 small - more accurate
python -c "from ultralytics import YOLO; YOLO('yolov8s.pt')"
```

### 3. Configure settings

Mở file `greenhouse_gui.py` và chỉnh sửa class `Config`:

```python
class Config:
    # MQTT Settings
    MQTT_BROKER = "broker.hivemq.com"  # Thay đổi nếu dùng broker khác
    MQTT_PORT = 1883

    # ESP32-CAM Settings
    CAM_URL = "http://192.168.4.1"  # IP của ESP32-CAM

    # YOLO Settings
    YOLO_WEIGHTS = "yolov8n.pt"  # yolov8n.pt, yolov8s.pt, yolov8m.pt
    YOLO_CONF = 0.25  # Confidence threshold
```

## 🎮 Chạy ứng dụng

```bash
python greenhouse_gui.py
```

## 📋 Yêu cầu hệ thống

### Minimal (không AI)
- **Python**: 3.8+
- **RAM**: 512 MB
- **CPU**: Dual-core
- **Dependencies**: PyQt6, paho-mqtt, opencv-python

### Full (với YOLO)
- **Python**: 3.8+
- **RAM**: 4 GB (8 GB recommended)
- **CPU**: Quad-core (GPU recommended cho YOLO)
- **GPU** (optional): CUDA-compatible cho fast YOLO inference

## 🖼️ Screenshots

### Dashboard Tab
```
┌─────────────────────────────────────────────────────────────┐
│  🌡️ Temperature  │  💧 Humidity  │  🌱 Soil  │  💡 Light   │
│      25.3°C      │      65.2%    │   45.8%   │   12500 lux │
├─────────────────────────────────────────────────────────────┤
│  💨 Gas MQ-3     │  🔥 Flame     │  🔊 Sound │  💧 Tank    │
│      1.23V       │      120      │    450    │   15.2 cm   │
└─────────────────────────────────────────────────────────────┘
```

### Control Tab
```
┌─────────────────────────────────────────────┐
│  🎯 Control Mode                            │
│  [ AUTO Mode ]  [ MANUAL Mode ]             │
├─────────────────────────────────────────────┤
│  💧 Pump    │  🌀 Fan      │  🌀 AuxFan    │
│  [ON] [OFF] │  [ON] [OFF]  │  [ON] [OFF]   │
│                                             │
│  💡 Light   │  🪟 Window   │  🚪 Door      │
│  [ON] [OFF] │  [ON] [OFF]  │  [ON] [OFF]   │
├─────────────────────────────────────────────┤
│  🌈 RGB LED Control                         │
│  Color: [#00FF00] [Set] [OFF]               │
└─────────────────────────────────────────────┘
```

## 🔧 Troubleshooting

### Camera không hiển thị
1. Kiểm tra ESP32-CAM đã kết nối WiFi
2. Kiểm tra IP address trong Config
3. Test với browser: `http://192.168.4.1/capture`

### MQTT không kết nối
1. Kiểm tra broker address & port
2. Kiểm tra firewall
3. Test với MQTT Explorer

### YOLO không hoạt động
1. Kiểm tra đã cài ultralytics: `pip install ultralytics`
2. Download model: `yolo task=detect mode=predict model=yolov8n.pt source='https://ultralytics.com/images/bus.jpg'`
3. Kiểm tra model file tồn tại

### Charts không update
1. Kiểm tra MQTT nhận được sensor data
2. Kiểm tra topic names match với firmware
3. Xem console logs cho errors

## 📡 MQTT Topics

### Subscribe (từ firmware)
- `greenhouse/data/*` - Sensor data
- `greenhouse/status/*` - Device status
- `greenhouse/event/*` - Events (flame, sound, error)
- `greenhouse/sys/*` - System messages

### Publish (gửi control)
- `greenhouse/control/mode` - AUTO/MANUAL
- `greenhouse/control/pump` - ON/OFF
- `greenhouse/control/fan` - ON/OFF
- `greenhouse/control/auxFan` - ON/OFF
- `greenhouse/control/mainGrowLight` - ON/OFF
- `greenhouse/control/window1` - OPEN/CLOSE
- `greenhouse/control/mainDoor` - OPEN/CLOSE
- `greenhouse/control/rgbLed` - #RRGGBB hoặc OFF

## 🎨 Customization

### Dark Theme
Uncomment trong `main()`:
```python
# Set dark palette
palette = QPalette()
palette.setColor(QPalette.ColorRole.Window, QColor(53, 53, 53))
palette.setColor(QPalette.ColorRole.WindowText, Qt.GlobalColor.white)
# ... more colors
app.setPalette(palette)
```

### Chart Colors
Trong `create_charts_tab()`:
```python
self.temp_curve = self.temp_plot.plot(pen='r')  # red
self.hum_curve = self.hum_plot.plot(pen='b')    # blue
self.soil_curve = self.soil_plot.plot(pen='g')  # green
```

### Update Intervals
Trong class `Config`:
```python
CAMERA_UPDATE_MS = 100   # Camera FPS = 1000/100 = 10 FPS
SENSOR_UPDATE_MS = 1000  # Sensor update every 1s
CHART_UPDATE_MS = 2000   # Charts update every 2s
```

## 🐛 Debug Mode

Enable verbose logging:
```python
import logging
logging.basicConfig(level=logging.DEBUG)
```

## 📜 License

Same as main project (see LICENSE file in root)

## 🤝 Contributing

1. Fork the repo
2. Create feature branch
3. Commit changes
4. Push to branch
5. Create Pull Request

## 📞 Support

- GitHub Issues: https://github.com/c1k3nx/khkt-bao/issues
- Email: [your-email]
- Documentation: See main README.md

---

**Made with ❤️ for v1.4 COMPREHENSIVE OVERHAUL**
