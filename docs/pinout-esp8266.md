# ESP8266 Pinout (NodeMCU/WeMos)

## Bảng chân đầy đủ

| GPIO | NodeMCU Pin | Chức năng | Kết nối | Ghi chú |
|------|-------------|-----------|---------|---------|
| **UART0** (Hardware Serial) |
| GPIO1 | TX | UART0 TX | → UNO D0 (RX) | **Bridge với UNO**, 3.3V logic |
| GPIO3 | RX | UART0 RX | ← UNO D1 (TX) | **Cần chia áp từ UNO 5V!** |
| **GPS** (SoftwareSerial) |
| GPIO12 | D6 | GPS RX | ← NEO-6 TX | SoftwareSerial, 9600 baud |
| GPIO14 | D5 | GPS TX | → NEO-6 RX | SoftwareSerial |
| **WS2812B LED Rings** |
| GPIO2 | D4 | LED Ring 1 | → WS2812B DIN (Ring 1) | 8 LEDs, 800kHz |
| GPIO5 | D1 | LED Ring 2 | → WS2812B DIN (Ring 2) | 8 LEDs, 800kHz |
| **Reserved/Available** |
| GPIO4 | D2 | I/O (dự phòng) | - | Có thể dùng cho mở rộng |
| GPIO15 | D8 | I/O (dự phòng) | - | Pulldown khi boot! |
| **Power** |
| 3.3V | 3V3 | Power out | Sensor 3.3V | Max 500mA |
| 5V | VIN | Power in | USB 5V | Từ USB hoặc nguồn ngoài |
| GND | GND | Ground | Chung với UNO, GPS, LED | **BẮT BUỘC chung** |

## GPIO Mapping (NodeMCU)

NodeMCU có mapping riêng:

| NodeMCU Label | GPIO Number | Arduino Pin |
|---------------|-------------|-------------|
| D0 | GPIO16 | 16 |
| D1 | GPIO5 | 5 |
| D2 | GPIO4 | 4 |
| D3 | GPIO0 | 0 (boot mode) |
| D4 | GPIO2 | 2 (boot mode, onboard LED) |
| D5 | GPIO14 | 14 |
| D6 | GPIO12 | 12 |
| D7 | GPIO13 | 13 |
| D8 | GPIO15 | 15 (boot mode, pulldown!) |
| TX | GPIO1 | 1 (UART0 TX) |
| RX | GPIO3 | 3 (UART0 RX) |

**QUAN TRỌNG**: Trong code Arduino, dùng GPIO number (không phải D-number)!

```cpp
#define LED_RING1_PIN 2   // GPIO2 = D4
#define LED_RING2_PIN 5   // GPIO5 = D1
#define GPS_RX 12         // GPIO12 = D6
#define GPS_TX 14         // GPIO14 = D5
```

## Sơ đồ kết nối chi tiết

### UART với Arduino UNO

```
ESP8266 GPIO1 (TX) ────────────────────────── UNO D0 (RX)
         3.3V logic                             5V tolerant

ESP8266 GPIO3 (RX) ──┬── 1kΩ ──┬────────────── UNO D1 (TX)
         3.3V max    │         │                5V output
                     └─ 2kΩ ───┴─ GND

ESP8266 GND ──────────────────────────────── UNO GND
```

**Giải thích chia áp**:
- UNO TX xuất 5V (quá mức cho ESP8266 RX 3.3V max)
- Voltage divider: Vout = 5V × (2kΩ / 3kΩ) = 3.33V ✓
- ESP8266 TX (3.3V) → UNO RX: OK (UNO chấp nhận 3.3V)

### GPS NEO-6

```
NEO-6 VCC ─── ESP8266 3.3V (hoặc 5V, module có regulator)
NEO-6 TX ─── ESP8266 GPIO12 (SoftwareSerial RX)
NEO-6 RX ─── ESP8266 GPIO14 (SoftwareSerial TX)
NEO-6 GND ─── ESP8266 GND
```

