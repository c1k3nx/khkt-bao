# Arduino UNO R3 Pinout

## Bảng chân đầy đủ

| Chân | Chức năng | Kết nối | Ghi chú |
|------|-----------|---------|---------|
| **Digital Pins** |
| D0 | UART RX | ← ESP8266 TX (GPIO1) | **UART bridge**, cần chia áp 5V→3.3V |
| D1 | UART TX | → ESP8266 RX (GPIO3) | **UART bridge**, chia áp! |
| D2 | DHT11 Data | DHT11 DATA pin | Pullup 10kΩ (nếu cần) |
| D3 | JSN-SR04T Echo | JSN ECHO | Có thể dùng interrupt |
| D4 | Relay Pump | Relay IN1 | Active LOW/HIGH (cấu hình) |
| D5 | Servo Door (MG90) | Servo signal (yellow) | 180° servo |
| D7 | Relay Fan Main | Relay IN2 | Active LOW/HIGH |
| D8 | Relay Light 12V | Relay IN3 | Active LOW/HIGH |
| D9 | Servo Window (MG996R) | Servo signal (orange) | 180° servo, cần nguồn riêng |
| D10 | DFPlayer RX | ← DFPlayer TX | SoftwareSerial |
| D11 | DFPlayer TX | → DFPlayer RX | SoftwareSerial |
| D12 | JSN-SR04T Trig | JSN TRIG | Output pulse |
| D13 | Relay Aux Fan | Relay IN4 | Active LOW/HIGH + onboard LED |
| **Analog Pins** |
| A0 | Flame Sensor | Flame AO | 0-1023, cao = phát hiện lửa |
| A1 | Sound Sensor | Sound AO | 0-1023, cao = âm thanh lớn |
| A2 | Soil Moisture | Soil AO | 0-1023, 0=ướt, 1023=khô |
| A3 | MQ-3 Alcohol | MQ-3 AO | 0-1023, cao = có alcohol |
| A4 | I²C SDA | BH1750 SDA + LCD SDA | Pullup 4.7kΩ (có sẵn) |
| A5 | I²C SCL | BH1750 SCL + LCD SCL | Pullup 4.7kΩ (có sẵn) |
| **Power** |
| 5V | Power out | Sensor power | Max 200mA |
| 3.3V | Power out | Không dùng | Yếu, không đủ cho nhiều sensor |
| GND | Ground | Chung với tất cả module | **BẮT BUỘC nối chung** |
| VIN | Power in | 7-12V DC | Nếu dùng DC jack |

## Sơ đồ kết nối chi tiết

### UART với ESP8266

```
UNO D0 (RX) ────────────────────────────────── ESP8266 GPIO1 (TX)
                                                 3.3V logic

UNO D1 (TX) ──┬── 1kΩ ──┬─────────────────────── ESP8266 GPIO3 (RX)
              │         │
              └─ 2kΩ ───┴─ GND

UNO GND ────────────────────────────────────── ESP8266 GND
```

**QUAN TRỌNG**: UNO TX (5V) phải chia áp xuống 3.3V trước khi vào ESP8266 RX!

### DHT11

```
DHT11 VCC ─── UNO 5V
DHT11 DATA ─── UNO D2 (có thể cần pullup 10kΩ lên 5V)
DHT11 GND ─── UNO GND
```

### BH1750 (I²C)

```
BH1750 VCC ─── UNO 5V (hoặc 3.3V, chip chấp nhận cả hai)
BH1750 SDA ─── UNO A4 (có pullup internal)
BH1750 SCL ─── UNO A5 (có pullup internal)
BH1750 GND ─── UNO GND
BH1750 ADDR ─── GND (address 0x23) hoặc VCC (address 0x5C)
```

### LCD1602 I²C

```
LCD VCC ─── UNO 5V
LCD SDA ─── UNO A4 (cùng bus với BH1750)
LCD SCL ─── UNO A5 (cùng bus với BH1750)
LCD GND ─── UNO GND
```

**Address mặc định**: 0x27 (hoặc 0x3F, scan bằng I2C Scanner)

### JSN-SR04T (Ultrasonic)

```
JSN VCC ─── UNO 5V
JSN TRIG ─── UNO D12
JSN ECHO ─── UNO D3
JSN GND ─── UNO GND
```

**Mode**: Mode 2 (automatic, không cần trigger liên tục)

### MQ-3 (Alcohol)

```
MQ-3 VCC ─── UNO 5V
MQ-3 AO ─── UNO A3
MQ-3 GND ─── UNO GND
```

