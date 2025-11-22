# Auto Control Logic

## Overview

Hệ thống hỗ trợ 2 chế độ điều khiển:

- **MANUAL** (THỦ CÔNG): Điều khiển trực tiếp từ App, không áp dụng quy tắc tự động
- **AUTO** (TỰ ĐỘNG): App áp dụng quy tắc dựa trên sensor readings và thresholds

**Logic AUTO chạy trên App** (PyQt6), không phải firmware!

## Mode Switching

**Topic**: `gh/cmd/mode` hoặc `gh/server/mode`

**Values**: `"AUTO"` | `"MANUAL"`

**Behavior**:
- ESP8266 nhận mode change, publish lên `gh/state/mode` (retained)
- App subscribe `gh/state/mode` để đồng bộ UI
- Khi `mode = AUTO`: App bật auto control loop
- Khi `mode = MANUAL`: App tắt auto control, chỉ thực thi lệnh thủ công từ user

## Default Thresholds

Ngưỡng mặc định được lưu trong App settings (`app_settings.json`):

```json
{
  "auto_rules": {
    "enable": true,
    "use_soil": true,
    "soil_low": 35,
    "use_temp": true,
    "temp_high": 35,
    "use_light": true,
    "light_low": 120,
    "use_tank": true,
    "tank_low": 20
  }
}
```

**User có thể chỉnh trong App UI** (Auto Rules panel).

## Auto Control Rules

### Rule 1: Soil Moisture (Độ ẩm đất)

**Condition**: `soil_pct < soil_low`

**Action**:
- ✅ **Pump ON** → tưới nước

**Until**: `soil_pct ≥ soil_target` (hoặc timeout 5 phút)

**Logic**:
```python
if auto_rules['use_soil'] and sensor['soil_pct'] < auto_rules['soil_low']:
    publish("gh/cmd/relay/pump", "ON")
else:
    publish("gh/cmd/relay/pump", "OFF")
```

**Exceptions**:
- Nếu `tank_pct < tank_low`: **Chặn pump** (tránh bơm khô)

---

### Rule 2: Temperature (Nhiệt độ)

**Condition**: `temp_c > temp_high`

**Action**:
- ✅ **Fan Main ON** → làm mát
- ✅ **Window OPEN** → thông gió (optional)

**Until**: `temp_c ≤ temp_high`

**Logic**:
```python
if auto_rules['use_temp'] and sensor['temp_c'] > auto_rules['temp_high']:
    publish("gh/cmd/relay/fan", "ON")
    # publish("gh/cmd/servo/window", "OPEN")  # Optional
else:
    publish("gh/cmd/relay/fan", "OFF")
```

---

### Rule 3: Light Intensity (Ánh sáng)

**Condition**: `light_lux < light_low`

**Action**:
- ✅ **Light 12V ON** → bổ sung ánh sáng
- ✅ **LED Rings COLOR** (warm white/yellow)

**Until**: `light_lux ≥ light_low`

**Logic**:
```python
if auto_rules['use_light'] and sensor['light_lux'] < auto_rules['light_low']:
    publish("gh/cmd/relay/light12v", "ON")
    publish("gh/cmd/led/power", "ON")
    publish("gh/cmd/led/color", "#FFAA00")  # Warm color
else:
    publish("gh/cmd/relay/light12v", "OFF")
```

---

### Rule 4: Tank Low (Mực nước thấp)

**Condition**: `tank_pct < tank_low`

**Action**:
- 🚫 **Block Pump** (prevent dry pumping)
- ⚠️ **Alert** on UI

**Logic**:
```python
if auto_rules['use_tank'] and sensor['tank_pct'] < auto_rules['tank_low']:
    # Force pump OFF
    publish("gh/cmd/relay/pump", "OFF")
    alert("Tank low! Refill water.")
```

---

### Emergency Rule 1: Fire Detected 🔥

**Condition**: `flame_do == 1` (digital sensor triggered)

**Priority**: **HIGHEST** (override all other rules)

**Action**:
- 🚫 **Pump OFF** (prevent electrical hazard)
- ✅ **All Fans ON** (ventilate smoke)
- 🚨 **LED Rings ALERT** (red blink)
- 📢 **DFPlayer** play track 8: "Phát hiện lửa, kích hoạt dập"
- ⚠️ **UI Alert**

