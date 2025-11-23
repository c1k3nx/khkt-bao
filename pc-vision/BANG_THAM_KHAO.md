# 📚 BẢNG THAM KHẢO - GREENHOUSE IoT GUI

## 📋 MỤC LỤC
1. [Bảng API ConfigManager](#bảng-api-configmanager)
2. [Bảng API DataLogger](#bảng-api-datalogger)
3. [Bảng Cấu Hình Chi Tiết](#bảng-cấu-hình-chi-tiết)
4. [Bảng MQTT Topics](#bảng-mqtt-topics)
5. [Bảng Sensor Data Format](#bảng-sensor-data-format)
6. [Bảng Troubleshooting](#bảng-troubleshooting)
7. [Bảng So Sánh YOLO Models](#bảng-so-sánh-yolo-models)
8. [Bảng Phím Tắt](#bảng-phím-tắt)

---

## 📊 BẢNG API ConfigManager

### Các Phương Thức Chính

| Phương Thức | Tham Số | Trả Về | Mô Tả |
|------------|---------|---------|-------|
| `get(key_path, default)` | `key_path`: str<br>`default`: Any | Any | Lấy giá trị cấu hình theo đường dẫn<br>Ví dụ: `config.get("mqtt.broker")` |
| `set(key_path, value, auto_save)` | `key_path`: str<br>`value`: Any<br>`auto_save`: bool | bool | Đặt giá trị cấu hình<br>Tự động lưu nếu `auto_save=True` |
| `save()` | Không có | bool | Lưu cấu hình vào file JSON |
| `load()` | Không có | bool | Tải cấu hình từ file JSON |
| `validate()` | Không có | (bool, list[str]) | Kiểm tra tính hợp lệ<br>Trả về: (có_hợp_lệ, danh_sách_lỗi) |
| `reset_to_defaults()` | Không có | bool | Khôi phục cấu hình mặc định |
| `export_to_json(file_path)` | `file_path`: str | bool | Xuất cấu hình ra file JSON |
| `import_from_json(file_path)` | `file_path`: str | bool | Nhập cấu hình từ file JSON |
| `get_category(category)` | `category`: str | Dict | Lấy toàn bộ một danh mục<br>Ví dụ: `get_category("mqtt")` |
| `get_all()` | Không có | Dict | Lấy toàn bộ cấu hình |

### Ví Dụ Sử Dụng

```python
from config_manager import get_config

# Lấy singleton instance
config = get_config()

# Đọc giá trị
broker = config.get("mqtt.broker")  # "broker.hivemq.com"
fps = config.get("camera.fps")      # 10

# Đặt giá trị
config.set("mqtt.broker", "test.mosquitto.org")
config.set("camera.fps", 20)

# Kiểm tra hợp lệ
is_valid, errors = config.validate()
if not is_valid:
    for error in errors:
        print(f"Lỗi: {error}")

# Khôi phục mặc định
config.reset_to_defaults()
```

---

## 📊 BẢNG API DataLogger

### CSVDataLogger - Phương Thức

| Phương Thức | Tham Số | Trả Về | Mô Tả |
|------------|---------|---------|-------|
| `log_sensor_data(data)` | `data`: Dict[str, Any] | bool | Ghi dữ liệu sensor vào CSV<br>Tự động xoay file khi quá lớn |
| `get_latest_file()` | Không có | Optional[Path] | Lấy đường dẫn file CSV hiện tại |
| `get_all_files()` | Không có | List[Path] | Lấy danh sách tất cả file CSV |
| `close()` | Không có | None | Đóng logger và file |

### EventLogger - Phương Thức

| Phương Thức | Tham Số | Trả Về | Mô Tả |
|------------|---------|---------|-------|
| `log_event(type, msg, severity, metadata)` | `type`: str<br>`msg`: str<br>`severity`: str<br>`metadata`: Dict | None | Ghi event với mức độ nghiêm trọng |
| `get_events(limit, event_type, severity)` | `limit`: int<br>`event_type`: str<br>`severity`: str | List[Dict] | Lấy events có lọc |
| `get_latest(n)` | `n`: int | List[Dict] | Lấy N events mới nhất |
| `clear()` | Không có | None | Xóa tất cả events |
| `count(event_type, severity)` | `event_type`: str<br>`severity`: str | int | Đếm số events |
| `export_to_csv(output_file)` | `output_file`: str | bool | Xuất events ra file CSV |

### Mức Độ Severity

| Severity | Mô Tả | Sử Dụng Khi |
|----------|-------|-------------|
| `info` | Thông tin thông thường | Mode đổi, device bật/tắt, khởi động |
| `warning` | Cảnh báo | Giá trị sensor gần ngưỡng, kết nối chậm |
| `error` | Lỗi | Kết nối thất bại, lỗi camera, lỗi MQTT |
| `critical` | Nghiêm trọng | Phát hiện lửa, lỗi hệ thống, mất kết nối lâu |

### Ví Dụ Sử Dụng

```python
from data_logger import get_csv_logger, get_event_logger

# CSV Logger
csv_logger = get_csv_logger()

# Ghi dữ liệu sensor
sensor_data = {
    'ts': 1700000000,
    'temperature': {'v': 25.3, 'u': '°C'},
    'humidity': {'v': 65.2, 'u': '%'},
    'soilMoisture': {'v': 45.8, 'u': '%'}
}
csv_logger.log_sensor_data(sensor_data)

# Event Logger
event_logger = get_event_logger()

# Ghi events
event_logger.log_event("flame", "Phát hiện lửa!", "critical")
event_logger.log_event("sound", "Tiếng động lớn", "warning")
event_logger.log_event("mode", "Chuyển sang AUTO mode", "info")

# Lấy events
recent = event_logger.get_latest(10)
critical_events = event_logger.get_events(event_type="flame", severity="critical")
```

---

## 📊 BẢNG CẤU HÌNH CHI TIẾT

### 1. Cấu Hình MQTT (`mqtt.*`)

| Key | Kiểu | Mặc Định | Mô Tả |
|-----|------|----------|-------|
| `mqtt.broker` | string | `"broker.hivemq.com"` | Địa chỉ MQTT broker |
| `mqtt.port` | int | `1883` | Port MQTT (1883 = không mã hóa, 8883 = SSL) |
| `mqtt.client_id` | string | `"greenhouse-gui"` | ID của client kết nối |
| `mqtt.keepalive` | int | `60` | Thời gian keepalive (giây) |
| `mqtt.auto_reconnect` | bool | `true` | Tự động kết nối lại khi mất kết nối |
| `mqtt.max_reconnect_attempts` | int | `5` | Số lần thử kết nối lại tối đa |
| `mqtt.reconnect_delay` | int | `2` | Độ trễ giữa các lần thử (giây) |

**Ví dụ:**
```json
{
  "mqtt": {
    "broker": "test.mosquitto.org",
    "port": 1883,
    "auto_reconnect": true
  }
}
```

### 2. Cấu Hình Camera (`camera.*`)

| Key | Kiểu | Mặc Định | Mô Tả |
|-----|------|----------|-------|
| `camera.url` | string | `"http://192.168.4.1"` | URL của ESP32-CAM |
| `camera.timeout` | int | `5` | Timeout cho requests (giây) |
| `camera.fps` | int | `10` | Tốc độ khung hình (1-60) |
| `camera.enable_yolo` | bool | `true` | Bật/tắt YOLO detection |
| `camera.yolo_model` | string | `"yolov8n.pt"` | Model YOLO sử dụng |
| `camera.yolo_confidence` | float | `0.25` | Ngưỡng confidence (0.0-1.0) |
| `camera.yolo_iou` | float | `0.45` | Ngưỡng IOU cho NMS (0.0-1.0) |
| `camera.auto_save_captures` | bool | `false` | Tự động lưu ảnh chụp |
| `camera.captures_dir` | string | `"./captures"` | Thư mục lưu ảnh |

**Ví dụ:**
```json
{
  "camera": {
    "url": "http://192.168.1.150",
    "fps": 15,
    "enable_yolo": true,
    "yolo_model": "yolov8s.pt"
  }
}
```

### 3. Cấu Hình Giao Diện (`ui.*`)

| Key | Kiểu | Mặc Định | Mô Tả |
|-----|------|----------|-------|
| `ui.theme` | string | `"light"` | Theme giao diện ("light"/"dark") |
| `ui.window_width` | int | `1400` | Chiều rộng cửa sổ (800-3840) |
| `ui.window_height` | int | `900` | Chiều cao cửa sổ (600-2160) |
| `ui.window_x` | int | `100` | Vị trí X của cửa sổ |
| `ui.window_y` | int | `100` | Vị trí Y của cửa sổ |
| `ui.font_size` | int | `10` | Cỡ chữ mặc định |
| `ui.show_toolbar` | bool | `true` | Hiển thị toolbar |
| `ui.show_statusbar` | bool | `true` | Hiển thị status bar |
| `ui.enable_animations` | bool | `true` | Bật hiệu ứng animation |

### 4. Cấu Hình Charts (`charts.*`)

| Key | Kiểu | Mặc Định | Mô Tả |
|-----|------|----------|-------|
| `charts.history_size` | int | `100` | Số điểm dữ liệu hiển thị (10-1000) |
| `charts.update_interval` | int | `2000` | Interval cập nhật (ms) |
| `charts.enable_grid` | bool | `true` | Hiển thị lưới |
| `charts.enable_legend` | bool | `true` | Hiển thị chú thích |
| `charts.line_width` | int | `2` | Độ dày đường vẽ |
| `charts.colors.temperature` | string | `"#FF0000"` | Màu graph nhiệt độ |
| `charts.colors.humidity` | string | `"#0000FF"` | Màu graph độ ẩm |
| `charts.colors.soilMoisture` | string | `"#00FF00"` | Màu graph độ ẩm đất |
| `charts.colors.lightIntensity` | string | `"#FFFF00"` | Màu graph ánh sáng |

### 5. Cấu Hình Alerts (`alerts.*`)

| Key | Kiểu | Mặc Định | Mô Tả |
|-----|------|----------|-------|
| `alerts.enable_sound` | bool | `false` | Bật âm thanh cảnh báo |
| `alerts.enable_popup` | bool | `true` | Hiển thị popup cho critical events |
| `alerts.enable_logging` | bool | `true` | Ghi log events |
| `alerts.max_log_entries` | int | `1000` | Số events tối đa trong bộ nhớ |
| `alerts.auto_clear` | bool | `false` | Tự động xóa log cũ |
| `alerts.critical_events` | array | `["flame", "fire", "error"]` | Các event được coi là critical |

### 6. Cấu Hình Export (`export.*`)

| Key | Kiểu | Mặc Định | Mô Tả |
|-----|------|----------|-------|
| `export.enable_csv` | bool | `false` | Bật tự động ghi CSV |
| `export.csv_interval` | int | `60` | Interval ghi CSV (giây) |
| `export.csv_dir` | string | `"./data"` | Thư mục lưu CSV |
| `export.auto_export` | bool | `false` | Tự động export khi đóng app |
| `export.include_timestamp` | bool | `true` | Bao gồm timestamp trong CSV |
| `export.decimal_places` | int | `2` | Số chữ số thập phân |

### 7. Ngưỡng Cảnh Báo (`thresholds.*`)

| Key | Kiểu | Mặc Định | Mô Tả |
|-----|------|----------|-------|
| `thresholds.temp_warning_high` | float | `35.0` | Nhiệt độ cao (°C) |
| `thresholds.temp_warning_low` | float | `15.0` | Nhiệt độ thấp (°C) |
| `thresholds.humidity_warning_high` | float | `80.0` | Độ ẩm cao (%) |
| `thresholds.humidity_warning_low` | float | `40.0` | Độ ẩm thấp (%) |
| `thresholds.soil_warning_low` | float | `30.0` | Độ ẩm đất thấp (%) |
| `thresholds.light_warning_low` | float | `5000` | Ánh sáng thấp (lux) |
| `thresholds.flame_warning` | int | `800` | Ngưỡng phát hiện lửa |
| `thresholds.sound_warning` | int | `700` | Ngưỡng tiếng động |

**Lưu ý:** Đây chỉ là ngưỡng hiển thị trên GUI. Điều khiển thực tế được xử lý trên ESP8266.

### 8. Cấu Hình Nâng Cao (`advanced.*`)

| Key | Kiểu | Mặc Định | Mô Tả |
|-----|------|----------|-------|
| `advanced.enable_logging` | bool | `true` | Bật logging hệ thống |
| `advanced.log_level` | string | `"INFO"` | Mức log (DEBUG/INFO/WARNING/ERROR) |
| `advanced.log_file` | string | `"greenhouse_gui.log"` | File log |
| `advanced.max_log_size_mb` | int | `10` | Kích thước tối đa của log file (MB) |
| `advanced.sensor_timeout` | int | `10` | Timeout cho sensor data (giây) |
| `advanced.camera_retry` | int | `3` | Số lần thử lại camera |
| `advanced.mqtt_qos` | int | `0` | QoS level MQTT (0/1/2) |

---

## 📊 BẢNG MQTT TOPICS

### Topics Subscribe (GUI nhận dữ liệu)

| Topic | Payload Format | Mô Tả | Ví Dụ |
|-------|----------------|-------|-------|
| `greenhouse/data/temperature` | `{"v": float, "u": string, "t": int}` | Nhiệt độ | `{"v": 25.3, "u": "°C", "t": 1700000000}` |
| `greenhouse/data/humidity` | `{"v": float, "u": string, "t": int}` | Độ ẩm không khí | `{"v": 65.2, "u": "%", "t": 1700000000}` |
| `greenhouse/data/soilMoisture` | `{"v": float, "u": string, "t": int}` | Độ ẩm đất | `{"v": 45.8, "u": "%", "t": 1700000000}` |
| `greenhouse/data/lightIntensity` | `{"v": float, "u": string, "t": int}` | Cường độ ánh sáng | `{"v": 12500, "u": "lux", "t": 1700000000}` |
| `greenhouse/data/gasMQ3` | `{"v": float, "u": string, "t": int}` | Cảm biến gas MQ-3 | `{"v": 1.23, "u": "V", "t": 1700000000}` |
| `greenhouse/data/flameAnalog` | `{"v": int, "u": string, "t": int}` | Cảm biến lửa (analog) | `{"v": 120, "u": "", "t": 1700000000}` |
| `greenhouse/data/soundLevel` | `{"v": int, "u": string, "t": int}` | Mức độ âm thanh | `{"v": 450, "u": "", "t": 1700000000}` |
| `greenhouse/data/waterTankLevel` | `{"v": float, "u": string, "t": int}` | Mức nước trong bể | `{"v": 15.2, "u": "cm", "t": 1700000000}` |
| `greenhouse/data/gps` | `{"lat": float, "lng": float, "valid": bool}` | GPS location | `{"lat": 10.762622, "lng": 106.660172, "valid": true}` |
| `greenhouse/status/mode` | `{"mode": string}` | Mode hiện tại | `{"mode": "AUTO"}` hoặc `{"mode": "MANUAL"}` |
| `greenhouse/status/pump` | `{"state": int}` | Trạng thái pump | `{"state": 1}` (1=ON, 0=OFF) |
| `greenhouse/status/fan` | `{"state": int}` | Trạng thái fan | `{"state": 0}` |
| `greenhouse/status/auxFan` | `{"state": int}` | Trạng thái aux fan | `{"state": 0}` |
| `greenhouse/status/growLight` | `{"state": int}` | Trạng thái đèn | `{"state": 1}` |
| `greenhouse/status/window` | `{"state": int}` | Trạng thái cửa sổ | `{"state": 0}` (0=ĐÓNG, 1=MỞ) |
| `greenhouse/status/door` | `{"state": int}` | Trạng thái cửa | `{"state": 0}` |
| `greenhouse/events/flame` | `{"value": int}` | Sự kiện lửa | `{"value": 850}` |
| `greenhouse/events/sound` | `{"value": int}` | Sự kiện tiếng động | `{"value": 750}` |
| `greenhouse/events/error` | `{"msg": string}` | Lỗi hệ thống | `{"msg": "Sensor timeout"}` |

### Topics Publish (GUI gửi lệnh điều khiển)

| Topic | Payload Format | Mô Tả | Ví Dụ |
|-------|----------------|-------|-------|
| `greenhouse/control/mode` | `{"mode": string}` | Đặt mode | `{"mode": "AUTO"}` hoặc `{"mode": "MANUAL"}` |
| `greenhouse/control/pump` | `{"cmd": string}` | Điều khiển pump | `{"cmd": "ON"}` hoặc `{"cmd": "OFF"}` |
| `greenhouse/control/fan` | `{"cmd": string}` | Điều khiển fan | `{"cmd": "ON"}` hoặc `{"cmd": "OFF"}` |
| `greenhouse/control/auxFan` | `{"cmd": string}` | Điều khiển aux fan | `{"cmd": "ON"}` hoặc `{"cmd": "OFF"}` |
| `greenhouse/control/growLight` | `{"cmd": string}` | Điều khiển đèn | `{"cmd": "ON"}` hoặc `{"cmd": "OFF"}` |
| `greenhouse/control/window` | `{"cmd": string}` | Điều khiển cửa sổ | `{"cmd": "OPEN"}` hoặc `{"cmd": "CLOSE"}` |
| `greenhouse/control/door` | `{"cmd": string}` | Điều khiển cửa | `{"cmd": "OPEN"}` hoặc `{"cmd": "CLOSE"}` |
| `greenhouse/control/rgb` | `{"r": int, "g": int, "b": int}` | Điều khiển RGB LED | `{"r": 255, "g": 0, "b": 0}` (Đỏ) |

**Lưu ý:**
- `v` = value (giá trị)
- `u` = unit (đơn vị)
- `t` = timestamp (Unix timestamp)
- `cmd` = command (lệnh)

---

## 📊 BẢNG SENSOR DATA FORMAT

### Format Chuẩn cho Sensor Data

Tất cả sensor data theo format:
```json
{
  "v": <giá trị>,
  "u": "<đơn vị>",
  "t": <timestamp>
}
```

### Chi Tiết Từng Sensor

| Sensor | Key | Giá Trị (v) | Đơn Vị (u) | Min | Max | Mô Tả |
|--------|-----|-------------|------------|-----|-----|-------|
| Nhiệt độ | `temperature` | float | `°C` | -40 | 85 | DHT11 sensor |
| Độ ẩm | `humidity` | float | `%` | 0 | 100 | DHT11 sensor |
| Độ ẩm đất | `soilMoisture` | float | `%` | 0 | 100 | Capacitive soil sensor |
| Ánh sáng | `lightIntensity` | float | `lux` | 0 | 65535 | BH1750 sensor |
| Gas MQ-3 | `gasMQ3` | float | `V` | 0 | 5 | Analog voltage |
| Lửa analog | `flameAnalog` | int | - | 0 | 1023 | Analog value |
| Âm thanh | `soundLevel` | int | - | 0 | 1023 | Analog value |
| Mức nước | `waterTankLevel` | float | `cm` | 0 | 400 | Ultrasonic HC-SR04 |

### GPS Data Format

```json
{
  "lat": <latitude>,
  "lng": <longitude>,
  "valid": <boolean>
}
```

**Ví dụ:**
```json
{
  "lat": 10.762622,
  "lng": 106.660172,
  "valid": true
}
```

### Device Status Format

```json
{
  "state": <0 hoặc 1>
}
```

- `0` = OFF/CLOSED (Tắt/Đóng)
- `1` = ON/OPEN (Bật/Mở)

---

## 📊 BẢNG TROUBLESHOOTING

### Lỗi Kết Nối

| Vấn Đề | Triệu Chứng | Nguyên Nhân | Giải Pháp |
|--------|-------------|-------------|-----------|
| MQTT không kết nối | Status bar: "✗ MQTT" | - Broker offline<br>- Sai địa chỉ<br>- Firewall chặn | 1. Ping broker: `ping broker.hivemq.com`<br>2. Thử broker khác<br>3. Kiểm tra firewall port 1883 |
| Camera không hiển thị | "Camera Offline" | - ESP32-CAM tắt<br>- Sai IP<br>- Mạng không kết nối | 1. Kiểm tra ESP32-CAM đã bật<br>2. Test: `http://192.168.4.1/capture`<br>3. Ping IP camera |
| Sensor không update | Giá trị "--" hoặc cũ | - ESP8266 không publish<br>- MQTT disconnect<br>- Topic sai | 1. Kiểm tra MQTT connected<br>2. Subscribe test: `mosquitto_sub -h broker.hivemq.com -t "greenhouse/#"`<br>3. Check firmware topics |
| Charts không vẽ | Graph trống | - Chưa có dữ liệu<br>- Format sai<br>- Lỗi pyqtgraph | 1. Đợi 2-3 giây<br>2. Check console logs<br>3. Restart app |

### Lỗi Performance

| Vấn Đề | Triệu Chứng | Nguyên Nhân | Giải Pháp |
|--------|-------------|-------------|-----------|
| GUI chậm/lag | Giao diện đơ, chậm | - YOLO dùng nhiều CPU<br>- FPS quá cao<br>- Chart quá nhiều data | 1. Tắt YOLO detection<br>2. Giảm FPS: `camera.fps = 5`<br>3. Giảm history: `charts.history_size = 50` |
| CPU usage cao | Quạt chạy ồn, máy nóng | - YOLO model lớn<br>- Camera FPS cao | 1. Dùng yolov8n (nano)<br>2. Giảm FPS xuống 5-10<br>3. Tăng update_interval |
| Memory leak | RAM tăng dần | - Events không clear<br>- Chart history lớn | 1. Clear alerts thường xuyên<br>2. Giảm `alerts.max_log_entries`<br>3. Giảm `charts.history_size` |

### Lỗi Cài Đặt

| Vấn Đề | Triệu Chứng | Nguyên Nhân | Giải Pháp |
|--------|-------------|-------------|-----------|
| ModuleNotFoundError | `No module named 'PyQt6'` | Chưa cài dependencies | `pip install -r requirements.txt` |
| YOLO không hoạt động | "YOLO not available" | - Chưa cài ultralytics<br>- Chưa có model file | 1. `pip install ultralytics`<br>2. Download model: `yolo task=detect mode=predict model=yolov8n.pt source='https://ultralytics.com/images/bus.jpg'` |
| QT plugin error | Cannot load xcb/wayland | Thiếu Qt dependencies | Ubuntu: `sudo apt install libxcb-xinerama0`<br>Arch: `sudo pacman -S qt6-wayland` |

### Lỗi Cấu Hình

| Vấn Đề | Triệu Chứng | Nguyên Nhân | Giải Pháp |
|--------|-------------|-------------|-----------|
| Config không load | Dùng default config | - File JSON lỗi<br>- File không tồn tại | 1. Xóa `config.json`<br>2. Restart app (tạo file mới) |
| Validation failed | Errors khi validate | - Giá trị không hợp lệ<br>- Type sai | 1. Check console logs<br>2. `config.reset_to_defaults()` |
| CSV file lỗi | Export failed | - Không có quyền ghi<br>- Disk đầy | 1. Check permissions: `ls -la data/`<br>2. Giải phóng dung lượng |

### Lỗi Network

| Vấn Đề | Triệu Chứng | Nguyên Nhân | Giải Pháp |
|--------|-------------|-------------|-----------|
| Timeout | Connection timeout | - Mạng chậm<br>- Server xa | 1. Tăng timeout: `camera.timeout = 10`<br>2. Dùng server gần hơn |
| Disconnected thường xuyên | MQTT mất kết nối liên tục | - Mạng không ổn định<br>- Broker quá tải | 1. Tăng keepalive: `mqtt.keepalive = 120`<br>2. Enable auto_reconnect<br>3. Dùng broker khác |
| Camera stream chậm | FPS thấp, giật lag | - Bandwidth thấp<br>- ESP32-CAM xa | 1. Giảm resolution trên ESP32<br>2. Giảm FPS<br>3. Đến gần router |

---

## 📊 BẢNG SO SÁNH YOLO MODELS

### Hiệu Năng và Độ Chính Xác

| Model | Kích Thước | Tốc Độ (CPU) | Tốc Độ (GPU) | mAP | Sử Dụng Cho |
|-------|------------|--------------|--------------|-----|-------------|
| **yolov8n.pt** | 6.2 MB | ~100ms/frame | ~5ms/frame | 37.3 | ✅ **Khuyến nghị** - Máy yếu, laptop |
| **yolov8s.pt** | 21.5 MB | ~200ms/frame | ~8ms/frame | 44.9 | Cân bằng tốc độ/độ chính xác |
| **yolov8m.pt** | 49.7 MB | ~500ms/frame | ~12ms/frame | 50.2 | Máy mạnh, cần độ chính xác cao |
| **yolov8l.pt** | 83.7 MB | ~800ms/frame | ~18ms/frame | 52.9 | Workstation, offline processing |
| **yolov8x.pt** | 130.5 MB | ~1200ms/frame | ~25ms/frame | 53.9 | Research, batch processing |

### Yêu Cầu Hệ Thống

| Model | RAM | CPU Cores | GPU VRAM | Disk Space |
|-------|-----|-----------|----------|------------|
| yolov8n | 2 GB | 2+ | 1 GB | 100 MB |
| yolov8s | 4 GB | 4+ | 2 GB | 200 MB |
| yolov8m | 8 GB | 4+ | 4 GB | 500 MB |
| yolov8l | 16 GB | 8+ | 6 GB | 1 GB |
| yolov8x | 32 GB | 8+ | 8 GB | 2 GB |

### Khuyến Nghị Sử Dụng

| Tình Huống | Model Khuyến Nghị | FPS Khuyến Nghị | Ghi Chú |
|------------|-------------------|-----------------|---------|
| Laptop cơ bản | yolov8n | 5 FPS | Tắt YOLO khi không cần |
| Laptop gaming | yolov8s | 10 FPS | Bật GPU nếu có |
| Desktop | yolov8m | 15 FPS | Sử dụng GPU |
| Workstation | yolov8l | 20 FPS | GPU với CUDA |
| Server | yolov8x | 30 FPS | Multiple GPUs |

### Cấu Hình YOLO

```python
# Trong config.json
{
  "camera": {
    "yolo_model": "yolov8n.pt",  # ← Thay đổi model ở đây
    "yolo_confidence": 0.25,     # ← Ngưỡng confidence
    "yolo_iou": 0.45             # ← Ngưỡng IOU cho NMS
  }
}
```

**Confidence Threshold:**
- `0.1` - Nhiều detection, nhiều false positives
- `0.25` - ✅ **Khuyến nghị** - Cân bằng
- `0.5` - Ít detection, ít false positives
- `0.7` - Rất ít detection, chính xác cao

**IOU Threshold:**
- `0.3` - Loại bỏ nhiều overlapping boxes
- `0.45` - ✅ **Khuyến nghị** - Cân bằng
- `0.7` - Giữ nhiều overlapping boxes

---

## 📊 BẢNG PHÍM TẮT

### Phím Tắt Global

| Phím | Chức Năng | Mô Tả |
|------|-----------|-------|
| `Ctrl+Q` | Thoát | Đóng ứng dụng |
| `Ctrl+R` | Refresh | Làm mới tất cả dữ liệu |
| `Ctrl+S` | Save Config | Lưu cấu hình hiện tại |
| `Ctrl+,` | Settings | Mở dialog cài đặt |
| `F11` | Fullscreen | Chế độ toàn màn hình |
| `F5` | Reload | Reload camera stream |

### Phím Tắt Tab-Specific

| Tab | Phím | Chức Năng |
|-----|------|-----------|
| Dashboard | `Ctrl+D` | Chuyển đến Dashboard |
| Control | `Ctrl+C` | Chuyển đến Control |
| Camera | `Ctrl+M` | Chuyển đến Camera |
| Charts | `Ctrl+G` | Chuyển đến Charts |
| Alerts | `Ctrl+A` | Chuyển đến Alerts |

### Phím Tắt Control Tab

| Phím | Chức Năng |
|------|-----------|
| `Ctrl+1` | Toggle AUTO/MANUAL mode |
| `Ctrl+P` | Toggle Pump |
| `Ctrl+F` | Toggle Fan |
| `Ctrl+L` | Toggle Light |

### Phím Tắt Camera Tab

| Phím | Chức Năng |
|------|-----------|
| `Space` | Capture photo |
| `Y` | Toggle YOLO detection |
| `Ctrl+O` | Open captures folder |

### Phím Tắt Alerts Tab

| Phím | Chức Năng |
|------|-----------|
| `Delete` | Clear all alerts |
| `Ctrl+E` | Export alerts to CSV |

---

## 📊 BẢNG QUY TRÌNH SỬ DỤNG

### Quy Trình Khởi Động Lần Đầu

| Bước | Hành Động | Chi Tiết | Thời Gian |
|------|-----------|----------|-----------|
| 1 | Cài Python | `sudo apt install python3 python3-pip` | 2 phút |
| 2 | Clone repo | `git clone https://github.com/c1k3nx/khkt-bao.git` | 1 phút |
| 3 | Vào thư mục | `cd khkt-bao/pc-vision` | - |
| 4 | Tạo venv | `python3 -m venv venv` | 1 phút |
| 5 | Activate venv | `source venv/bin/activate` | - |
| 6 | Cài dependencies | `pip install -r requirements.txt` | 5 phút |
| 7 | Download YOLO | `python -c "from ultralytics import YOLO; YOLO('yolov8n.pt')"` | 2 phút |
| 8 | Chạy GUI | `python greenhouse_gui.py` | - |
| **Tổng** | | | **~11 phút** |

### Quy Trình Sử Dụng Hàng Ngày

| Bước | Hành Động | Lệnh |
|------|-----------|------|
| 1 | Mở terminal | - |
| 2 | Vào thư mục | `cd khkt-bao/pc-vision` |
| 3 | Activate venv | `source venv/bin/activate` |
| 4 | Chạy GUI | `./run_gui.sh` hoặc `python greenhouse_gui.py` |

### Quy Trình Cấu Hình MQTT

| Bước | Hành Động | Chi Tiết |
|------|-----------|----------|
| 1 | Mở file config | `config.json` |
| 2 | Tìm section `mqtt` | |
| 3 | Đổi broker | `"broker": "your-broker.com"` |
| 4 | Đổi port (nếu cần) | `"port": 1883` |
| 5 | Lưu file | Ctrl+S |
| 6 | Restart GUI | Để áp dụng thay đổi |

### Quy Trình Cấu Hình Camera

| Bước | Hành Động | Chi Tiết |
|------|-----------|----------|
| 1 | Kết nối ESP32-CAM | Bật camera, kết nối WiFi |
| 2 | Lấy IP address | Check serial monitor hoặc router |
| 3 | Test camera | `http://[IP]/capture` trong browser |
| 4 | Mở file config | `config.json` |
| 5 | Đổi camera URL | `"camera": {"url": "http://[IP]"}` |
| 6 | Lưu và restart GUI | |

### Quy Trình Export Dữ Liệu

| Bước | Hành Động | Kết Quả |
|------|-----------|---------|
| 1 | Bật CSV export | `config.set("export.enable_csv", true)` |
| 2 | Chạy GUI | Dữ liệu tự động ghi |
| 3 | Kiểm tra file | `ls data/` |
| 4 | Mở file CSV | `libreoffice data/greenhouse_data_*.csv` |
| 5 | Phân tích dữ liệu | Excel, Python pandas, etc. |

### Quy Trình Troubleshooting

| Bước | Hành Động | Ghi Chú |
|------|-----------|---------|
| 1 | Đọc console logs | Terminal output |
| 2 | Check file logs | `greenhouse_gui.log` |
| 3 | Test MQTT | `mosquitto_sub -h broker -t "greenhouse/#"` |
| 4 | Test camera | Browser: `http://[IP]/capture` |
| 5 | Validate config | `config.validate()` |
| 6 | Reset config | `config.reset_to_defaults()` |
| 7 | Reinstall deps | `pip install --force-reinstall -r requirements.txt` |
| 8 | Check GitHub Issues | https://github.com/c1k3nx/khkt-bao/issues |

---

## 📚 PHỤ LỤC

### A. Đơn Vị Đo Lường

| Đại Lượng | Đơn Vị | Ký Hiệu | Giải Thích |
|-----------|--------|---------|------------|
| Nhiệt độ | Độ C | °C | Celsius, thang nhiệt độ phổ biến |
| Độ ẩm | Phần trăm | % | Độ ẩm tương đối (0-100%) |
| Ánh sáng | Lux | lux | Cường độ ánh sáng |
| Điện áp | Volt | V | Đơn vị điện áp |
| Khoảng cách | Centimet | cm | Đo mức nước |

### B. Thuật Ngữ Viết Tắt

| Viết Tắt | Đầy Đủ | Giải Thích |
|----------|--------|------------|
| MQTT | Message Queuing Telemetry Transport | Giao thức IoT pub/sub |
| YOLO | You Only Look Once | Thuật toán detect object |
| GUI | Graphical User Interface | Giao diện đồ họa |
| CSV | Comma-Separated Values | Format file dữ liệu |
| FPS | Frames Per Second | Số khung hình/giây |
| QoS | Quality of Service | Chất lượng dịch vụ MQTT |
| JSON | JavaScript Object Notation | Format dữ liệu |
| API | Application Programming Interface | Giao diện lập trình |
| RGB | Red Green Blue | Màu sắc (Đỏ Xanh Lá Xanh Dương) |

### C. Giá Trị Khuyến Nghị

| Tham Số | Laptop Yếu | Laptop Trung Bình | Desktop Mạnh |
|---------|------------|-------------------|--------------|
| Camera FPS | 5 | 10 | 15-20 |
| YOLO Model | yolov8n | yolov8s | yolov8m |
| Chart History | 50 | 100 | 200 |
| Update Interval | 5000ms | 2000ms | 1000ms |

---

**🌱 Tài liệu v1.4 - COMPREHENSIVE OVERHAUL**

**Cập nhật:** 2024-11-23
