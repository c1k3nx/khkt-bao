# FIRMWARE BUG ANALYSIS - v1.1 FINAL

**Date:** 2025-11-22
**Status:** CRITICAL ISSUES FOUND ⚠️

---

## 🔴 CRITICAL BUGS

### 1. **PIN CONFLICT: DHT11 vs WS2812 on D13**

**Location:** `firmware-uno/greenhouse_uno.ino`
**Lines:** 38, 62

```cpp
#define DHT_PIN 13        // Line 38
#define WS2812_PIN 13     // Line 62 - CONFLICT!
```

**Problem:**
- Cả DHT11 và WS2812 Ring #1 đều được gán vào D13
- DHT11 cần control line cho data đọc/ghi
- WS2812 cần control line cho chuỗi LED
- **Hai thiết bị không thể chia sẻ cùng 1 pin digital**

**Impact:**
- ❌ DHT11 sẽ đọc sai dữ liệu nhiệt độ/độ ẩm
- ❌ WS2812 sẽ nhận tín hiệu nhiễu từ DHT11
- ❌ Cả hai thiết bị đều không hoạt động ổn định

**Solutions:**

**Option A - Recommended:** Chỉ dùng WS2812 Ring #1 trên UNO, bỏ DHT11 lên ESP8266
```cpp
// UNO: Remove DHT11
// #define DHT_PIN 13  // REMOVED
#define WS2812_PIN 13  // Keep this

// ESP8266: Add DHT11 on free GPIO
#define DHT_PIN 4  // GPIO4 (D2) is free
```

**Option B:** Dùng DHT11, bỏ WS2812 Ring #1 (chỉ dùng Ring #2 trên ESP8266)
```cpp
#define DHT_PIN 13
// #define WS2812_PIN 13  // REMOVED
```

**Option C:** DHT11 lên pin khác (nhưng UNO đã hết pin!)
- ❌ Không khả thi - tất cả digital pins đã được dùng

---

### 2. **MQTT TOPIC MISMATCH: Device Names**

**Location:** `firmware-esp8266/greenhouse_esp8266.ino`

**Problem:** Device names không consistent giữa control và status:

```cpp
// Control topics expect:
greenhouse/control/pump
greenhouse/control/fan
greenhouse/control/auxFan         // camelCase
greenhouse/control/mainGrowLight  // camelCase
greenhouse/control/window1
greenhouse/control/mainDoor       // camelCase
greenhouse/control/rgbLed         // camelCase

// But status publishes:
greenhouse/status/pump            // OK
greenhouse/status/fan             // OK
greenhouse/status/auxFan          // OK
greenhouse/status/mainGrowLight   // OK
greenhouse/status/window1         // OK
greenhouse/status/mainDoor        // OK
greenhouse/status/rgbLed          // OK
```

**Actually OK** - code is consistent. False alarm. ✅

---

## 🟡 WARNING ISSUES

### 3. **UNO Pin Shortage for Full Relay Control**

**Problem:** UNO chỉ có 2 relay (D7, D10), thiếu 2 relay cho Fan + AuxFan

**Current State:**
```cpp
#define RELAY_PUMP 7      // ✅ Defined
#define RELAY_LED12V 10   // ✅ Defined
// RELAY_FAN - Missing!
// RELAY_AUXFAN - Missing!
```

**ESP8266 sends commands for devices UNO can't control:**
```cpp
handleDeviceCommand("fan", "ON");     // ❌ UNO không có relay này
handleDeviceCommand("auxFan", "ON");  // ❌ UNO không có relay này
```

**Impact:**
- App/MQTT có thể gửi lệnh bật Fan/AuxFan
- ESP8266 forward lệnh xuống UNO
- UNO nhận lệnh nhưng **không làm gì** (không có code xử lý)
- App nghĩ Fan đã bật, nhưng thực tế không có gì xảy ra

**Solution:** Cần thêm code xử lý fan trên UNO bằng PCF8574:

```cpp
// UNO: Add PCF8574 support
#include <PCF8574.h>
PCF8574 relayExpander(0x20);  // I2C address 0x20

void handleControlCommand(JsonDocument& doc) {
  const char* device = doc["device"];
  const char* action = doc["action"];

  // ... existing code ...

  // Add fan control via I2C expander
  else if (strcmp(device, "fan") == 0) {
    bool state = (strcmp(action, "ON") == 0);
    relayExpander.digitalWrite(0, state ? LOW : HIGH);  // Relay 1 on PCF8574
    pushLcdMessage("FAN", action);
  }
  else if (strcmp(device, "auxFan") == 0) {
    bool state = (strcmp(action, "ON") == 0);
    relayExpander.digitalWrite(1, state ? LOW : HIGH);  // Relay 2 on PCF8574
    pushLcdMessage("AUX FAN", action);
  }
}
```

