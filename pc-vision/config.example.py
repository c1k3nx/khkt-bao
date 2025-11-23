#!/usr/bin/env python3
"""
Configuration Example for Greenhouse GUI
========================================
Copy this file to config.py and customize your settings
"""

class Config:
    """Application configuration"""

    # ==================== MQTT SETTINGS ====================
    # Public MQTT broker (free tier)
    MQTT_BROKER = "broker.hivemq.com"
    MQTT_PORT = 1883
    MQTT_CLIENT_ID = "greenhouse-gui"

    # For private broker, use:
    # MQTT_BROKER = "192.168.1.100"
    # MQTT_USERNAME = "your_username"  # Not implemented yet
    # MQTT_PASSWORD = "your_password"  # Not implemented yet

    # ==================== ESP32-CAM SETTINGS ====================
    # ESP32-CAM Access Point mode
    CAM_URL = "http://192.168.4.1"

    # OR Station mode (connected to your WiFi)
    # CAM_URL = "http://192.168.1.150"

    # Camera resolution (set on ESP32-CAM)
    # FRAMESIZE_QVGA (320x240)
    # FRAMESIZE_VGA (640x480)
    # FRAMESIZE_SVGA (800x600)
    # FRAMESIZE_XGA (1024x768)

    # ==================== YOLO SETTINGS ====================
    # Model weights file
    # yolov8n.pt - Nano (fastest, least accurate)
    # yolov8s.pt - Small (balanced)
    # yolov8m.pt - Medium (slower, more accurate)
    # yolov8l.pt - Large (very slow, very accurate)
    # yolov8x.pt - Extra large (slowest, most accurate)
    YOLO_WEIGHTS = "yolov8n.pt"

    # Confidence threshold (0.0 - 1.0)
    # Lower = more detections but more false positives
    # Higher = fewer detections but more accurate
    YOLO_CONF = 0.25

    # IOU threshold for NMS (Non-Maximum Suppression)
    YOLO_IOU = 0.45

    # Use GPU if available
    # 0 = CPU only
    # "0" = First GPU
    # "0,1" = Multiple GPUs
    YOLO_DEVICE = "0"  # Change to "cpu" to force CPU

    # ==================== DISEASE DETECTION SETTINGS ====================
    # Hugging Face model for plant disease detection
    DISEASE_MODEL = "linkanjarad/mobilenet_v2_1.0_224-plant-disease-identification"

    # Other good models:
    # "nateraw/vit-base-beans"
    # "fxmarty/resnet-tiny-beans"
    # "linkanjarad/mobilenet_v2_1.0_224-plant-disease-identification"

    # Confidence threshold for disease classification
    DISEASE_THRESHOLD = 0.7

    # ==================== UPDATE INTERVALS ====================
    # Camera update interval (milliseconds)
    # Lower = higher FPS but more CPU usage
    # 100ms = 10 FPS
    # 50ms = 20 FPS (not recommended unless fast network)
    CAMERA_UPDATE_MS = 100

    # Sensor data update interval (milliseconds)
    SENSOR_UPDATE_MS = 1000  # 1 second

    # Chart redraw interval (milliseconds)
    CHART_UPDATE_MS = 2000  # 2 seconds

    # ==================== CHART SETTINGS ====================
    # Number of samples to keep in history
    CHART_HISTORY_SIZE = 100

    # Chart colors (use any valid matplotlib color)
    CHART_COLORS = {
        'temperature': 'r',    # red
        'humidity': 'b',       # blue
        'soilMoisture': 'g',   # green
        'lightIntensity': 'y', # yellow
    }

    # ==================== UI SETTINGS ====================
    # Window size (width, height)
    WINDOW_SIZE = (1400, 900)

    # Window title
    WINDOW_TITLE = "🌱 Greenhouse IoT Control Center - v1.4 Phase 7"

    # Enable dark theme
    DARK_THEME = False

    # Font size
    HEADER_FONT_SIZE = 16
    SENSOR_VALUE_FONT_SIZE = 24

    # ==================== ALERT SETTINGS ====================
    # Enable sound alerts (requires pygame or winsound)
    SOUND_ALERTS = False

    # Show popup for critical alerts
    POPUP_ALERTS = True

    # Alert keywords that trigger popups
    CRITICAL_EVENTS = ["flame", "fire", "error"]

    # ==================== LOGGING SETTINGS ====================
    # Log level: DEBUG, INFO, WARNING, ERROR, CRITICAL
    LOG_LEVEL = "INFO"

    # Log to file
    LOG_TO_FILE = False
    LOG_FILE = "greenhouse_gui.log"

    # ==================== ADVANCED SETTINGS ====================
    # Enable YOLO detection (requires ultralytics)
    ENABLE_YOLO = True

    # Enable disease detection (requires transformers)
    ENABLE_DISEASE_DETECTION = False

    # Number of frames to skip for YOLO (process every Nth frame)
    # Higher = faster but less responsive
    YOLO_SKIP_FRAMES = 1

    # Maximum MQTT reconnect attempts
    MQTT_MAX_RECONNECT = 5

    # MQTT reconnect delay (seconds)
    MQTT_RECONNECT_DELAY = 2

    # Camera request timeout (seconds)
    CAMERA_TIMEOUT = 5

    # ==================== GREENHOUSE THRESHOLDS ====================
    # These are display thresholds (for UI warnings)
    # Actual control thresholds are set on ESP8266

    TEMP_WARNING_HIGH = 35.0  # °C
    TEMP_WARNING_LOW = 15.0   # °C

    HUMIDITY_WARNING_HIGH = 80.0  # %
    HUMIDITY_WARNING_LOW = 40.0   # %

    SOIL_WARNING_LOW = 30.0  # %

    LIGHT_WARNING_LOW = 5000  # lux

    FLAME_WARNING = 800  # raw value
    SOUND_WARNING = 700  # raw value

    # ==================== DATA EXPORT ====================
    # Enable CSV export of sensor data
    ENABLE_CSV_EXPORT = False

    # CSV export interval (seconds)
    CSV_EXPORT_INTERVAL = 60

    # CSV export directory
    CSV_EXPORT_DIR = "./data"


# ==================== USAGE ====================
if __name__ == "__main__":
    print("Greenhouse GUI Configuration")
    print("=" * 60)
    print(f"MQTT Broker: {Config.MQTT_BROKER}:{Config.MQTT_PORT}")
    print(f"Camera URL: {Config.CAM_URL}")
    print(f"YOLO Model: {Config.YOLO_WEIGHTS}")
    print(f"Disease Model: {Config.DISEASE_MODEL}")
    print(f"Update Intervals:")
    print(f"  - Camera: {Config.CAMERA_UPDATE_MS}ms")
    print(f"  - Sensors: {Config.SENSOR_UPDATE_MS}ms")
    print(f"  - Charts: {Config.CHART_UPDATE_MS}ms")
    print("=" * 60)
