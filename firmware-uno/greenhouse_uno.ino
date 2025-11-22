/*******************************************************************************
 * GREENHOUSE UNO R3 FIRMWARE - v1.3.2 STABILITY FIX
 * ============================================================================
 * Chức năng:
 * - Đọc cảm biến: BH1750, JSN-SR04T, MQ-3, Flame, Sound, Soil moisture
 * - Điều khiển: 2 relay (direct), 2 servo, DFPlayer Mini, WS2812 Ring #1
 * - Hiển thị: LCD1602 I2C multi-screen rotation (ALL sensors)
 * - Giao tiếp: AltSoftSerial 115200 baud với ESP8266 (JSON protocol)
 * - Fallback an toàn khi mất kết nối
 *
 * BUGFIX v1.2: DHT11 removed from UNO (moved to ESP8266 GPIO4)
 * - Fixes D13 pin conflict between DHT11 and WS2812
 * - Temperature/Humidity now read by ESP8266 and forwarded via UART
 *
 * ENHANCEMENT v1.3: LCD multi-screen rotation + Data sync from ESP8266
 * - Receives temp/humidity/GPS from ESP8266 via UART "env" message
 * - LCD rotates through 5 screens showing ALL sensors without text corruption
 * - Screen 0: Temperature, Humidity
 * - Screen 1: Soil Moisture, Light Intensity
 * - Screen 2: Flame, Sound
 * - Screen 3: Gas MQ3, Water Tank
 * - Screen 4: GPS coordinates
 *
 * BUGFIX v1.3.2: LCD flicker prevention
 * - Only clears LCD when screen actually changes (not every 1 second)
 * - Reduces flicker and improves readability
 * - Tracks previous screen to detect changes
 *
 * Pin mapping (OFFICIAL - see PIN_MAPPING_FINAL.md):
 * - AltSoftSerial: D8 (RX) ← ESP8266 TX, D9 (TX) → ESP8266 RX (115200 baud)
 * - DFPlayer: D12 (RX), D11 (TX) - SoftwareSerial 9600 baud
 * - Servos: D6 (Window MG996R), D5 (Door MG90)
 * - JSN-SR04T: D3 (TRIG), D2 (ECHO)
 * - WS2812 Ring #1: D13 (8 LEDs) - NOW EXCLUSIVE, no conflict!
 * - Relays (direct): D7 (Pump), D10 (LED 12V Grow Light)
 * - Button: D4 (Open Main Door, INPUT_PULLUP)
 * - Analog: A0 (Flame), A1 (Sound), A2 (Soil), A3 (MQ-3)
 * - I2C: A4 (SDA), A5 (SCL) - BH1750 + LCD1602
 *
 * NOTE: DHT11 (temp/humidity) is now on ESP8266 GPIO4
 * NOTE: For full 4-relay control, use PCF8574 I2C expander (recommended)
 ******************************************************************************/

#include <Wire.h>
#include <Servo.h>
// #include <DHT.h>  // REMOVED - DHT11 moved to ESP8266
#include <BH1750.h>
#include <LiquidCrystal_I2C.h>
#include <AltSoftSerial.h>  // D8 RX, D9 TX (fixed pins)
#include <SoftwareSerial.h>  // For DFPlayer
#include <Adafruit_NeoPixel.h>
#include <ArduinoJson.h>

// ==================== CONFIGURATION ====================
// Sensors
// DHT11 REMOVED - now on ESP8266 GPIO4
// #define DHT_PIN 13
// #define DHT_TYPE DHT11

#define JSN_TRIG 3
#define JSN_ECHO 2

#define ANALOG_FLAME A0
#define ANALOG_SOUND A1
#define ANALOG_SOIL A2
#define ANALOG_MQ3 A3

// Actuators - Relays (direct control)
#define RELAY_PUMP 7
#define RELAY_LED12V 10

// Actuators - Servos
#define SERVO_WINDOW 6
#define SERVO_DOOR 5

// DFPlayer - SoftwareSerial
#define DF_RX 12
#define DF_TX 11

// WS2812 Ring #1
#define WS2812_PIN 13  // Shares with DHT11 - use carefully
#define WS2812_COUNT 8

// Button
#define BUTTON_DOOR 4

// Relay logic (Active LOW)
#define RELAY_ON LOW
#define RELAY_OFF HIGH

// Thresholds for fallback safety
#define FLAME_THRESHOLD 800
#define SOUND_THRESHOLD 700
#define TEMP_SAFE_MAX 40.0

// UART protocol with ESP8266
#define UART_BAUD 115200
#define UART_TIMEOUT 3000
#define UART_RETRY 3

