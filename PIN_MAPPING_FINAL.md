# Pin Mapping - FINAL (Official Version)

**Last updated**: 2025-11-23
**Status**: LOCKED - Use this as single source of truth

---

## Design Decision

**Approach chosen**: Keep DFPlayer on UNO, use AltSoftSerial for UNO↔ESP8266

**Rationale**:
- ✅ Simpler architecture (less hardware changes)
- ✅ DFPlayer close to audio decision logic (UNO)
- ✅ AltSoftSerial more stable than SoftwareSerial for inter-MCU comms
- ✅ I²C bus stays clean (LCD + BH1750 only)

**Alternatives considered**:
- ❌ Hardware UART (D0/D1) for ESP8266: Blocks USB debugging
- ❌ I²C for UNO↔ESP8266: Complexity with UNO as both master (LCD/BH1750) and slave
- ❌ DFPlayer on ESP8266: More hardware changes, less intuitive

---

## Arduino UNO R3 (ATmega328P)

### Digital Pins

| Pin | Function | Direction | Type | Notes |
|-----|----------|-----------|------|-------|
| D0 | **USB RX** | Input | Serial | Reserved for programming/debug |
| D1 | **USB TX** | Output | Serial | Reserved for programming/debug |
| D2 | **JSN Echo** | Input | Interrupt | INT0, pulseIn() for ultrasonic |
| D3 | **JSN Trig** | Output | Digital | Trigger pulse for JSN-SR04T |
| D4 | **Button: Open Main Door** | Input | Digital | INPUT_PULLUP, LOW when pressed |
| D5 | **Servo: Door (MG90)** | Output | PWM | 180° servo, manual button trigger |
| D6 | **Servo: Window (MG996R)** | Output | PWM | 180° servo, auto control |
| D7 | **Relay: Pump** | Output | Digital | Active LOW relay |
| D8 | **AltSoftSerial RX** | Input | Serial | ← ESP8266 TX (115200 baud) |
| D9 | **AltSoftSerial TX** | Output | Serial | → ESP8266 RX (115200 baud) |
| D10 | **Relay: LED 12V** | Output | Digital | Active LOW relay |
| D11 | **DFPlayer TX** | Output | Serial | → DFPlayer RX (9600 baud) |
| D12 | **DFPlayer RX** | Input | Serial | ← DFPlayer TX (9600 baud) |
| D13 | **WS2812 Ring #1** | Output | Digital | 8 LEDs, NeoPixel library |

**Notes**:
- **D8/D9**: AltSoftSerial (better timing than SoftwareSerial for critical comms)
- **D11/D12**: SoftwareSerial for DFPlayer (9600 baud OK for audio commands)
- **D13**: Also has onboard LED, but WS2812 will override it

### Analog Pins

| Pin | Function | Type | Range | Notes |
|-----|----------|------|-------|-------|
| A0 | **Flame Sensor (Analog)** | Input | 0-1023 | Higher value = flame detected |
| A1 | **Sound Sensor (Analog)** | Input | 0-1023 | Higher value = louder sound |
| A2 | **Soil Moisture** | Input | 0-1023 | 0=wet, 1023=dry (inverted in code) |
| A3 | **MQ-3 Alcohol/Gas** | Input | 0-1023 | Voltage proportional to gas concentration |
| A4 | **I²C SDA** | Bidir | - | BH1750 + LCD1602 I²C (shared bus) |
| A5 | **I²C SCL** | Bidir | - | BH1750 + LCD1602 I²C (shared bus) |

**Notes**:
- **Flame/Sound**: Use analog pins as specified (requirement)
- Can also attach **flame digital out** to D2 for interrupt if sensor has DO pin
- Can also attach **sound digital out** to D3 for interrupt if sensor has DO pin

### Relays

| Relay | Pin | Device | Active Level | Default State |
|-------|-----|--------|--------------|---------------|
| Relay 1 | D7 | Water Pump | LOW | OFF (HIGH) |
| Relay 2 | D10 | LED 12V Grow Light | LOW | OFF (HIGH) |
| Relay 3 | (D4 used for button) | - | - | - |
| Relay 4 | (Need to assign) | Main Fan | LOW | OFF (HIGH) |

