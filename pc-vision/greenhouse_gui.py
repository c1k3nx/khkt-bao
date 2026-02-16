#!/usr/bin/env python3
"""
Greenhouse IoT - PyQt6 GUI Application
======================================
Tính năng:
- Realtime sensor monitoring (temp, humidity, soil, light, etc.)
- ESP32-CAM video stream với YOLO plant detection overlay
- Device control (pump, fans, lights, servos, RGB LED)
- Charts & graphs cho sensor history
- MQTT integration
- Auto/Manual mode switching
- Alert notifications

Dependencies:
    pip install PyQt6 paho-mqtt opencv-python numpy pyqtgraph pillow requests ultralytics
"""

import sys
import json
import time
import cv2
import numpy as np
import requests
from datetime import datetime
from typing import Dict, List, Optional
from collections import deque

from PyQt6.QtWidgets import (
    QApplication, QMainWindow, QWidget, QVBoxLayout, QHBoxLayout,
    QGridLayout, QLabel, QPushButton, QGroupBox, QSlider, QLineEdit,
    QTabWidget, QTableWidget, QTableWidgetItem, QComboBox, QCheckBox,
    QSpinBox, QDoubleSpinBox, QTextEdit, QSplitter, QFrame, QMessageBox
)
from PyQt6.QtCore import (
    Qt, QTimer, pyqtSignal, QThread, pyqtSlot, QSize
)
from PyQt6.QtGui import (
    QPixmap, QImage, QFont, QPalette, QColor, QIcon
)

import paho.mqtt.client as mqtt
import pyqtgraph as pg

# Try to import YOLO, optional
try:
    from ultralytics import YOLO
    YOLO_AVAILABLE = True
except ImportError:
    YOLO_AVAILABLE = False
    print("⚠️  YOLO not available. Install with: pip install ultralytics")


# ==================== CONFIGURATION ====================
class Config:
    """Application configuration"""

    # MQTT Settings
    MQTT_BROKER = "broker.hivemq.com"
    MQTT_PORT = 1883
    MQTT_CLIENT_ID = "greenhouse-gui"

    # ESP32-CAM Settings
    CAM_URL = "http://192.168.4.1"

    # YOLO Settings
    YOLO_WEIGHTS = "yolov8n.pt"
    YOLO_CONF = 0.25

    # Update intervals (ms)
    CAMERA_UPDATE_MS = 100
    SENSOR_UPDATE_MS = 1000
    CHART_UPDATE_MS = 2000

    # Chart history size
    CHART_HISTORY_SIZE = 100


# ==================== MQTT CLIENT THREAD ====================
class MqttClientThread(QThread):
    """MQTT client running in separate thread"""

    # Signals
    sensor_data_received = pyqtSignal(dict)
    device_status_received = pyqtSignal(str, str)  # device, status
    event_received = pyqtSignal(str, str)  # event_type, value
    connection_status = pyqtSignal(bool)

    def __init__(self, broker: str, port: int):
        super().__init__()
        self.broker = broker
        self.port = port
        self.client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION1, Config.MQTT_CLIENT_ID)
        self.client.on_connect = self._on_connect
        self.client.on_message = self._on_message
        self.client.on_disconnect = self._on_disconnect
        self.running = True

    def _on_connect(self, client, userdata, flags, rc):
        """MQTT connect callback"""
        if rc == 0:
            print("✓ MQTT Connected")
            self.connection_status.emit(True)

            # Subscribe to all topics
            client.subscribe("greenhouse/data/#")
            client.subscribe("greenhouse/status/#")
            client.subscribe("greenhouse/event/#")
            client.subscribe("greenhouse/sys/#")
        else:
            print(f"✗ MQTT Connect failed: {rc}")
            self.connection_status.emit(False)

    def _on_disconnect(self, client, userdata, rc):
        """MQTT disconnect callback"""
        print("⚠️  MQTT Disconnected")
        self.connection_status.emit(False)

    def _on_message(self, client, userdata, msg):
        """MQTT message callback"""
        topic = msg.topic

        try:
            # Parse JSON if possible
            try:
                payload = json.loads(msg.payload.decode())
            except:
                payload = msg.payload.decode()

            # Route messages
            if topic.startswith("greenhouse/data/"):
                sensor_name = topic.split("/")[-1]
                self.sensor_data_received.emit({sensor_name: payload})

            elif topic.startswith("greenhouse/status/"):
                device = topic.split("/")[-1]
                self.device_status_received.emit(device, str(payload))

            elif topic.startswith("greenhouse/event/"):
                event_type = topic.split("/")[-1]
                self.event_received.emit(event_type, str(payload))

        except Exception as e:
            print(f"Error processing message: {e}")

    def run(self):
        """Thread main loop"""
        try:
            self.client.connect(self.broker, self.port, 60)
            self.client.loop_start()

            while self.running:
                self.msleep(100)

        except Exception as e:
            print(f"MQTT thread error: {e}")
        finally:
            self.client.loop_stop()
            self.client.disconnect()

    def publish(self, topic: str, payload):
        """Publish message to MQTT"""
        try:
            if isinstance(payload, dict):
                payload = json.dumps(payload)
            self.client.publish(topic, str(payload))
        except Exception as e:
            print(f"Publish error: {e}")

    def stop(self):
        """Stop thread"""
        self.running = False


