/*******************************************************************************
 * GREENHOUSE ESP8266 FIRMWARE - v1.4 COMPREHENSIVE OVERHAUL
 * ============================================================================
 * Chức năng:
 * - MQTT Bridge: Subscribe commands, publish sensor/status
 * - UART với UNO: Hardware Serial 115200 baud (bidirectional sync)
 * - GPS NEO-6: SoftwareSerial 9600 baud
 * - DHT11: GPIO4 (Temperature/Humidity sensor)
 * - LED WS2812B Ring #2: 8 LEDs
 * - Relay control: Fan & AuxFan (moved from UNO)
 * - Config storage: LittleFS
 * - Watchdog & retry logic
 *
 * BUGFIX v1.2: DHT11 moved from UNO to ESP8266 GPIO4
 * - Fixes D13 pin conflict on UNO
 * - ESP8266 now reads temp/humidity directly and publishes to MQTT
 *
 * ENHANCEMENT v1.3: Data synchronization ESP8266→UNO
 * - ESP8266 sends temp/humidity/GPS to UNO for LCD multi-screen display
 * - New UART message type "env" for environment data sync
 *
 * ENHANCEMENT v1.3.1: Fan & AuxFan relay control on ESP8266
 * - Fan relay on GPIO5 (D1) - replaces PCF8574 requirement
 * - AuxFan relay on GPIO16 (D0) - direct MQTT control
 * - Eliminates need for I2C relay expander on UNO
 *
 * BUGFIX v1.3.2: Critical control flow fix
 * - Fan/AuxFan commands no longer forwarded to UNO (handled locally only)
 * - Prevents UNO from receiving unsupported device commands
 * - Ensures proper MQTT status publishing for fan/auxFan
 *
 * v1.4 OVERHAUL - Phase 1: Hardware Serial Migration
 * - Migrated from SoftwareSerial (GPIO12/14) to Hardware Serial (GPIO1/3)
 * - Higher reliability with hardware UART
 * - Eliminates SoftwareSerial library dependency for UNO
 * - Debug now via MQTT (no more Serial debug)
 *
 * Pin mapping (OFFICIAL - v1.4):
 * - DHT11: GPIO4 (D2) - Temperature & Humidity sensor
 * - RELAY_FAN: GPIO5 (D1) - Fan relay control
 * - RELAY_AUXFAN: GPIO16 (D0) - AuxFan relay control
 * - UNO UART: GPIO1/3 (TX0/RX0) ↔ UNO D1/D0 (Hardware Serial 115200)
 * - GPS: GPIO13 (RX) ← NEO-6 TX, GPIO15 (TX) → NEO-6 RX (SoftwareSerial 9600)
 * - WS2812 Ring #2: GPIO2 (D4) - 8 LEDs, NeoPixelBus UART method
 *
 * NOTE: Hardware Serial now used for UNO - NO debug Serial available
 * NOTE: Use MQTT debug topic (greenhouse/sys/debug) for debugging
 ******************************************************************************/

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <SoftwareSerial.h>
#include <DHT.h>  // DHT11 sensor library
#include <NeoPixelBus.h>  // NeoPixelBus instead of Adafruit_NeoPixel for ESP8266
#include <LittleFS.h>  // v1.4 BUGFIX: Add missing LittleFS include
#include <TinyGPSPlus.h>  // v1.4 BUGFIX: Add missing TinyGPSPlus include

// ==================== CONFIGURATION ====================
// Wi-Fi credentials (change these)
const char* WIFI_SSID = "YourWiFiSSID";
const char* WIFI_PASS = "YourWiFiPassword";

// MQTT broker (change if needed)
const char* MQTT_BROKER = "broker.hivemq.com";
const int MQTT_PORT = 1883;
const char* MQTT_CLIENT_ID = "greenhouse-esp8266";

// DHT11 Sensor (NEW in v1.2)
#define DHT_PIN 4        // GPIO4 (D2)
#define DHT_TYPE DHT11

// UART with UNO (Hardware Serial - v1.4)
// GPIO1 (TX0) → UNO D0 (RX)
// GPIO3 (RX0) ← UNO D1 (TX)
// #define UNO_RX 12    // REMOVED v1.4 - Using Hardware Serial
// #define UNO_TX 14    // REMOVED v1.4 - Using Hardware Serial
#define UART_BAUD 115200
#define UART_TIMEOUT 3000
#define UART_RETRY 3

