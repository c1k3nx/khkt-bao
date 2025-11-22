# DFPlayer Mini Tracks List

## Thẻ TF Card Setup

### Format

- **File system**: FAT32
- **Cluster size**: 4KB hoặc 8KB (khuyến nghị)
- **Max capacity**: 32GB

### Folder Structure

```
/
└── 01/
    ├── 001.mp3
    ├── 002.mp3
    ├── 003.mp3
    ├── ...
    └── 010.mp3
```

**Lưu ý**:
- Thư mục **phải** là `01`, `02`, v.v. (2 chữ số)
- File **phải** là `001.mp3`, `002.mp3`, v.v. (3 chữ số)
- Format: MP3, bitrate 128kbps (khuyến nghị)
- Sample rate: 44.1kHz hoặc 48kHz

## Track Definitions

### Track 001: Bật chế độ thủ công

**File**: `01/001.mp3`

**Nội dung**: "Bật chế độ thủ công"

**Trigger**: Khi user chuyển từ AUTO → MANUAL

**TTS Command**:
```bash
# Using Google TTS (gtts-cli)
gtts-cli "Bật chế độ thủ công" --lang vi --output 001.mp3
```

---

### Track 002: Chuyển sang tự động

**File**: `01/002.mp3`

**Nội dung**: "Tắt chế độ thủ công, chuyển sang tự động"

**Trigger**: Khi user chuyển từ MANUAL → AUTO

**TTS Command**:
```bash
gtts-cli "Tắt chế độ thủ công, chuyển sang tự động" --lang vi --output 002.mp3
```

---

### Track 003: Đang mở quạt chính

**File**: `01/003.mp3`

**Nội dung**: "Đang mở quạt chính"

**Trigger**: Manual control - Fan main ON

**TTS Command**:
```bash
gtts-cli "Đang mở quạt chính" --lang vi --output 003.mp3
```

---

### Track 004: Đang tắt quạt chính

**File**: `01/004.mp3`

**Nội dung**: "Đang tắt quạt chính"

**Trigger**: Manual control - Fan main OFF

**TTS Command**:
```bash
gtts-cli "Đang tắt quạt chính" --lang vi --output 004.mp3
```

---

### Track 005: Đang mở cửa sổ

**File**: `01/005.mp3`

**Nội dung**: "Đang mở cửa sổ thông gió"

**Trigger**: Manual control - Window OPEN

**TTS Command**:
```bash
gtts-cli "Đang mở cửa sổ thông gió" --lang vi --output 005.mp3
```

---

### Track 006: Nhiệt độ cao

**File**: `01/006.mp3`

**Nội dung**: "Nhiệt độ cao, mở cửa sổ và bật quạt"

**Trigger**: AUTO mode - Temperature > threshold

**TTS Command**:
```bash
gtts-cli "Nhiệt độ cao, mở cửa sổ và bật quạt" --lang vi --output 006.mp3
```

---

### Track 007: Độ ẩm thấp

**File**: `01/007.mp3`

**Nội dung**: "Độ ẩm đất thấp, bơm nước đang chạy"

**Trigger**: AUTO mode - Soil moisture < threshold

**TTS Command**:
```bash
gtts-cli "Độ ẩm đất thấp, bơm nước đang chạy" --lang vi --output 007.mp3
```

---

### Track 008: Phát hiện lửa 🚨

**File**: `01/008.mp3`

**Nội dung**: "Phát hiện lửa! Kích hoạt hệ thống dập. Vui lòng kiểm tra ngay!"

**Trigger**: Emergency - Flame detected

**TTS Command**:
```bash
gtts-cli "Phát hiện lửa! Kích hoạt hệ thống dập. Vui lòng kiểm tra ngay!" --lang vi --output 008.mp3
```

**Priority**: HIGHEST

---

### Track 009: Âm thanh lớn 🔊

**File**: `01/009.mp3`

**Nội dung**: "Âm thanh lớn phát hiện. Vui lòng giảm tiếng ồn."

**Trigger**: Sound sensor > threshold

**TTS Command**:
```bash
gtts-cli "Âm thanh lớn phát hiện. Vui lòng giảm tiếng ồn." --lang vi --output 009.mp3
```

---

### Track 010: Phát hiện bệnh cây 🌱

**File**: `01/010.mp3`

**Nội dung**: "Cây có dấu hiệu bệnh. Cần chăm sóc đặc biệt."

**Trigger**: PC-Vision detects diseased plant

**TTS Command**:
```bash
gtts-cli "Cây có dấu hiệu bệnh. Cần chăm sóc đặc biệt." --lang vi --output 010.mp3
```

**Note**: Nếu muốn thông báo cụ thể plant ID, cần dynamic TTS (chưa implement trong firmware).

---

## Optional Tracks (Mở rộng)

### Track 011: Bật bơm nước

**Nội dung**: "Đang bật bơm nước"

---

### Track 012: Tắt bơm nước

**Nội dung**: "Đang tắt bơm nước"

---

### Track 013: Mực nước thấp

**Nội dung**: "Mực nước bồn thấp. Vui lòng đổ nước."

---

### Track 014: Đang đóng cửa sổ

**Nội dung**: "Đang đóng cửa sổ"

---

### Track 015: Hệ thống khởi động

**Nội dung**: "Hệ thống nhà kính thông minh đã khởi động"

---

## MQTT Control

### Play Track

**Topic**: `gh/cmd/dfp/play`

**Payload**:
```json
{
  "track": 8,
  "vol": 25
}
```

**Volume**: 0-30 (30 = max)

### Set Volume

**Topic**: `gh/cmd/dfp/volume`

