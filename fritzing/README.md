# Fritzing Diagrams

Thư mục này chứa sơ đồ Fritzing cho dự án IoT Greenhouse.

## File Structure

```
fritzing/
├── greenhouse-full.fzz         # Sơ đồ tổng thể (tất cả boards)
├── uno-detail.fzz              # Chi tiết Arduino UNO connections
├── esp8266-detail.fzz          # Chi tiết ESP8266 connections
├── esp32cam-detail.fzz         # Chi tiết ESP32-CAM connections
└── README.md                   # This file
```

## Tạo sơ đồ Fritzing

### Yêu cầu

- Tải Fritzing: https://fritzing.org/download/
- Phiên bản khuyến nghị: 0.9.x hoặc mới hơn

### Các board cần thiết

1. **Arduino UNO R3**
2. **ESP8266 (NodeMCU/WeMos D1 Mini)**
3. **ESP32-CAM (AI-Thinker)**

### Linh kiện cần thiết

**Sensors:**
- DHT11 (3 pins)
- BH1750 (I²C, 4 pins)
- JSN-SR04T (4 pins)
- MQ-3 (3 pins)
- Flame sensor (3 pins: VCC, GND, AO)
- Sound sensor (3 pins: VCC, GND, AO)
- Soil moisture sensor (2-3 pins)
- GPS NEO-6 (4 pins: VCC, GND, TX, RX)

**Actuators:**
- Relay module 4-channel
- Servo MG996R (3 pins)
- Servo MG90 (3 pins)
- DFPlayer Mini (8+ pins)
- LCD1602 I²C (4 pins: VCC, GND, SDA, SCL)
- WS2812B LED ring × 2 (3 pins each: VCC, GND, DIN)

**Power:**
- 5V power supply (2-3A)
- 12V power supply (for relays/actuators)
- Voltage regulator (if needed)

**Misc:**
- Breadboard (large)
- Jumper wires (M-M, M-F, F-F)
- Resistors: 1kΩ, 2kΩ (for voltage divider)
- Capacitors: 100µF, 470µF (for servo/LED)
- Diodes: 1N4007 (for relay protection)

## Sơ đồ tổng thể (Conceptual)