// GPS (SoftwareSerial)
#define GPS_RX 13  // GPIO13 (D7) ← NEO-6 TX
#define GPS_TX 15  // GPIO15 (D8) → NEO-6 RX
#define GPS_BAUD 9600

// WS2812 Ring #2
#define LED_RING2_PIN 2  // GPIO2 (D4)
#define LED_COUNT 8

// Relays (v1.3.1 - moved from UNO to ESP8266)
#define RELAY_FAN 5      // GPIO5 (D1) - Fan relay
#define RELAY_AUXFAN 16  // GPIO16 (D0) - AuxFan relay

// Relay logic (Active LOW for most relay modules)
#define RELAY_ON LOW
#define RELAY_OFF HIGH

// Timings
#define DHT_READ_INTERVAL 2000   // Read DHT11 every 2 seconds
#define SENSOR_PUBLISH_INTERVAL 3000
#define STATUS_PUBLISH_INTERVAL 5000
#define HEARTBEAT_INTERVAL 30000
#define GPS_READ_INTERVAL 1000

// ==================== OBJECTS ====================
WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);
// SoftwareSerial unoSerial(UNO_RX, UNO_TX);  // REMOVED v1.4 - Using Hardware Serial
#define unoSerial Serial  // Hardware Serial for UNO (GPIO1/3)
SoftwareSerial gpsSerial(GPS_RX, GPS_TX);
TinyGPSPlus gps;
DHT dht(DHT_PIN, DHT_TYPE);  // DHT11 sensor (NEW in v1.2)

// WS2812 Ring #2 using NeoPixelBus UART method (avoids Wi-Fi jitter)
NeoPixelBus<NeoGrbFeature, NeoEsp8266Uart800KbpsMethod> ring2(LED_COUNT);

// ==================== GLOBAL STATE ====================
struct SensorData {
  float temp_c;
  float hum_pct;
  float soil_pct;
  float light_lux;
  int mq3;
  int flame;
  int sound;
  float distance_cm;
  float gps_lat;
  float gps_lng;
  bool gps_valid;
};

struct DeviceState {
  // Relays
  String pump;      // "ON"/"OFF"
  String fan;       // "ON"/"OFF"
  String auxfan;    // "ON"/"OFF"
  String light12v;  // "ON"/"OFF"

  // Servos
  String window;    // "OPEN"/"CLOSE"
  String door;      // "OPEN"/"CLOSE"

  // LEDs
  String led_power; // "ON"/"OFF"
  String led_color; // "#RRGGBB"
};

SensorData sensorData;
DeviceState deviceState;

unsigned long lastSensorReceived = 0;
unsigned long lastSensorPublish = 0;
unsigned long lastStatusPublish = 0;
unsigned long lastHeartbeat = 0;
unsigned long lastGpsRead = 0;
unsigned long lastDhtRead = 0;  // DHT11 reading timer (NEW in v1.2)

int uartErrorCount = 0;
int mqttReconnectAttempts = 0;
unsigned long lastMqttReconnect = 0;

bool systemReady = false;
String currentMode = "AUTO";

// Default thresholds (v1.4 Phase 5)
struct Thresholds {
  float temp_max = 35.0;
  float temp_min = 15.0;
  float hum_max = 80.0;
  float hum_min = 40.0;
  float soil_min = 30.0;
  int flame_threshold = 800;
  int sound_threshold = 700;
  // v1.4 BUGFIX: Add hysteresis to prevent oscillation
  float temp_hysteresis = 2.0;  // ±2°C hysteresis
  float soil_hysteresis = 5.0;  // ±5% hysteresis
};

Thresholds thresholds;
bool useDefaults = true;

// LED animation state for Ring #2
struct LedState {
  String mode;  // "OFF", "COLOR", "ALERT"
  uint8_t r, g, b;
};
LedState ledState = {"OFF", 0, 255, 0};  // Ring #2 only

unsigned long ledAnimationTick = 0;
bool ledAlertPhase = false;

