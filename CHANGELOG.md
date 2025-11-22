# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.0.0] - 2025-01-22

### Added
- **Firmware Arduino UNO R3**
  - Sensor reading: DHT11, BH1750, JSN-SR04T, MQ-3, Flame, Sound, Soil moisture
  - Actuator control: 4 relays, 2 servos, DFPlayer Mini
  - LCD1602 I²C display with message queue
  - UART bridge with ESP8266 (JSON protocol, 57600 baud)
  - Fallback safety mode when UART lost
  - Emergency detection: Fire, high temperature

- **Firmware ESP8266**
  - MQTT bridge (HiveMQ public broker)
  - UART communication with UNO (JSON protocol)
  - GPS NEO-6 support (SoftwareSerial)
  - WS2812B LED rings control (2 × 8 LEDs)
  - Watchdog & reconnect logic
  - LittleFS config storage

- **Firmware ESP32-CAM**
  - HTTP WebServer on port 80
  - MJPEG streaming (`/stream`)
  - Single image capture (`/capture`)
  - Flash LED PWM control (`/led?duty=0..255`)
  - Camera control API (`/control`)
  - Status endpoint (`/status`)
  - PSRAM support
  - SoftAP fallback

- **PC-Vision (Python)**
  - YOLO v8/v11 plant detection (counting)
  - Hugging Face disease detection model
  - Image fetching from ESP32-CAM via HTTP
  - MQTT result publishing to `gh/cv/detections`
  - Configurable via `.env` file
  - Test suite included

- **App (PyQt6)**
  - Dark/Light theme support
  - Bilingual (Vietnamese/English)
  - Real-time sensor monitoring
  - Camera live streaming (HTTP)
  - AUTO/MANUAL mode control
  - Manual device control
  - Auto rules configuration with thresholds
  - LED RGB color picker (bidirectional sync)
  - PlantTalk (DFPlayer) integration
  - Alert configuration
  - Activity logs
  - MQTT connection with retry logic

- **Documentation**
  - Complete README with quickstart guide
  - Arduino UNO pinout detailed
  - ESP8266 pinout detailed
  - MQTT topics reference
  - UART protocol specification
  - Auto control logic documentation
  - Fritzing diagram guidelines

- **Tooling**
  - `.gitignore` for Python/Arduino/IDE
  - Requirements.txt for PC-Vision and App
  - `.env.example` for PC-Vision configuration

### Security
- UART level shifter (5V → 3.3V) for safe ESP8266 communication
- Relay protection with diodes
- Servo power isolation
- Emergency safe state on connection loss

### Fixed
- LCD display race conditions with message queue
- MQTT reconnect exponential backoff
- Sensor reading edge cases (NaN, -1, error values)
- LED color sync between MQTT and UI

## [Unreleased]

### Planned
- Web dashboard (additional to desktop app)
- Historical data logging (InfluxDB/Grafana)
- Push notifications (Telegram/Email)
- Multi-language support (Spanish, Chinese)
- OTA firmware updates
- Cloud MQTT broker integration
- Machine learning model training pipeline
- Mobile app (React Native)

---

[1.0.0]: https://github.com/yourusername/greenhouse-iot/releases/tag/v1.0.0
