# ESP32 Receiver — TinyML Processing Hub

This directory contains the firmware and TinyML model files for the **ESP32 Receiver**
that collects raw sensor data from ESP32 Transmitter nodes and runs on-device
inference before forwarding processed results to the monitoring dashboard.

## Architecture

```
┌─────────────────────┐        ESP-NOW        ┌──────────────────────────┐       Serial/JSON       ┌──────────────────┐
│  ESP32 TRANSMITTER  │ ─────────────────────► │   ESP32 RECEIVER         │ ─────────────────────► │  Node.js Server   │
│  (Sensor Node)      │   Raw sensor packets   │   (TinyML Processing)    │   Processed packets    │  (Dashboard)      │
│                     │                        │                          │                        │                   │
│  • DHT22 (Temp)     │                        │  • Activity Classifier   │                        │  • WebSocket UI   │
│  • MQ2/MQ9/MQ135    │                        │  • Fall Detector         │                        │  • Logging        │
│  • MPU6050 (IMU)    │                        │  • Anomaly Detector      │                        │  • Alerts         │
│  • GPS (NEO-6M)     │                        │  • Fire Validator        │                        │                   │
└─────────────────────┘                        │  • Air Quality Classifier│                        └──────────────────┘
                                               └──────────────────────────┘
```

## Files

| File | Description |
|------|-------------|
| `receiver_main.ino` | Main Arduino sketch — ESP-NOW receive, TinyML inference, Serial output |
| `model_activity.h` | TFLite model for activity classification (STANDING/WALKING/RUNNING/CRAWLING) |
| `model_fall_detection.h` | TFLite model for fall detection (STABLE/FALL) |
| `model_anomaly.h` | TFLite model for sensor anomaly detection |
| `model_fire_validation.h` | TFLite model for fire event validation |
| `model_air_quality.h` | TFLite model for air quality classification (CLEAN/MODERATE/TOXIC) |
| `tinyml_engine.h` | Unified TinyML inference wrapper |
| `tinyml_engine.cpp` | Implementation of the inference engine |
| `config.h` | Configuration constants and thresholds |
| `transmitter_firmware.ino` | Reference transmitter firmware for sensor nodes |

## Setup

### Hardware Requirements

**Receiver:**
- ESP32 DevKit V1 (or compatible)
- USB connection to PC running the Node.js server

**Transmitter (per firefighter node):**
- ESP32 DevKit V1
- DHT22 temperature/humidity sensor
- MQ2 (smoke), MQ9 (CO), MQ135 (air quality) gas sensors
- MPU6050 6-axis IMU (accelerometer + gyroscope)
- NEO-6M GPS module

### Software Requirements

1. Arduino IDE 2.x or PlatformIO
2. ESP32 Board Support Package
3. Libraries:
   - `TensorFlowLite_ESP32` (TFLite Micro)
   - `ESP-NOW` (built-in)
   - `WiFi` (built-in)

### Flashing

1. Open `receiver_main.ino` in Arduino IDE
2. Select Board: **ESP32 Dev Module**
3. Set Upload Speed: **115200**
4. Set the correct COM port
5. Upload

### Connecting to Dashboard

The receiver outputs processed JSON over Serial at **115200 baud**.
The Node.js `server.js` auto-detects this serial port and ingests the data.

## TinyML Models

All models are stored as C header files (`const unsigned char model_data[]`)
converted from TensorFlow Lite `.tflite` files using `xxd`.

| Model | Input Shape | Output | Size |
|-------|-------------|--------|------|
| Activity Classifier | [1, 30, 6] — 30 timesteps × 6 IMU axes | 4 classes | ~12 KB |
| Fall Detector | [1, 20, 6] — 20 timesteps × 6 IMU axes | 2 classes | ~8 KB |
| Anomaly Detector | [1, 8] — 8 sensor features | anomaly score | ~6 KB |
| Fire Validator | [1, 5] — temp + 3 gas + humidity | 2 classes | ~4 KB |
| Air Quality | [1, 3] — 3 gas sensor readings | 3 classes | ~4 KB |

## Data Flow

1. **Transmitter** reads all sensors → packs into struct → sends via ESP-NOW
2. **Receiver** catches ESP-NOW callback → buffers IMU for time-series models
3. **TinyML Engine** runs inference on buffered data
4. **Output** is a JSON string printed to Serial:
   ```json
   {
     "id": 101,
     "temp": 34.5,
     "mq2": 142,
     "mq9": 88,
     "mq135": 165,
     "lat": 12.9692,
     "lon": 79.1559,
     "status": "SAFE",
     "fall_status": "STABLE",
     "air_quality": "CLEAN",
     "fire_valid": false,
     "activity": "WALKING",
     "anomaly": false,
     "emergency": false
   }
   ```