**Payload**: `"25"` (string, 0-30)

### Stop Playback

**Topic**: `gh/cmd/dfp/stop`

**Payload**: `"STOP"`

---

## Python TTS Generation Script

### Install gtts-cli

```bash
pip install gtts-cli
```

### Batch Generate All Tracks

```bash
#!/bin/bash
# generate_tracks.sh

mkdir -p 01

gtts-cli "Bật chế độ thủ công" --lang vi --output 01/001.mp3
gtts-cli "Tắt chế độ thủ công, chuyển sang tự động" --lang vi --output 01/002.mp3
gtts-cli "Đang mở quạt chính" --lang vi --output 01/003.mp3
gtts-cli "Đang tắt quạt chính" --lang vi --output 01/004.mp3
gtts-cli "Đang mở cửa sổ thông gió" --lang vi --output 01/005.mp3
gtts-cli "Nhiệt độ cao, mở cửa sổ và bật quạt" --lang vi --output 01/006.mp3
gtts-cli "Độ ẩm đất thấp, bơm nước đang chạy" --lang vi --output 01/007.mp3
gtts-cli "Phát hiện lửa! Kích hoạt hệ thống dập. Vui lòng kiểm tra ngay!" --lang vi --output 01/008.mp3
gtts-cli "Âm thanh lớn phát hiện. Vui lòng giảm tiếng ồn." --lang vi --output 01/009.mp3
gtts-cli "Cây có dấu hiệu bệnh. Cần chăm sóc đặc biệt." --lang vi --output 01/010.mp3

echo "All tracks generated in ./01/"
```

**Run**:
```bash
chmod +x generate_tracks.sh
./generate_tracks.sh
```

### Python Script (Alternative)

```python
from gtts import gTTS
import os

tracks = {
    1: "Bật chế độ thủ công",
    2: "Tắt chế độ thủ công, chuyển sang tự động",
    3: "Đang mở quạt chính",
    4: "Đang tắt quạt chính",
    5: "Đang mở cửa sổ thông gió",
    6: "Nhiệt độ cao, mở cửa sổ và bật quạt",
    7: "Độ ẩm đất thấp, bơm nước đang chạy",
    8: "Phát hiện lửa! Kích hoạt hệ thống dập. Vui lòng kiểm tra ngay!",
    9: "Âm thanh lớn phát hiện. Vui lòng giảm tiếng ồn.",
    10: "Cây có dấu hiệu bệnh. Cần chăm sóc đặc biệt."
}

os.makedirs("01", exist_ok=True)

for track_num, text in tracks.items():
    tts = gTTS(text=text, lang='vi', slow=False)
    filename = f"01/{track_num:03d}.mp3"
    tts.save(filename)
    print(f"✓ Generated {filename}")

print("All tracks generated!")
```

**Run**:
```bash
pip install gtts
python generate_tracks.py
```

---

## Testing DFPlayer

### Test via Serial Monitor

```cpp
// Arduino UNO test code
#include <SoftwareSerial.h>

SoftwareSerial dfSerial(10, 11);  // RX, TX

void setup() {
  Serial.begin(9600);
  dfSerial.begin(9600);
  delay(1000);

  // Init
  sendDFCommand(0x3F, 0, 0);
  delay(500);

  // Set volume 25
  sendDFCommand(0x06, 0, 25);
  delay(500);

  // Play track 1
  sendDFCommand(0x03, 0, 1);
}

void loop() {
  // Listen for user input
  if (Serial.available()) {
    int track = Serial.parseInt();
    if (track > 0) {
      Serial.print("Playing track ");
      Serial.println(track);
      sendDFCommand(0x03, 0, track);
    }
  }
}

void sendDFCommand(byte cmd, byte param1, byte param2) {
  byte buffer[10] = {0x7E, 0xFF, 0x06, cmd, 0x00, param1, param2, 0x00, 0x00, 0xEF};

  int sum = -(buffer[1] + buffer[2] + buffer[3] + buffer[4] + buffer[5] + buffer[6]);
  buffer[7] = (sum >> 8) & 0xFF;
  buffer[8] = sum & 0xFF;

  dfSerial.write(buffer, 10);
}
```

**Test**:
1. Upload code
2. Open Serial Monitor (9600 baud)
3. Type `1` → Enter (play track 1)
4. Type `8` → Enter (play track 8)

---

## Troubleshooting

### DFPlayer không phát

1. **Kiểm tra thẻ TF**:
   - Format FAT32 (not exFAT!)
   - File naming: `01/001.mp3`, `01/002.mp3`, ...
   - Try 16GB or smaller card (some DFPlayer có vấn đề với 32GB)

2. **Kiểm tra kết nối**:
   - VCC = 5V (hoặc 3.3-5V)
   - GND chung
   - TX → D10 (UNO RX)
   - RX → D11 (UNO TX) + resistor 1kΩ

3. **Kiểm tra volume**:
   - Tăng volume lên 25-30
   - Test với headphones/speaker

4. **Kiểm tra file MP3**:
   - Bitrate: 128kbps (not too high!)
   - Sample rate: 44.1kHz
   - Try re-encode với ffmpeg:
     ```bash
     ffmpeg -i input.mp3 -ar 44100 -ab 128k output.mp3
     ```

### DFPlayer phát bị chậm/nhanh

- Kiểm tra sample rate (phải 44.1kHz hoặc 48kHz)
- Thử card khác (một số card kém chất lượng)

### DFPlayer noise/static

- Thêm capacitor 100-470µF gần VCC pin
- Kiểm tra nguồn 5V ổn định
- Chống nhiễu từ relay (tách nguồn)

---

**Last updated**: 2025-01-22