**Missing assignments**:
- **Main Fan**: Recommend **A6** (if available, or reassign D4 and move button elsewhere)
- **Aux Fan**: Recommend **A7** (if available)

**Alternative** (if A6/A7 not available):
- Use **I²C relay expander** (PCF8574) for 4 relays
- Address: 0x20 (different from LCD 0x27, BH1750 0x23)

### I²C Devices

| Device | Address | Purpose | Pins |
|--------|---------|---------|------|
| BH1750 | 0x23 | Light intensity sensor | A4 (SDA), A5 (SCL) |
| LCD1602 I²C | 0x27 | Display | A4 (SDA), A5 (SCL) |

**Pull-up resistors**: 4.7kΩ on SDA/SCL (usually built-in on modules)

### Power Requirements

| Component | Voltage | Current | Notes |
|-----------|---------|---------|-------|
| UNO board | 5V | 50mA | Via USB or VIN (7-12V) |
| Servos (2×) | 5V | 500-2500mA | **Separate 5V/3A supply required!** |
| DFPlayer | 5V | 20-200mA | From UNO 5V OK |
| WS2812 (8 LEDs) | 5V | ~480mA max | From UNO 5V OK (if not full white) |
| Sensors | 5V | ~200mA total | From UNO 5V OK |
| Relays (logic) | 5V | 20mA | From UNO 5V OK |

**CRITICAL**:
- All **GND must be common** (UNO, ESP8266, servos, relays, sensors)
- Servos need **separate power supply** (not from UNO 5V pin!)
- Relay coils can be 5V or 12V depending on module (check datasheet)

---

## ESP8266 (NodeMCU / ESP-12E)

### GPIO Mapping

| GPIO | NodeMCU Label | Function | Direction | Notes |
|------|---------------|----------|-----------|-------|
| GPIO0 | D3 | (Boot mode) | - | Must be HIGH at boot (has pull-up) |
| GPIO1 | TX | **UART0 TX** | Output | USB debug (avoid using) |
| GPIO2 | D4 | **WS2812 Ring #2** | Output | 8 LEDs, NeoPixelBus UART method |
| GPIO3 | RX | **UART0 RX** | Input | USB debug (avoid using) |
| GPIO4 | D2 | (Reserved) | - | Can use for I²C SDA if needed |
| GPIO5 | D1 | (Reserved) | - | Can use for I²C SCL if needed |
| GPIO12 | D6 | **From UNO TX** | Input | ← AltSoftSerial D9 (via level shift!) |
| GPIO13 | D7 | **GPS RX** | Input | ← NEO-6 TX (SoftwareSerial) |
| GPIO14 | D5 | **To UNO RX** | Output | → AltSoftSerial D8 (3.3V OK) |
| GPIO15 | D8 | **GPS TX** | Output | → NEO-6 RX (SoftwareSerial) |
| GPIO16 | D0 | (Boot mode) | - | Must be LOW at boot (has pull-down) |

### Level Shifter (CRITICAL!)

**UNO (5V) → ESP8266 (3.3V)**: MUST use level shifter

```
UNO D9 (TX, 5V) ──┬── 1kΩ ──┬── ESP8266 GPIO12 (RX, 3.3V max)
                  │         │
                  └─ 2kΩ ───┴── GND

Output voltage: 5V × (2kΩ / 3kΩ) = 3.33V ✓
```

**ESP8266 (3.3V) → UNO (5V)**: NO shifter needed
```
ESP8266 GPIO14 (TX, 3.3V) ────── UNO D8 (RX, tolerates 3.3V) ✓
```

### SoftwareSerial Instances

```cpp
// In ESP8266 firmware
SoftwareSerial unoSerial(12, 14);  // RX=GPIO12, TX=GPIO14, 115200 baud
SoftwareSerial gpsSerial(13, 15);  // RX=GPIO13, TX=GPIO15, 9600 baud

void setup() {
  unoSerial.begin(115200);
  gpsSerial.begin(9600);
}
```

