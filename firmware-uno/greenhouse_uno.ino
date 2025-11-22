/*******************************************************************************
 * GREENHOUSE UNO R3 FIRMWARE
 * ============================================================================
 * Chức năng:
 * - Đọc cảm biến: DHT11, BH1750, JSN-SR04T, MQ-3, Flame, Sound, Soil moisture
 * - Điều khiển: 4 relay, 2 servo, DFPlayer Mini
 * - Hiển thị: LCD1602 I2C
 * - Giao tiếp: UART 57600 baud với ESP8266 (JSON protocol)
 * - Fallback an toàn khi mất kết nối
 *
 * Pin mapping (BẮT BUỘC):
 * - UART: D0 (RX) ↔ ESP8266 TX, D1 (TX) ↔ ESP8266 RX
 * - DFPlayer: D10 (RX), D11 (TX)
 * - Servos: D9 (Window MG996R), D5 (Door MG90)
 * - JSN-SR04T: D12 (TRIG), D3 (ECHO)
 * - DHT11: D2
 * - Relays: D4 (Pump), D7 (Fan), D8 (Light12V), D13 (AuxFan)
 * - Analog: A0 (Flame), A1 (Sound), A2 (Soil), A3 (MQ-3)
 * - I2C: A4 (SDA), A5 (SCL) - BH1750 + LCD1602
 ******************************************************************************/

#include <Wire.h>
#include <Servo.h>
#include <DHT.h>
#include <BH1750.h>
#include <LiquidCrystal_I2C.h>
#include <SoftwareSerial.h>
#include <ArduinoJson.h>

// ==================== CONFIGURATION ====================
#define DHT_PIN 2
#define DHT_TYPE DHT11

#define JSN_TRIG 12
#define JSN_ECHO 3

#define RELAY_PUMP 4
#define RELAY_FAN 7
#define RELAY_LIGHT12V 8
#define RELAY_AUXFAN 13

#define SERVO_WINDOW 9
#define SERVO_DOOR 5

#define ANALOG_FLAME A0
#define ANALOG_SOUND A1
#define ANALOG_SOIL A2
#define ANALOG_MQ3 A3

#define DF_RX 10
#define DF_TX 11

// Relay logic (adjust if needed)
#define RELAY_ON LOW
#define RELAY_OFF HIGH

// Thresholds for fallback safety
#define FLAME_THRESHOLD 800
#define SOUND_THRESHOLD 700
#define TEMP_SAFE_MAX 40.0

// UART protocol
#define UART_BAUD 57600
#define UART_TIMEOUT 3000
#define UART_RETRY 3

// LCD queue
#define LCD_QUEUE_SIZE 5
#define LCD_MSG_DURATION 1000

// ==================== OBJECTS ====================
DHT dht(DHT_PIN, DHT_TYPE);
BH1750 lightMeter(0x23);
LiquidCrystal_I2C lcd(0x27, 16, 2);
Servo servoWindow;
Servo servoDoor;
SoftwareSerial dfSerial(DF_RX, DF_TX);

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
};

struct ActuatorState {
  bool pump;
  bool fan_main;
  bool fan_aux;
  bool led12v;
  bool window;  // true=OPEN, false=CLOSE
  bool door;
};

SensorData sensors;
ActuatorState actuators;

unsigned long lastSensorRead = 0;
unsigned long lastSensorSend = 0;
unsigned long lastUartCheck = 0;
unsigned long lastLcdUpdate = 0;
unsigned long lastWatchdog = 0;

bool uartConnected = true;
int uartErrorCount = 0;

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
  pinMode(RELAY_FAN, OUTPUT);
  pinMode(RELAY_LIGHT12V, OUTPUT);
  pinMode(RELAY_AUXFAN, OUTPUT);

  pinMode(JSN_TRIG, OUTPUT);
  pinMode(JSN_ECHO, INPUT);

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
  lcd.print("INITIALIZING...");

  // Init DHT
  dht.begin();

  // Init BH1750
  if (lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE)) {
    // OK
  } else {
    lcd.setCursor(0, 1);
    lcd.print("BH1750 ERROR!   ");
  }

  // Init Servos
  servoWindow.attach(SERVO_WINDOW);
  servoDoor.attach(SERVO_DOOR);
  servoWindow.write(0);   // CLOSE
  servoDoor.write(0);     // CLOSE
  actuators.window = false;
  actuators.door = false;

  // Init DFPlayer
  dfSerial.begin(9600);
  delay(500);
  initDFPlayer();

  // Init UART (hardware Serial)
  Serial.begin(UART_BAUD);
  while (!Serial) ; // Wait for serial ready

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

  // Check UART for commands
  if (Serial.available()) {
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

  // Emergency checks (always active)
  emergencyChecks();

  delay(10);
}

// ==================== SENSOR READING ====================
void readAllSensors() {
  // DHT11
  sensors.temp_c = dht.readTemperature();
  sensors.hum_pct = dht.readHumidity();
  if (isnan(sensors.temp_c)) sensors.temp_c = -999;
  if (isnan(sensors.hum_pct)) sensors.hum_pct = -999;

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

  JsonObject payload = doc.createNestedObject("payload");
  payload["temp_c"] = sensors.temp_c;
  payload["hum_pct"] = sensors.hum_pct;
  payload["light_lux"] = sensors.light_lux;
  payload["soil_pct"] = sensors.soil_pct;
  payload["mq3"] = sensors.mq3;
  payload["flame"] = sensors.flame;
  payload["sound"] = sensors.sound;
  payload["distance_cm"] = sensors.distance_cm;

  serializeJson(doc, Serial);
  Serial.println();
}