**Lưu ý**: Cần preheat 24-48h để ổn định!

### Flame Sensor (Analog)

```
Flame VCC ─── UNO 5V
Flame AO ─── UNO A0
Flame GND ─── UNO GND
```

**Logic**: Giá trị cao (>800) = phát hiện lửa

### Sound Sensor (Analog)

```
Sound VCC ─── UNO 5V
Sound AO ─── UNO A1
Sound GND ─── UNO GND
```

**Logic**: Giá trị cao (>700) = âm thanh lớn

### Soil Moisture (Analog)

```
Soil VCC ─── UNO 5V
Soil AO ─── UNO A2
Soil GND ─── UNO GND
```

**Logic**: 0=ướt, 1023=khô → chuyển sang % trong code

### Relay Module 4 kênh

```
Relay VCC ─── 5V (riêng, không qua UNO nếu relay có optoisolated)
Relay GND ─── GND chung
Relay IN1 ─── UNO D4 (Pump)
Relay IN2 ─── UNO D7 (Fan Main)
Relay IN3 ─── UNO D8 (Light 12V)
Relay IN4 ─── UNO D13 (Aux Fan)

Relay COM1/2/3/4 ─── 12V+ hoặc AC L
Relay NO1/2/3/4 ─── Load (pump, fan, light...)
```

**Lưu ý**:
- Kiểm tra relay là Active LOW hay HIGH
- Nối diode 1N4007 ngược với coil nếu relay không có bảo vệ
- Nguồn 12V riêng cho load, GND chung

### Servo (MG996R - Window, MG90 - Door)

```
Servo Window (MG996R):
  Brown ─── GND (nguồn riêng 5V/2A khuyến nghị)
  Red ─── 5V (nguồn riêng)
  Orange ─── UNO D9

Servo Door (MG90):
  Brown ─── GND
  Red ─── 5V
  Yellow ─── UNO D5
```

**QUAN TRỌNG**: Servo tiêu thụ dòng cao khi hoạt động!
- MG996R: 500mA-2.5A stall
- MG90: 200mA-650mA stall
- **BẮT BUỘC** dùng nguồn 5V riêng (>2A), **KHÔNG** lấy từ UNO 5V pin!
- GND phải nối chung với UNO

### DFPlayer Mini

```
DFPlayer VCC ─── UNO 5V (qua capacitor 100µF nếu có)
DFPlayer GND ─── UNO GND
DFPlayer TX ─── UNO D10 (SoftwareSerial RX)
DFPlayer RX ─── UNO D11 (SoftwareSerial TX, cần điện trở 1kΩ nối tiếp)
DFPlayer SPK1 ─── Speaker +
DFPlayer SPK2 ─── Speaker -
```

**Thẻ TF**: Format FAT32, tạo thư mục `/01`, đặt file `001.mp3`, `002.mp3`, ...

**Lưu ý**: Nếu DFPlayer không phát, thử nối điện trở 1kΩ nối tiếp với RX!

## Nguồn điện

### Cấp nguồn cho UNO

**Tùy chọn 1**: USB 5V (max 500mA)
- Phù hợp khi test, không nối servo/relay

**Tùy chọn 2**: DC Jack 7-12V/2A
- Khuyến nghị: 9V/2A
- Voltage regulator trên board sẽ hạ xuống 5V
- Có thể cấp cho cả servo (nếu không quá tải)

**Tùy chọn 3**: VIN pin (7-12V)
- Giống DC jack, vào qua voltage regulator

### Phân bố nguồn (ước lượng)

| Module | Dòng điện | Nguồn |
|--------|-----------|-------|
| UNO R3 | 50mA | - |
| DHT11 | 2.5mA | UNO 5V |
| BH1750 | 0.12mA | UNO 5V |
| LCD1602 | 20-80mA (backlight) | UNO 5V |
| JSN-SR04T | 30mA | UNO 5V |
| MQ-3 | 150mA | **Nguồn riêng** |
| Flame/Sound | 5mA×2 | UNO 5V |
| Soil | 35mA | UNO 5V |
| DFPlayer | 20-200mA (phụ thuộc volume) | UNO 5V |
| Servo MG996R | 500-2500mA | **Nguồn riêng 5V/3A** |
| Servo MG90 | 200-650mA | **Nguồn riêng 5V/1A** |
| Relay logic | 5mA×4 | UNO 5V |

**Khuyến nghị**:
- UNO: DC 9V/2A
- Servo: Nguồn 5V/3A riêng (buck converter từ 12V)
- Relay coil: 12V riêng (cho optoisolated)
- **Tất cả GND nối chung**