```
┌─────────────────────────────────────────────────────────────────┐
│                        POWER SUPPLY                              │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐                      │
│  │  5V/3A   │  │ 12V/2A   │  │ 3.3V LDO │                      │
│  └────┬─────┘  └────┬─────┘  └────┬─────┘                      │
└───────┼────────────┼─────────────┼────────────────────────────┘
        │            │              │
        │ 5V         │ 12V          │ 3.3V
        │            │              │
┌───────▼────────────▼──────────────▼────────────────────────────┐
│                    ARDUINO UNO R3                                │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │ ATmega328P                                              │   │
│  │                                                         │   │
│  │ D0/D1 ◄──UART──► ESP8266 (MQTT Bridge)                │   │
│  │ D2    ◄────────  DHT11                                 │   │
│  │ D3    ◄────────  JSN-SR04T Echo                        │   │
│  │ D4    ─────────► Relay Pump                            │   │
│  │ D5    ─────────► Servo Door (MG90)                     │   │
│  │ D7    ─────────► Relay Fan Main                        │   │
│  │ D8    ─────────► Relay Light 12V                       │   │
│  │ D9    ─────────► Servo Window (MG996R)                 │   │
│  │ D10/D11 ◄──────► DFPlayer Mini (SoftSerial)            │   │
│  │ D12   ─────────► JSN-SR04T Trig                        │   │
│  │ D13   ─────────► Relay Aux Fan                         │   │
│  │                                                         │   │
│  │ A0    ◄────────  Flame Sensor (analog)                 │   │
│  │ A1    ◄────────  Sound Sensor (analog)                 │   │
│  │ A2    ◄────────  Soil Moisture (analog)                │   │
│  │ A3    ◄────────  MQ-3 Alcohol (analog)                 │   │
│  │ A4/A5 ◄──I²C──► BH1750 + LCD1602                       │   │
│  └─────────────────────────────────────────────────────────┘   │
└──────────────────────────────────────────────────────────────────┘

┌──────────────────────────────────────────────────────────────────┐
│                    ESP8266 (NodeMCU)                              │
│  ┌─────────────────────────────────────────────────────────┐    │
│  │ ESP8266EX                                               │    │
│  │                                                         │    │
│  │ GPIO1/3  ◄─UART─► Arduino UNO (Bridge)                 │    │
│  │ GPIO12/14 ◄──────  GPS NEO-6 (SoftSerial)              │    │
│  │ GPIO2    ─────────► LED Ring 1 WS2812B (8 LEDs)        │    │
│  │ GPIO5    ─────────► LED Ring 2 WS2812B (8 LEDs)        │    │
│  │                                                         │    │
│  │ Wi-Fi: 2.4GHz ──► MQTT Broker                          │    │
│  └─────────────────────────────────────────────────────────┘    │
└──────────────────────────────────────────────────────────────────┘

┌──────────────────────────────────────────────────────────────────┐
│                    ESP32-CAM (AI-Thinker)                         │
│  ┌─────────────────────────────────────────────────────────┐    │
│  │ ESP32-S                                                 │    │
│  │                                                         │    │
│  │ Camera OV2640 ──► HTTP WebServer                       │    │
│  │ GPIO4 (Flash LED)                                      │    │
│  │                                                         │    │
│  │ Wi-Fi: 2.4GHz ──► PC/App (HTTP GET)                    │    │
│  └─────────────────────────────────────────────────────────┘    │
└──────────────────────────────────────────────────────────────────┘

┌──────────────────────────────────────────────────────────────────┐
│                    PC-VISION (Python)                             │
│  ┌─────────────────────────────────────────────────────────┐    │
│  │ YOLO v8/v11 (Plant Detection)                          │    │
│  │ HuggingFace (Disease Detection)                        │    │
│  │                                                         │    │
│  │ HTTP GET ──► ESP32-CAM /capture                        │    │
│  │ MQTT PUB ──► gh/cv/detections                          │    │
│  └─────────────────────────────────────────────────────────┘    │
└──────────────────────────────────────────────────────────────────┘

┌──────────────────────────────────────────────────────────────────┐
│                    APP (PyQt6)                                    │
│  ┌─────────────────────────────────────────────────────────┐    │
│  │ UI: Dark-Green Tech Theme                               │    │
│  │                                                         │    │
│  │ MQTT SUB ◄── gh/sensor/#, gh/status/#                  │    │
│  │ MQTT PUB ──► gh/cmd/#, gh/server/#                     │    │
│  │ HTTP GET ──► ESP32-CAM /stream (live camera)           │    │
│  └─────────────────────────────────────────────────────────┘    │
└──────────────────────────────────────────────────────────────────┘
```

## Voltage Divider (UART Level Shifter)

**UNO TX (5V) → ESP8266 RX (3.3V)**

```
UNO D1 (TX)
    │
    └──── 1kΩ ────┬──── ESP8266 GPIO3 (RX)
                  │
                 2kΩ
                  │
                 GND
```

**Calculation**:
```
Vout = Vin × (R2 / (R1 + R2))
     = 5V × (2kΩ / (1kΩ + 2kΩ))
     = 5V × (2/3)
     = 3.33V ✓
```

## Power Distribution

```
┌──────────────┐
│ 12V/2A PSU   │
└───────┬──────┘
        │
        ├──► Relay Coils (12V)
        ├──► Buck Converter (12V → 5V/3A)
        │      │
        │      ├──► Arduino UNO VIN
        │      ├──► ESP8266 VIN
        │      ├──► ESP32-CAM 5V
        │      ├──► Servo MG996R
        │      ├──► Servo MG90
        │      ├──► WS2812B Rings
        │      └──► Sensors 5V
        │
        └──► Actuators (12V loads: pump, fan, light)

GND ────●──── All GND pins connected (CRITICAL!)
```

## Wiring Checklist

### UNO Board