### WS2812 Ring #2

**Library**: NeoPixelBus (UART method to avoid Wi-Fi jitter)

```cpp
#include <NeoPixelBus.h>

#define LED_COUNT 8
#define LED_PIN 2  // GPIO2

NeoPixelBus<NeoGrbFeature, NeoEsp8266Uart800KbpsMethod> strip(LED_COUNT);

void setup() {
  strip.Begin();
  strip.Show();  // Clear
}
```

### Power

- **VIN**: 5V (via USB or external)
- **3.3V output**: Max 500mA (after AMS1117 regulator)
- **WS2812 Ring #2**: Powered from **separate 5V**, GND common

---

## ESP32-CAM (AI-Thinker)

### Pins (Mostly Internal)

| Pin | Function | Notes |
|-----|----------|-------|
| GPIO0 | Boot mode | Pull LOW for flash mode |
| GPIO1 | UART TX | Debug (optional) |
| GPIO3 | UART RX | Debug (optional) |
| GPIO4 | **Flash LED** | PWM control via `/led?duty=0..255` |
| GPIO12-15 | Camera data | Internal |
| GPIO25-27 | Camera control | Internal |

**No external wiring needed** except:
- 5V + GND (via FTDI or USB adapter)
- GPIO0 to GND when flashing (then remove)

---

## Wiring Checklist

### UNO Connections

- [ ] **D0/D1**: Leave free (USB)
- [ ] **D2**: JSN Echo
- [ ] **D3**: JSN Trig
- [ ] **D4**: Button (INPUT_PULLUP to GND)
- [ ] **D5**: Servo Door signal (orange/yellow wire)
- [ ] **D6**: Servo Window signal (orange wire)
- [ ] **D7**: Relay Pump IN
- [ ] **D8**: AltSoftSerial RX ← ESP8266 GPIO14 TX (3.3V)
- [ ] **D9**: AltSoftSerial TX → ESP8266 GPIO12 RX (via divider!)
- [ ] **D10**: Relay LED 12V IN
- [ ] **D11**: DFPlayer TX → RX (via 1kΩ resistor)
- [ ] **D12**: DFPlayer RX ← TX
- [ ] **D13**: WS2812 Ring #1 DIN
- [ ] **A0**: Flame sensor AO
- [ ] **A1**: Sound sensor AO
- [ ] **A2**: Soil moisture AO
- [ ] **A3**: MQ-3 AO
- [ ] **A4**: I²C SDA (BH1750 + LCD)
- [ ] **A5**: I²C SCL (BH1750 + LCD)
- [ ] **5V**: Sensors VCC (NOT servos!)
- [ ] **GND**: All GND common

### ESP8266 Connections

- [ ] **GPIO2 (D4)**: WS2812 Ring #2 DIN
- [ ] **GPIO12 (D6)**: ← UNO D9 TX (via voltage divider)
- [ ] **GPIO13 (D7)**: ← GPS NEO-6 TX
- [ ] **GPIO14 (D5)**: → UNO D8 RX (direct 3.3V)
- [ ] **GPIO15 (D8)**: → GPS NEO-6 RX (direct 3.3V)
- [ ] **VIN**: 5V (USB or external)
- [ ] **GND**: Common with UNO

### ESP32-CAM Connections

- [ ] **5V**: From FTDI or USB adapter (stable 5V/2A)
- [ ] **GND**: Common
- [ ] **GPIO0**: To GND when flashing only

---

## Relay Assignment (Final)

Since we're short on pins, **recommended solution**:

### Option A: Use I²C Relay Module (PCF8574)

```cpp
// Add PCF8574 library
#include <PCF8574.h>

PCF8574 relays(0x20);  // I²C address 0x20

void setup() {
  Wire.begin();
  relays.begin();

  // Map relay pins
  // P0 = Pump
  // P1 = Main Fan
  // P2 = Aux Fan
  // P3 = LED 12V
}

void setRelay(int relay, bool state) {
  relays.write(relay, state ? LOW : HIGH);  // Active LOW
}
```