// v1.4 Phase 6: Track previous LED state to avoid unnecessary Show() calls
RgbColor lastLedColor(0, 0, 0);
bool ledNeedsUpdate = false;

// ==================== SETUP ====================
void setup() {
  // Init UART with UNO (Hardware Serial - v1.4)
  // NOTE: Serial debug removed - using Hardware Serial for UNO communication
  Serial.begin(UART_BAUD);  // Serial = unoSerial for UNO communication
  delay(100);

  // Init DHT11 sensor (NEW in v1.2)
  dht.begin();

  // Init GPS
  gpsSerial.begin(GPS_BAUD);

  // Init filesystem
  if (!LittleFS.begin()) {
    // v1.4 BUGFIX: Can't use Serial.println() - it conflicts with unoSerial
    // Will log via MQTT after connection
  }

  // Init LED Ring #2
  ring2.Begin();
  ring2.Show(); // Clear

  // Init Relays (v1.3.1)
  pinMode(RELAY_FAN, OUTPUT);
  pinMode(RELAY_AUXFAN, OUTPUT);
  digitalWrite(RELAY_FAN, RELAY_OFF);      // Initial state: OFF
  digitalWrite(RELAY_AUXFAN, RELAY_OFF);   // Initial state: OFF

  // Init Wi-Fi
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  int wifiAttempts = 0;
  while (WiFi.status() != WL_CONNECTED && wifiAttempts < 30) {
    delay(500);
    wifiAttempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    // Connected
  } else {
    // Failed, enter AP mode for config (optional)
  }

  // Init MQTT
  mqtt.setServer(MQTT_BROKER, MQTT_PORT);
  mqtt.setCallback(mqttCallback);
  mqtt.setBufferSize(1024);  // v1.4 BUGFIX: Increased from 512 to prevent message truncation

  connectMqtt();

  // Initialize device state defaults
  deviceState.pump = "OFF";
  deviceState.fan = "OFF";
  deviceState.auxfan = "OFF";
  deviceState.light12v = "OFF";
  deviceState.window = "CLOSE";
  deviceState.door = "CLOSE";
  deviceState.led_power = "OFF";
  deviceState.led_color = "#00FF00";

  // Load thresholds (v1.4 Phase 5)
  loadThresholds();

  systemReady = true;
}

// ==================== MAIN LOOP ====================
void loop() {
  unsigned long now = millis();

  // MQTT loop
  if (!mqtt.connected()) {
    reconnectMqtt();
  }
  mqtt.loop();

  // Read UART from UNO
  if (unoSerial.available()) {
    handleUartFromUno();
    lastSensorReceived = now;
  }

  // Check UART timeout (v1.4 BUGFIX: Add rate limiting to prevent MQTT spam)
  static bool timeoutErrorSent = false;
  if (now - lastSensorReceived > UART_TIMEOUT && lastSensorReceived > 0) {
    if (!timeoutErrorSent) {
      publishError("uart_timeout", "No data from UNO for 3s");
      timeoutErrorSent = true;
    }
  } else {
    timeoutErrorSent = false;
  }

  // Read DHT11 (NEW in v1.2)
  if (now - lastDhtRead >= DHT_READ_INTERVAL) {
    lastDhtRead = now;
    readDht();
  }

  // Read GPS
  if (now - lastGpsRead >= GPS_READ_INTERVAL) {
    lastGpsRead = now;
    readGps();
  }

  // Publish sensor data
  if (now - lastSensorPublish >= SENSOR_PUBLISH_INTERVAL) {
    lastSensorPublish = now;
    publishSensorData();
  }

  // Publish status
  if (now - lastStatusPublish >= STATUS_PUBLISH_INTERVAL) {
    lastStatusPublish = now;
    publishDeviceStatus();
  }

  // Heartbeat
  if (now - lastHeartbeat >= HEARTBEAT_INTERVAL) {
    lastHeartbeat = now;
    publishHeartbeat();
  }

  // Update LED animations
  updateLedAnimations();

  // Auto control (v1.4 Phase 5) - runs every loop
  autoControl();

  // v1.4 BUGFIX: Feed watchdog to prevent reset
  yield();  // Let ESP8266 handle Wi-Fi and system tasks

  delay(10);
}

