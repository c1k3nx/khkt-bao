#!/usr/bin/env python3
"""
Configuration Manager - Quản lý cấu hình ứng dụng
Tự động load/save settings, validation, defaults
"""

import json
import os
from typing import Dict, Any, Optional
from pathlib import Path
import logging

logger = logging.getLogger(__name__)


class ConfigManager:
    """
    Quản lý cấu hình ứng dụng
    - Load/save JSON config
    - Validation
    - Default values
    - Auto-save on change
    """

    DEFAULT_CONFIG = {
        # MQTT Settings
        "mqtt": {
            "broker": "broker.hivemq.com",
            "port": 1883,
            "client_id": "greenhouse-gui",
            "keepalive": 60,
            "auto_reconnect": True,
            "max_reconnect_attempts": 5,
            "reconnect_delay": 2
        },

        # Camera Settings
        "camera": {
            "url": "http://192.168.4.1",
            "timeout": 5,
            "fps": 10,
            "enable_yolo": True,
            "yolo_model": "yolov8n.pt",
            "yolo_confidence": 0.25,
            "yolo_iou": 0.45,
            "auto_save_captures": False,
            "captures_dir": "./captures"
        },

        # UI Settings
        "ui": {
            "theme": "light",  # light/dark
            "window_width": 1400,
            "window_height": 900,
            "window_x": 100,
            "window_y": 100,
            "font_size": 10,
            "show_toolbar": True,
            "show_statusbar": True,
            "enable_animations": True
        },

        # Chart Settings
        "charts": {
            "history_size": 100,
            "update_interval": 2000,
            "enable_grid": True,
            "enable_legend": True,
            "line_width": 2,
            "colors": {
                "temperature": "#FF0000",
                "humidity": "#0000FF",
                "soilMoisture": "#00FF00",
                "lightIntensity": "#FFFF00"
            }
        },

        # Alerts Settings
        "alerts": {
            "enable_sound": False,
            "enable_popup": True,
            "enable_logging": True,
            "max_log_entries": 1000,
            "auto_clear": False,
            "critical_events": ["flame", "fire", "error"]
        },

        # Data Export
        "export": {
            "enable_csv": False,
            "csv_interval": 60,
            "csv_dir": "./data",
            "auto_export": False,
            "include_timestamp": True,
            "decimal_places": 2
        },

        # Thresholds (display only, actual control on ESP8266)
        "thresholds": {
            "temp_warning_high": 35.0,
            "temp_warning_low": 15.0,
            "humidity_warning_high": 80.0,
            "humidity_warning_low": 40.0,
            "soil_warning_low": 30.0,
            "light_warning_low": 5000,
            "flame_warning": 800,
            "sound_warning": 700
        },

        # Advanced
        "advanced": {
            "enable_logging": True,
            "log_level": "INFO",
            "log_file": "greenhouse_gui.log",
            "max_log_size_mb": 10,
            "sensor_timeout": 10,
            "camera_retry": 3,
            "mqtt_qos": 0
        }
    }

    def __init__(self, config_file: str = "config.json"):
        """
        Initialize config manager

        Args:
            config_file: Path to JSON config file
        """
        self.config_file = Path(config_file)
        self.config: Dict[str, Any] = {}
        self.load()

    def load(self) -> bool:
        """
        Load configuration from file

        Returns:
            True if successful, False otherwise
        """
        try:
            if self.config_file.exists():
                with open(self.config_file, 'r', encoding='utf-8') as f:
                    loaded_config = json.load(f)

                # Merge with defaults (keep new keys from defaults)
                self.config = self._merge_configs(self.DEFAULT_CONFIG, loaded_config)
                logger.info(f"Config loaded from {self.config_file}")
                return True
            else:
                # Use defaults
                self.config = self.DEFAULT_CONFIG.copy()
                logger.info("Using default configuration")
                self.save()  # Save defaults to file
                return True

        except json.JSONDecodeError as e:
            logger.error(f"Invalid JSON in config file: {e}")
            self.config = self.DEFAULT_CONFIG.copy()
            return False

        except Exception as e:
            logger.error(f"Error loading config: {e}")
            self.config = self.DEFAULT_CONFIG.copy()
            return False

    def save(self) -> bool:
        """
        Save configuration to file

        Returns:
            True if successful, False otherwise
        """
        try:
            # Create parent directory if needed
            self.config_file.parent.mkdir(parents=True, exist_ok=True)

            with open(self.config_file, 'w', encoding='utf-8') as f:
                json.dump(self.config, f, indent=2, ensure_ascii=False)

            logger.info(f"Config saved to {self.config_file}")
            return True

        except Exception as e:
            logger.error(f"Error saving config: {e}")
            return False

    def get(self, key_path: str, default: Any = None) -> Any:
        """
        Get config value by dot-separated path

        Args:
            key_path: Dot-separated key path (e.g. "mqtt.broker")
            default: Default value if key not found

        Returns:
            Config value or default

        Example:
            >>> config.get("mqtt.broker")
            "broker.hivemq.com"
        """
        keys = key_path.split('.')
        value = self.config

        for key in keys:
            if isinstance(value, dict) and key in value:
                value = value[key]
            else:
                return default

        return value

    def set(self, key_path: str, value: Any, auto_save: bool = True) -> bool:
        """
        Set config value by dot-separated path

        Args:
            key_path: Dot-separated key path
            value: Value to set
            auto_save: Auto-save after setting

        Returns:
            True if successful

        Example:
            >>> config.set("mqtt.broker", "test.mosquitto.org")
        """
        try:
            keys = key_path.split('.')
            target = self.config

            # Navigate to parent dict
            for key in keys[:-1]:
                if key not in target:
                    target[key] = {}
                target = target[key]

            # Set value
            target[keys[-1]] = value

            if auto_save:
                self.save()

            return True

        except Exception as e:
            logger.error(f"Error setting config {key_path}: {e}")
            return False

    def reset_to_defaults(self) -> bool:
        """
        Reset all settings to defaults

        Returns:
            True if successful
        """
        try:
            self.config = self.DEFAULT_CONFIG.copy()
            self.save()
            logger.info("Config reset to defaults")
            return True

        except Exception as e:
            logger.error(f"Error resetting config: {e}")
            return False

    def validate(self) -> tuple[bool, list[str]]:
        """
        Validate configuration

        Returns:
            (is_valid, list_of_errors)
        """
        errors = []

        # Validate MQTT
        if not isinstance(self.get("mqtt.port"), int):
            errors.append("MQTT port must be integer")

        if not (1 <= self.get("mqtt.port") <= 65535):
            errors.append("MQTT port must be 1-65535")

        # Validate Camera
        if not self.get("camera.url"):
            errors.append("Camera URL cannot be empty")

        if not (1 <= self.get("camera.fps") <= 60):
            errors.append("Camera FPS must be 1-60")

        # Validate UI
        if not (800 <= self.get("ui.window_width") <= 3840):
            errors.append("Window width must be 800-3840")

        if not (600 <= self.get("ui.window_height") <= 2160):
            errors.append("Window height must be 600-2160")

        # Validate Charts
        if not (10 <= self.get("charts.history_size") <= 1000):
            errors.append("Chart history must be 10-1000")

        # Validate thresholds
        if self.get("thresholds.temp_warning_high") <= self.get("thresholds.temp_warning_low"):
            errors.append("Temp high must be > temp low")

        return len(errors) == 0, errors

    def export_to_json(self, file_path: str) -> bool:
        """Export config to JSON file"""
        try:
            with open(file_path, 'w', encoding='utf-8') as f:
                json.dump(self.config, f, indent=2, ensure_ascii=False)
            return True
        except Exception as e:
            logger.error(f"Export error: {e}")
            return False

    def import_from_json(self, file_path: str) -> bool:
        """Import config from JSON file"""
        try:
            with open(file_path, 'r', encoding='utf-8') as f:
                imported = json.load(f)

            self.config = self._merge_configs(self.DEFAULT_CONFIG, imported)
            self.save()
            return True

        except Exception as e:
            logger.error(f"Import error: {e}")
            return False

    def _merge_configs(self, default: Dict, override: Dict) -> Dict:
        """
        Recursively merge configs (deep merge)

        Args:
            default: Default config
            override: Override config

        Returns:
            Merged config
        """
        result = default.copy()

        for key, value in override.items():
            if key in result and isinstance(result[key], dict) and isinstance(value, dict):
                result[key] = self._merge_configs(result[key], value)
            else:
                result[key] = value

        return result

    def get_all(self) -> Dict[str, Any]:
        """Get entire config dict"""
        return self.config.copy()

    def get_category(self, category: str) -> Dict[str, Any]:
        """
        Get entire category

        Args:
            category: Category name (mqtt, camera, ui, etc.)

        Returns:
            Category dict or empty dict
        """
        return self.config.get(category, {}).copy()


# Singleton instance
_config_instance: Optional[ConfigManager] = None


def get_config() -> ConfigManager:
    """
    Get singleton config instance

    Returns:
        ConfigManager instance
    """
    global _config_instance
    if _config_instance is None:
        _config_instance = ConfigManager()
    return _config_instance


# Testing
if __name__ == "__main__":
    logging.basicConfig(level=logging.INFO)

    config = ConfigManager("test_config.json")

    print("MQTT Broker:", config.get("mqtt.broker"))
    print("Camera URL:", config.get("camera.url"))
    print("Chart colors:", config.get("charts.colors"))

    # Test set
    config.set("mqtt.broker", "test.mosquitto.org")
    print("New broker:", config.get("mqtt.broker"))

    # Test validation
    is_valid, errors = config.validate()
    print(f"Valid: {is_valid}")
    if errors:
        print("Errors:", errors)

    # Cleanup
    if Path("test_config.json").exists():
        Path("test_config.json").unlink()