// LCD queue
#define LCD_QUEUE_SIZE 5
#define LCD_MSG_DURATION 1000

// LCD multi-screen rotation (v1.3)
#define LCD_SCREEN_COUNT 5
#define LCD_SCREEN_INTERVAL 3000  // 3 seconds per screen

// ==================== OBJECTS ====================
// DHT dht(DHT_PIN, DHT_TYPE);  // REMOVED - now on ESP8266
BH1750 lightMeter(0x23);
LiquidCrystal_I2C lcd(0x27, 16, 2);
Servo servoWindow;
Servo servoDoor;
AltSoftSerial espSerial;  // RX=D8, TX=D9 (fixed by library)
SoftwareSerial dfSerial(DF_RX, DF_TX);
Adafruit_NeoPixel strip(WS2812_COUNT, WS2812_PIN, NEO_GRB + NEO_KHZ800);

// ==================== GLOBAL STATE ====================
struct SensorData {
  // v1.3: temp/humidity/GPS received from ESP8266 for LCD display only
  float temp_c;       // From ESP8266 DHT11
  float hum_pct;      // From ESP8266 DHT11
  float gps_lat;      // From ESP8266 GPS
  float gps_lng;      // From ESP8266 GPS
  bool gps_valid;     // GPS fix status

  // Sensors read locally by UNO
  float soil_pct;
  float light_lux;
  int mq3;
  int flame;
  int sound;
  float distance_cm;
};

struct ActuatorState {
  bool pump;
  bool led12v;
  bool window;  // true=OPEN, false=CLOSE
  bool door;
  // Note: For fan control, use PCF8574 I2C expander (not in minimal version)
};

SensorData sensors;
ActuatorState actuators;

unsigned long lastSensorRead = 0;
unsigned long lastSensorSend = 0;
unsigned long lastUartCheck = 0;
unsigned long lastLcdUpdate = 0;
unsigned long lastWatchdog = 0;
unsigned long lastScreenSwitch = 0;  // v1.3: Screen rotation timer

bool uartConnected = true;
int uartErrorCount = 0;
int currentScreen = 0;         // v1.3: Current LCD screen (0-4)
int previousScreen = -1;       // v1.3.2: Track screen changes

// LCD queue
struct LcdMessage {
  String line1;
  String line2;
  unsigned long showUntil;
};
LcdMessage lcdQueue[LCD_QUEUE_SIZE];
int lcdQueueHead = 0;
int lcdQueueTail = 0;
bool lcdShowingSensor = true;

// DFPlayer state
int currentVolume = 25;
bool dfPlayerReady = false;

// ==================== SETUP ====================
void setup() {
  // Init pins
  pinMode(RELAY_PUMP, OUTPUT);
  pinMode(RELAY_LED12V, OUTPUT);

  pinMode(JSN_TRIG, OUTPUT);
  pinMode(JSN_ECHO, INPUT);

  pinMode(BUTTON_DOOR, INPUT_PULLUP);

  // Safe initial state
  safeState();

  // Init I2C
  Wire.begin();

  // Init LCD
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("GREENHOUSE UNO");
  lcd.setCursor(0, 1);
  lcd.print("v1.3.2 STABLE");
  delay(1000);

  // v1.3: Initialize sensor data
  sensors.temp_c = 0;
  sensors.hum_pct = 0;
  sensors.gps_lat = 0;
  sensors.gps_lng = 0;
  sensors.gps_valid = false;

  // DHT init removed - now on ESP8266
  // dht.begin();

  // Init BH1750
  if (lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE)) {
    lcd.setCursor(0, 1);
    lcd.print("BH1750 OK       ");
  } else {
    lcd.setCursor(0, 1);
    lcd.print("BH1750 ERROR!   ");
  }
  delay(500);

  // Init Servos
  servoWindow.attach(SERVO_WINDOW);
  servoDoor.attach(SERVO_DOOR);
  servoWindow.write(0);   // CLOSE
  servoDoor.write(0);     // CLOSE
  actuators.window = false;
  actuators.door = false;

  // Init WS2812 Ring #1
  strip.begin();
  strip.clear();
  strip.show();

  // Init DFPlayer
  dfSerial.begin(9600);
  delay(500);
  initDFPlayer();

  // Init UART with ESP8266 (AltSoftSerial on D8/D9)
  espSerial.begin(UART_BAUD);

  // Hardware Serial for debugging (optional, can be removed)
  Serial.begin(115200);
  Serial.println("Greenhouse UNO v1.3.2 STABILITY FIX");

  delay(1000);
  lcd.clear();
  lcd.print("READY!");
  delay(500);
}