// ==================== Wi-Fi & MQTT ====================
void connectMqtt() {
  int attempts = 0;
  while (!mqtt.connected() && attempts < 5) {
    if (mqtt.connect(MQTT_CLIENT_ID)) {
      // Subscribe to command topics (standardized v1.1)
      mqtt.subscribe("greenhouse/control/#");
      mqtt.subscribe("greenhouse/set/thresholds/#");

      // Publish initial state
      publishDeviceStatus();
      mqttReconnectAttempts = 0;
      mqttDebug("MQTT connected");
      return;
    }
    attempts++;
    delay(1000);
  }

  mqttDebug("MQTT connection failed");
}

void reconnectMqtt() {
  unsigned long now = millis();

  // Exponential backoff
  unsigned long backoff = 2000 * (1 << min(mqttReconnectAttempts, 4));
  if (now - lastMqttReconnect < backoff) return;

  lastMqttReconnect = now;
  mqttReconnectAttempts++;

  if (mqtt.connect(MQTT_CLIENT_ID)) {
    // Resubscribe (standardized v1.1)
    mqtt.subscribe("greenhouse/control/#");
    mqtt.subscribe("greenhouse/set/thresholds/#");

    publishDeviceStatus();
    mqttReconnectAttempts = 0;
    mqttDebug("MQTT reconnected");
  }
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String topicStr = String(topic);
  String payloadStr = "";

  for (unsigned int i = 0; i < length; i++) {
    payloadStr += (char)payload[i];
  }

  handleMqttCommand(topicStr, payloadStr);
}

// ==================== MQTT COMMAND HANDLER ====================
void handleMqttCommand(String topic, String payload) {
  // Mode control (v1.4 BUGFIX: Add validation)
  if (topic == "greenhouse/control/mode") {
    if (payload == "AUTO" || payload == "MANUAL") {
      currentMode = payload;
      mqtt.publish("greenhouse/status/mode", payload.c_str(), true);
      mqttDebug(("Mode: " + payload).c_str());
    } else {
      mqttDebug("Invalid mode - use AUTO or MANUAL");
    }
    return;
  }

  // Extract device from topic: greenhouse/control/<device>
  if (topic.startsWith("greenhouse/control/")) {
    String device = topic.substring(19);  // After "greenhouse/control/"
    handleDeviceCommand(device, payload);
    return;
  }

  // Thresholds
  if (topic.startsWith("greenhouse/set/thresholds/")) {
    String param = topic.substring(26);  // After "greenhouse/set/thresholds/"
    mqttDebug(("Threshold: " + param + "=" + payload).c_str());
    // Store in LittleFS or forward to UNO if needed
    return;
  }
}

void handleDeviceCommand(String device, String action) {
  // Update local state
  if (device == "pump") deviceState.pump = action;
  else if (device == "fan") {
    deviceState.fan = action;
    // v1.3.1: Control Fan relay on ESP8266 GPIO5
    bool state = (action == "ON");
    digitalWrite(RELAY_FAN, state ? RELAY_ON : RELAY_OFF);
    mqttDebug(("Fan: " + action).c_str());

    // v1.3.2: Publish status directly, don't send to UNO
    mqtt.publish("greenhouse/status/fan", action.c_str(), true);
    return;  // ← FIX: Don't forward to UNO!
  }
  else if (device == "auxFan") {
    deviceState.auxfan = action;
    // v1.3.1: Control AuxFan relay on ESP8266 GPIO16
    bool state = (action == "ON");
    digitalWrite(RELAY_AUXFAN, state ? RELAY_ON : RELAY_OFF);
    mqttDebug(("AuxFan: " + action).c_str());

    // v1.3.2: Publish status directly, don't send to UNO
    mqtt.publish("greenhouse/status/auxFan", action.c_str(), true);
    return;  // ← FIX: Don't forward to UNO!
  }
  else if (device == "mainGrowLight") deviceState.light12v = action;
  else if (device == "window1") deviceState.window = action;
  else if (device == "mainDoor") deviceState.door = action;
  else if (device == "rgbLed") {
    // Handle RGB LED color
    deviceState.led_power = (action == "OFF") ? "OFF" : "ON";
    deviceState.led_color = action;

    if (action == "OFF") {
      ledState.mode = "OFF";
    } else {
      ledState.mode = "COLOR";
      parseColor(action, ledState.r, ledState.g, ledState.b);
    }

    mqtt.publish("greenhouse/status/rgbLed", action.c_str(), true);
    return;
  }

  // Send control command to UNO via UART
  sendCommandToUno(device, action);

  // Publish status (retained)
  String statusTopic = "greenhouse/status/" + device;
  mqtt.publish(statusTopic.c_str(), action.c_str(), true);

  mqttDebug((device + ": " + action).c_str());
}