---

### 4. **Missing Error Handling for Invalid Device Names**

**Location:** `firmware-uno/greenhouse_uno.ino:321-363`

**Problem:** Nếu ESP8266 gửi device name không hợp lệ, UNO không reply ACK

```cpp
void handleControlCommand(JsonDocument& doc) {
  const char* device = doc["device"];
  const char* action = doc["action"];

  if (device == NULL || action == NULL) return;  // ❌ Silent fail, no ACK sent

  bool ok = true;

  // Handle devices...

  // ❌ If device name doesn't match any case, ok stays true but nothing happens!

  // Send ACK (even when device not recognized!)
  ack["ok"] = ok;  // ❌ Always true
}
```

**Solution:** Track if device was actually handled:

```cpp
void handleControlCommand(JsonDocument& doc) {
  const char* device = doc["device"];
  const char* action = doc["action"];

  if (device == NULL || action == NULL) {
    sendNack("Invalid command format");
    return;
  }

  bool handled = false;

  if (strcmp(device, "pump") == 0) {
    actuators.pump = (strcmp(action, "ON") == 0);
    setRelay(RELAY_PUMP, actuators.pump);
    pushLcdMessage("PUMP", action);
    handled = true;
  }
  // ... other devices ...

  if (!handled) {
    sendNack("Unknown device");
    Serial.print("Unknown device: ");
    Serial.println(device);
    return;
  }

  // Send ACK
  sendAck(device, action);
}
```

---

## 🟢 POTENTIAL ISSUES (Not Bugs, But Worth Noting)

### 5. **SoftwareSerial Conflicts on ESP8266**

**Location:** `firmware-esp8266/greenhouse_esp8266.ino:62-63`

```cpp
SoftwareSerial unoSerial(UNO_RX, UNO_TX);  // GPIO12, 14 @ 115200 baud
SoftwareSerial gpsSerial(GPS_RX, GPS_TX);  // GPIO13, 15 @ 9600 baud
```

**Warning:** ESP8266 SoftwareSerial có thể mất data khi:
- Cả 2 SoftwareSerial active cùng lúc
- WiFi interrupt xảy ra
- MQTT publish/subscribe đang diễn ra

**Likelihood:** ⚠️ Medium - có thể mất vài gói UART khi tải cao

**Mitigation Already in Code:**
```cpp
#define UART_TIMEOUT 3000
if (now - lastSensorReceived > UART_TIMEOUT) {
  publishError("uart_timeout", "No data from UNO for 3s");
}
```

**Recommendation:** Thêm retry logic:
```cpp
if (uartErrorCount > 5) {
  // Reset UNO connection
  unoSerial.end();
  delay(100);
  unoSerial.begin(UART_BAUD);
  uartErrorCount = 0;
}
```

---

### 6. **JSON Buffer Size Might Be Tight**

**Location:** Multiple places

```cpp
StaticJsonDocument<512> doc;  // UNO sensor data
StaticJsonDocument<256> doc;  // ESP8266 control commands
```

**Current Payload Size Estimate:**
```json
{
  "type": "sensor",
  "ts": 12345,
  "data": {
    "temperature": "25.5",
    "humidity": "68.0",
    "lightIntensity": "1200",
    "soilMoisture": "45.0",
    "gasMQ3": "1.23",
    "flameAnalog": "123",
    "soundLevel": "456",
    "waterTankLevel": "78"
  }
}
```

**Estimated Size:** ~280 bytes

**Buffer:** 512 bytes ✅ OK with margin

**Recommendation:** Monitor for `DeserializationError::NoMemory`:

```cpp
if (error == DeserializationError::NoMemory) {
  Serial.println("JSON buffer too small!");
  // Increase buffer size in next version
}
```

---

## 📊 COMMUNICATION PROTOCOL VERIFICATION

### UART Protocol Check (UNO ↔ ESP8266)

