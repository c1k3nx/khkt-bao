#!/usr/bin/env python3
"""
PC-Vision for Greenhouse IoT System
====================================
- Fetch images from ESP32-CAM via HTTP
- YOLO v8/v11 for plant detection (counting)
- Hugging Face model for disease detection
- Publish results to MQTT

Dependencies:
- ultralytics (YOLOv8/v11)
- opencv-python
- paho-mqtt
- requests
- transformers (Hugging Face)
- torch
- pillow

Configuration: .env file or environment variables
"""

import os
import sys
import time
import json
import logging
from typing import Dict, List, Tuple, Optional
from datetime import datetime
from io import BytesIO

import cv2
import numpy as np
import requests
from PIL import Image
import paho.mqtt.client as mqtt
from ultralytics import YOLO
from transformers import AutoFeatureExtractor, AutoModelForImageClassification
import torch

# ==================== CONFIGURATION ====================
class Config:
    """Configuration from environment variables"""

    # MQTT
    BROKER = os.getenv("MQTT_BROKER", "broker.hivemq.com")
    PORT = int(os.getenv("MQTT_PORT", "1883"))
    CLIENT_ID = os.getenv("MQTT_CLIENT_ID", "gh-pc-vision")

    # ESP32-CAM
    CAM_URL = os.getenv("CAM_URL", "http://192.168.4.1")
    CAM_IP = CAM_URL.split("//")[-1].split(":")[0]

    # YOLO
    YOLO_WEIGHTS = os.getenv("YOLO_WEIGHTS", "yolov8n.pt")  # Change to custom model
    YOLO_CONF = float(os.getenv("YOLO_CONF", "0.25"))
    YOLO_IOU = float(os.getenv("YOLO_IOU", "0.45"))

    # Hugging Face Disease Model
    DISEASE_MODEL = os.getenv("DISEASE_MODEL", "linkanjarad/mobilenet_v2_1.0_224-plant-disease-identification")
    DISEASE_THRESHOLD = float(os.getenv("DISEASE_THRESHOLD", "0.7"))

    # Intervals
    INTERVAL_S = float(os.getenv("INTERVAL_S", "5.0"))

    # Logging
    LOG_LEVEL = os.getenv("LOG_LEVEL", "INFO")


# ==================== LOGGING ====================
logging.basicConfig(
    level=getattr(logging, Config.LOG_LEVEL),
    format='%(asctime)s [%(levelname)s] %(message)s',
    handlers=[
        logging.StreamHandler(sys.stdout)
    ]
)
logger = logging.getLogger(__name__)


# ==================== MQTT CLIENT ====================
class MqttClient:
    """MQTT client wrapper"""

    def __init__(self):
        self.client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION1, Config.CLIENT_ID)
        self.client.on_connect = self._on_connect
        self.client.on_disconnect = self._on_disconnect
        self.connected = False

    def _on_connect(self, client, userdata, flags, rc):
        if rc == 0:
            logger.info("MQTT connected")
            self.connected = True
        else:
            logger.error(f"MQTT connect failed: {rc}")
            self.connected = False

    def _on_disconnect(self, client, userdata, rc):
        logger.warning("MQTT disconnected")
        self.connected = False

    def connect(self):
        """Connect to MQTT broker with retry"""
        attempts = 0
        while attempts < 5:
            try:
                self.client.connect(Config.BROKER, Config.PORT, 60)
                self.client.loop_start()
                return True
            except Exception as e:
                logger.error(f"MQTT connect error: {e}")
                attempts += 1
                time.sleep(2 ** attempts)
        return False

    def publish(self, topic: str, payload: Dict):
        """Publish JSON payload to topic"""
        if not self.connected:
            logger.warning("MQTT not connected, skipping publish")
            return False

        try:
            json_str = json.dumps(payload)
            result = self.client.publish(topic, json_str, qos=0)
            if result.rc == mqtt.MQTT_ERR_SUCCESS:
                logger.debug(f"Published to {topic}: {json_str[:100]}")
                return True
            else:
                logger.error(f"Publish failed: {result.rc}")
                return False
        except Exception as e:
            logger.error(f"Publish exception: {e}")
            return False

    def disconnect(self):
        """Disconnect from broker"""
        self.client.loop_stop()
        self.client.disconnect()