# ==================== CAMERA THREAD ====================
class CameraThread(QThread):
    """Camera stream thread with YOLO detection"""

    frame_ready = pyqtSignal(np.ndarray)

    def __init__(self, cam_url: str, yolo_enabled: bool = False):
        super().__init__()
        self.cam_url = cam_url
        self.running = True
        self.yolo_enabled = yolo_enabled

        # Load YOLO model if available
        self.yolo_model = None
        if YOLO_AVAILABLE and yolo_enabled:
            try:
                self.yolo_model = YOLO(Config.YOLO_WEIGHTS)
                print("✓ YOLO model loaded")
            except:
                print("⚠️  YOLO model load failed")

    def run(self):
        """Thread main loop"""
        while self.running:
            try:
                # Fetch image from ESP32-CAM
                response = requests.get(f"{self.cam_url}/capture", timeout=2)
                if response.status_code == 200:
                    # Decode image
                    img_array = np.frombuffer(response.content, np.uint8)
                    frame = cv2.imdecode(img_array, cv2.IMREAD_COLOR)

                    if frame is not None:
                        # Run YOLO detection if enabled
                        if self.yolo_model is not None:
                            results = self.yolo_model(frame, conf=Config.YOLO_CONF, verbose=False)
                            frame = results[0].plot()  # Draw boxes

                        self.frame_ready.emit(frame)

            except Exception as e:
                # Create error frame
                error_frame = np.zeros((480, 640, 3), dtype=np.uint8)
                cv2.putText(error_frame, "Camera Offline", (200, 240),
                           cv2.FONT_HERSHEY_SIMPLEX, 1, (0, 0, 255), 2)
                self.frame_ready.emit(error_frame)

            self.msleep(Config.CAMERA_UPDATE_MS)

    def stop(self):
        """Stop thread"""
        self.running = False


# ==================== SENSOR CARD WIDGET ====================
class SensorCard(QGroupBox):
    """Widget to display single sensor value"""

    def __init__(self, name: str, unit: str = "", icon: str = "📊"):
        super().__init__()
        self.setTitle(f"{icon} {name}")

        layout = QVBoxLayout()

        # Value label
        self.value_label = QLabel("--")
        self.value_label.setAlignment(Qt.AlignmentFlag.AlignCenter)
        font = QFont()
        font.setPointSize(24)
        font.setBold(True)
        self.value_label.setFont(font)

        # Unit label
        self.unit_label = QLabel(unit)
        self.unit_label.setAlignment(Qt.AlignmentFlag.AlignCenter)

        # Timestamp
        self.time_label = QLabel("")
        self.time_label.setAlignment(Qt.AlignmentFlag.AlignCenter)
        self.time_label.setStyleSheet("color: gray; font-size: 10px;")

        layout.addWidget(self.value_label)
        layout.addWidget(self.unit_label)
        layout.addWidget(self.time_label)

        self.setLayout(layout)
        self.setMinimumHeight(120)

    def update_value(self, value, timestamp: Optional[int] = None):
        """Update sensor value"""
        try:
            if isinstance(value, dict):
                # Extract value from standardized format
                val = value.get('v', '--')
                unit = value.get('u', '')
                ts = value.get('t', timestamp)
            else:
                val = value
                ts = timestamp

            # Format value
            if isinstance(val, (int, float)):
                self.value_label.setText(f"{val:.1f}")
            else:
                self.value_label.setText(str(val))

            # Update timestamp
            if ts:
                time_str = datetime.fromtimestamp(ts / 1000 if ts > 10000000000 else ts).strftime("%H:%M:%S")
                self.time_label.setText(time_str)

        except Exception as e:
            print(f"Error updating sensor card: {e}")