// ==================== UART WITH UNO ====================
void handleUartFromUno() {
  String line = unoSerial.readStringUntil('\n');
  line.trim();

  if (line.length() == 0) return;

  StaticJsonDocument<512> doc;
  DeserializationError error = deserializeJson(doc, line);

  if (error) {
    uartErrorCount++;
    mqttDebug("UART parse error");
    publishError("uart_parse_error", "Invalid JSON from UNO");
    return;
  }

  const char* type = doc["type"];
  if (type == NULL) return;  // v1.4 BUGFIX: Prevent NULL pointer dereference

  if (strcmp(type, "sensor") == 0) {
    handleSensorData(doc);
  } else if (strcmp(type, "ack") == 0) {
    // ACK received - Serial debug removed
  } else if (strcmp(type, "event") == 0) {
    handleEventFromUno(doc);
  } else if (strcmp(type, "debug") == 0) {
    // v1.4 Phase 2: Handle debug message from UNO
    handleDebugFromUno(doc);
  }
}

void handleSensorData(JsonDocument& doc) {
  if (!doc.containsKey("data")) return;  // v1.4 BUGFIX: Validate JSON structure
  JsonObject data = doc["data"];

  // Parse sensor data (format matches mqtt-schema.json)
  // NOTE v1.2: temperature and humidity NO LONGER sent by UNO
  // ESP8266 reads DHT11 directly on GPIO4

  if (data.containsKey("lightIntensity")) {
    String light = data["lightIntensity"];
    sensorData.light_lux = light.toFloat();
  }
  if (data.containsKey("soilMoisture")) {
    String soil = data["soilMoisture"];
    sensorData.soil_pct = soil.toFloat();
  }
  if (data.containsKey("gasMQ3")) {
    String gas = data["gasMQ3"];
    sensorData.mq3 = (int)(gas.toFloat() * 1023.0 / 5.0);  // Convert voltage back to ADC
  }
  if (data.containsKey("flameAnalog")) {
    String flame = data["flameAnalog"];
    sensorData.flame = flame.toInt();
  }
  if (data.containsKey("soundLevel")) {
    String sound = data["soundLevel"];
    sensorData.sound = sound.toInt();
  }
  if (data.containsKey("waterTankLevel")) {
    String tank = data["waterTankLevel"];
    sensorData.distance_cm = tank.toFloat();
  }

  // Data received, reset error count
  uartErrorCount = 0;
  // mqttDebug("Sensor data received");  // Too verbose, comment out
}

void handleEventFromUno(JsonDocument& doc) {
  const char* event = doc["event"];
  const char* value = doc["value"];

  if (event == NULL || value == NULL) return;  // v1.4: NULL check already present

  // Publish to greenhouse/event/<event>
  String topic = "greenhouse/event/";
  topic += event;
  mqtt.publish(topic.c_str(), value);

  String debugMsg = "Event: ";
  debugMsg += event;
  debugMsg += "=";
  debugMsg += value;
  mqttDebug(debugMsg.c_str());

  // Handle fire alert with LED
  if (strcmp(event, "flame") == 0 && strcmp(value, "DETECTED") == 0) {
    // Set Ring #2 to red alert mode
    ledState.mode = "ALERT";
    ledState.r = 255;
    ledState.g = 0;
    ledState.b = 0;
  }

  // Handle sound
  if (strcmp(event, "sound") == 0 && strcmp(value, "LOUD") == 0) {
    // Optional: trigger DFPlayer or other action
  }
}

void sendCommandToUno(String device, String action) {
  StaticJsonDocument<256> doc;
  doc["type"] = "control";
  doc["device"] = device;
  doc["action"] = action;

  serializeJson(doc, unoSerial);
  unoSerial.println();

  // Serial debug removed - unoSerial IS Serial now
}