# ==================== IMAGE FETCHER ====================
class ImageFetcher:
    """Fetch images from ESP32-CAM"""

    @staticmethod
    def fetch_image(url: str, timeout: int = 5) -> Optional[np.ndarray]:
        """
        Fetch image from camera endpoint
        Returns: OpenCV image (BGR) or None
        """
        try:
            capture_url = f"{url}/capture"
            logger.debug(f"Fetching image from {capture_url}")

            response = requests.get(capture_url, timeout=timeout)
            response.raise_for_status()

            # Convert to OpenCV image
            img_array = np.asarray(bytearray(response.content), dtype=np.uint8)
            img = cv2.imdecode(img_array, cv2.IMREAD_COLOR)

            if img is None:
                logger.error("Failed to decode image")
                return None

            logger.info(f"Image fetched: {img.shape}")
            return img

        except requests.exceptions.RequestException as e:
            logger.error(f"Image fetch error: {e}")
            return None
        except Exception as e:
            logger.error(f"Image decode error: {e}")
            return None


# ==================== PLANT DETECTOR (YOLO) ====================
class PlantDetector:
    """YOLO-based plant detection"""

    def __init__(self, weights_path: str):
        """
        Initialize YOLO model

        Args:
            weights_path: Path to YOLO weights (.pt file)
        """
        try:
            logger.info(f"Loading YOLO model: {weights_path}")
            self.model = YOLO(weights_path)
            logger.info("YOLO model loaded successfully")
        except Exception as e:
            logger.error(f"YOLO model load failed: {e}")
            self.model = None

    def detect_plants(self, image: np.ndarray) -> Tuple[int, List[List]]:
        """
        Detect plants in image

        Args:
            image: OpenCV image (BGR)

        Returns:
            (num_plants, boxes)
            boxes: List of [x, y, w, h, "plant_id"]
        """
        if self.model is None:
            logger.warning("YOLO model not loaded")
            return 0, []

        try:
            results = self.model(
                image,
                conf=Config.YOLO_CONF,
                iou=Config.YOLO_IOU,
                verbose=False
            )

            boxes = []
            for result in results:
                for box in result.boxes:
                    # Get box coordinates
                    x1, y1, x2, y2 = box.xyxy[0].cpu().numpy()
                    x, y, w, h = int(x1), int(y1), int(x2 - x1), int(y2 - y1)

                    # Get class (assume all detections are plants for now)
                    # In production, filter by class_id for "plant" or "leaf"
                    plant_id = f"plant_{len(boxes) + 1}"

                    boxes.append([x, y, w, h, plant_id])

            num_plants = len(boxes)
            logger.info(f"Detected {num_plants} plants")

            return num_plants, boxes

        except Exception as e:
            logger.error(f"YOLO detection error: {e}")
            return 0, []


# ==================== DISEASE DETECTOR (Hugging Face) ====================
class DiseaseDetector:
    """Hugging Face model for disease detection"""

    def __init__(self, model_name: str):
        """
        Initialize disease detection model

        Args:
            model_name: Hugging Face model name
        """
        try:
            logger.info(f"Loading disease model: {model_name}")

            self.device = "cuda" if torch.cuda.is_available() else "cpu"
            logger.info(f"Using device: {self.device}")

            self.feature_extractor = AutoFeatureExtractor.from_pretrained(model_name)
            self.model = AutoModelForImageClassification.from_pretrained(model_name)
            self.model.to(self.device)
            self.model.eval()

            logger.info("Disease model loaded successfully")

        except Exception as e:
            logger.error(f"Disease model load failed: {e}")
            self.model = None

    def detect_disease(self, image: np.ndarray) -> Tuple[bool, str, float]:
        """
        Detect disease in image

        Args:
            image: OpenCV image (BGR)

        Returns:
            (diseased, disease_label, score)
        """
        if self.model is None:
            logger.warning("Disease model not loaded")
            return False, "unknown", 0.0

        try:
            # Convert BGR to RGB
            rgb_image = cv2.cvtColor(image, cv2.COLOR_BGR2RGB)
            pil_image = Image.fromarray(rgb_image)

            # Preprocess
            inputs = self.feature_extractor(images=pil_image, return_tensors="pt")
            inputs = {k: v.to(self.device) for k, v in inputs.items()}

            # Inference
            with torch.no_grad():
                outputs = self.model(**inputs)
                logits = outputs.logits

            # Get prediction
            probs = torch.nn.functional.softmax(logits, dim=-1)
            top_prob, top_class = torch.max(probs, dim=1)

            score = top_prob.item()
            class_idx = top_class.item()

            # Get label name
            label = self.model.config.id2label.get(class_idx, "unknown")

            # Check if diseased (not "healthy")
            diseased = ("healthy" not in label.lower()) and (score > Config.DISEASE_THRESHOLD)

            logger.info(f"Disease detection: {label} ({score:.2f}), diseased={diseased}")

            return diseased, label, score

        except Exception as e:
            logger.error(f"Disease detection error: {e}")
            return False, "error", 0.0


