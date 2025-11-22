/*******************************************************************************
 * GREENHOUSE ESP8266 FIRMWARE
 * ============================================================================
 * Chức năng:
 * - MQTT Bridge: Subscribe commands, publish sensor/status
 * - UART với UNO: JSON protocol 57600 baud
 * - GPS NEO-6: SoftwareSerial 9600 baud
 * - LED WS2812B: 2 rings × 8 LEDs
 * - Config storage: LittleFS
 * - Watchdog & retry logic
 *
 * Pin mapping (BẮT BUỘC):
 * - UART0: GPIO1 (TX) ↔ UNO D0, GPIO3 (RX) ↔ UNO D1
 * - GPS: GPIO12 (RX), GPIO14 (TX) - SoftwareSerial 9600
 * - WS2812: Ring1 = GPIO2 (D4), Ring2 = GPIO5 (D1)
 * - Reserve: GPIO4, GPIO15
 ******************************************************************************/

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <SoftwareSerial.h>
#include <Adafruit_NeoPixel.h>
#include <LittleFS.h>
#include <TinyGPSPlus.h>

// ==================== CONFIGURATION ====================
// Wi-Fi credentials (change these)
const char* WIFI_SSID = "YourWiFiSSID";
const char* WIFI_PASS = "YourWiFiPassword";

// MQTT broker (change if needed)
const char* MQTT_BROKER = "broker.hivemq.com";
const int MQTT_PORT = 1883;
const char* MQTT_CLIENT_ID = "gh-esp8266";

// UART with UNO
#define UART_BAUD 57600
#define UART_TIMEOUT 3000
#define UART_RETRY 3

// GPS
#define GPS_RX 12  // GPIO12 = D6
#define GPS_TX 14  // GPIO14 = D5
#define GPS_BAUD 9600

// WS2812
#define LED_RING1_PIN 2  // GPIO2 = D4
#define LED_RING2_PIN 5  // GPIO5 = D1
#define LED_COUNT 8

// Timings
#define SENSOR_PUBLISH_INTERVAL 3000
#define STATUS_PUBLISH_INTERVAL 5000
#define HEARTBEAT_INTERVAL 30000
#define GPS_READ_INTERVAL 1000

// ==================== OBJECTS ====================
WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);
SoftwareSerial gpsSerial(GPS_RX, GPS_TX);
TinyGPSPlus gps;