// ==================== MAIN LOOP ====================
void loop() {
  unsigned long now = millis();

  // Read sensors (every 2s)
  if (now - lastSensorRead >= 2000) {
    lastSensorRead = now;
    readAllSensors();
  }

  // Send sensor data via UART (every 3s)
  if (now - lastSensorSend >= 3000) {
    lastSensorSend = now;
    sendSensorData();
  }

  // Check UART for commands from ESP8266
  if (espSerial.available()) {
    handleUartCommand();
    lastUartCheck = now;
    uartConnected = true;
    uartErrorCount = 0;
  }

  // Watchdog: check UART timeout
  if (now - lastUartCheck >= UART_TIMEOUT) {
    if (uartConnected) {
      uartConnected = false;
      uartErrorCount++;
      // Enter fallback mode
      fallbackSafety();
    }
  }

  // Update LCD (every 1s or queue)
  if (now - lastLcdUpdate >= 1000) {
    lastLcdUpdate = now;
    updateLcd();
  }

  // v1.3: Switch LCD screen every 3 seconds
  if (lcdShowingSensor && now - lastScreenSwitch >= LCD_SCREEN_INTERVAL) {
    lastScreenSwitch = now;
    currentScreen = (currentScreen + 1) % LCD_SCREEN_COUNT;
  }

  // Emergency checks (always active)
  emergencyChecks();

  delay(10);
}

// ==================== SENSOR READING ====================
void readAllSensors() {
  // DHT11 removed - temp/humidity now read by ESP8266
  // Temp/humidity will be received via UART from ESP8266

  // BH1750
  sensors.light_lux = lightMeter.readLightLevel();
  if (sensors.light_lux < 0) sensors.light_lux = 0;

  // Soil moisture (analog, 0-1023)
  int soilRaw = analogRead(ANALOG_SOIL);
  sensors.soil_pct = map(soilRaw, 0, 1023, 0, 100);

  // MQ-3 alcohol
  sensors.mq3 = analogRead(ANALOG_MQ3);

  // Flame sensor
  sensors.flame = analogRead(ANALOG_FLAME);

  // Sound sensor
  sensors.sound = analogRead(ANALOG_SOUND);

  // JSN-SR04T ultrasonic
  sensors.distance_cm = readUltrasonic();
}