# ==================== MAIN VISION PROCESSOR ====================
class VisionProcessor:
    """Main vision processing pipeline"""

    def __init__(self):
        self.mqtt_client = MqttClient()
        self.image_fetcher = ImageFetcher()
        self.plant_detector = PlantDetector(Config.YOLO_WEIGHTS)
        self.disease_detector = DiseaseDetector(Config.DISEASE_MODEL)

    def process_frame(self) -> Optional[Dict]:
        """
        Process one frame from camera

        Returns:
            Detection result dict or None
        """
        # Fetch image
        image = self.image_fetcher.fetch_image(Config.CAM_URL)
        if image is None:
            return None

        # Detect plants
        num_plants, boxes = self.plant_detector.detect_plants(image)

        # Detect disease
        diseased, disease_label, disease_score = self.disease_detector.detect_disease(image)

        # Build result
        result = {
            "ts": int(time.time()),
            "num_plants": num_plants,
            "diseased": diseased,
            "disease_label": disease_label,
            "disease_score": round(disease_score, 3),
            "boxes": boxes,
            "cam_ip": Config.CAM_IP
        }

        logger.info(f"Processing result: {num_plants} plants, diseased={diseased}")

        return result

    def publish_result(self, result: Dict):
        """Publish result to MQTT"""
        self.mqtt_client.publish("gh/cv/detections", result)

    def run(self):
        """Main run loop"""
        logger.info("Vision processor starting...")

        # Connect to MQTT
        if not self.mqtt_client.connect():
            logger.error("MQTT connection failed, exiting")
            return

        logger.info(f"Processing interval: {Config.INTERVAL_S}s")
        logger.info(f"Camera URL: {Config.CAM_URL}/capture")

        try:
            while True:
                start_time = time.time()

                # Process frame
                result = self.process_frame()

                # Publish result
                if result:
                    self.publish_result(result)

                # Wait for next interval
                elapsed = time.time() - start_time
                sleep_time = max(0, Config.INTERVAL_S - elapsed)
                if sleep_time > 0:
                    time.sleep(sleep_time)

        except KeyboardInterrupt:
            logger.info("Interrupted by user")
        except Exception as e:
            logger.error(f"Runtime error: {e}", exc_info=True)
        finally:
            self.mqtt_client.disconnect()
            logger.info("Vision processor stopped")


# ==================== MAIN ENTRY POINT ====================
def main():
    """Main entry point"""
    logger.info("=" * 60)
    logger.info("Greenhouse PC-Vision Starting")
    logger.info("=" * 60)
    logger.info(f"MQTT Broker: {Config.BROKER}:{Config.PORT}")
    logger.info(f"Camera URL: {Config.CAM_URL}")
    logger.info(f"YOLO Model: {Config.YOLO_WEIGHTS}")
    logger.info(f"Disease Model: {Config.DISEASE_MODEL}")
    logger.info("=" * 60)

    processor = VisionProcessor()
    processor.run()


if __name__ == "__main__":
    main()
