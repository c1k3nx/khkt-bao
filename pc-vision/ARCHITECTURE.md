# 🏗️ GREENHOUSE IoT GUI - ARCHITECTURE

## 📁 CẤU TRÚC FILE

```
pc-vision/
├── greenhouse_gui.py          # Main GUI application (existing)
├── config_manager.py          # ✅ Configuration management
├── data_logger.py             # ✅ CSV export & event logging
├── vision.py                  # AI vision module (existing)
├── test_vision.py             # Vision tests (existing)
│
├── config.json                # User configuration (auto-generated)
├── config.example.py          # Example configuration
├── requirements.txt           # Python dependencies
├── run_gui.sh                 # Quick launcher
│
├── README_GUI.md              # English documentation
├── HUONG_DAN_SU_DUNG.md       # Vietnamese guide
└── ARCHITECTURE.md            # This file
```

---

## 🎯 MODULES OVERVIEW

### 1. **config_manager.py** ✅ COMPLETED

**Chức năng:**
- Quản lý toàn bộ cấu hình ứng dụng
- Auto-load/save JSON config
- Validation với error checking
- Default values
- Singleton pattern

**API:**
```python
from config_manager import get_config

config = get_config()

# Get values
broker = config.get("mqtt.broker")
fps = config.get("camera.fps")
theme = config.get("ui.theme")

# Set values
config.set("mqtt.broker", "test.mosquitto.org")
config.set("camera.fps", 20)

# Validate
is_valid, errors = config.validate()

# Reset
config.reset_to_defaults()
```

**Cấu hình:**
- `mqtt.*` - MQTT settings
- `camera.*` - Camera & YOLO settings
- `ui.*` - UI preferences
- `charts.*` - Chart settings
- `alerts.*` - Alert preferences
- `export.*` - Data export settings
- `thresholds.*` - Display thresholds
- `advanced.*` - Advanced settings

---

### 2. **data_logger.py** ✅ COMPLETED

**Chức năng:**
- CSV data logging với auto-rotation
- Event logging với circular buffer
- Thread-safe operations
- Export functions

**API:**
```python
from data_logger import get_csv_logger, get_event_logger

# CSV Logger
csv_logger = get_csv_logger()

# Log sensor data
sensor_data = {
    'ts': timestamp,
    'temperature': {'v': 25.3, 'u': '°C'},
    'humidity': {'v': 65.2, 'u': '%'}
}
csv_logger.log_sensor_data(sensor_data)

# Event Logger
event_logger = get_event_logger()

# Log events
event_logger.log_event("flame", "Fire detected!", "critical")
event_logger.log_event("sound", "Loud noise", "warning")

# Get events
recent = event_logger.get_latest(10)
all_events = event_logger.get_events()

# Export
event_logger.export_to_csv("events.csv")
```

---

### 3. **greenhouse_gui.py** (Main Application)

**Chức năng:**
- Main PyQt6 GUI
- 5 tabs (Dashboard, Control, Camera, Charts, Alerts)
- MQTT integration
- Camera stream với YOLO
- Device control
- Realtime charts

**Cần integrate:**
1. Import config_manager
2. Import data_logger
3. Add Settings dialog
4. Add Menu bar
5. Add About dialog
6. Implement auto-export

---

## 🔄 DATA FLOW

```
┌─────────────────────────────────────────────────────────────┐
│                    Main Application                          │
│                  (greenhouse_gui.py)                         │
└───────────┬─────────────────────────────────┬───────────────┘
            │                                 │
            │                                 │
    ┌───────▼────────┐                ┌──────▼────────┐
    │ Config Manager │                │ Data Loggers  │
    │                │                │               │
    │ • Load/Save    │                │ • CSV Export  │
    │ • Validate     │                │ • Events Log  │
    │ • Defaults     │                │ • Thread-safe │
    └────────────────┘                └───────────────┘
            │                                 │
            │                                 │
            ▼                                 ▼
    ┌────────────────┐                ┌──────────────┐
    │  config.json   │                │ data/*.csv   │
    │  (persisted)   │                │ events.log   │
    └────────────────┘                └──────────────┘
```

---

## 🔌 INTEGRATION GUIDE

### Bước 1: Update greenhouse_gui.py imports

```python
#!/usr/bin/env python3
import sys
from config_manager import get_config
from data_logger import get_csv_logger, get_event_logger

# ... existing imports ...
```