**Logic**:
```python
if alerts['fire'] and sensor['flame_do'] == 1:
    publish("gh/cmd/relay/pump", "OFF")
    publish("gh/cmd/relay/fan", "ON")
    publish("gh/cmd/relay/auxfan", "ON")
    publish("gh/cmd/led/power", "ON")
    publish("gh/cmd/led/color", "#FF0000")  # Red alert
    publish("gh/cmd/dfp/play", {"track": 8, "vol": 30})
```

**Note**: UNO cũng có fallback logic cho fire detection (ngay cả khi mất MQTT).

---

### Emergency Rule 2: Loud Sound 🔊

**Condition**: `sound_do == 1`

**Action**:
- 📢 **DFPlayer** play track 9: "Âm thanh lớn, vui lòng giảm tiếng ồn"
- ⚠️ **UI notification**

**Logic**:
```python
if alerts['sound'] and sensor['sound_do'] == 1:
    publish("gh/cmd/dfp/play", {"track": 9, "vol": 25})
```

---

### Emergency Rule 3: Disease Detected 🌱

**Condition**: `cv.diseased == true` (from PC-Vision)

**Action**:
- 📢 **DFPlayer** play track 10: "Cây số {id} có dấu hiệu bệnh, cần chăm sóc"
- 🔍 **UI highlight** affected plants (from `cv.boxes`)
- 📊 **Log** disease label and score

**Logic**:
```python
# Subscribe to gh/cv/detections
def on_cv_detection(payload):
    if payload['diseased']:
        plant_id = payload['boxes'][0][4]  # First diseased plant
        label = payload['disease_label']
        score = payload['disease_score']

        publish("gh/cmd/dfp/play", {"track": 10, "vol": 25})
        ui_alert(f"Disease detected: {label} ({score:.2f}) on {plant_id}")
```

---

## Auto Control Loop (App)

Trong App, khi `mode == AUTO`:

```python
def run_auto_logic(self):
    """Execute AUTO mode control logic"""
    ar = self.st.auto_rules
    s = self.sensor

    # Rule: Tank low - block pump
    if ar['use_tank'] and s["tank_pct"] < ar['tank_low']:
        self.auto_set("pump", "OFF")

    # Rule: Soil moisture
    if ar['use_soil'] and s["soil_pct"] < ar['soil_low']:
        self.auto_set("pump", "ON")
    else:
        self.auto_set("pump", "OFF")

    # Rule: Temperature
    if ar['use_temp'] and s["temp_c"] > ar['temp_high']:
        self.auto_set("fan", "ON")
    else:
        self.auto_set("fan", "OFF")

    # Rule: Light
    if ar['use_light'] and s["light_lux"] < ar['light_low']:
        self.auto_set("light12v", "ON")
    else:
        self.auto_set("light12v", "OFF")

    # Emergency: Fire
    if self.chkFire.isChecked() and int(s["flame_do"]) == 1:
        self.auto_set("pump", "OFF")
        self.auto_set("auxfan", "ON")

def auto_set(self, relay_key: str, desired: str):
    """Set relay with deduplication"""
    # Only publish if state changed (avoid MQTT spam)
    if self.last_auto_cmd.get(relay_key) == desired:
        return

    self.last_auto_cmd[relay_key] = desired
    self.mqtt.publish(f"gh/cmd/relay/{relay_key}", desired)
    self.log(f"🤖 AUTO: {relay_key} → {desired}")
```

**Trigger**: Mỗi khi nhận sensor data mới từ `gh/sensor/*`

---

## Fallback Logic (UNO)

**Khi nào kích hoạt**: UART connection lost (>3s no data from ESP8266)

**Purpose**: Đảm bảo an toàn ngay cả khi mất kết nối App/MQTT

**Logic** (trong firmware UNO):

```cpp
void fallbackSafety() {
    // Priority 1: Fire
    if (sensors.flame > FLAME_THRESHOLD) {
        setRelay(RELAY_PUMP, false);   // OFF
        setRelay(RELAY_FAN, true);     // ON
        setRelay(RELAY_AUXFAN, true);  // ON
        pushLcdMessage("FIRE DETECT!", "FANS ON");
        return;
    }

    // Priority 2: High temp
    if (sensors.temp_c > TEMP_SAFE_MAX) {
        setRelay(RELAY_FAN, true);
        setRelay(RELAY_AUXFAN, true);
        servoWindow.write(90);  // Open window
        pushLcdMessage("TEMP HIGH", "COOLING");
        return;
    }

    // Default: Safe state
    safeState();  // All OFF except critical sensors
}

void safeState() {
    digitalWrite(RELAY_PUMP, RELAY_OFF);
    digitalWrite(RELAY_FAN, RELAY_OFF);
    digitalWrite(RELAY_LIGHT12V, RELAY_OFF);
    digitalWrite(RELAY_AUXFAN, RELAY_OFF);
}
```

