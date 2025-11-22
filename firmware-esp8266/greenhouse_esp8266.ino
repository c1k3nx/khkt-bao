/*******************************************************************************
 * GREENHOUSE ESP8266 FIRMWARE - v1.1 FINAL
 * ============================================================================
 * Chức năng:
 * - MQTT Bridge: Subscribe commands, publish sensor/status
 * - UART với UNO: SoftwareSerial 115200 baud
 * - GPS NEO-6: SoftwareSerial 9600 baud
 * - LED WS2812B Ring #2: 8 LEDs
 * - Config storage: LittleFS
 * - Watchdog & retry logic
 *
 * Pin mapping (OFFICIAL - see PIN_MAPPING_FINAL.md):
 * - UNO UART: GPIO12 (RX) ← UNO D9 TX, GPIO14 (TX) → UNO D8 RX (SoftwareSerial 115200)
 * - GPS: GPIO13 (RX) ← NEO-6 TX, GPIO15 (TX) → NEO-6 RX (SoftwareSerial 9600)
 * - WS2812 Ring #2: GPIO2 (D4) - 8 LEDs, NeoPixelBus UART method
 * - Reserve: GPIO1/3 (UART0 for debug), GPIO4, GPIO5
 ******************************************************************************/

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <SoftwareSerial.h>
#include <NeoPixelBus.h>  // NeoPixelBus instead of Adafruit_NeoPixel for ESP8266
#include <LittleFS.h>
#include <TinyGPSPlus.h>

// ==================== CONFIGURATION ====================
// Wi-Fi credentials (change these)
const char* WIFI_SSID = "YourWiFiSSID";
const char* WIFI_PASS = "YourWiFiPassword";

// MQTT broker (change if needed)
const char* MQTT_BROKER = "broker.hivemq.com";
const int MQTT_PORT = 1883;
const char* MQTT_CLIENT_ID = "greenhouse-esp8266";

// UART with UNO (SoftwareSerial)
#define UNO_RX 12    // GPIO12 (D6) ← UNO TX (D9 via level shift)
#define UNO_TX 14    // GPIO14 (D5) → UNO RX (D8)
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

// Timings
#define SENSOR_PUBLISH_INTERVAL 3000
#define STATUS_PUBLISH_INTERVAL 5000
#define HEARTBEAT_INTERVAL 30000
#define GPS_READ_INTERVAL 1000

// ==================== OBJECTS ====================
WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);
SoftwareSerial unoSerial(UNO_RX, UNO_TX);  // For communication with UNO
SoftwareSerial gpsSerial(GPS_RX, GPS_TX);
TinyGPSPlus gps;

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

int uartErrorCount = 0;
int mqttReconnectAttempts = 0;
unsigned long lastMqttReconnect = 0;

bool systemReady = false;
String currentMode = "AUTO";

// LED animation state for Ring #2
struct LedState {
  String mode;  // "OFF", "COLOR", "ALERT"
  uint8_t r, g, b;
};
LedState ledState = {"OFF", 0, 255, 0};  // Ring #2 only

unsigned long ledAnimationTick = 0;
bool ledAlertPhase = false;

// ==================== SETUP ====================
void setup() {
  // Hardware UART for debugging (optional)
  Serial.begin(115200);
  Serial.println("\nGreenhouse ESP8266 v1.1 FINAL");
  delay(100);

  // Init UART with UNO
  unoSerial.begin(UART_BAUD);

  // Init GPS
  gpsSerial.begin(GPS_BAUD);

  // Init filesystem
  if (!LittleFS.begin()) {
    Serial.println("LittleFS init failed");
  }

  // Init LED Ring #2
  ring2.Begin();
  ring2.Show(); // Clear

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
  if (unoSerial.available()) {
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
      // Subscribe to command topics (standardized v1.1)
      mqtt.subscribe("greenhouse/control/#");
      mqtt.subscribe("greenhouse/set/thresholds/#");

      // Publish initial state
      publishDeviceStatus();
      mqttReconnectAttempts = 0;
      Serial.println("MQTT connected");
      return;
    }
    attempts++;
    delay(1000);
  }

  Serial.println("MQTT connection failed");
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
    Serial.println("MQTT reconnected");
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
  // Mode control
  if (topic == "greenhouse/control/mode") {
    currentMode = payload;
    mqtt.publish("greenhouse/status/mode", payload.c_str(), true);
    Serial.print("Mode changed to: ");
    Serial.println(payload);
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
    Serial.print("Threshold update: ");
    Serial.print(param);
    Serial.print(" = ");
    Serial.println(payload);
    // Store in LittleFS or forward to UNO if needed
    return;
  }
}