**Wiring**: PCF8574 connects to A4 (SDA) and A5 (SCL), shares I²C bus

### Option B: Reassign Pins (if no I²C expander)

- **Main Fan**: Use **D4** (move button to A6 analog read)
- **Aux Fan**: Use external MOSFET driven by **D10** (remove LED 12V relay)
- **LED 12V**: Control via PWM through existing relay

---

## Software Pin Definitions

### Arduino UNO (`firmware-uno/greenhouse_uno.ino`)

```cpp
// UART - AltSoftSerial (fixed pins D8/D9)
#include <AltSoftSerial.h>
AltSoftSerial espSerial;  // RX=D8, TX=D9, 115200 baud

// DFPlayer - SoftwareSerial
#include <SoftwareSerial.h>
SoftwareSerial dfSerial(12, 11);  // RX=D12, TX=D11, 9600 baud

// Sensors
#define DHT_PIN 13       // Can be any digital pin
#define JSN_TRIG 3
#define JSN_ECHO 2

#define FLAME_ANALOG A0
#define SOUND_ANALOG A1
#define SOIL_ANALOG A2
#define MQ3_ANALOG A3

// I²C (fixed)
// A4 = SDA, A5 = SCL

// Actuators
#define SERVO_DOOR 5
#define SERVO_WINDOW 6
#define RELAY_PUMP 7
#define RELAY_LED12V 10

// WS2812
#define WS2812_PIN 13
#define WS2812_COUNT 8

// Button
#define BUTTON_DOOR 4  // INPUT_PULLUP

// Relays via I²C expander (if using PCF8574)
PCF8574 relays(0x20);
#define RELAY_PUMP_I2C 0
#define RELAY_FAN_I2C 1
#define RELAY_AUXFAN_I2C 2
#define RELAY_LED12V_I2C 3
```

### ESP8266 (`firmware-esp8266/greenhouse_esp8266.ino`)

```cpp
// UNO UART - SoftwareSerial
#include <SoftwareSerial.h>
SoftwareSerial unoSerial(12, 14);  // RX=GPIO12, TX=GPIO14, 115200

// GPS - SoftwareSerial
SoftwareSerial gpsSerial(13, 15);  // RX=GPIO13, TX=GPIO15, 9600

// WS2812 Ring #2
#include <NeoPixelBus.h>
#define LED_COUNT 8
NeoPixelBus<NeoGrbFeature, NeoEsp8266Uart800KbpsMethod> ring2(LED_COUNT);
```

---

## Validation Checklist

Before declaring "done":

- [ ] **Pin mapping table** updated in all docs (README, this file, code comments)
- [ ] **Hardware wired** exactly per this spec
- [ ] **Firmware pin defs** match this table
- [ ] **MQTT topics** match [`mqtt-schema.json`](mqtt-schema.json)
- [ ] **Test UART**: UNO ↔ ESP8266 JSON exchange
- [ ] **Test DFPlayer**: Play track 1-10
- [ ] **Test WS2812**: Both rings change color
- [ ] **Test servos**: Door/window move on command
- [ ] **Test relays**: All 4 relays click
- [ ] **Test sensors**: All analog sensors return valid values
- [ ] **Test I²C**: BH1750 reads lux, LCD displays text
- [ ] **Test GPS**: ESP8266 receives NMEA sentences
- [ ] **Test interrupts** (if using DO pins): Flame/sound trigger events

---

## Migration from Previous Versions

If you have **v1.0** or **v1.1-draft** with different pinout:

1. **Disconnect all wires**
2. **Rewire according to this document** (ONLY)
3. **Update firmware** with pin defs above
4. **Test each subsystem** individually before full integration

**DO NOT** mix pin mappings from different documents!

---

**This is the FINAL and OFFICIAL pin mapping. Any deviations must be documented here first.**

**Last verified**: 2025-11-23
**Next review**: After first hardware deployment