- [ ] DHT11: VCC=5V, DATA=D2, GND=GND
- [ ] BH1750: VCC=5V, SDA=A4, SCL=A5, GND=GND
- [ ] LCD1602: VCC=5V, SDA=A4, SCL=A5, GND=GND
- [ ] JSN-SR04T: VCC=5V, TRIG=D12, ECHO=D3, GND=GND
- [ ] MQ-3: VCC=5V, AO=A3, GND=GND
- [ ] Flame: VCC=5V, AO=A0, GND=GND
- [ ] Sound: VCC=5V, AO=A1, GND=GND
- [ ] Soil: VCC=5V, AO=A2, GND=GND
- [ ] Relay: IN1=D4, IN2=D7, IN3=D8, IN4=D13
- [ ] Servo MG996R: Signal=D9, VCC=5V(separate), GND=GND
- [ ] Servo MG90: Signal=D5, VCC=5V(separate), GND=GND
- [ ] DFPlayer: TX=D10, RX=D11, VCC=5V, GND=GND
- [ ] UART to ESP8266: TX=D1→ESP RX, RX=D0←ESP TX

### ESP8266 Board

- [ ] UART to UNO: TX=GPIO1→UNO D0, RX=GPIO3←UNO D1 (via divider!)
- [ ] GPS NEO-6: VCC=3.3V, TX=GPIO12, RX=GPIO14, GND=GND
- [ ] LED Ring 1: VCC=5V(separate), DIN=GPIO2, GND=GND
- [ ] LED Ring 2: VCC=5V(separate), DIN=GPIO5, GND=GND

### ESP32-CAM Board

- [ ] Power: 5V=VIN, GND=GND
- [ ] Camera: Already connected on board
- [ ] Flash LED: GPIO4 (already on board)

### Safety Components

- [ ] Diode 1N4007 across each relay coil (cathode to VCC)
- [ ] Capacitor 100-470µF near servo power pins
- [ ] Capacitor 1000µF near WS2812B power pins
- [ ] Fuse on 12V line (2A)

## Notes

### Breadboard vs PCB

**Breadboard** (prototype):
- ✅ Dễ sửa/debug
- ❌ Kém ổn định (loose connections)
- ❌ Nhiễu cao (long wires)

**PCB** (production):
- ✅ Ổn định, compact
- ✅ Ít nhiễu
- ❌ Khó sửa

**Khuyến nghị**: Prototype trên breadboard, sau đó thiết kế PCB.

### Wire Gauge

| Connection | Wire Gauge | Length |
|------------|------------|--------|
| Power 5V/12V | 20-22 AWG | Short as possible |
| Signal (digital) | 24-26 AWG | <30cm |
| I²C | 24 AWG (twisted pair) | <50cm |
| UART | 24 AWG (twisted pair) | <50cm |

### Shielding

- Dùng twisted pair cho I²C, UART
- Tách nguồn analog/digital
- Ground plane nếu dùng PCB

## Fritzing Tips

1. **Snap to grid**: Căn chỉnh linh kiện cho gọn
2. **Color code wires**:
   - Red = VCC (5V/12V)
   - Black = GND
   - Blue = Signal
   - Yellow = I²C/UART
3. **Label nets**: Đặt tên cho các net (VCC, GND, UART_TX, v.v.)
4. **Add notes**: Ghi chú điện áp, dòng điện
5. **Export**:
   - Breadboard view → PNG (for tutorial)
   - Schematic view → PDF (for documentation)
   - PCB view → Gerber (for fabrication)

## Download Parts

Nếu thiếu linh kiện trong Fritzing:

- [Fritzing Parts Library](https://github.com/fritzing/fritzing-parts)
- [AdaFruit Fritzing Library](https://github.com/adafruit/Fritzing-Library)
- [Sparkfun Fritzing Parts](https://github.com/sparkfun/Fritzing_Parts)

## References

- [Fritzing Official Site](https://fritzing.org/)
- [Fritzing Tutorial](https://learn.sparkfun.com/tutorials/fritzing-tutorial/all)
- [PCB Design Best Practices](https://www.autodesk.com/products/eagle/blog/pcb-design-tips/)

---

**Note**: Các file `.fzz` thực tế cần được tạo bằng phần mềm Fritzing. File README này cung cấp hướng dẫn và specification để tạo sơ đồ.

**Last updated**: 2025-01-22