void handleDeviceCommand(String device, String action) {
  // Update local state
  if (device == "pump") deviceState.pump = action;
  else if (device == "fan") deviceState.fan = action;
  else if (device == "auxFan") deviceState.auxfan = action;
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

  Serial.print("Device control: ");
  Serial.print(device);
  Serial.print(" = ");
  Serial.println(action);
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
    Serial.print("UART parse error: ");
    Serial.println(line);
    publishError("uart_parse_error", "Invalid JSON from UNO");
    return;
  }

  const char* type = doc["type"];

  if (strcmp(type, "sensor") == 0) {
    handleSensorData(doc);
  } else if (strcmp(type, "ack") == 0) {
    // ACK received
    Serial.println("ACK from UNO");
  } else if (strcmp(type, "event") == 0) {
    handleEventFromUno(doc);
  }
}

void handleSensorData(JsonDocument& doc) {
  JsonObject data = doc["data"];

  // Parse sensor data (format matches mqtt-schema.json)
  if (data.containsKey("temperature")) {
    String temp = data["temperature"];
    sensorData.temp_c = temp.toFloat();
  }
  if (data.containsKey("humidity")) {
    String hum = data["humidity"];
    sensorData.hum_pct = hum.toFloat();
  }
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
  Serial.println("Sensor data received from UNO");
}

void handleEventFromUno(JsonDocument& doc) {
  const char* event = doc["event"];
  const char* value = doc["value"];

  if (event == NULL || value == NULL) return;

  // Publish to greenhouse/event/<event>
  String topic = "greenhouse/event/";
  topic += event;
  mqtt.publish(topic.c_str(), value);

  Serial.print("Event from UNO: ");
  Serial.print(event);
  Serial.print(" = ");
  Serial.println(value);

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

  // Also log to debug Serial
  Serial.print("Sent to UNO: ");
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

  // Publish to greenhouse/data/* topics (matches mqtt-schema.json)
  if (sensorData.temp_c > -500) {
    mqtt.publish("greenhouse/data/temperature", String(sensorData.temp_c, 1).c_str());
  }

  if (sensorData.hum_pct > -500) {
    mqtt.publish("greenhouse/data/humidity", String(sensorData.hum_pct, 1).c_str());
  }

  mqtt.publish("greenhouse/data/soilMoisture", String(sensorData.soil_pct, 1).c_str());
  mqtt.publish("greenhouse/data/lightIntensity", String((int)sensorData.light_lux).c_str());
  mqtt.publish("greenhouse/data/waterTankLevel", String((int)sensorData.distance_cm).c_str());
  mqtt.publish("greenhouse/data/gasMQ3", String(sensorData.mq3 * 5.0 / 1023.0, 2).c_str());
  mqtt.publish("greenhouse/data/flameAnalog", String(sensorData.flame).c_str());
  mqtt.publish("greenhouse/data/soundLevel", String(sensorData.sound).c_str());

  // Rain sensor (placeholder)
  mqtt.publish("greenhouse/data/rainSensor", "NOT_DETECTED");

  // GPS (if valid)
  if (sensorData.gps_valid) {
    StaticJsonDocument<128> gpsDoc;
    gpsDoc["lat"] = sensorData.gps_lat;
    gpsDoc["lng"] = sensorData.gps_lng;

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

  Serial.print("Error: ");
  Serial.print(code);
  Serial.print(" - ");
  Serial.println(msg);
}

// ==================== LED WS2812 ====================
void updateLedAnimations() {
  unsigned long now = millis();

  if (now - ledAnimationTick < 100) return;  // Update every 100ms
  ledAnimationTick = now;

  // Ring #2 only (Ring #1 is on UNO)
  if (ledState.mode == "OFF") {
    for (int i = 0; i < LED_COUNT; i++) {
      ring2.SetPixelColor(i, RgbColor(0, 0, 0));
    }
  } else if (ledState.mode == "COLOR") {
    for (int i = 0; i < LED_COUNT; i++) {
      ring2.SetPixelColor(i, RgbColor(ledState.r, ledState.g, ledState.b));
    }
  } else if (ledState.mode == "ALERT") {
    ledAlertPhase = !ledAlertPhase;
    uint8_t brightness = ledAlertPhase ? 255 : 50;
    for (int i = 0; i < LED_COUNT; i++) {
      ring2.SetPixelColor(i, RgbColor(brightness, 0, 0));  // Red blink
    }
  }

  ring2.Show();
}

void parseColor(String hexColor, uint8_t& r, uint8_t& g, uint8_t& b) {
  if (hexColor.length() < 7 || hexColor[0] != '#') return;

  r = strtol(hexColor.substring(1, 3).c_str(), NULL, 16);
  g = strtol(hexColor.substring(3, 5).c_str(), NULL, 16);
  b = strtol(hexColor.substring(5, 7).c_str(), NULL, 16);
}