Adafruit_NeoPixel ring1(LED_COUNT, LED_RING1_PIN, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel ring2(LED_COUNT, LED_RING2_PIN, NEO_GRB + NEO_KHZ800);

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

int uartErrorCount = 0;
int mqttReconnectAttempts = 0;
unsigned long lastMqttReconnect = 0;

bool systemReady = false;
String currentMode = "AUTO";

// LED animation state
struct LedState {
  String mode;  // "OFF", "COLOR", "ALERT"
  uint8_t r, g, b;
};
LedState led1State = {"OFF", 0, 255, 0};
LedState led2State = {"OFF", 0, 255, 0};

unsigned long ledAnimationTick = 0;
bool ledAlertPhase = false;

// ==================== SETUP ====================
void setup() {
  Serial.begin(UART_BAUD);
  delay(100);

  // Init filesystem
  if (!LittleFS.begin()) {
    Serial.println("{\"error\":\"LittleFS init failed\"}");
  }

  // Init GPS
  gpsSerial.begin(GPS_BAUD);

  // Init LED rings
  ring1.begin();
  ring2.begin();
  ring1.show(); // Clear
  ring2.show();

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
  mqtt.setBufferSize(512);

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
  if (Serial.available()) {
    handleUartFromUno();
    lastSensorReceived = now;
  }

  // Check UART timeout
  if (now - lastSensorReceived > UART_TIMEOUT && lastSensorReceived > 0) {
    publishError("uart_timeout", "No data from UNO for 3s");
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

  delay(10);
}

// ==================== Wi-Fi & MQTT ====================
void connectMqtt() {
  int attempts = 0;
  while (!mqtt.connected() && attempts < 5) {
    if (mqtt.connect(MQTT_CLIENT_ID)) {
      // Subscribe to command topics
      mqtt.subscribe("gh/cmd/mode");
      mqtt.subscribe("gh/cmd/relay/#");
      mqtt.subscribe("gh/cmd/servo/#");
      mqtt.subscribe("gh/cmd/led/#");
      mqtt.subscribe("gh/cmd/dfp/#");
      mqtt.subscribe("gh/server/#");

      // Publish initial state
      publishDeviceStatus();
      publishMqttStatus(true);
      mqttReconnectAttempts = 0;
      return;
    }
    attempts++;
    delay(1000);
  }

  publishMqttStatus(false);
}

void reconnectMqtt() {
  unsigned long now = millis();

  // Exponential backoff
  unsigned long backoff = 2000 * (1 << min(mqttReconnectAttempts, 4));
  if (now - lastMqttReconnect < backoff) return;

  lastMqttReconnect = now;
  mqttReconnectAttempts++;

  if (mqtt.connect(MQTT_CLIENT_ID)) {
    // Resubscribe
    mqtt.subscribe("gh/cmd/mode");
    mqtt.subscribe("gh/cmd/relay/#");
    mqtt.subscribe("gh/cmd/servo/#");
    mqtt.subscribe("gh/cmd/led/#");
    mqtt.subscribe("gh/cmd/dfp/#");
    mqtt.subscribe("gh/server/#");

    publishDeviceStatus();
    publishMqttStatus(true);
    mqttReconnectAttempts = 0;
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
  // Mode change
  if (topic == "gh/cmd/mode") {
    currentMode = payload;
    mqtt.publish("gh/state/mode", payload.c_str(), true);
    return;
  }

  // Relay commands
  if (topic.startsWith("gh/cmd/relay/")) {
    String relay = topic.substring(13);  // Extract relay name
    handleRelayCommand(relay, payload);
    return;
  }

  // Servo commands
  if (topic.startsWith("gh/cmd/servo/")) {
    String servo = topic.substring(13);
    handleServoCommand(servo, payload);
    return;
  }

  // LED commands
  if (topic == "gh/cmd/led/power") {
    deviceState.led_power = payload;
    mqtt.publish("gh/status/led/power", payload.c_str(), true);

    if (payload == "ON") {
      led1State.mode = "COLOR";
      led2State.mode = "COLOR";
      parseColor(deviceState.led_color, led1State.r, led1State.g, led1State.b);
      led2State.r = led1State.r;
      led2State.g = led1State.g;
      led2State.b = led1State.b;
    } else {
      led1State.mode = "OFF";
      led2State.mode = "OFF";
    }
    return;
  }

  if (topic == "gh/cmd/led/color") {
    deviceState.led_color = payload;
    mqtt.publish("gh/status/led/color", payload.c_str(), true);

    if (deviceState.led_power == "ON") {
      parseColor(payload, led1State.r, led1State.g, led1State.b);
      led2State.r = led1State.r;
      led2State.g = led1State.g;
      led2State.b = led1State.b;
    }
    return;
  }

  // DFPlayer commands
  if (topic == "gh/cmd/dfp/volume") {
    // Forward to UNO (not implemented in minimal version)
    return;
  }

  if (topic == "gh/cmd/dfp/play" || topic == "gh/cmd/dfp/stop") {
    // Forward to UNO (not implemented in minimal version)
    return;
  }

  // Server commands (from app)
  if (topic == "gh/server/mode") {
    currentMode = payload;
    mqtt.publish("gh/state/mode", payload.c_str(), true);
    return;
  }

  if (topic == "gh/server/planttalk") {
    // Handle PlantTalk enable/disable
    return;
  }
}

void handleRelayCommand(String relay, String cmd) {
  int cmdVal = 0;
  if (cmd == "ON") cmdVal = 1;
  else if (cmd == "OFF") cmdVal = 0;
  else if (cmd == "BLINK") cmdVal = 2;

  // Update state
  if (relay == "fan") deviceState.fan = cmd;
  else if (relay == "auxfan") deviceState.auxfan = cmd;
  else if (relay == "pump") deviceState.pump = cmd;
  else if (relay == "light12v") deviceState.light12v = cmd;

  // Send to UNO
  sendCommandToUno(relay, cmdVal, true);

  // Publish status
  mqtt.publish(("gh/status/relay/" + relay).c_str(), cmd.c_str(), true);
}

void handleServoCommand(String servo, String cmd) {
  int cmdVal = (cmd == "OPEN") ? 1 : 0;

  // Update state
  if (servo == "window") deviceState.window = cmd;
  else if (servo == "door") deviceState.door = cmd;

  // Send to UNO
  sendCommandToUno(servo, cmdVal, false);

  // Publish status
  mqtt.publish(("gh/status/servo/" + servo).c_str(), cmd.c_str(), true);
}

// ==================== UART WITH UNO ====================
void handleUartFromUno() {
  String line = Serial.readStringUntil('\n');
  line.trim();

  if (line.length() == 0) return;

  StaticJsonDocument<512> doc;
  DeserializationError error = deserializeJson(doc, line);

  if (error) {
    uartErrorCount++;
    publishError("uart_parse_error", "Invalid JSON from UNO");
    return;
  }

  const char* type = doc["type"];

  if (strcmp(type, "sensor") == 0) {
    handleSensorData(doc);
  } else if (strcmp(type, "ack") == 0) {
    // ACK received
  } else if (strcmp(type, "event") == 0) {
    handleEventFromUno(doc);
  }
}

void handleSensorData(JsonDocument& doc) {
  JsonObject payload = doc["payload"];

  sensorData.temp_c = payload["temp_c"] | -999.0;
  sensorData.hum_pct = payload["hum_pct"] | -999.0;
  sensorData.light_lux = payload["light_lux"] | 0.0;
  sensorData.soil_pct = payload["soil_pct"] | 0.0;
  sensorData.mq3 = payload["mq3"] | 0;
  sensorData.flame = payload["flame"] | 0;
  sensorData.sound = payload["sound"] | 0;
  sensorData.distance_cm = payload["distance_cm"] | -1.0;

  // Data received, reset error count
  uartErrorCount = 0;
}

void handleEventFromUno(JsonDocument& doc) {
  const char* event = doc["event"];
  int level = doc["level"];

  if (strcmp(event, "FIRE") == 0) {
    // Fire detected
    mqtt.publish("gh/sensor/flame_do", "1");

    // Set LEDs to alert
    led1State.mode = "ALERT";
    led1State.r = 255;
    led1State.g = 0;
    led1State.b = 0;
    led2State = led1State;
  } else if (strcmp(event, "SOUND") == 0) {
    mqtt.publish("gh/sensor/sound_do", "1");
  }
}

void sendCommandToUno(String device, int value, bool isRelay) {
  StaticJsonDocument<256> doc;
  doc["type"] = "set";
  doc["ts"] = millis() / 1000;
  doc["req_id"] = random(1000, 9999);

  JsonObject payload = doc.createNestedObject("payload");

  if (isRelay) {
    if (device == "fan") payload["fan_main"] = value;
    else if (device == "auxfan") payload["fan_aux"] = value;
    else if (device == "pump") payload["pump"] = value;
    else if (device == "light12v") payload["led12v"] = value;
  } else {
    if (device == "window") payload["window"] = value;
    else if (device == "door") payload["door"] = value;
  }

  serializeJson(doc, Serial);
  Serial.println();
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
void publishSensorData() {
  if (!mqtt.connected()) return;

  // Publish individual sensor topics
  if (sensorData.temp_c > -500) {
    mqtt.publish("gh/sensor/temp_c", String(sensorData.temp_c, 1).c_str());
  }

  if (sensorData.hum_pct > -500) {
    mqtt.publish("gh/sensor/hum_pct", String(sensorData.hum_pct, 0).c_str());
  }

  mqtt.publish("gh/sensor/soil_pct", String(sensorData.soil_pct, 0).c_str());
  mqtt.publish("gh/sensor/light_lux", String(sensorData.light_lux, 0).c_str());
  mqtt.publish("gh/sensor/tank_pct", "50");  // Placeholder, add tank sensor if available

  // Digital sensors
  mqtt.publish("gh/sensor/flame_do", sensorData.flame > 800 ? "1" : "0");
  mqtt.publish("gh/sensor/sound_do", sensorData.sound > 700 ? "1" : "0");

  // GPS (if valid)
  if (sensorData.gps_valid) {
    StaticJsonDocument<128> gpsDoc;
    gpsDoc["lat"] = sensorData.gps_lat;
    gpsDoc["lng"] = sensorData.gps_lng;

    String gpsJson;
    serializeJson(gpsDoc, gpsJson);
    mqtt.publish("gh/sensor/gps", gpsJson.c_str());
  }
}

void publishDeviceStatus() {
  if (!mqtt.connected()) return;

  // Publish relay status
  mqtt.publish("gh/status/relay/fan", deviceState.fan.c_str(), true);
  mqtt.publish("gh/status/relay/auxfan", deviceState.auxfan.c_str(), true);
  mqtt.publish("gh/status/relay/pump", deviceState.pump.c_str(), true);
  mqtt.publish("gh/status/relay/light12v", deviceState.light12v.c_str(), true);

  // Publish servo status
  mqtt.publish("gh/status/servo/window", deviceState.window.c_str(), true);
  mqtt.publish("gh/status/servo/door", deviceState.door.c_str(), true);

  // Publish LED status
  mqtt.publish("gh/status/led/power", deviceState.led_power.c_str(), true);
  mqtt.publish("gh/status/led/color", deviceState.led_color.c_str(), true);
}

void publishHeartbeat() {
  if (!mqtt.connected()) return;

  StaticJsonDocument<128> doc;
  doc["ts"] = millis() / 1000;
  doc["uptime_s"] = millis() / 1000;
  doc["heap"] = ESP.getFreeHeap();

  String json;
  serializeJson(doc, json);
  mqtt.publish("gh/sys/heartbeat/esp8266", json.c_str());
}

void publishError(const char* code, const char* msg) {
  if (!mqtt.connected()) return;

  StaticJsonDocument<128> doc;
  doc["ts"] = millis() / 1000;
  doc["code"] = code;
  doc["msg"] = msg;

  String json;
  serializeJson(doc, json);
  mqtt.publish("gh/sys/error/esp8266", json.c_str());
}

void publishMqttStatus(bool connected) {
  // Could publish to a status topic if needed
}

// ==================== LED WS2812 ====================
void updateLedAnimations() {
  unsigned long now = millis();

  if (now - ledAnimationTick < 100) return;  // Update every 100ms
  ledAnimationTick = now;

  // Ring 1
  if (led1State.mode == "OFF") {
    for (int i = 0; i < LED_COUNT; i++) {
      ring1.setPixelColor(i, 0, 0, 0);
    }
  } else if (led1State.mode == "COLOR") {
    for (int i = 0; i < LED_COUNT; i++) {
      ring1.setPixelColor(i, led1State.r, led1State.g, led1State.b);
    }
  } else if (led1State.mode == "ALERT") {
    ledAlertPhase = !ledAlertPhase;
    uint8_t brightness = ledAlertPhase ? 255 : 50;
    for (int i = 0; i < LED_COUNT; i++) {
      ring1.setPixelColor(i, brightness, 0, 0);  // Red blink
    }
  }

  // Ring 2
  if (led2State.mode == "OFF") {
    for (int i = 0; i < LED_COUNT; i++) {
      ring2.setPixelColor(i, 0, 0, 0);
    }
  } else if (led2State.mode == "COLOR") {
    for (int i = 0; i < LED_COUNT; i++) {
      ring2.setPixelColor(i, led2State.r, led2State.g, led2State.b);
    }
  } else if (led2State.mode == "ALERT") {
    ledAlertPhase = !ledAlertPhase;
    uint8_t brightness = ledAlertPhase ? 255 : 50;
    for (int i = 0; i < LED_COUNT; i++) {
      ring2.setPixelColor(i, brightness, 0, 0);
    }
  }

  ring1.show();
  ring2.show();
}

void parseColor(String hexColor, uint8_t& r, uint8_t& g, uint8_t& b) {
  if (hexColor.length() < 7 || hexColor[0] != '#') return;

  r = strtol(hexColor.substring(1, 3).c_str(), NULL, 16);
  g = strtol(hexColor.substring(3, 5).c_str(), NULL, 16);
  b = strtol(hexColor.substring(5, 7).c_str(), NULL, 16);
}