**Lưu ý**:
- NEO-6 output 3.3V logic → trực tiếp vào ESP8266 OK
- Baud rate: 9600 (NMEA standard)
- Cần ăng-ten hoặc ngoài trời để bắt tín hiệu GPS

### WS2812B LED Rings

#### Ring 1

```
Ring1 VCC ─── 5V (riêng, không qua ESP8266)
Ring1 GND ─── GND chung
Ring1 DIN ─── ESP8266 GPIO2 (D4)
       └─── (có thể cần level shifter 3.3V→5V cho tín hiệu ổn định)
```

#### Ring 2

```
Ring2 VCC ─── 5V (riêng)
Ring2 GND ─── GND chung
Ring2 DIN ─── ESP8266 GPIO5 (D1)
```

**QUAN TRỌNG**:
- WS2812B cần nguồn 5V ổn định
- Mỗi LED: ~60mA max (white full brightness)
- 8 LEDs × 2 rings × 60mA = 960mA → Nguồn 5V/1.5A khuyến nghị
- Tín hiệu 3.3V từ ESP8266 **có thể hoạt động** với WS2812B, nhưng level shifter 3.3V→5V tốt hơn
- Nối capacitor 100-1000µF giữa VCC và GND (gần LED)
- Nối điện trở 330Ω nối tiếp với DIN (bảo vệ)

### Level Shifter (tùy chọn, khuyến nghị cho WS2812B)

```
ESP8266 3.3V ─── LV (74AHCT125 hoặc BS170)
5V ─── HV
ESP8266 GPIO2/5 ─── LV input
WS2812B DIN ─── HV output
GND ─── GND
```

## Boot Mode Pins (QUAN TRỌNG!)

ESP8266 boot mode được quyết định bởi GPIO0, GPIO2, GPIO15:

| GPIO0 | GPIO2 | GPIO15 | Mode |
|-------|-------|--------|------|
| HIGH | HIGH | LOW | Normal boot (Flash) |
| LOW | HIGH | LOW | UART bootloader (upload) |

**Trong dự án này**:
- GPIO2 (LED Ring 1): Kéo HIGH khi boot bằng pullup 10kΩ (đã có trên NodeMCU)
- GPIO15: Kéo LOW khi boot bằng pulldown 10kΩ (đã có trên NodeMCU)
- GPIO0: Không dùng trong dự án (để tự do cho boot mode)

**Lưu ý**: GPIO2 là chân onboard LED trên NodeMCU (LOW=ON), có thể nhấp nháy khi LED ring hoạt động.

## Nguồn điện

### Cấp nguồn cho ESP8266

**Tùy chọn 1**: USB Micro (5V)
- NodeMCU có voltage regulator AMS1117 (5V→3.3V)
- Max 800mA (sau khi trừ regulator loss)
- Phù hợp cho test

**Tùy chọn 2**: VIN pin (5V external)
- Vào qua AMS1117
- Cùng giới hạn 800mA

**Tùy chọn 3**: 3.3V pin (trực tiếp)
- Bypass regulator
- Phải là nguồn 3.3V ổn định
- Max 500mA

### Phân bố nguồn

| Module | Dòng điện | Nguồn |
|--------|-----------|-------|
| ESP8266 (Wi-Fi active) | 170-300mA | - |
| NEO-6 GPS | 45mA | ESP 3.3V |
| WS2812B × 16 LEDs | 960mA (max) | **5V riêng** |

**Khuyến nghị**:
- ESP8266: USB 5V/1A (hoặc VIN)
- WS2812B: Nguồn 5V/2A riêng
- **GND chung**

## Wi-Fi Configuration

Trong `firmware-esp8266/greenhouse_esp8266.ino`:

```cpp
const char* WIFI_SSID = "YourWiFiSSID";
const char* WIFI_PASS = "YourWiFiPassword";
```