### Bước 2: Initialize trong __init__

```python
class GreenhouseGUI(QMainWindow):
    def __init__(self):
        super().__init__()

        # Load configuration
        self.config = get_config()

        # Setup loggers
        self.csv_logger = get_csv_logger()
        self.event_logger = get_event_logger()

        # Apply config to UI
        self.apply_config()

        # ... rest of init ...
```

### Bước 3: Add apply_config method

```python
def apply_config(self):
    """Apply configuration to UI"""

    # Window size/position
    self.resize(
        self.config.get("ui.window_width"),
        self.config.get("ui.window_height")
    )
    self.move(
        self.config.get("ui.window_x"),
        self.config.get("ui.window_y")
    )

    # Update MQTT settings
    Config.MQTT_BROKER = self.config.get("mqtt.broker")
    Config.MQTT_PORT = self.config.get("mqtt.port")

    # Update camera settings
    Config.CAM_URL = self.config.get("camera.url")
    Config.CAMERA_UPDATE_MS = 1000 // self.config.get("camera.fps")

    # Update chart settings
    Config.CHART_HISTORY_SIZE = self.config.get("charts.history_size")
```

### Bước 4: Log sensor data

```python
@pyqtSlot(dict)
def on_sensor_data(self, data: Dict):
    """Handle incoming sensor data"""

    # ... existing update code ...

    # Log to CSV if enabled
    if self.config.get("export.enable_csv"):
        self.csv_logger.log_sensor_data(data)
```

### Bước 5: Log events

```python
@pyqtSlot(str, str)
def on_event(self, event_type: str, value: str):
    """Handle event"""

    # ... existing code ...

    # Log event
    severity = "critical" if event_type in ["flame", "fire"] else "warning"
    self.event_logger.log_event(event_type, f"{event_type}: {value}", severity)
```

### Bước 6: Add Settings dialog

```python
def show_settings_dialog(self):
    """Show settings dialog"""
    from PyQt6.QtWidgets import QDialog, QDialogButtonBox, QTabWidget

    dialog = QDialog(self)
    dialog.setWindowTitle("Settings")
    dialog.resize(600, 500)

    layout = QVBoxLayout()

    # Tabs for different settings
    tabs = QTabWidget()

    # MQTT tab
    mqtt_tab = self.create_mqtt_settings_tab()
    tabs.addTab(mqtt_tab, "MQTT")

    # Camera tab
    camera_tab = self.create_camera_settings_tab()
    tabs.addTab(camera_tab, "Camera")

    # UI tab
    ui_tab = self.create_ui_settings_tab()
    tabs.addTab(ui_tab, "UI")

    # Export tab
    export_tab = self.create_export_settings_tab()
    tabs.addTab(export_tab, "Export")

    layout.addWidget(tabs)

    # Buttons
    buttons = QDialogButtonBox(
        QDialogButtonBox.StandardButton.Ok |
        QDialogButtonBox.StandardButton.Cancel |
        QDialogButtonBox.StandardButton.RestoreDefaults
    )
    buttons.accepted.connect(dialog.accept)
    buttons.rejected.connect(dialog.reject)
    buttons.button(QDialogButtonBox.StandardButton.RestoreDefaults).clicked.connect(
        self.reset_settings
    )

    layout.addWidget(buttons)
    dialog.setLayout(layout)

    if dialog.exec() == QDialog.DialogCode.Accepted:
        self.apply_settings()
```

---

## 🎨 UI ENHANCEMENTS

### Menu Bar

```python
def create_menu_bar(self):
    """Create menu bar"""
    menubar = self.menuBar()

    # File Menu
    file_menu = menubar.addMenu("&File")

    export_action = file_menu.addAction("Export Data...")
    export_action.triggered.connect(self.export_data)

    export_config_action = file_menu.addAction("Export Settings...")
    export_config_action.triggered.connect(self.export_settings)

    file_menu.addSeparator()

    exit_action = file_menu.addAction("E&xit")
    exit_action.triggered.connect(self.close)

    # Edit Menu
    edit_menu = menubar.addMenu("&Edit")

    settings_action = edit_menu.addAction("Settings...")
    settings_action.triggered.connect(self.show_settings_dialog)

    # View Menu
    view_menu = menubar.addMenu("&View")

    fullscreen_action = view_menu.addAction("Fullscreen")
    fullscreen_action.setCheckable(True)
    fullscreen_action.triggered.connect(self.toggle_fullscreen)

    # Tools Menu
    tools_menu = menubar.addMenu("&Tools")

    clear_alerts_action = tools_menu.addAction("Clear Alerts")
    clear_alerts_action.triggered.connect(self.event_logger.clear)

    test_mqtt_action = tools_menu.addAction("Test MQTT Connection")
    test_mqtt_action.triggered.connect(self.test_mqtt_connection)

    # Help Menu
    help_menu = menubar.addMenu("&Help")

    docs_action = help_menu.addAction("Documentation")
    docs_action.triggered.connect(self.show_documentation)

    about_action = help_menu.addAction("About")
    about_action.triggered.connect(self.show_about_dialog)
```