## Code Configuration

Trong `firmware-uno/greenhouse_uno.ino`, kiểm tra các định nghĩa sau:

```cpp
// Pin definitions
#define DHT_PIN 2
#define JSN_TRIG 12
#define JSN_ECHO 3
#define RELAY_PUMP 4
#define RELAY_FAN 7
#define RELAY_LIGHT12V 8
#define RELAY_AUXFAN 13
#define SERVO_WINDOW 9
#define SERVO_DOOR 5
#define ANALOG_FLAME A0
#define ANALOG_SOUND A1
#define ANALOG_SOIL A2
#define ANALOG_MQ3 A3
#define DF_RX 10
#define DF_TX 11

// Relay logic (thay đổi nếu cần)
#define RELAY_ON LOW   // Active LOW
#define RELAY_OFF HIGH

// I2C addresses
// BH1750: 0x23 (mặc định)
// LCD: 0x27 hoặc 0x3F (scan bằng I2C Scanner)
```

## Test Checklist

- [ ] UART: Xem JSON output trong Serial Monitor (57600 baud)
- [ ] DHT11: Nhiệt độ và độ ẩm hợp lý (không phải NaN)
- [ ] BH1750: Ánh sáng thay đổi khi che/mở đèn
- [ ] LCD: Hiển thị đúng 16×2 ký tự
- [ ] JSN-SR04T: Khoảng cách thay đổi khi di chuyển tay
- [ ] MQ-3: Giá trị tăng khi phun alcohol
- [ ] Flame: Giá trị tăng khi lửa gần (~800+)
- [ ] Sound: Giá trị tăng khi vỗ tay
- [ ] Soil: Giá trị thay đổi khi nhúng vào nước
- [ ] Relay: Nghe click khi bật/tắt
- [ ] Servo: Quay đúng góc (0° và 90°)
- [ ] DFPlayer: Phát track 001.mp3

## Troubleshooting

### LCD không hiển thị

1. Kiểm tra address: Chạy I2C Scanner
2. Điều chỉnh contrast (xoay chiết áp sau LCD)
3. Kiểm tra backlight (có sáng không?)

### DHT11 trả về NaN

1. Kiểm tra delay đủ lớn (>2s giữa các lần đọc)
2. Thử thêm pullup 10kΩ
3. Thử thay sensor (có thể hỏng)

### Servo giật

1. Nguồn không đủ mạnh → dùng nguồn riêng
2. Thêm capacitor 100-470µF gần servo
3. GND không chung

### DFPlayer không phát

1. Kiểm tra thẻ TF: format FAT32, file đúng thư mục
2. Thử nối điện trở 1kΩ nối tiếp với RX
3. Kiểm tra volume (tăng lên >20)
4. Test với code đơn giản trước

### MQ-3 không ổn định

1. Cần preheat 24-48h lần đầu
2. Nguồn 5V ổn định
3. Đọc giá trị sau 180s warm-up mỗi lần bật nguồn

## Schematic Symbol

```
         Arduino UNO R3
    ┌─────────────────────────┐
    │  ┌───┐           ┌───┐  │
    │  │USB│           │DC │  │
    │  └─┬─┘           └─┬─┘  │
    │    │               │    │
    │ [ATMEGA328P-PU]    │    │
    │                    │    │
    │ D0  RX ← ESP8266 TX│    │
    │ D1  TX → ESP8266 RX│    │
    │ D2  ← DHT11        │    │
    │ D3  ← JSN ECHO     │    │
    │ D4  → RELAY PUMP   │    │
    │ D5  → SERVO DOOR   │    │
    │ D7  → RELAY FAN    │    │
    │ D8  → RELAY LIGHT  │    │
    │ D9  → SERVO WINDOW │    │
    │ D10 ← DF TX        │    │
    │ D11 → DF RX        │    │
    │ D12 → JSN TRIG     │    │
    │ D13 → RELAY AUXFAN │    │
    │                    │    │
    │ A0  ← FLAME        │    │
    │ A1  ← SOUND        │    │
    │ A2  ← SOIL         │    │
    │ A3  ← MQ-3         │    │
    │ A4  ↔ SDA (I²C)    │    │
    │ A5  ↔ SCL (I²C)    │    │
    │                    │    │
    │ 5V  ─────────●─────┴────┤
    │ 3.3V         │          │
    │ GND  ────────●──────────┤
    │ VIN  ────────┘          │
    └─────────────────────────┘
```