| Direction | Message Type | Format | Status |
|-----------|--------------|--------|--------|
| UNO → ESP | Sensor Data | `{"type":"sensor","ts":123,"data":{...}}` | ✅ Match |
| UNO → ESP | Event | `{"type":"event","event":"flame","value":"DETECTED"}` | ✅ Match |
| UNO → ESP | ACK | `{"type":"ack","ts":123,"device":"pump","action":"ON","ok":true}` | ✅ Match |
| ESP → UNO | Control | `{"type":"control","device":"pump","action":"ON"}` | ✅ Match |

**Baud Rate:** Both 115200 ✅
**Newline Delimiter:** Both use `\n` ✅
**JSON Library:** Both use ArduinoJson ✅

---

### MQTT Topics Check (ESP8266 ↔ Broker)

| Topic Pattern | Publisher | Subscriber | QoS | Retain | Status |
|---------------|-----------|------------|-----|--------|--------|
| `greenhouse/data/*` | ESP8266 | App | 0 | No | ✅ OK |
| `greenhouse/status/*` | ESP8266 | App | 0 | Yes | ✅ OK |
| `greenhouse/control/*` | App | ESP8266 | 0 | No | ✅ OK |
| `greenhouse/event/*` | ESP8266 | App | 0 | No | ✅ OK |
| `greenhouse/set/thresholds/*` | App | ESP8266 | 0 | No | ✅ OK |

**Prefix Standardization:** All use `greenhouse/*` ✅
**No Legacy `gh/*` Topics:** ✅ Clean migration

---

## 🔧 RECOMMENDED FIXES (Priority Order)

### Priority 1: CRITICAL - Fix D13 Pin Conflict

**Choose ONE solution:**

**Solution A: Remove DHT11 from UNO (Recommended)**
```cpp
// firmware-uno/greenhouse_uno.ino
// Comment out DHT11 (will be read from ESP8266 instead)
// #define DHT_PIN 13
// DHT dht(DHT_PIN, DHT_TYPE);

// Keep WS2812
#define WS2812_PIN 13  // Now exclusive to WS2812
```

```cpp
// firmware-esp8266/greenhouse_esp8266.ino
// Add DHT11 support
#include <DHT.h>
#define DHT_PIN 4  // GPIO4 (D2)
DHT dht(DHT_PIN, DHT11);

void readDhtSensor() {
  float temp = dht.readTemperature();
  float hum = dht.readHumidity();

  if (!isnan(temp) && !isnan(hum)) {
    sensorData.temp_c = temp;
    sensorData.hum_pct = hum;
  }
}
```

**Solution B: Remove WS2812 Ring #1 from UNO**
```cpp
// firmware-uno/greenhouse_uno.ino
#define DHT_PIN 13  // Keep DHT11
// #define WS2812_PIN 13  // Remove WS2812
// Adafruit_NeoPixel strip(...);  // Remove this
```

---

### Priority 2: HIGH - Add Fan Relay Support

Add PCF8574 I2C relay expander support to UNO firmware.

---

### Priority 3: MEDIUM - Improve Error Handling

- Add device name validation
- Send proper NACK for unknown devices
- Add UART retry logic

---

## ✅ THINGS THAT WORK CORRECTLY

1. ✅ AltSoftSerial pin assignment (D8/D9) on UNO
2. ✅ SoftwareSerial pin assignment (GPIO12/14) on ESP8266
3. ✅ UART baud rate matching (115200)
4. ✅ JSON protocol structure (sensor/control/event/ack)
5. ✅ MQTT topic naming (`greenhouse/*` prefix)
6. ✅ Level shifting documented (1kΩ + 2kΩ voltage divider)
7. ✅ Data type conversions (string ↔ float)
8. ✅ Timeout and watchdog logic
9. ✅ Fallback safety mode
10. ✅ Emergency event detection (flame/sound)

---

## 🎯 SUMMARY

**Total Issues Found:** 6
**Critical:** 1 (Pin conflict D13)
**High:** 1 (Missing fan relay support)
**Medium:** 2 (Error handling, UART retry)
**Low:** 2 (SoftwareSerial conflicts, JSON buffer)

**Recommendation:**
1. **Fix D13 conflict immediately** - choose Solution A or B
2. Add PCF8574 support for full relay control
3. Improve error handling for robustness

**Overall Assessment:**
Firmware có thể chạy được với **minor issues**, nhưng cần fix D13 conflict để tránh đọc sai cảm biến. Communication protocol giữa UNO-ESP8266-MQTT đã đúng và consistent.