**Thresholds**:
- `FLAME_THRESHOLD = 800` (ADC value)
- `TEMP_SAFE_MAX = 40.0` (°C)

---

## Hysteresis (Chống dao động)

Để tránh relay bật/tắt liên tục (chattering), áp dụng **hysteresis**:

### Example: Soil Moisture

```python
# Bad: No hysteresis
if soil_pct < 35:
    pump = ON
else:
    pump = OFF
# → Pump sẽ bật/tắt liên tục khi soil_pct dao động quanh 35%

# Good: With hysteresis
if soil_pct < 35:
    pump = ON
elif soil_pct > 40:  # Hysteresis gap: 5%
    pump = OFF
# → Pump chỉ tắt khi soil_pct vượt 40%, tránh dao động
```

**Khuyến nghị**:
- Soil: Gap 5% (`low=35`, `high=40`)
- Temp: Gap 2°C (`high=35`, `low=33`)
- Light: Gap 50 lux (`low=120`, `high=170`)

---

## Priority Order

Khi nhiều rule conflict, áp dụng **priority**:

1. **Fire** (highest)
2. **Tank low** (block pump)
3. **Temperature**
4. **Soil moisture**
5. **Light** (lowest)

**Example**:
- Nếu fire AND soil low: **Fire wins** → Pump OFF (safety first)
- Nếu tank low AND soil low: **Tank low wins** → Pump blocked

---

## Timing & Delays

### Pump ON duration

**Max continuous ON time**: 5 minutes

**Logic**:
```python
if pump_on_time > 300:  # 5 minutes
    publish("gh/cmd/relay/pump", "OFF")
    alert("Pump timeout - check for clogged pipes")
```

### Fan ON cooldown

**Min OFF time between cycles**: 1 minute (prevent motor wear)

### Sensor read interval

**Sensor data update**: 3 seconds (from UNO via UART)

**Auto rule evaluation**: Every sensor update (3s)

---

## UI/UX Indicators

### Visual Feedback

**LED Rings**:
- 🟢 **Green**: Normal (AUTO mode, all OK)
- 🟡 **Yellow**: Warning (tank low, soil low)
- 🔴 **Red blink**: Emergency (fire detected)

**App UI**:
- 🤖 Icon when AUTO mode active
- 🔵 Blue highlight for auto-triggered devices
- 🔴 Red highlight for alerts

### Logging

Mọi AUTO action được log vào App:

```
[12:34:56] 🤖 AUTO: pump → ON (soil_pct=32% < 35%)
[12:35:10] 🤖 AUTO: pump → OFF (soil_pct=42% > 40%)
[12:36:02] 🚨 FIRE DETECTED! fans → ON
```

---

## Testing Auto Rules

### Test 1: Soil Moisture

1. Chuyển sang AUTO mode
2. Thử giả lập soil sensor (ngắt khỏi đất = 0%)
3. Kiểm tra: Pump ON
4. Nhúng sensor vào nước (100%)
5. Kiểm tra: Pump OFF

### Test 2: Temperature

1. Giả lập DHT11 (heat up sensor)
2. Kiểm tra: Fan ON khi temp > threshold
3. Cool down sensor
4. Kiểm tra: Fan OFF

### Test 3: Fire Emergency

1. Dùng bật lửa gần flame sensor
2. Kiểm tra:
   - Pump OFF
   - Fans ON
   - LED red blink
   - DFPlayer phát cảnh báo

### Test 4: Tank Low

1. Giả lập tank level thấp (disconnect sensor)
2. Thử bật pump thủ công → Bị chặn
3. Kiểm tra UI: "Tank low" alert

---

## Troubleshooting

### Pump không bật dù soil thấp

1. Kiểm tra `use_soil` enabled trong Auto Rules
2. Kiểm tra tank level không thấp
3. Kiểm tra mode = AUTO
4. Xem logs: có command publish không?

### Relay bật/tắt liên tục

1. Tăng hysteresis gap
2. Kiểm tra sensor ổn định (không nhiễu)
3. Tăng sensor read interval (>3s)

### Auto rule không trigger

1. Kiểm tra MQTT connection OK
2. Kiểm tra App đang chạy auto loop
3. Xem logs: sensor data có nhận không?
4. Kiểm tra threshold values hợp lý

---

**Last updated**: 2025-01-22
