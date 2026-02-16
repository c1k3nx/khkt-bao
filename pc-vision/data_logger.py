#!/usr/bin/env python3
"""
Data Logger - CSV Export và Event Logging
Tự động log sensor data ra CSV files, event logging
"""

import csv
import logging
from datetime import datetime
from pathlib import Path
from typing import Dict, List, Any, Optional
from collections import deque
import threading


logger = logging.getLogger(__name__)


class CSVDataLogger:
    """
    CSV Data Logger cho sensor data
    - Auto-export sensor readings
    - Timestamped files
    - Rotation khi file quá lớn
    - Thread-safe
    """

    def __init__(self, output_dir: str = "./data", max_file_size_mb: int = 10):
        """
        Initialize CSV logger

        Args:
            output_dir: Output directory cho CSV files
            max_file_size_mb: Max file size trước khi rotate (MB)
        """
        self.output_dir = Path(output_dir)
        self.max_file_size = max_file_size_mb * 1024 * 1024  # Convert to bytes
        self.current_file: Optional[Path] = None
        self.csv_writer: Optional[csv.DictWriter] = None
        self.file_handle = None
        self.lock = threading.Lock()

        # Create output directory
        self.output_dir.mkdir(parents=True, exist_ok=True)

        # Sensor fields
        self.sensor_fields = [
            'timestamp',
            'datetime',
            'temperature',
            'humidity',
            'soilMoisture',
            'lightIntensity',
            'gasMQ3',
            'flameAnalog',
            'soundLevel',
            'waterTankLevel',
            'gps_lat',
            'gps_lng',
            'gps_valid'
        ]

        logger.info(f"CSV Logger initialized: {self.output_dir}")

    def log_sensor_data(self, data: Dict[str, Any]) -> bool:
        """
        Log sensor data to CSV

        Args:
            data: Sensor data dict

        Returns:
            True if successful
        """
        try:
            with self.lock:
                # Check if need to rotate file
                if self._should_rotate():
                    self._rotate_file()

                # Create new file if needed
                if self.csv_writer is None:
                    self._create_new_file()

                # Prepare row
                timestamp = data.get('ts', int(datetime.now().timestamp()))
                row = {
                    'timestamp': timestamp,
                    'datetime': datetime.fromtimestamp(timestamp).isoformat(),
                }

                # Extract sensor values
                for field in self.sensor_fields[2:]:  # Skip timestamp/datetime
                    sensor_value = data.get(field)

                    # Handle standardized format {v, u, t}
                    if isinstance(sensor_value, dict):
                        row[field] = sensor_value.get('v', '')
                    else:
                        row[field] = sensor_value if sensor_value is not None else ''

                # Write row
                self.csv_writer.writerow(row)
                self.file_handle.flush()  # Ensure data is written

                return True

        except Exception as e:
            logger.error(f"Error logging sensor data: {e}")
            return False

    def _should_rotate(self) -> bool:
        """Check if file should be rotated"""
        if self.current_file is None:
            return False

        if not self.current_file.exists():
            return True

        file_size = self.current_file.stat().st_size
        return file_size >= self.max_file_size

    def _rotate_file(self):
        """Rotate to new file"""
        self._close_current_file()
        logger.info(f"Rotated CSV file (size limit reached)")

    def _create_new_file(self):
        """Create new CSV file"""
        try:
            # Generate filename with timestamp
            timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
            filename = f"greenhouse_data_{timestamp}.csv"
            self.current_file = self.output_dir / filename

            # Open file and create CSV writer
            self.file_handle = open(self.current_file, 'w', newline='', encoding='utf-8')
            self.csv_writer = csv.DictWriter(
                self.file_handle,
                fieldnames=self.sensor_fields,
                extrasaction='ignore'
            )
            self.csv_writer.writeheader()

            logger.info(f"Created new CSV file: {self.current_file}")

        except Exception as e:
            logger.error(f"Error creating CSV file: {e}")

    def _close_current_file(self):
        """Close current CSV file"""
        if self.file_handle:
            try:
                self.file_handle.close()
                self.file_handle = None
                self.csv_writer = None
            except Exception as e:
                logger.error(f"Error closing CSV file: {e}")

    def close(self):
        """Close logger"""
        with self.lock:
            self._close_current_file()

    def get_latest_file(self) -> Optional[Path]:
        """Get path to latest CSV file"""
        return self.current_file

    def get_all_files(self) -> List[Path]:
        """Get all CSV files"""
        return sorted(self.output_dir.glob("greenhouse_data_*.csv"))

    def export_summary(self, output_file: str) -> bool:
        """
        Export summary statistics to file

        Args:
            output_file: Output file path

        Returns:
            True if successful
        """
        try:
            # TODO: Implement summary statistics
            logger.warning("Summary export not implemented yet")
            return False

        except Exception as e:
            logger.error(f"Error exporting summary: {e}")
            return False