# ==================== DEVICE CONTROL WIDGET ====================
class DeviceControl(QGroupBox):
    """Widget to control a device (ON/OFF)"""

    control_changed = pyqtSignal(str, str)  # device, action

    def __init__(self, device_name: str, mqtt_topic: str, icon: str = "🔌"):
        super().__init__()
        self.setTitle(f"{icon} {device_name}")
        self.device_name = device_name
        self.mqtt_topic = mqtt_topic

        layout = QHBoxLayout()

        # Status label
        self.status_label = QLabel("OFF")
        self.status_label.setAlignment(Qt.AlignmentFlag.AlignCenter)
        font = QFont()
        font.setBold(True)
        self.status_label.setFont(font)

        # ON button
        self.on_btn = QPushButton("ON")
        self.on_btn.clicked.connect(lambda: self.set_state("ON"))
        self.on_btn.setStyleSheet("background-color: #4CAF50; color: white;")

        # OFF button
        self.off_btn = QPushButton("OFF")
        self.off_btn.clicked.connect(lambda: self.set_state("OFF"))
        self.off_btn.setStyleSheet("background-color: #f44336; color: white;")

        layout.addWidget(self.status_label, 2)
        layout.addWidget(self.on_btn, 1)
        layout.addWidget(self.off_btn, 1)

        self.setLayout(layout)

    def set_state(self, action: str):
        """Set device state"""
        self.control_changed.emit(self.mqtt_topic, action)

    def update_status(self, status: str):
        """Update status label"""
        self.status_label.setText(status)
        if status == "ON":
            self.status_label.setStyleSheet("color: #4CAF50;")
        else:
            self.status_label.setStyleSheet("color: #f44336;")