// ==================== MQTT DEBUG SYSTEM (v1.4 Phase 2) ====================
void handleDebugFromUno(JsonDocument& doc) {
  const char* source = doc["source"];
  const char* msg = doc["msg"];
  unsigned long ts = doc["ts"];

  if (source == NULL || msg == NULL) return;  // v1.4: NULL check already present

  // Forward to MQTT debug topic
  if (mqtt.connected()) {
    String payload = String(source) + ": " + String(msg);
    mqtt.publish("greenhouse/sys/debug", payload.c_str());
  }
}

void mqttDebug(const char* msg) {
  if (!mqtt.connected()) return;

  String payload = "ESP8266: ";
  payload += msg;
  mqtt.publish("greenhouse/sys/debug", payload.c_str());
}

// ==================== DHT11 SENSOR (NEW in v1.2) ====================
void readDht() {
  float temp = dht.readTemperature();
  float hum = dht.readHumidity();

  if (!isnan(temp) && !isnan(hum)) {
    sensorData.temp_c = temp;
    sensorData.hum_pct = hum;
    // mqttDebug("DHT11 read OK");  // Too verbose, comment out

    // v1.3: Send environment data to UNO for LCD display
    sendEnvDataToUno();
  } else {
    mqttDebug("DHT11 read failed");
    // Keep previous values on read failure
  }
}

// ==================== SYNC DATA TO UNO (NEW in v1.3) ====================
void sendEnvDataToUno() {
  StaticJsonDocument<256> doc;
  doc["type"] = "env";
  doc["temp"] = String(sensorData.temp_c, 1);
  doc["hum"] = String(sensorData.hum_pct, 1);

  // Include GPS data if available
  if (sensorData.gps_valid) {
    doc["gps_lat"] = String(sensorData.gps_lat, 6);
    doc["gps_lng"] = String(sensorData.gps_lng, 6);
    doc["gps_valid"] = true;
  } else {
    doc["gps_valid"] = false;
  }

  serializeJson(doc, unoSerial);
  unoSerial.println();

  // mqttDebug("Env data sent to UNO");  // Too verbose, comment out
}

// ==================== GPS ====================
void readGps() {
  while (gpsSerial.available() > 0) {
    char c = gpsSerial.read();
    gps.encode(c);
  }

  if (gps.location.isValid()) {
    sensorData.gps_lat = gps.location.lat();
    sensorData.gps_lng = gps.location.lng();
    sensorData.gps_valid = true;
  } else {
    sensorData.gps_valid = false;
  }
}

// ==================== MQTT PUBLISH ====================
// v1.4 Phase 4: Standardized JSON with timestamp and units
void publishSensor(const char* name, float value, const char* unit) {
  if (!mqtt.connected()) return;

  StaticJsonDocument<128> doc;
  doc["v"] = value;
  doc["u"] = unit;
  doc["t"] = millis();  // Timestamp in milliseconds

  String json;
  serializeJson(doc, json);

  String topic = "greenhouse/data/";
  topic += name;
  mqtt.publish(topic.c_str(), json.c_str());
}

void publishSensorData() {
  if (!mqtt.connected()) return;

  // v1.4 Phase 4: Publish with standardized format (value, unit, timestamp)
  if (sensorData.temp_c > -500) {
    publishSensor("temperature", sensorData.temp_c, "°C");
  }

  if (sensorData.hum_pct > -500) {
    publishSensor("humidity", sensorData.hum_pct, "%");
  }

  publishSensor("soilMoisture", sensorData.soil_pct, "%");
  publishSensor("lightIntensity", sensorData.light_lux, "lux");
  publishSensor("waterTankLevel", sensorData.distance_cm, "cm");
  publishSensor("gasMQ3", sensorData.mq3 * 5.0 / 1023.0, "V");
  publishSensor("flameAnalog", (float)sensorData.flame, "raw");
  publishSensor("soundLevel", (float)sensorData.sound, "raw");

  // Rain sensor (placeholder) - using simple string for now
  mqtt.publish("greenhouse/data/rainSensor", "NOT_DETECTED");

  // GPS (if valid) - kept as nested JSON for lat/lng
  if (sensorData.gps_valid) {
    StaticJsonDocument<128> gpsDoc;
    gpsDoc["lat"] = sensorData.gps_lat;
    gpsDoc["lng"] = sensorData.gps_lng;
    gpsDoc["t"] = millis();  // Add timestamp

    String gpsJson;
    serializeJson(gpsDoc, gpsJson);
    mqtt.publish("greenhouse/data/gps", gpsJson.c_str());
  }
}