### Status Bar

```python
def update_status_bar(self):
    """Update status bar với detailed info"""

    # Connection status
    mqtt_status = "✓ MQTT" if self.mqtt_connected else "✗ MQTT"

    # Sensor update
    if time.time() - self.last_sensor_time < 10:
        sensor_status = "✓ Sensors"
    else:
        sensor_status = "⚠ Sensors timeout"

    # Camera status
    camera_status = "✓ Camera" if self.camera_active else "✗ Camera"

    # Data export
    if self.config.get("export.enable_csv"):
        export_status = f"📊 Logging to {self.csv_logger.get_latest_file().name}"
    else:
        export_status = "Export: OFF"

    # Combine
    status = f"{mqtt_status} | {sensor_status} | {camera_status} | {export_status}"

    self.statusBar().showMessage(status)
```

---

## ✨ TÍNH NĂNG PRODUCTION

### 1. Auto-save Configuration
- Tự động save khi thay đổi settings
- Backup config trước khi save
- Validation before apply

### 2. Data Persistence
- CSV export với timestamps
- Event logging
- Configuration history

### 3. Error Handling
- Try-except ở mọi I/O operations
- Graceful degradation
- User-friendly error messages
- Logging để debug

### 4. Performance
- Thread-safe operations
- Non-blocking UI
- Efficient data structures
- Memory management

### 5. User Experience
- Tooltips
- Status indicators
- Progress bars
- Keyboard shortcuts
- Responsive UI

---

## 🚀 USAGE EXAMPLES

### Example 1: Run with custom config

```bash
# Create custom config
cp config.example.py my_config.json

# Edit my_config.json
nano my_config.json

# Run with custom config
python greenhouse_gui.py --config my_config.json
```

### Example 2: Export data programmatically

```python
from data_logger import get_csv_logger

logger = get_csv_logger()

# Get all CSV files
files = logger.get_all_files()
print(f"Found {len(files)} CSV files")

# Get latest
latest = logger.get_latest_file()
print(f"Latest: {latest}")
```

### Example 3: Backup configuration

```python
from config_manager import get_config

config = get_config()

# Export to backup
config.export_to_json("backup_config.json")

# Later, restore
config.import_from_json("backup_config.json")
```

---

## 🔧 TROUBLESHOOTING

### Config not loading
```bash
# Delete corrupt config
rm config.json

# Restart app (will create default)
python greenhouse_gui.py
```

### CSV files too large
```python
# Adjust in config.json
{
  "export": {
    "max_file_size_mb": 5  // Default: 10
  }
}
```

### Memory usage high
```python
# Reduce chart history
{
  "charts": {
    "history_size": 50  // Default: 100
  }
}
```

---

## 📚 API REFERENCE

### ConfigManager

| Method | Description |
|--------|-------------|
| `get(key_path, default)` | Get config value |
| `set(key_path, value)` | Set config value |
| `save()` | Save to file |
| `load()` | Load from file |
| `validate()` | Validate config |
| `reset_to_defaults()` | Reset all |

### CSVDataLogger

| Method | Description |
|--------|-------------|
| `log_sensor_data(data)` | Log sensor reading |
| `get_latest_file()` | Get current file |
| `get_all_files()` | List all CSVs |
| `close()` | Close logger |

### EventLogger

| Method | Description |
|--------|-------------|
| `log_event(type, msg, severity)` | Log event |
| `get_events(limit, type)` | Get filtered events |
| `get_latest(n)` | Get latest N |
| `clear()` | Clear all |
| `export_to_csv(file)` | Export to CSV |

---

**🌱 v1.4 COMPREHENSIVE OVERHAUL - Production Ready!**