void handleUartCommand() {
  String line = Serial.readStringUntil('\n');
  line.trim();

  if (line.length() == 0) return;

  StaticJsonDocument<512> doc;
  DeserializationError error = deserializeJson(doc, line);

  if (error) {
    uartErrorCount++;
    return;
  }

  const char* type = doc["type"];
  if (strcmp(type, "set") == 0) {
    handleSetCommand(doc);
  }
}

void handleSetCommand(JsonDocument& doc) {
  long req_id = doc["req_id"];
  JsonObject payload = doc["payload"];

  bool ok = true;

  // Relays
  if (payload.containsKey("pump")) {
    int val = payload["pump"];
    actuators.pump = (val == 1);
    setRelay(RELAY_PUMP, actuators.pump);
  }

  if (payload.containsKey("fan_main")) {
    int val = payload["fan_main"];
    actuators.fan_main = (val == 1);
    setRelay(RELAY_FAN, actuators.fan_main);
  }

  if (payload.containsKey("fan_aux")) {
    int val = payload["fan_aux"];
    actuators.fan_aux = (val == 1);
    setRelay(RELAY_AUXFAN, actuators.fan_aux);
  }

  if (payload.containsKey("led12v")) {
    int val = payload["led12v"];
    actuators.led12v = (val == 1 || val == 2);
    setRelay(RELAY_LIGHT12V, actuators.led12v);
    // Note: BLINK (val==2) not implemented on UNO, ESP8266 handles LED effects
  }

  // Servos
  if (payload.containsKey("window")) {
    int val = payload["window"];
    actuators.window = (val == 1);
    servoWindow.write(actuators.window ? 90 : 0);
    pushLcdMessage("WINDOW", actuators.window ? "OPENING" : "CLOSING");
  }

  if (payload.containsKey("door")) {
    int val = payload["door"];
    actuators.door = (val == 1);
    servoDoor.write(actuators.door ? 90 : 0);
    pushLcdMessage("DOOR", actuators.door ? "OPENING" : "CLOSING");
  }

  // Send ACK
  StaticJsonDocument<128> ack;
  ack["type"] = "ack";
  ack["ts"] = millis() / 1000;
  ack["req_id"] = req_id;
  ack["ok"] = ok;

  serializeJson(ack, Serial);
  Serial.println();
}

void setRelay(int pin, bool state) {
  digitalWrite(pin, state ? RELAY_ON : RELAY_OFF);
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

void updateLcd() {
  unsigned long now = millis();

  // Check queue
  if (lcdQueueHead != lcdQueueTail) {
    LcdMessage& msg = lcdQueue[lcdQueueHead];
    if (now < msg.showUntil) {
      // Show queued message
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(msg.line1);
      lcd.setCursor(0, 1);
      lcd.print(msg.line2);
      lcdShowingSensor = false;
      return;
    } else {
      // Message expired
      lcdQueueHead = (lcdQueueHead + 1) % LCD_QUEUE_SIZE;
      lcdShowingSensor = true;
    }
  }

  // Show sensor data
  if (lcdShowingSensor) {
    lcd.clear();

    // Line 1: T:25.5C H:68%
    lcd.setCursor(0, 0);
    lcd.print("T:");
    if (sensors.temp_c > -500) {
      lcd.print(sensors.temp_c, 1);
    } else {
      lcd.print("--.-");
    }
    lcd.print("C H:");
    if (sensors.hum_pct > -500) {
      lcd.print((int)sensors.hum_pct);
    } else {
      lcd.print("--");
    }
    lcd.print("%");

    // Line 2: SOIL:45% LUX:120
    lcd.setCursor(0, 1);
    lcd.print("S:");
    lcd.print((int)sensors.soil_pct);
    lcd.print("% L:");
    lcd.print((int)sensors.light_lux);
  }
}

// ==================== SAFETY & FALLBACK ====================
void safeState() {
  digitalWrite(RELAY_PUMP, RELAY_OFF);
  digitalWrite(RELAY_FAN, RELAY_OFF);
  digitalWrite(RELAY_LIGHT12V, RELAY_OFF);
  digitalWrite(RELAY_AUXFAN, RELAY_OFF);

  actuators.pump = false;
  actuators.fan_main = false;
  actuators.fan_aux = false;
  actuators.led12v = false;
}

void fallbackSafety() {
  // When UART lost, apply safe defaults
  pushLcdMessage("UART LOST", "FALLBACK MODE");

  // Check flame
  if (sensors.flame > FLAME_THRESHOLD) {
    setRelay(RELAY_PUMP, false);  // Turn off pump
    setRelay(RELAY_FAN, true);    // Fan ON
    setRelay(RELAY_AUXFAN, true); // Aux fan ON
    pushLcdMessage("FIRE DETECT!", "FANS ON");
    return;
  }

  // Check temperature
  if (sensors.temp_c > TEMP_SAFE_MAX) {
    setRelay(RELAY_FAN, true);
    setRelay(RELAY_AUXFAN, true);
    servoWindow.write(90); // Open window
    pushLcdMessage("TEMP HIGH", "COOLING");
    return;
  }

  // Default safe
  safeState();
}

void emergencyChecks() {
  // Always check flame regardless of mode
  if (sensors.flame > FLAME_THRESHOLD) {
    // Send event
    StaticJsonDocument<128> doc;
    doc["type"] = "event";
    doc["ts"] = millis() / 1000;
    doc["event"] = "FIRE";
    doc["level"] = sensors.flame;

    serializeJson(doc, Serial);
    Serial.println();
  }

  // Sound alert
  if (sensors.sound > SOUND_THRESHOLD) {
    StaticJsonDocument<128> doc;
    doc["type"] = "event";
    doc["ts"] = millis() / 1000;
    doc["event"] = "SOUND";
    doc["level"] = sensors.sound;

    serializeJson(doc, Serial);
    Serial.println();
  }
}