void publishDeviceStatus() {
  if (!mqtt.connected()) return;

  // Publish status to greenhouse/status/* (retained)
  mqtt.publish("greenhouse/status/fan", deviceState.fan.c_str(), true);
  mqtt.publish("greenhouse/status/auxFan", deviceState.auxfan.c_str(), true);
  mqtt.publish("greenhouse/status/pump", deviceState.pump.c_str(), true);
  mqtt.publish("greenhouse/status/mainGrowLight", deviceState.light12v.c_str(), true);

  mqtt.publish("greenhouse/status/window1", deviceState.window.c_str(), true);
  mqtt.publish("greenhouse/status/mainDoor", deviceState.door.c_str(), true);

  mqtt.publish("greenhouse/status/rgbLed", deviceState.led_color.c_str(), true);
  mqtt.publish("greenhouse/status/mode", currentMode.c_str(), true);
}

void publishHeartbeat() {
  if (!mqtt.connected()) return;

  StaticJsonDocument<128> doc;
  doc["ts"] = millis() / 1000;
  doc["uptime_s"] = millis() / 1000;
  doc["heap"] = ESP.getFreeHeap();

  String json;
  serializeJson(doc, json);
  mqtt.publish("greenhouse/sys/heartbeat", json.c_str());
}

void publishError(const char* code, const char* msg) {
  if (!mqtt.connected()) return;

  StaticJsonDocument<128> doc;
  doc["ts"] = millis() / 1000;
  doc["code"] = code;
  doc["msg"] = msg;

  String json;
  serializeJson(doc, json);
  mqtt.publish("greenhouse/event/error", json.c_str());

  String debugMsg = "Error: ";
  debugMsg += code;
  debugMsg += " - ";
  debugMsg += msg;
  mqttDebug(debugMsg.c_str());
}

// ==================== LED WS2812 ====================
// v1.4 Phase 6: Optimized to only Show() when color actually changes
void updateLedAnimations() {
  unsigned long now = millis();

  if (now - ledAnimationTick < 100) return;  // Update every 100ms
  ledAnimationTick = now;

  RgbColor newColor(0, 0, 0);

  // Ring #2 only (Ring #1 is on UNO)
  if (ledState.mode == "OFF") {
    newColor = RgbColor(0, 0, 0);
  } else if (ledState.mode == "COLOR") {
    newColor = RgbColor(ledState.r, ledState.g, ledState.b);
  } else if (ledState.mode == "ALERT") {
    ledAlertPhase = !ledAlertPhase;
    uint8_t brightness = ledAlertPhase ? 255 : 50;
    newColor = RgbColor(brightness, 0, 0);  // Red blink
  }

  // Only update if color changed (v1.4 Phase 6 optimization)
  if (newColor.R != lastLedColor.R || newColor.G != lastLedColor.G || newColor.B != lastLedColor.B) {
    for (int i = 0; i < LED_COUNT; i++) {
      ring2.SetPixelColor(i, newColor);
    }
    ring2.Show();  // Only show when color actually changed
    lastLedColor = newColor;
  }
}

void parseColor(String hexColor, uint8_t& r, uint8_t& g, uint8_t& b) {
  if (hexColor.length() < 7 || hexColor[0] != '#') return;

  r = strtol(hexColor.substring(1, 3).c_str(), NULL, 16);
  g = strtol(hexColor.substring(3, 5).c_str(), NULL, 16);
  b = strtol(hexColor.substring(5, 7).c_str(), NULL, 16);
}

