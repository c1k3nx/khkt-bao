#!/usr/bin/env python3
"""
Simple test script for PC-Vision components
"""

import os
import sys
import json
import cv2
import numpy as np

# Test image fetch
def test_image_fetch():
    """Test fetching image from ESP32-CAM"""
    from vision import ImageFetcher, Config

    print("Testing image fetch...")
    print(f"Camera URL: {Config.CAM_URL}")

    fetcher = ImageFetcher()
    image = fetcher.fetch_image(Config.CAM_URL)

    if image is not None:
        print(f"✓ Image fetched: {image.shape}")
        # Save test image
        cv2.imwrite("test_image.jpg", image)
        print("✓ Saved to test_image.jpg")
        return True
    else:
        print("✗ Image fetch failed")
        return False


# Test YOLO detection
def test_yolo():
    """Test YOLO plant detection"""
    from vision import PlantDetector, Config

    print("\nTesting YOLO detection...")

    # Load test image
    if not os.path.exists("test_image.jpg"):
        print("✗ test_image.jpg not found, run test_image_fetch first")
        return False

    image = cv2.imread("test_image.jpg")
    if image is None:
        print("✗ Failed to load test_image.jpg")
        return False

    detector = PlantDetector(Config.YOLO_WEIGHTS)
    num_plants, boxes = detector.detect_plants(image)

    print(f"✓ Detected {num_plants} plants")
    for i, box in enumerate(boxes):
        x, y, w, h, plant_id = box
        print(f"  [{i+1}] {plant_id}: ({x}, {y}, {w}, {h})")

        # Draw box
        cv2.rectangle(image, (x, y), (x + w, y + h), (0, 255, 0), 2)
        cv2.putText(image, plant_id, (x, y - 10), cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0, 255, 0), 2)

    # Save annotated image
    cv2.imwrite("test_yolo_output.jpg", image)
    print("✓ Saved annotated image to test_yolo_output.jpg")

    return True


# Test disease detection
def test_disease():
    """Test disease detection"""
    from vision import DiseaseDetector, Config

    print("\nTesting disease detection...")

    # Load test image
    if not os.path.exists("test_image.jpg"):
        print("✗ test_image.jpg not found, run test_image_fetch first")
        return False

    image = cv2.imread("test_image.jpg")
    if image is None:
        print("✗ Failed to load test_image.jpg")
        return False

    detector = DiseaseDetector(Config.DISEASE_MODEL)
    diseased, label, score = detector.detect_disease(image)

    print(f"✓ Disease detection result:")
    print(f"  Diseased: {diseased}")
    print(f"  Label: {label}")
    print(f"  Score: {score:.3f}")

    return True


# Test MQTT publish
def test_mqtt():
    """Test MQTT connection and publish"""
    from vision import MqttClient

    print("\nTesting MQTT...")

    client = MqttClient()
    if not client.connect():
        print("✗ MQTT connection failed")
        return False

    print("✓ MQTT connected")

    # Test publish
    payload = {
        "ts": 1234567890,
        "num_plants": 5,
        "diseased": False,
        "disease_label": "healthy",
        "disease_score": 0.95,
        "boxes": [[10, 20, 30, 40, "plant_1"]],
        "cam_ip": "192.168.4.1"
    }

    success = client.publish("gh/cv/detections", payload)

    if success:
        print("✓ MQTT publish successful")
    else:
        print("✗ MQTT publish failed")

    client.disconnect()
    return success


def main():
    """Run all tests"""
    print("=" * 60)
    print("PC-Vision Component Tests")
    print("=" * 60)

    tests = [
        ("Image Fetch", test_image_fetch),
        ("YOLO Detection", test_yolo),
        ("Disease Detection", test_disease),
        ("MQTT Publish", test_mqtt),
    ]

    results = []
    for name, test_func in tests:
        try:
            result = test_func()
            results.append((name, result))
        except Exception as e:
            print(f"✗ {name} error: {e}")
            results.append((name, False))

    # Summary
    print("\n" + "=" * 60)
    print("Test Summary")
    print("=" * 60)
    for name, result in results:
        status = "✓ PASS" if result else "✗ FAIL"
        print(f"{status} - {name}")

    passed = sum(1 for _, r in results if r)
    total = len(results)
    print(f"\nPassed: {passed}/{total}")


if __name__ == "__main__":
    main()