float readUltrasonic() {
  digitalWrite(JSN_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(JSN_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(JSN_TRIG, LOW);

  long duration = pulseIn(JSN_ECHO, HIGH, 30000);
  if (duration == 0) return -1;

  float distance = duration * 0.034 / 2.0;
  return distance;
}

// ==================== UART COMMUNICATION ====================
void sendSensorData() {
  StaticJsonDocument<512> doc;
  doc["type"] = "sensor";
  doc["ts"] = millis() / 1000;

  JsonObject data = doc.createNestedObject("data");
  // temperature and humidity removed - ESP8266 reads DHT11 directly
  data["lightIntensity"] = String((int)sensors.light_lux);
  data["soilMoisture"] = String(sensors.soil_pct, 1);
  data["gasMQ3"] = String(sensors.mq3 * 5.0 / 1023.0, 2);  // Convert to voltage
  data["flameAnalog"] = String(sensors.flame);
  data["soundLevel"] = String(sensors.sound);
  data["waterTankLevel"] = String((int)sensors.distance_cm);  // Placeholder

  serializeJson(doc, espSerial);
  espSerial.println();

  // Also send to debug Serial (optional)
  serializeJson(doc, Serial);
  Serial.println();
}

void handleUartCommand() {
  String line = espSerial.readStringUntil('\n');
  line.trim();

  if (line.length() == 0) return;

  StaticJsonDocument<512> doc;
  DeserializationError error = deserializeJson(doc, line);

  if (error) {
    uartErrorCount++;
    Serial.print("UART parse error: ");
    Serial.println(line);
    return;
  }

  const char* type = doc["type"];
  if (strcmp(type, "control") == 0) {
    handleControlCommand(doc);
  }
  else if (strcmp(type, "env") == 0) {
    // v1.3: Handle environment data from ESP8266
    handleEnvData(doc);
  }
}

void handleControlCommand(JsonDocument& doc) {
  const char* device = doc["device"];
  const char* action = doc["action"];

  if (device == NULL || action == NULL) return;

  bool ok = true;

  // Relays
  if (strcmp(device, "pump") == 0) {
    actuators.pump = (strcmp(action, "ON") == 0);
    setRelay(RELAY_PUMP, actuators.pump);
    pushLcdMessage("PUMP", action);
  }
  else if (strcmp(device, "mainGrowLight") == 0) {
    actuators.led12v = (strcmp(action, "ON") == 0);
    setRelay(RELAY_LED12V, actuators.led12v);
    pushLcdMessage("LIGHT", action);
  }

  // Servos
  else if (strcmp(device, "window1") == 0) {
    actuators.window = (strcmp(action, "OPEN") == 0);
    servoWindow.write(actuators.window ? 90 : 0);
    pushLcdMessage("WINDOW", action);
  }
  else if (strcmp(device, "mainDoor") == 0) {
    actuators.door = (strcmp(action, "OPEN") == 0);
    servoDoor.write(actuators.door ? 90 : 0);
    pushLcdMessage("DOOR", action);
  }

  // Send ACK back to ESP8266
  StaticJsonDocument<128> ack;
  ack["type"] = "ack";
  ack["ts"] = millis() / 1000;
  ack["device"] = device;
  ack["action"] = action;
  ack["ok"] = ok;

  serializeJson(ack, espSerial);
  espSerial.println();
}

void setRelay(int pin, bool state) {
  digitalWrite(pin, state ? RELAY_ON : RELAY_OFF);
}

// ==================== ENVIRONMENT DATA FROM ESP8266 (v1.3) ====================
void handleEnvData(JsonDocument& doc) {
  // Parse temp/humidity/GPS from ESP8266
  if (doc.containsKey("temp")) {
    String temp = doc["temp"];
    sensors.temp_c = temp.toFloat();
  }

  if (doc.containsKey("hum")) {
    String hum = doc["hum"];
    sensors.hum_pct = hum.toFloat();
  }

  if (doc.containsKey("gps_valid")) {
    sensors.gps_valid = doc["gps_valid"];

    if (sensors.gps_valid && doc.containsKey("gps_lat") && doc.containsKey("gps_lng")) {
      String lat = doc["gps_lat"];
      String lng = doc["gps_lng"];
      sensors.gps_lat = lat.toFloat();
      sensors.gps_lng = lng.toFloat();
    }
  }

  // Debug log
  Serial.print("Env data from ESP: T=");
  Serial.print(sensors.temp_c, 1);
  Serial.print("°C H=");
  Serial.print(sensors.hum_pct, 1);
  Serial.println("%");
}

// ==================== DFPLAYER ====================
void initDFPlayer() {
  // Simple init sequence
  sendDFCommand(0x3F, 0, 0); // Init
  delay(200);
  sendDFCommand(0x06, 0, currentVolume); // Set volume
  delay(200);
  dfPlayerReady = true;
}

void sendDFCommand(byte cmd, byte param1, byte param2) {
  byte buffer[10] = {0x7E, 0xFF, 0x06, cmd, 0x00, param1, param2, 0x00, 0x00, 0xEF};

  // Calculate checksum
  int sum = -(buffer[1] + buffer[2] + buffer[3] + buffer[4] + buffer[5] + buffer[6]);
  buffer[7] = (sum >> 8) & 0xFF;
  buffer[8] = sum & 0xFF;

  dfSerial.write(buffer, 10);
}

void playTrack(int track) {
  if (!dfPlayerReady) return;
  sendDFCommand(0x03, 0, track);
  pushLcdMessage("PLANTTALK", "PLAYING...");
}

void stopTrack() {
  if (!dfPlayerReady) return;
  sendDFCommand(0x16, 0, 0);
}

// ==================== LCD ====================
void pushLcdMessage(String line1, String line2) {
  int next = (lcdQueueTail + 1) % LCD_QUEUE_SIZE;
  if (next == lcdQueueHead) return; // Queue full

  lcdQueue[lcdQueueTail].line1 = line1;
  lcdQueue[lcdQueueTail].line2 = line2;
  lcdQueue[lcdQueueTail].showUntil = millis() + LCD_MSG_DURATION;
  lcdQueueTail = next;
}

// ==================== LCD MULTI-SCREEN (v1.3) ====================
void updateLcd() {
  unsigned long now = millis();

  // Check queue - priority messages
  if (lcdQueueHead != lcdQueueTail) {
    LcdMessage& msg = lcdQueue[lcdQueueHead];
    if (now < msg.showUntil) {
      // Show queued message
      if (!lcdShowingSensor) {
        // Entering queue mode, clear once
        lcd.clear();
      }
      lcd.setCursor(0, 0);
      lcd.print(msg.line1);
      lcd.setCursor(0, 1);
      lcd.print(msg.line2);
      lcdShowingSensor = false;
      previousScreen = -1;  // Reset screen tracking
      return;
    } else {
      // Message expired
      lcdQueueHead = (lcdQueueHead + 1) % LCD_QUEUE_SIZE;
      lcdShowingSensor = true;
      previousScreen = -1;  // Force clear on next screen
    }
  }

  // Show sensor data - rotate through screens
  if (lcdShowingSensor) {
    // v1.3.2: Only clear when screen changes (prevent flicker)
    if (currentScreen != previousScreen) {
      lcd.clear();
      previousScreen = currentScreen;
    }

    switch (currentScreen) {
      case 0:  // Screen 0: Temperature & Humidity
        lcd.setCursor(0, 0);
        lcd.print("TEMP: ");
        lcd.print(sensors.temp_c, 1);
        lcd.print((char)223);  // Degree symbol
        lcd.print("C");

        lcd.setCursor(0, 1);
        lcd.print("HUMI: ");
        lcd.print(sensors.hum_pct, 1);
        lcd.print("%");
        break;

      case 1:  // Screen 1: Soil & Light
        lcd.setCursor(0, 0);
        lcd.print("SOIL: ");
        lcd.print((int)sensors.soil_pct);
        lcd.print("%");

        lcd.setCursor(0, 1);
        lcd.print("LIGHT: ");
        lcd.print((int)sensors.light_lux);
        lcd.print("lx");
        break;

      case 2:  // Screen 2: Flame & Sound
        lcd.setCursor(0, 0);
        lcd.print("FLAME: ");
        lcd.print(sensors.flame);

        lcd.setCursor(0, 1);
        lcd.print("SOUND: ");
        lcd.print(sensors.sound);
        break;

      case 3:  // Screen 3: Gas & Water Tank
        lcd.setCursor(0, 0);
        lcd.print("GAS: ");
        lcd.print(sensors.mq3 * 5.0 / 1023.0, 2);
        lcd.print("V");

        lcd.setCursor(0, 1);
        lcd.print("TANK: ");
        lcd.print((int)sensors.distance_cm);
        lcd.print("cm");
        break;

      case 4:  // Screen 4: GPS coordinates
        lcd.setCursor(0, 0);
        if (sensors.gps_valid) {
          lcd.print("LAT:");
          lcd.print(sensors.gps_lat, 4);
        } else {
          lcd.print("GPS: NO FIX");
        }

        lcd.setCursor(0, 1);
        if (sensors.gps_valid) {
          lcd.print("LNG:");
          lcd.print(sensors.gps_lng, 4);
        } else {
          lcd.print("Waiting...");
        }
        break;
    }
  }
}

// ==================== SAFETY & FALLBACK ====================
void safeState() {
  digitalWrite(RELAY_PUMP, RELAY_OFF);
  digitalWrite(RELAY_LED12V, RELAY_OFF);

  actuators.pump = false;
  actuators.led12v = false;
}

void fallbackSafety() {
  // When UART lost, apply safe defaults
  pushLcdMessage("UART LOST", "FALLBACK MODE");

  // Check flame
  if (sensors.flame > FLAME_THRESHOLD) {
    setRelay(RELAY_PUMP, false);  // Turn off pump
    // NOTE: Fan control requires PCF8574 I2C expander
    pushLcdMessage("FIRE DETECT!", "PUMP OFF");
    servoWindow.write(90);  // Open window for ventilation
    return;
  }

  // Temperature check removed - DHT11 now on ESP8266
  // ESP8266 will handle temperature-based control

  // Default safe state
  safeState();
}

void emergencyChecks() {
  static unsigned long lastFlameEvent = 0;
  static unsigned long lastSoundEvent = 0;
  unsigned long now = millis();

  // Always check flame regardless of mode (debounce: once per second)
  if (sensors.flame > FLAME_THRESHOLD && (now - lastFlameEvent > 1000)) {
    lastFlameEvent = now;

    // Send event to ESP8266
    StaticJsonDocument<128> doc;
    doc["type"] = "event";
    doc["event"] = "flame";
    doc["value"] = "DETECTED";

    serializeJson(doc, espSerial);
    espSerial.println();

    Serial.println("FLAME DETECTED!");
  }

  // Sound alert (debounce: once per second)
  if (sensors.sound > SOUND_THRESHOLD && (now - lastSoundEvent > 1000)) {
    lastSoundEvent = now;

    StaticJsonDocument<128> doc;
    doc["type"] = "event";
    doc["event"] = "sound";
    doc["value"] = "LOUD";

    serializeJson(doc, espSerial);
    espSerial.println();

    Serial.println("LOUD SOUND!");
  }
}