# ==================== MAIN WINDOW ====================
class GreenhouseGUI(QMainWindow):
    """Main application window"""

    def __init__(self):
        super().__init__()
        self.setWindowTitle("🌱 Greenhouse IoT Control Center - v1.4 Phase 7")
        self.setGeometry(100, 100, 1400, 900)

        # Data storage
        self.sensor_data = {}
        self.sensor_history = {
            'temperature': deque(maxlen=Config.CHART_HISTORY_SIZE),
            'humidity': deque(maxlen=Config.CHART_HISTORY_SIZE),
            'soilMoisture': deque(maxlen=Config.CHART_HISTORY_SIZE),
            'lightIntensity': deque(maxlen=Config.CHART_HISTORY_SIZE),
        }

        # Setup UI
        self.setup_ui()

        # Start MQTT client
        self.mqtt_thread = MqttClientThread(Config.MQTT_BROKER, Config.MQTT_PORT)
        self.mqtt_thread.sensor_data_received.connect(self.on_sensor_data)
        self.mqtt_thread.device_status_received.connect(self.on_device_status)
        self.mqtt_thread.event_received.connect(self.on_event)
        self.mqtt_thread.connection_status.connect(self.on_mqtt_connection)
        self.mqtt_thread.start()

        # Start camera thread
        self.camera_thread = CameraThread(Config.CAM_URL, yolo_enabled=True)
        self.camera_thread.frame_ready.connect(self.on_camera_frame)
        self.camera_thread.start()

        # Chart update timer
        self.chart_timer = QTimer()
        self.chart_timer.timeout.connect(self.update_charts)
        self.chart_timer.start(Config.CHART_UPDATE_MS)

        print("✓ Greenhouse GUI initialized")

    def setup_ui(self):
        """Setup user interface"""
        central_widget = QWidget()
        self.setCentralWidget(central_widget)

        main_layout = QVBoxLayout()
        central_widget.setLayout(main_layout)

        # Header
        header = self.create_header()
        main_layout.addWidget(header)

        # Tab widget
        tabs = QTabWidget()

        # Tab 1: Dashboard
        tabs.addTab(self.create_dashboard_tab(), "📊 Dashboard")

        # Tab 2: Control
        tabs.addTab(self.create_control_tab(), "🎛️ Control")

        # Tab 3: Camera
        tabs.addTab(self.create_camera_tab(), "📷 Camera")

        # Tab 4: Charts
        tabs.addTab(self.create_charts_tab(), "📈 Charts")

        # Tab 5: Alerts
        tabs.addTab(self.create_alerts_tab(), "🚨 Alerts")

        main_layout.addWidget(tabs)

        # Status bar
        self.statusBar().showMessage("Initializing...")

    def create_header(self) -> QWidget:
        """Create header with connection status"""
        header = QWidget()
        layout = QHBoxLayout()

        # Title
        title = QLabel("🌱 GREENHOUSE IoT - v1.4 Phase 7")
        font = QFont()
        font.setPointSize(16)
        font.setBold(True)
        title.setFont(font)

        # MQTT status
        self.mqtt_status = QLabel("MQTT: Connecting...")
        self.mqtt_status.setStyleSheet("color: orange;")

        # Mode indicator
        self.mode_label = QLabel("Mode: AUTO")
        self.mode_label.setStyleSheet("color: #4CAF50; font-weight: bold;")

        layout.addWidget(title)
        layout.addStretch()
        layout.addWidget(self.mqtt_status)
        layout.addWidget(self.mode_label)

        header.setLayout(layout)
        return header

    def create_dashboard_tab(self) -> QWidget:
        """Create dashboard tab with sensor cards"""
        widget = QWidget()
        layout = QGridLayout()

        # Create sensor cards
        self.temp_card = SensorCard("Temperature", "°C", "🌡️")
        self.hum_card = SensorCard("Humidity", "%", "💧")
        self.soil_card = SensorCard("Soil Moisture", "%", "🌱")
        self.light_card = SensorCard("Light", "lux", "💡")
        self.gas_card = SensorCard("Gas (MQ-3)", "V", "💨")
        self.flame_card = SensorCard("Flame", "raw", "🔥")
        self.sound_card = SensorCard("Sound", "raw", "🔊")
        self.tank_card = SensorCard("Water Tank", "cm", "💧")

        # Layout cards in grid
        layout.addWidget(self.temp_card, 0, 0)
        layout.addWidget(self.hum_card, 0, 1)
        layout.addWidget(self.soil_card, 0, 2)
        layout.addWidget(self.light_card, 0, 3)
        layout.addWidget(self.gas_card, 1, 0)
        layout.addWidget(self.flame_card, 1, 1)
        layout.addWidget(self.sound_card, 1, 2)
        layout.addWidget(self.tank_card, 1, 3)

        widget.setLayout(layout)
        return widget

    def create_control_tab(self) -> QWidget:
        """Create device control tab"""
        widget = QWidget()
        layout = QVBoxLayout()

        # Mode control
        mode_group = QGroupBox("🎯 Control Mode")
        mode_layout = QHBoxLayout()

        self.auto_btn = QPushButton("AUTO Mode")
        self.auto_btn.clicked.connect(lambda: self.set_mode("AUTO"))
        self.auto_btn.setStyleSheet("background-color: #4CAF50; color: white; padding: 10px;")

        self.manual_btn = QPushButton("MANUAL Mode")
        self.manual_btn.clicked.connect(lambda: self.set_mode("MANUAL"))
        self.manual_btn.setStyleSheet("background-color: #FF9800; color: white; padding: 10px;")

        mode_layout.addWidget(self.auto_btn)
        mode_layout.addWidget(self.manual_btn)
        mode_group.setLayout(mode_layout)

        layout.addWidget(mode_group)

        # Device controls
        devices_group = QGroupBox("⚙️ Device Controls")
        devices_layout = QGridLayout()

        # Create device controls
        self.pump_control = DeviceControl("Water Pump", "pump", "💧")
        self.fan_control = DeviceControl("Fan", "fan", "🌀")
        self.auxfan_control = DeviceControl("Aux Fan", "auxFan", "🌀")
        self.light_control = DeviceControl("Grow Light", "mainGrowLight", "💡")
        self.window_control = DeviceControl("Window", "window1", "🪟")
        self.door_control = DeviceControl("Door", "mainDoor", "🚪")

        # Connect signals
        for control in [self.pump_control, self.fan_control, self.auxfan_control,
                       self.light_control, self.window_control, self.door_control]:
            control.control_changed.connect(self.on_device_control)

        # Layout controls
        devices_layout.addWidget(self.pump_control, 0, 0)
        devices_layout.addWidget(self.fan_control, 0, 1)
        devices_layout.addWidget(self.auxfan_control, 0, 2)
        devices_layout.addWidget(self.light_control, 1, 0)
        devices_layout.addWidget(self.window_control, 1, 1)
        devices_layout.addWidget(self.door_control, 1, 2)

        devices_group.setLayout(devices_layout)
        layout.addWidget(devices_group)

        # RGB LED Control
        rgb_group = QGroupBox("🌈 RGB LED Control")
        rgb_layout = QHBoxLayout()

        rgb_layout.addWidget(QLabel("Color:"))

        self.color_input = QLineEdit("#00FF00")
        self.color_input.setMaxLength(7)

        color_btn = QPushButton("Set Color")
        color_btn.clicked.connect(self.set_rgb_color)

        off_btn = QPushButton("OFF")
        off_btn.clicked.connect(lambda: self.mqtt_thread.publish("greenhouse/control/rgbLed", "OFF"))

        rgb_layout.addWidget(self.color_input)
        rgb_layout.addWidget(color_btn)
        rgb_layout.addWidget(off_btn)

        rgb_group.setLayout(rgb_layout)
        layout.addWidget(rgb_group)

        layout.addStretch()

        widget.setLayout(layout)
        return widget

    def create_camera_tab(self) -> QWidget:
        """Create camera view tab"""
        widget = QWidget()
        layout = QVBoxLayout()

        # Camera feed
        self.camera_label = QLabel()
        self.camera_label.setAlignment(Qt.AlignmentFlag.AlignCenter)
        self.camera_label.setMinimumSize(640, 480)
        self.camera_label.setStyleSheet("border: 2px solid #ccc;")

        layout.addWidget(self.camera_label)

        # Camera controls
        controls_layout = QHBoxLayout()

        refresh_btn = QPushButton("📷 Capture")
        refresh_btn.clicked.connect(self.capture_image)

        self.yolo_check = QCheckBox("YOLO Detection")
        self.yolo_check.setChecked(True)

        controls_layout.addWidget(refresh_btn)
        controls_layout.addWidget(self.yolo_check)
        controls_layout.addStretch()

        layout.addLayout(controls_layout)

        widget.setLayout(layout)
        return widget

    def create_charts_tab(self) -> QWidget:
        """Create charts tab"""
        widget = QWidget()
        layout = QGridLayout()

        # Temperature chart
        self.temp_plot = pg.PlotWidget(title="Temperature (°C)")
        self.temp_plot.setBackground('w')
        self.temp_plot.setLabel('left', 'Temperature', units='°C')
        self.temp_plot.setLabel('bottom', 'Samples')
        self.temp_plot.showGrid(x=True, y=True)
        self.temp_curve = self.temp_plot.plot(pen='r')

        # Humidity chart
        self.hum_plot = pg.PlotWidget(title="Humidity (%)")
        self.hum_plot.setBackground('w')
        self.hum_plot.setLabel('left', 'Humidity', units='%')
        self.hum_plot.setLabel('bottom', 'Samples')
        self.hum_plot.showGrid(x=True, y=True)
        self.hum_curve = self.hum_plot.plot(pen='b')

        # Soil chart
        self.soil_plot = pg.PlotWidget(title="Soil Moisture (%)")
        self.soil_plot.setBackground('w')
        self.soil_plot.setLabel('left', 'Soil', units='%')
        self.soil_plot.setLabel('bottom', 'Samples')
        self.soil_plot.showGrid(x=True, y=True)
        self.soil_curve = self.soil_plot.plot(pen='g')

        # Light chart
        self.light_plot = pg.PlotWidget(title="Light Intensity (lux)")
        self.light_plot.setBackground('w')
        self.light_plot.setLabel('left', 'Light', units='lux')
        self.light_plot.setLabel('bottom', 'Samples')
        self.light_plot.showGrid(x=True, y=True)
        self.light_curve = self.light_plot.plot(pen='y')

        # Layout charts
        layout.addWidget(self.temp_plot, 0, 0)
        layout.addWidget(self.hum_plot, 0, 1)
        layout.addWidget(self.soil_plot, 1, 0)
        layout.addWidget(self.light_plot, 1, 1)

        widget.setLayout(layout)
        return widget

    def create_alerts_tab(self) -> QWidget:
        """Create alerts log tab"""
        widget = QWidget()
        layout = QVBoxLayout()

        label = QLabel("🚨 System Alerts & Events")
        font = QFont()
        font.setPointSize(12)
        font.setBold(True)
        label.setFont(font)

        self.alerts_text = QTextEdit()
        self.alerts_text.setReadOnly(True)
        self.alerts_text.setStyleSheet("background-color: #f5f5f5;")

        clear_btn = QPushButton("Clear Alerts")
        clear_btn.clicked.connect(self.alerts_text.clear)

        layout.addWidget(label)
        layout.addWidget(self.alerts_text)
        layout.addWidget(clear_btn)

        widget.setLayout(layout)
        return widget

    # ==================== SLOTS ====================
    @pyqtSlot(dict)
    def on_sensor_data(self, data: Dict):
        """Handle incoming sensor data"""
        for sensor_name, value in data.items():
            # Update sensor card
            if sensor_name == "temperature":
                self.temp_card.update_value(value)
                if isinstance(value, dict):
                    self.sensor_history['temperature'].append(value.get('v', 0))
            elif sensor_name == "humidity":
                self.hum_card.update_value(value)
                if isinstance(value, dict):
                    self.sensor_history['humidity'].append(value.get('v', 0))
            elif sensor_name == "soilMoisture":
                self.soil_card.update_value(value)
                if isinstance(value, dict):
                    self.sensor_history['soilMoisture'].append(value.get('v', 0))
            elif sensor_name == "lightIntensity":
                self.light_card.update_value(value)
                if isinstance(value, dict):
                    self.sensor_history['lightIntensity'].append(value.get('v', 0))
            elif sensor_name == "gasMQ3":
                self.gas_card.update_value(value)
            elif sensor_name == "flameAnalog":
                self.flame_card.update_value(value)
            elif sensor_name == "soundLevel":
                self.sound_card.update_value(value)
            elif sensor_name == "waterTankLevel":
                self.tank_card.update_value(value)

    @pyqtSlot(str, str)
    def on_device_status(self, device: str, status: str):
        """Handle device status update"""
        # Update device control widgets
        controls = {
            'pump': self.pump_control,
            'fan': self.fan_control,
            'auxFan': self.auxfan_control,
            'mainGrowLight': self.light_control,
            'window1': self.window_control,
            'mainDoor': self.door_control,
        }

        if device in controls:
            controls[device].update_status(status)

        # Update mode
        if device == 'mode':
            self.mode_label.setText(f"Mode: {status}")
            if status == "AUTO":
                self.mode_label.setStyleSheet("color: #4CAF50; font-weight: bold;")
            else:
                self.mode_label.setStyleSheet("color: #FF9800; font-weight: bold;")

    @pyqtSlot(str, str)
    def on_event(self, event_type: str, value: str):
        """Handle event (flame, sound, error, etc.)"""
        timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")

        # Format message
        if event_type == "flame":
            msg = f"🔥 FIRE DETECTED! Value: {value}"
            color = "red"
        elif event_type == "sound":
            msg = f"🔊 LOUD SOUND! Value: {value}"
            color = "orange"
        elif event_type == "error":
            msg = f"❌ ERROR: {value}"
            color = "red"
        else:
            msg = f"ℹ️  {event_type}: {value}"
            color = "blue"

        # Add to alerts
        alert = f'<span style="color: {color};">[{timestamp}] {msg}</span>'
        self.alerts_text.append(alert)

        # Show notification
        if event_type in ["flame", "error"]:
            QMessageBox.warning(self, "Alert", msg)

    @pyqtSlot(bool)
    def on_mqtt_connection(self, connected: bool):
        """Handle MQTT connection status"""
        if connected:
            self.mqtt_status.setText("MQTT: ✓ Connected")
            self.mqtt_status.setStyleSheet("color: green;")
            self.statusBar().showMessage("Connected to MQTT broker")
        else:
            self.mqtt_status.setText("MQTT: ✗ Disconnected")
            self.mqtt_status.setStyleSheet("color: red;")
            self.statusBar().showMessage("MQTT connection lost")

    @pyqtSlot(np.ndarray)
    def on_camera_frame(self, frame: np.ndarray):
        """Handle camera frame"""
        try:
            # Convert BGR to RGB
            rgb_frame = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
            h, w, ch = rgb_frame.shape
            bytes_per_line = ch * w

            # Create QImage
            qt_image = QImage(rgb_frame.data, w, h, bytes_per_line, QImage.Format.Format_RGB888)

            # Scale to fit label
            pixmap = QPixmap.fromImage(qt_image)
            scaled_pixmap = pixmap.scaled(
                self.camera_label.size(),
                Qt.AspectRatioMode.KeepAspectRatio,
                Qt.TransformationMode.SmoothTransformation
            )

            self.camera_label.setPixmap(scaled_pixmap)

        except Exception as e:
            print(f"Error displaying frame: {e}")

    def update_charts(self):
        """Update all charts"""
        try:
            # Temperature
            if self.sensor_history['temperature']:
                self.temp_curve.setData(list(self.sensor_history['temperature']))

            # Humidity
            if self.sensor_history['humidity']:
                self.hum_curve.setData(list(self.sensor_history['humidity']))

            # Soil
            if self.sensor_history['soilMoisture']:
                self.soil_curve.setData(list(self.sensor_history['soilMoisture']))

            # Light
            if self.sensor_history['lightIntensity']:
                self.light_curve.setData(list(self.sensor_history['lightIntensity']))

        except Exception as e:
            print(f"Error updating charts: {e}")

    # ==================== CONTROL METHODS ====================
    def set_mode(self, mode: str):
        """Set AUTO/MANUAL mode"""
        self.mqtt_thread.publish("greenhouse/control/mode", mode)

    def on_device_control(self, device: str, action: str):
        """Handle device control"""
        topic = f"greenhouse/control/{device}"
        self.mqtt_thread.publish(topic, action)
        self.statusBar().showMessage(f"Sent: {device} → {action}")

    def set_rgb_color(self):
        """Set RGB LED color"""
        color = self.color_input.text()
        if len(color) == 7 and color[0] == '#':
            self.mqtt_thread.publish("greenhouse/control/rgbLed", color)
            self.statusBar().showMessage(f"RGB color set to {color}")
        else:
            QMessageBox.warning(self, "Invalid Color", "Please enter a valid hex color (#RRGGBB)")

    def capture_image(self):
        """Capture and save current camera frame"""
        try:
            filename = f"capture_{datetime.now().strftime('%Y%m%d_%H%M%S')}.jpg"
            pixmap = self.camera_label.pixmap()
            if pixmap:
                pixmap.save(filename)
                self.statusBar().showMessage(f"Saved: {filename}")
                QMessageBox.information(self, "Capture", f"Image saved: {filename}")
        except Exception as e:
            QMessageBox.warning(self, "Error", f"Failed to save image: {e}")

    # ==================== CLEANUP ====================
    def closeEvent(self, event):
        """Cleanup on close"""
        print("Shutting down...")

        # Stop threads
        self.mqtt_thread.stop()
        self.camera_thread.stop()

        # Wait for threads
        self.mqtt_thread.wait()
        self.camera_thread.wait()

        event.accept()


# ==================== MAIN ====================
def main():
    """Main entry point"""
    app = QApplication(sys.argv)

    # Set dark theme (optional)
    app.setStyle("Fusion")

    # Create and show window
    window = GreenhouseGUI()
    window.show()

    sys.exit(app.exec())


if __name__ == "__main__":
    main()