**Fallback**: Nếu STA fail, ESP8266 sẽ mở SoftAP:
- SSID: `ESP32-CAM-GH`
- Password: `12345678`
- IP: `192.168.4.1`

## MQTT Configuration

```cpp
const char* MQTT_BROKER = "broker.hivemq.com";
const int MQTT_PORT = 1883;
const char* MQTT_CLIENT_ID = "gh-esp8266";
```

## Code Configuration

Kiểm tra trong `firmware-esp8266/greenhouse_esp8266.ino`:

```cpp
// UART with UNO
#define UART_BAUD 57600

// GPS
#define GPS_RX 12  // GPIO12 = D6
#define GPS_TX 14  // GPIO14 = D5
#define GPS_BAUD 9600

// WS2812
#define LED_RING1_PIN 2  // GPIO2 = D4
#define LED_RING2_PIN 5  // GPIO5 = D1
#define LED_COUNT 8
```

## Flashing Firmware

### Arduino IDE

1. **Cài đặt ESP8266 board support**:
   - File → Preferences
   - Additional Boards Manager URLs:
     ```
     http://arduino.esp8266.com/stable/package_esp8266com_index.json
     ```
   - Tools → Board → Boards Manager → "ESP8266" → Install

2. **Chọn board**:
   - Board: "NodeMCU 1.0 (ESP-12E Module)"
   - Upload Speed: 115200
   - CPU Frequency: 80MHz
   - Flash Size: "4MB (FS:2MB OTA:~1019KB)"
   - Port: COM port tương ứng

3. **Upload**:
   - Nối USB
   - Không cần nhấn nút FLASH (NodeMCU tự động)
   - Click Upload

### PlatformIO

```ini
[env:nodemcuv2]
platform = espressif8266
board = nodemcuv2
framework = arduino
upload_speed = 115200
monitor_speed = 57600  # Để xem UART log với UNO
lib_deps =
    ESP8266WiFi
    PubSubClient
    ArduinoJson
    Adafruit NeoPixel
    TinyGPSPlus
```

## Test Checklist

- [ ] Wi-Fi: ESP8266 connect và lấy IP (xem Serial Monitor)
- [ ] MQTT: Connect broker thành công
- [ ] UART: Nhận JSON từ UNO (57600 baud)
- [ ] GPS: Nhận NMEA sentences (9600 baud)
- [ ] LED Ring 1: Đổi màu khi gửi command MQTT
- [ ] LED Ring 2: Đổi màu khi gửi command MQTT
- [ ] Publish sensor: Kiểm tra MQTT explorer có nhận topic `gh/sensor/*`
- [ ] Subscribe command: Gửi `gh/cmd/relay/fan` → UNO relay bật

## Troubleshooting

### ESP8266 không connect Wi-Fi

1. Kiểm tra SSID/Password đúng chưa (case-sensitive!)
2. Kiểm tra Wi-Fi 2.4GHz (ESP8266 không hỗ trợ 5GHz)
3. Thử reset Wi-Fi credentials: `WiFi.disconnect(true);`
4. Kiểm tra signal strength: `WiFi.RSSI()`

### UART không nhận được data từ UNO

1. Kiểm tra chia áp đúng chưa
2. GND chung chưa
3. Baud rate giống nhau (57600)
4. Thử đổi chỗ RX/TX (nếu nhầm)
5. Xem Serial Monitor của UNO để đảm bảo UNO đang gửi data

### GPS không có tín hiệu

1. GPS cần ăng-ten ceramic (đã có trên module)
2. **BẮT BUỘC** ra ngoài trời hoặc gần cửa sổ
3. Đợi 1-5 phút để cold start
4. Kiểm tra baud rate: 9600 (default NEO-6)
5. Xem raw NMEA: `Serial.println(gpsSerial.read())`

### WS2812B không sáng

