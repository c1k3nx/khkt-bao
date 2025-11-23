# ⚙️ HƯỚNG DẪN CẤU HÌNH CHI TIẾT

## 📋 MỤC LỤC
1. [Cấu Hình Cơ Bản](#cấu-hình-cơ-bản)
2. [Cấu Hình Nâng Cao](#cấu-hình-nâng-cao)
3. [Tối Ưu Hiệu Năng](#tối-ưu-hiệu-năng)
4. [Troubleshooting Chi Tiết](#troubleshooting-chi-tiết)
5. [Ví Dụ Cấu Hình Thực Tế](#ví-dụ-cấu-hình-thực-tế)

---

## 🎯 CẤU HÌNH CƠ BẢN

### Bước 1: Tạo File Cấu Hình

File cấu hình được tự động tạo khi chạy lần đầu: `config.json`

**Cách tạo thủ công:**

```bash
cd pc-vision
cp config.example.py config.json
```

### Bước 2: Chỉnh Sửa Cấu Hình MQTT

#### Option 1: Dùng Code (Khuyến nghị)

```python
from config_manager import get_config

config = get_config()

# Đổi MQTT broker
config.set("mqtt.broker", "test.mosquitto.org")
config.set("mqtt.port", 1883)

# Lưu tự động
```

#### Option 2: Chỉnh Sửa JSON Trực Tiếp

Mở file `config.json`:

```json
{
  "mqtt": {
    "broker": "test.mosquitto.org",
    "port": 1883,
    "client_id": "greenhouse-gui",
    "keepalive": 60,
    "auto_reconnect": true,
    "max_reconnect_attempts": 5,
    "reconnect_delay": 2
  }
}
```

#### Bảng MQTT Brokers Phổ Biến

| Broker | URL | Port | Miễn Phí | Yêu Cầu Auth | Ghi Chú |
|--------|-----|------|----------|--------------|---------|
| HiveMQ Public | `broker.hivemq.com` | 1883 | ✅ Có | ❌ Không | Khuyến nghị cho testing |
| Eclipse Mosquitto | `test.mosquitto.org` | 1883 | ✅ Có | ❌ Không | Ổn định, phổ biến |
| EMQX Public | `broker.emqx.io` | 1883 | ✅ Có | ❌ Không | Nhanh, hiện đại |
| Private Broker | `192.168.1.100` | 1883 | - | ⚠️ Tùy chọn | Tự host, bảo mật cao |

**Test MQTT Broker:**

```bash
# Cài mosquitto clients
sudo apt install mosquitto-clients

# Subscribe để test
mosquitto_sub -h broker.hivemq.com -t "greenhouse/#" -v

# Publish để test
mosquitto_pub -h broker.hivemq.com -t "greenhouse/test" -m "Hello"
```

### Bước 3: Cấu Hình Camera

#### Tìm IP của ESP32-CAM

**Method 1: Serial Monitor**
```
Connecting to WiFi...
Connected!
IP Address: 192.168.1.150
```

**Method 2: Router Admin Panel**
- Vào router (thường `192.168.1.1`)
- Tìm "Connected Devices" hoặc "DHCP Clients"
- Tìm device tên "ESP32-CAM"

**Method 3: Network Scan**
```bash
# Cài nmap
sudo apt install nmap

# Scan mạng
nmap -sn 192.168.1.0/24

# Hoặc dùng arp
arp -a
```

#### Test Camera

```bash
# Method 1: Browser
firefox http://192.168.1.150/capture

# Method 2: curl
curl -I http://192.168.1.150/capture

# Method 3: Python
python3 << EOF
import requests
response = requests.get("http://192.168.1.150/capture", timeout=5)
print(f"Status: {response.status_code}")
print(f"Size: {len(response.content)} bytes")
EOF
```

#### Cấu Hình trong config.json

```json
{
  "camera": {
    "url": "http://192.168.1.150",
    "timeout": 5,
    "fps": 10,
    "enable_yolo": true,
    "yolo_model": "yolov8n.pt",
    "yolo_confidence": 0.25,
    "yolo_iou": 0.45
  }
}
```

#### Bảng Resolution ESP32-CAM

| Resolution | Kích Thước | FPS Max | Bandwidth | Sử Dụng Cho |
|------------|------------|---------|-----------|-------------|
| QVGA | 320x240 | 30 | ~100 KB/s | Testing, mạng chậm |
| VGA | 640x480 | 25 | ~300 KB/s | ✅ **Khuyến nghị** |
| SVGA | 800x600 | 20 | ~500 KB/s | YOLO detection tốt |
| XGA | 1024x768 | 15 | ~800 KB/s | Chất lượng cao |
| HD | 1280x720 | 10 | ~1 MB/s | Recording, archive |

**Đổi resolution trên ESP32-CAM firmware:**
```cpp
// Trong ESP32-CAM code
config.frame_size = FRAMESIZE_VGA;  // 640x480 - Khuyến nghị
```

### Bước 4: Cấu Hình UI

#### Window Size theo Màn Hình

| Loại Màn Hình | Resolution | Window Width | Window Height |
|---------------|------------|--------------|---------------|
| Laptop 13" | 1366x768 | 1200 | 700 |
| Laptop 15" | 1920x1080 | 1400 | 900 |
| Desktop 24" | 1920x1080 | 1600 | 1000 |
| Desktop 27" | 2560x1440 | 2000 | 1200 |
| 4K Monitor | 3840x2160 | 2400 | 1400 |

```json
{
  "ui": {
    "window_width": 1400,
    "window_height": 900,
    "window_x": 100,
    "window_y": 100,
    "theme": "light",
    "font_size": 10
  }
}
```

#### Theme Configuration

```json
{
  "ui": {
    "theme": "light"  // Hoặc "dark"
  }
}
```

**Light Theme:**
- ✅ Tiết kiệm pin (OLED)
- ✅ Dễ đọc ban ngày
- ❌ Mỏi mắt ban đêm

**Dark Theme:**
- ✅ Dễ chịu ban đêm
- ✅ Chuyên nghiệp
- ❌ Khó đọc ban ngày

---

## 🔧 CẤU HÌNH NÂNG CAO

### 1. Tối Ưu YOLO Detection

#### Bảng So Sánh Cấu Hình YOLO

| Cấu Hình | Confidence | IOU | Model | FPS | CPU % | Accuracy | Sử Dụng |
|----------|-----------|-----|-------|-----|-------|----------|---------|
| **Nhanh** | 0.5 | 0.3 | yolov8n | 10 | 30% | Trung bình | Laptop yếu |
| **Cân Bằng** | 0.25 | 0.45 | yolov8s | 8 | 50% | Tốt | ✅ **Khuyến nghị** |
| **Chính Xác** | 0.15 | 0.6 | yolov8m | 5 | 80% | Rất tốt | Desktop mạnh |
| **Chất Lượng Cao** | 0.1 | 0.7 | yolov8l | 3 | 100% | Xuất sắc | Workstation |

#### Cấu Hình Theo Tình Huống

**Tình huống 1: Laptop Yếu (CPU i3, 4GB RAM)**
```json
{
  "camera": {
    "fps": 5,
    "enable_yolo": false,  // Tắt YOLO
    "yolo_model": "yolov8n.pt"
  },
  "charts": {
    "history_size": 50,
    "update_interval": 5000
  }
}
```

**Tình huống 2: Laptop Trung Bình (CPU i5, 8GB RAM)**
```json
{
  "camera": {
    "fps": 10,
    "enable_yolo": true,
    "yolo_model": "yolov8n.pt",
    "yolo_confidence": 0.25
  },
  "charts": {
    "history_size": 100,
    "update_interval": 2000
  }
}
```

**Tình huống 3: Desktop Mạnh (CPU i7, 16GB RAM, GPU)**
```json
{
  "camera": {
    "fps": 20,
    "enable_yolo": true,
    "yolo_model": "yolov8m.pt",
    "yolo_confidence": 0.15,
    "yolo_iou": 0.6
  },
  "charts": {
    "history_size": 200,
    "update_interval": 1000
  }
}
```

### 2. Cấu Hình Data Export

#### Kịch Bản Export

**Kịch bản 1: Monitoring Ngắn Hạn (Vài Giờ)**
```json
{
  "export": {
    "enable_csv": false,  // Không cần export
    "csv_interval": 60
  },
  "alerts": {
    "enable_logging": true,
    "max_log_entries": 100
  }
}
```

**Kịch bản 2: Research/Phân Tích (Vài Ngày)**
```json
{
  "export": {
    "enable_csv": true,
    "csv_interval": 60,  // Ghi mỗi 60 giây
    "csv_dir": "./data",
    "include_timestamp": true,
    "decimal_places": 2
  },
  "alerts": {
    "enable_logging": true,
    "max_log_entries": 1000
  }
}
```

**Kịch bản 3: Production/Long-term (Nhiều Tuần)**
```json
{
  "export": {
    "enable_csv": true,
    "csv_interval": 300,  // Ghi mỗi 5 phút (tiết kiệm disk)
    "csv_dir": "/mnt/data/greenhouse",  // External drive
    "auto_export": true,
    "max_file_size_mb": 50  // Rotation lớn hơn
  }
}
```

#### Bảng CSV Interval

| Interval | Số Entries/Giờ | Số Entries/Ngày | Kích Thước/Ngày | Sử Dụng |
|----------|----------------|-----------------|-----------------|---------|
| 10s | 360 | 8,640 | ~2 MB | High precision research |
| 30s | 120 | 2,880 | ~700 KB | Detailed monitoring |
| 60s | 60 | 1,440 | ~350 KB | ✅ **Khuyến nghị** |
| 300s (5min) | 12 | 288 | ~70 KB | Long-term, tiết kiệm disk |
| 600s (10min) | 6 | 144 | ~35 KB | Basic logging |

### 3. Cấu Hình Charts

#### Chart Colors Customization

```json
{
  "charts": {
    "colors": {
      "temperature": "#FF0000",     // Đỏ - nóng
      "humidity": "#0000FF",        // Xanh dương - nước
      "soilMoisture": "#00FF00",    // Xanh lá - cây
      "lightIntensity": "#FFFF00"   // Vàng - mặt trời
    }
  }
}
```

#### Bảng Màu Gợi Ý

| Sensor | Màu | Hex Code | Lý Do |
|--------|-----|----------|-------|
| Temperature | 🔴 Đỏ | `#FF0000` | Liên tưởng "nóng" |
| Humidity | 🔵 Xanh Dương | `#0000FF` | Liên tưởng "nước" |
| Soil Moisture | 🟢 Xanh Lá | `#00FF00` | Liên tưởng "cây cối" |
| Light | 🟡 Vàng | `#FFFF00` | Liên tưởng "mặt trời" |
| Gas | 🟣 Tím | `#FF00FF` | Cảnh báo độc |
| Sound | 🟠 Cam | `#FFA500` | Cảnh báo |

#### Chart History Size

```json
{
  "charts": {
    "history_size": 100  // Số data points hiển thị
  }
}
```

| History Size | Memory Usage | Time Span (60s interval) | Sử Dụng |
|--------------|--------------|--------------------------|---------|
| 20 | ~5 KB | ~30 phút | Quick glance |
| 50 | ~12 KB | ~1 giờ | Short-term |
| 100 | ~25 KB | ~2 giờ | ✅ **Khuyến nghị** |
| 200 | ~50 KB | ~4 giờ | Long-term view |
| 500 | ~120 KB | ~8 giờ | Detailed analysis |

### 4. Cấu Hình Alerts

#### Alert Levels Configuration

```json
{
  "alerts": {
    "enable_sound": false,      // Âm thanh cảnh báo
    "enable_popup": true,       // Popup cho critical
    "enable_logging": true,     // Ghi log
    "max_log_entries": 1000,    // Circular buffer size
    "auto_clear": false,        // Tự động xóa
    "critical_events": [
      "flame",
      "fire",
      "error"
    ]
  }
}
```

#### Bảng Cấu Hình Alert Theo Môi Trường

| Môi Trường | Sound | Popup | Max Entries | Auto Clear | Ghi Chú |
|------------|-------|-------|-------------|------------|---------|
| **Office** | ❌ OFF | ✅ ON | 500 | ✅ ON | Không làm phiền |
| **Lab** | ⚠️ Critical only | ✅ ON | 1000 | ❌ OFF | Cần log đầy đủ |
| **Home** | ✅ ON | ✅ ON | 200 | ✅ ON | Cảnh báo rõ ràng |
| **Production** | ✅ ON | ✅ ON | 5000 | ❌ OFF | Critical, full logging |

### 5. Cấu Hình Thresholds

```json
{
  "thresholds": {
    "temp_warning_high": 35.0,
    "temp_warning_low": 15.0,
    "humidity_warning_high": 80.0,
    "humidity_warning_low": 40.0,
    "soil_warning_low": 30.0,
    "light_warning_low": 5000,
    "flame_warning": 800,
    "sound_warning": 700
  }
}
```

#### Bảng Ngưỡng Theo Loại Cây

| Loại Cây | Temp Min | Temp Max | Humidity Min | Humidity Max | Soil Min | Light Min |
|----------|----------|----------|--------------|--------------|----------|-----------|
| **Rau Ăn Lá** | 15°C | 25°C | 60% | 80% | 40% | 8,000 lux |
| **Cà Chua** | 18°C | 30°C | 50% | 70% | 30% | 15,000 lux |
| **Dâu Tây** | 15°C | 25°C | 60% | 75% | 35% | 12,000 lux |
| **Ớt** | 20°C | 32°C | 50% | 70% | 25% | 20,000 lux |
| **Xà Lách** | 10°C | 20°C | 60% | 80% | 40% | 6,000 lux |

**Ví dụ cấu hình cho Cà Chua:**
```json
{
  "thresholds": {
    "temp_warning_high": 30.0,
    "temp_warning_low": 18.0,
    "humidity_warning_high": 70.0,
    "humidity_warning_low": 50.0,
    "soil_warning_low": 30.0,
    "light_warning_low": 15000
  }
}
```

---

## ⚡ TỐI ƯU HIỆU NĂNG

### 1. Giảm CPU Usage

#### Bảng Tối Ưu CPU

| Thay Đổi | CPU Giảm | Trade-off | Khuyến Nghị |
|----------|----------|-----------|-------------|
| Tắt YOLO | -40% | Mất detection | ✅ Khi không cần |
| Giảm FPS 10→5 | -20% | Ít smooth hơn | ✅ Laptop yếu |
| Tăng chart interval 2s→5s | -10% | Update chậm hơn | ⚠️ Chấp nhận được |
| Giảm history 100→50 | -5% | Ít data history | ⚠️ OK |
| Dùng yolov8n thay v8s | -25% | Ít chính xác hơn | ✅ Luôn luôn |

**Cấu hình tối ưu CPU:**
```json
{
  "camera": {
    "fps": 5,
    "enable_yolo": false
  },
  "charts": {
    "history_size": 50,
    "update_interval": 5000
  },
  "ui": {
    "enable_animations": false
  }
}
```

### 2. Giảm Memory Usage

#### Bảng Tối Ưu Memory

| Component | Memory Normal | Memory Optimized | Cách Tối Ưu |
|-----------|---------------|------------------|-------------|
| YOLO Model | 500 MB | 150 MB | Dùng yolov8n thay v8m |
| Chart History | 50 MB | 10 MB | Giảm history_size 200→50 |
| Event Log | 20 MB | 5 MB | Giảm max_log_entries 5000→1000 |
| Camera Buffer | 30 MB | 10 MB | Giảm resolution VGA→QVGA |

**Cấu hình tối ưu Memory:**
```json
{
  "camera": {
    "yolo_model": "yolov8n.pt"
  },
  "charts": {
    "history_size": 50
  },
  "alerts": {
    "max_log_entries": 500
  }
}
```

### 3. Giảm Bandwidth

#### Bảng Bandwidth Usage

| Component | Bandwidth/s | Cách Giảm |
|-----------|-------------|-----------|
| Camera Stream (VGA 10fps) | ~300 KB/s | Giảm FPS hoặc resolution |
| MQTT Data (1s interval) | ~1 KB/s | Tăng interval |
| Total | ~301 KB/s | |

**Để giảm bandwidth:**

1. **Giảm Camera FPS:**
   ```json
   {"camera": {"fps": 5}}  // 300KB/s → 150KB/s
   ```

2. **Giảm Resolution:**
   ```cpp
   // ESP32-CAM firmware
   config.frame_size = FRAMESIZE_QVGA;  // 300KB/s → 100KB/s
   ```

3. **Tăng Sensor Interval:**
   ```json
   {"export": {"csv_interval": 120}}  // Publish mỗi 2 phút
   ```

### 4. Cấu Hình cho Raspberry Pi

**Raspberry Pi 3B+ / 4:**
```json
{
  "camera": {
    "fps": 8,
    "enable_yolo": true,
    "yolo_model": "yolov8n.pt"
  },
  "charts": {
    "history_size": 80,
    "update_interval": 3000
  },
  "ui": {
    "window_width": 1200,
    "window_height": 700,
    "enable_animations": false
  }
}
```

**Raspberry Pi Zero:**
```json
{
  "camera": {
    "fps": 3,
    "enable_yolo": false
  },
  "charts": {
    "history_size": 30,
    "update_interval": 10000
  }
}
```

---

## 🐛 TROUBLESHOOTING CHI TIẾT

### 1. Lỗi Kết Nối MQTT

#### Checklist Debug

```bash
# 1. Test internet
ping 8.8.8.8

# 2. Test DNS
ping broker.hivemq.com

# 3. Test MQTT port
telnet broker.hivemq.com 1883

# 4. Test với mosquitto
mosquitto_sub -h broker.hivemq.com -t "test" -v

# 5. Check firewall
sudo ufw status
sudo ufw allow 1883/tcp
```

#### Bảng Lỗi MQTT

| Lỗi | Nguyên Nhân | Giải Pháp |
|-----|-------------|-----------|
| Connection refused | Port sai hoặc broker offline | Thử broker khác, check port |
| Timeout | Firewall chặn | `sudo ufw allow 1883` |
| Authentication failed | Cần username/password | Thêm credentials vào config |
| Connection lost | Mạng không ổn định | Bật `auto_reconnect` |

### 2. Lỗi Camera

#### Bảng Lỗi Camera

| Lỗi | HTTP Code | Nguyên Nhân | Giải Pháp |
|-----|-----------|-------------|-----------|
| "Camera Offline" | Timeout | ESP32 tắt hoặc sai IP | Ping IP, check ESP32 serial |
| "Connection refused" | - | Sai port hoặc ESP32 chưa boot | Đợi ESP32 khởi động (~10s) |
| "404 Not Found" | 404 | Sai URL path | Dùng `/capture` không phải `/cam` |
| Ảnh đen | 200 | Camera module lỗi | Kiểm tra kết nối camera ribbon |
| Ảnh nhiễu | 200 | Nguồn yếu | Dùng nguồn 5V/2A tốt hơn |

#### Debug Camera Step-by-Step

```bash
# Step 1: Kiểm tra ESP32-CAM đã boot chưa
# Mở Serial Monitor, xem output:
# "Camera initialized"
# "IP: 192.168.x.x"

# Step 2: Ping IP
ping 192.168.1.150

# Step 3: Test với curl
curl -I http://192.168.1.150/capture
# Expected: HTTP/1.1 200 OK

# Step 4: Download ảnh test
curl http://192.168.1.150/capture -o test.jpg
file test.jpg  # Phải là JPEG image

# Step 5: Test trong GUI
# Mở GUI, vào Camera tab
# Check console logs
```

### 3. Lỗi YOLO

#### Bảng Lỗi YOLO

| Lỗi | Nguyên Nhân | Giải Pháp |
|-----|-------------|-----------|
| "YOLO not available" | Chưa cài ultralytics | `pip install ultralytics` |
| "Model file not found" | Chưa download model | `python -c "from ultralytics import YOLO; YOLO('yolov8n.pt')"` |
| CUDA error | Lỗi GPU driver | Dùng CPU: `yolo_device = "cpu"` |
| Out of memory | Model quá lớn | Dùng yolov8n thay vì v8m |
| Detection chậm | CPU yếu | Giảm FPS, dùng yolov8n |

#### Download YOLO Models

```bash
# Nano (nhỏ nhất, nhanh nhất)
python3 << EOF
from ultralytics import YOLO
model = YOLO('yolov8n.pt')
print("Downloaded yolov8n.pt")
EOF

# Small
python3 << EOF
from ultralytics import YOLO
model = YOLO('yolov8s.pt')
print("Downloaded yolov8s.pt")
EOF

# Check models
ls -lh ~/.config/Ultralytics/*.pt
```

### 4. Lỗi Performance

#### Bảng Performance Issues

| Vấn Đề | CPU | Memory | Giải Pháp |
|--------|-----|--------|-----------|
| GUI đơ | >80% | - | Tắt YOLO, giảm FPS |
| Memory leak | - | Tăng dần | Giảm history_size, clear events |
| Camera lag | >60% | - | Giảm FPS, giảm resolution |
| Chart lag | >40% | - | Tăng update_interval |

#### Monitor Performance

```bash
# CPU usage
top -p $(pgrep -f greenhouse_gui.py)

# Memory usage
ps aux | grep greenhouse_gui.py

# Hoặc dùng htop
htop
# Press F5 (tree view)
# Find "python greenhouse_gui.py"
```

---

## 📝 VÍ DỤ CẤU HÌNH THỰC TẾ

### Cấu Hình 1: Lab Research (Desktop Mạnh)

```json
{
  "mqtt": {
    "broker": "192.168.1.100",
    "port": 1883,
    "keepalive": 60,
    "auto_reconnect": true
  },
  "camera": {
    "url": "http://192.168.1.150",
    "fps": 20,
    "enable_yolo": true,
    "yolo_model": "yolov8m.pt",
    "yolo_confidence": 0.15,
    "auto_save_captures": true
  },
  "ui": {
    "theme": "light",
    "window_width": 2000,
    "window_height": 1200
  },
  "charts": {
    "history_size": 200,
    "update_interval": 1000
  },
  "export": {
    "enable_csv": true,
    "csv_interval": 30,
    "csv_dir": "/mnt/research/greenhouse"
  },
  "alerts": {
    "enable_sound": true,
    "enable_popup": true,
    "max_log_entries": 5000
  }
}
```

### Cấu Hình 2: Home Monitoring (Laptop)

```json
{
  "mqtt": {
    "broker": "test.mosquitto.org",
    "port": 1883
  },
  "camera": {
    "url": "http://192.168.1.150",
    "fps": 10,
    "enable_yolo": true,
    "yolo_model": "yolov8n.pt"
  },
  "ui": {
    "theme": "dark",
    "window_width": 1400,
    "window_height": 900
  },
  "charts": {
    "history_size": 100,
    "update_interval": 2000
  },
  "export": {
    "enable_csv": false
  },
  "alerts": {
    "enable_sound": false,
    "enable_popup": true,
    "max_log_entries": 500
  }
}
```

### Cấu Hình 3: Production (24/7 Monitoring)

```json
{
  "mqtt": {
    "broker": "192.168.1.100",
    "port": 1883,
    "keepalive": 120,
    "auto_reconnect": true,
    "max_reconnect_attempts": 10
  },
  "camera": {
    "url": "http://192.168.1.150",
    "fps": 15,
    "enable_yolo": true,
    "yolo_model": "yolov8s.pt",
    "auto_save_captures": true,
    "captures_dir": "/var/greenhouse/captures"
  },
  "charts": {
    "history_size": 500,
    "update_interval": 2000
  },
  "export": {
    "enable_csv": true,
    "csv_interval": 60,
    "csv_dir": "/var/greenhouse/data",
    "auto_export": true
  },
  "alerts": {
    "enable_sound": true,
    "enable_popup": true,
    "enable_logging": true,
    "max_log_entries": 10000,
    "auto_clear": false
  },
  "advanced": {
    "enable_logging": true,
    "log_level": "INFO",
    "log_file": "/var/log/greenhouse/app.log",
    "max_log_size_mb": 50
  }
}
```

### Cấu Hình 4: Raspberry Pi (Low Power)

```json
{
  "mqtt": {
    "broker": "broker.hivemq.com",
    "port": 1883
  },
  "camera": {
    "url": "http://192.168.1.150",
    "fps": 5,
    "enable_yolo": false,
    "timeout": 10
  },
  "ui": {
    "window_width": 1200,
    "window_height": 700,
    "enable_animations": false
  },
  "charts": {
    "history_size": 50,
    "update_interval": 5000
  },
  "export": {
    "enable_csv": true,
    "csv_interval": 300
  },
  "alerts": {
    "max_log_entries": 200
  }
}
```

### Cấu Hình 5: Demo/Presentation

```json
{
  "mqtt": {
    "broker": "test.mosquitto.org",
    "port": 1883
  },
  "camera": {
    "url": "http://192.168.4.1",
    "fps": 15,
    "enable_yolo": true,
    "yolo_model": "yolov8s.pt",
    "yolo_confidence": 0.3
  },
  "ui": {
    "theme": "light",
    "window_width": 1920,
    "window_height": 1080,
    "font_size": 12,
    "enable_animations": true
  },
  "charts": {
    "history_size": 100,
    "update_interval": 1500,
    "enable_legend": true,
    "line_width": 3
  },
  "alerts": {
    "enable_popup": true,
    "max_log_entries": 100
  }
}
```

---

## 🔧 CÔNG CỤ HỖ TRỢ

### 1. Script Validate Config

Tạo file `validate_config.py`:

```python
#!/usr/bin/env python3
from config_manager import get_config

config = get_config()

print("=" * 60)
print("VALIDATING CONFIGURATION")
print("=" * 60)

is_valid, errors = config.validate()

if is_valid:
    print("✅ Configuration is VALID!")
else:
    print("❌ Configuration has ERRORS:")
    for error in errors:
        print(f"  - {error}")

print("\nCurrent Settings:")
print(f"  MQTT Broker: {config.get('mqtt.broker')}:{config.get('mqtt.port')}")
print(f"  Camera URL: {config.get('camera.url')}")
print(f"  YOLO Model: {config.get('camera.yolo_model')}")
print(f"  FPS: {config.get('camera.fps')}")
print(f"  CSV Export: {config.get('export.enable_csv')}")
```

Chạy:
```bash
python validate_config.py
```

### 2. Script Backup/Restore Config

Tạo file `backup_config.sh`:

```bash
#!/bin/bash

BACKUP_DIR="./config_backups"
mkdir -p "$BACKUP_DIR"

# Backup
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
cp config.json "$BACKUP_DIR/config_$TIMESTAMP.json"

echo "✅ Backed up to: $BACKUP_DIR/config_$TIMESTAMP.json"

# List backups
echo ""
echo "Available backups:"
ls -lh "$BACKUP_DIR"
```

Restore:
```bash
#!/bin/bash
# restore_config.sh

if [ -z "$1" ]; then
    echo "Usage: ./restore_config.sh <backup_file>"
    exit 1
fi

cp "$1" config.json
echo "✅ Restored from: $1"
```

### 3. Script Monitor Performance

Tạo file `monitor.sh`:

```bash
#!/bin/bash

PID=$(pgrep -f "greenhouse_gui.py")

if [ -z "$PID" ]; then
    echo "❌ GUI not running"
    exit 1
fi

echo "Monitoring PID: $PID"
echo "Press Ctrl+C to stop"
echo ""

while true; do
    CPU=$(ps -p $PID -o %cpu --no-headers)
    MEM=$(ps -p $PID -o %mem --no-headers)
    VSZ=$(ps -p $PID -o vsz --no-headers)

    echo "$(date +%H:%M:%S) - CPU: ${CPU}% | Memory: ${MEM}% | VSZ: ${VSZ} KB"

    sleep 2
done
```

Chạy:
```bash
chmod +x monitor.sh
./monitor.sh
```

---

**🌱 Tài liệu v1.4 - COMPREHENSIVE OVERHAUL**

**Cập nhật:** 2024-11-23