class EventLogger:
    """
    Event Logger cho alerts và system events
    - In-memory circular buffer
    - Optional file logging
    - Event filtering
    - Thread-safe
    """

    def __init__(self, max_events: int = 1000, log_to_file: bool = False):
        """
        Initialize event logger

        Args:
            max_events: Maximum events to keep in memory
            log_to_file: Enable file logging
        """
        self.max_events = max_events
        self.log_to_file = log_to_file
        self.events = deque(maxlen=max_events)
        self.lock = threading.Lock()

        logger.info(f"Event Logger initialized (max: {max_events})")

    def log_event(self, event_type: str, message: str, severity: str = "info", metadata: Optional[Dict] = None):
        """
        Log an event

        Args:
            event_type: Type of event (flame, sound, error, etc.)
            message: Event message
            severity: Severity level (info, warning, error, critical)
            metadata: Additional metadata dict
        """
        try:
            timestamp = datetime.now()

            event = {
                'timestamp': timestamp,
                'type': event_type,
                'message': message,
                'severity': severity,
                'metadata': metadata or {}
            }

            with self.lock:
                self.events.append(event)

            # Log to file if enabled
            if self.log_to_file:
                log_msg = f"[{severity.upper()}] {event_type}: {message}"
                if severity == "critical" or severity == "error":
                    logger.error(log_msg)
                elif severity == "warning":
                    logger.warning(log_msg)
                else:
                    logger.info(log_msg)

        except Exception as e:
            logger.error(f"Error logging event: {e}")

    def get_events(self, limit: Optional[int] = None, event_type: Optional[str] = None,
                   severity: Optional[str] = None) -> List[Dict]:
        """
        Get events with optional filtering

        Args:
            limit: Max number of events to return
            event_type: Filter by event type
            severity: Filter by severity

        Returns:
            List of events
        """
        with self.lock:
            events = list(self.events)

        # Filter by type
        if event_type:
            events = [e for e in events if e['type'] == event_type]

        # Filter by severity
        if severity:
            events = [e for e in events if e['severity'] == severity]

        # Limit
        if limit:
            events = events[-limit:]

        return events

    def get_latest(self, n: int = 10) -> List[Dict]:
        """Get latest N events"""
        with self.lock:
            return list(self.events)[-n:]

    def clear(self):
        """Clear all events"""
        with self.lock:
            self.events.clear()
        logger.info("Event log cleared")

    def count(self, event_type: Optional[str] = None, severity: Optional[str] = None) -> int:
        """Count events with optional filtering"""
        events = self.get_events(event_type=event_type, severity=severity)
        return len(events)

    def export_to_csv(self, output_file: str) -> bool:
        """
        Export events to CSV file

        Args:
            output_file: Output file path

        Returns:
            True if successful
        """
        try:
            with self.lock:
                events = list(self.events)

            if not events:
                logger.warning("No events to export")
                return False

            with open(output_file, 'w', newline='', encoding='utf-8') as f:
                writer = csv.DictWriter(
                    f,
                    fieldnames=['timestamp', 'type', 'severity', 'message'],
                    extrasaction='ignore'
                )
                writer.writeheader()

                for event in events:
                    writer.writerow({
                        'timestamp': event['timestamp'].isoformat(),
                        'type': event['type'],
                        'severity': event['severity'],
                        'message': event['message']
                    })

            logger.info(f"Exported {len(events)} events to {output_file}")
            return True

        except Exception as e:
            logger.error(f"Error exporting events: {e}")
            return False


# Global instances
_csv_logger: Optional[CSVDataLogger] = None
_event_logger: Optional[EventLogger] = None


def get_csv_logger() -> CSVDataLogger:
    """Get CSV logger singleton"""
    global _csv_logger
    if _csv_logger is None:
        _csv_logger = CSVDataLogger()
    return _csv_logger


def get_event_logger() -> EventLogger:
    """Get event logger singleton"""
    global _event_logger
    if _event_logger is None:
        _event_logger = EventLogger()
    return _event_logger


# Testing
if __name__ == "__main__":
    logging.basicConfig(level=logging.INFO)

    # Test CSV logger
    csv_logger = CSVDataLogger("./test_data")

    # Test data
    test_data = {
        'ts': int(datetime.now().timestamp()),
        'temperature': {'v': 25.3, 'u': '°C'},
        'humidity': {'v': 65.2, 'u': '%'},
        'soilMoisture': {'v': 45.8, 'u': '%'},
        'lightIntensity': {'v': 12500, 'u': 'lux'}
    }

    csv_logger.log_sensor_data(test_data)
    print("CSV file:", csv_logger.get_latest_file())

    # Test event logger
    event_logger = EventLogger(max_events=100)

    event_logger.log_event("flame", "Fire detected!", "critical")
    event_logger.log_event("sound", "Loud noise", "warning")
    event_logger.log_event("info", "System started", "info")

    print("Events:", len(event_logger.get_events()))
    print("Latest:", event_logger.get_latest(2))

    # Cleanup
    csv_logger.close()
    import shutil
    if Path("./test_data").exists():
        shutil.rmtree("./test_data")