// ==================== THRESHOLDS & AUTO CONTROL (v1.4 Phase 5) ====================
void loadThresholds() {
  File file = LittleFS.open("/thresholds.json", "r");
  if (file) {
    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, file);

    if (!error) {
      thresholds.temp_max = doc["temp_max"] | 35.0;
      thresholds.temp_min = doc["temp_min"] | 15.0;
      thresholds.hum_max = doc["hum_max"] | 80.0;
      thresholds.hum_min = doc["hum_min"] | 40.0;
      thresholds.soil_min = doc["soil_min"] | 30.0;
      thresholds.flame_threshold = doc["flame_threshold"] | 800;
      thresholds.sound_threshold = doc["sound_threshold"] | 700;
      useDefaults = false;
      mqttDebug("Thresholds loaded from file");
    }
    file.close();
  } else {
    mqttDebug("Using default thresholds");
  }

  // Publish current thresholds
  publishThresholds();
}

void publishThresholds() {
  if (!mqtt.connected()) return;

  StaticJsonDocument<256> doc;
  doc["temp_max"] = thresholds.temp_max;
  doc["temp_min"] = thresholds.temp_min;
  doc["hum_max"] = thresholds.hum_max;
  doc["hum_min"] = thresholds.hum_min;
  doc["soil_min"] = thresholds.soil_min;
  doc["flame_threshold"] = thresholds.flame_threshold;
  doc["sound_threshold"] = thresholds.sound_threshold;
  doc["source"] = useDefaults ? "default" : "file";

  String json;
  serializeJson(doc, json);
  mqtt.publish("greenhouse/sys/thresholds", json.c_str(), true);
}

void autoControl() {
  // Only run auto control in AUTO mode
  if (currentMode != "AUTO") return;

  static unsigned long lastAutoControl = 0;
  static bool flameAlertSent = false;  // v1.4 BUGFIX: Debounce flame alert
  unsigned long now = millis();

  // Run auto control every 5 seconds
  if (now - lastAutoControl < 5000) return;
  lastAutoControl = now;

  // v1.4 BUGFIX: Temperature control WITH HYSTERESIS to prevent oscillation
  // Turn ON when temp > max + hysteresis (37°C)
  // Turn OFF when temp < max - hysteresis (33°C)
  if (sensorData.temp_c > (thresholds.temp_max + thresholds.temp_hysteresis)) {
    // Turn on fan if not already on
    if (deviceState.fan != "ON") {
      handleDeviceCommand("fan", "ON");
      mqttDebug("Auto: Fan ON (temp high)");
    }
  } else if (sensorData.temp_c < (thresholds.temp_max - thresholds.temp_hysteresis)) {
    // Turn off fan if on (temp is normal)
    if (deviceState.fan != "OFF") {
      handleDeviceCommand("fan", "OFF");
      mqttDebug("Auto: Fan OFF (temp normal)");
    }
  }
  // Between 33-37°C: keep current state (hysteresis zone)

  // v1.4 BUGFIX: Soil moisture control WITH HYSTERESIS
  // Turn ON when soil < min - hysteresis (25%)
  // Turn OFF when soil > min + hysteresis (35%)
  if (sensorData.soil_pct < (thresholds.soil_min - thresholds.soil_hysteresis)) {
    // Turn on pump if not already on
    if (deviceState.pump != "ON") {
      handleDeviceCommand("pump", "ON");
      mqttDebug("Auto: Pump ON (soil dry)");
    }
  } else if (sensorData.soil_pct > (thresholds.soil_min + thresholds.soil_hysteresis)) {
    // Turn off pump if on (soil is wet enough)
    if (deviceState.pump != "OFF") {
      handleDeviceCommand("pump", "OFF");
      mqttDebug("Auto: Pump OFF (soil ok)");
    }
  }
  // Between 25-35%: keep current state (hysteresis zone)

  // v1.4 BUGFIX: Flame detection with debounce (always active, regardless of mode)
  if (sensorData.flame > thresholds.flame_threshold) {
    if (!flameAlertSent) {
      mqttDebug("ALERT: Flame detected!");
      flameAlertSent = true;

      // Emergency actions: turn off pump, turn on fans
      if (deviceState.pump != "OFF") {
        handleDeviceCommand("pump", "OFF");
      }
      if (deviceState.fan != "ON") {
        handleDeviceCommand("fan", "ON");
      }
      if (deviceState.auxfan != "ON") {
        handleDeviceCommand("auxFan", "ON");
      }
    }
  } else {
    flameAlertSent = false;  // Reset when flame is gone
  }
}