1. Nguồn 5V đủ mạnh chưa (>1A)
2. GND chung chưa
3. Nối đúng DIN (không phải DOUT)
4. Thử test code đơn giản: `NeoPixel.setPixelColor(0, 255, 0, 0); NeoPixel.show();`
5. Kiểm tra thứ tự GRB hay RGB (WS2812B là GRB)
6. Thử level shifter 3.3V→5V

### MQTT disconnect liên tục

1. Kiểm tra Wi-Fi ổn định
2. Tăng keepalive: `mqtt.setKeepAlive(60)`
3. Tăng buffer: `mqtt.setBufferSize(512)`
4. Thử broker khác (test.mosquitto.org)
5. Kiểm tra firewall không chặn port 1883

### ESP8266 crash/restart liên tục

1. Nguồn không đủ mạnh → dùng USB tốt (>500mA)
2. Watchdog timeout → tăng `yield()` trong loop
3. Stack overflow → giảm biến local, dùng heap
4. Xem exception decoder với stack trace

## Pin Safety

### Pins tránh khi boot

- **GPIO0**: Kéo LOW = boot vào bootloader
- **GPIO2**: Phải HIGH khi boot (OK vì LED ring có pullup)
- **GPIO15**: Phải LOW khi boot (OK vì module có pulldown)

### Pins an toàn

- GPIO4, GPIO5, GPIO12, GPIO13, GPIO14: An toàn, dùng thoải mái
- GPIO1/GPIO3 (UART0): Dùng cho bridge UNO (không dùng cho debug log nữa!)

### Debug logging

Vì UART0 dùng cho bridge UNO, không thể dùng `Serial.println()` để debug!

**Workaround**:
1. Dùng UART1 (TX only, GPIO2): `Serial1.begin(115200); Serial1.println("Debug");`
2. Hoặc publish debug message lên MQTT: `mqtt.publish("gh/debug", msg)`

## Schematic Symbol

```
         ESP8266 (NodeMCU)
    ┌─────────────────────────┐
    │  ┌───┐                  │
    │  │USB│                  │
    │  └─┬─┘                  │
    │    │ 5V                 │
    │ [ESP8266EX]             │
    │                         │
    │ GPIO1 (TX) → UNO RX     │
    │ GPIO3 (RX) ← UNO TX     │
    │                         │
    │ GPIO12 ← NEO-6 TX       │
    │ GPIO14 → NEO-6 RX       │
    │                         │
    │ GPIO2 → LED Ring1 DIN   │
    │ GPIO5 → LED Ring2 DIN   │
    │                         │
    │ GPIO4 (reserve)         │
    │ GPIO15 (reserve)        │
    │                         │
    │ 3.3V ─────●─────────────┤
    │ 5V ───────┼────●────────┤
    │ GND ──────●────●────────┤
    └─────────────────────────┘
            │    │    │
            │    │    └─ LED 5V
            │    └─ NEO-6 VCC
            └─ GND chung
```

## Advanced: OTA Updates

Để enable OTA (Over-The-Air) firmware updates:

```cpp
#include <ArduinoOTA.h>

void setup() {
  // ... existing setup ...

  ArduinoOTA.setHostname("gh-esp8266");
  ArduinoOTA.setPassword("admin");
  ArduinoOTA.begin();
}

void loop() {
  ArduinoOTA.handle();
  // ... existing loop ...
}
```

Upload qua Wi-Fi:
- Tools → Port → "gh-esp8266 at 192.168.x.x"
- Upload như bình thường

## Reference

- [ESP8266 Arduino Core Docs](https://arduino-esp8266.readthedocs.io/)
- [NodeMCU Pinout](https://circuits4you.com/2017/12/31/nodemcu-pinout/)
- [WS2812B Datasheet](https://cdn-shop.adafruit.com/datasheets/WS2812B.pdf)
- [NEO-6 GPS Manual](https://www.u-blox.com/sites/default/files/products/documents/NEO-6_DataSheet_(GPS.G6-HW-09005).pdf)
