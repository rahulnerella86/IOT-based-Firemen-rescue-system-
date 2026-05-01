# 🔥 Firefighter Monitoring System

## 📌 Overview

The **Firefighter Monitoring System** is an IoT-based safety solution designed to monitor firefighters in real-time during hazardous operations. The system integrates **health monitoring, environmental sensing, location tracking, and long-range communication**, ensuring faster emergency response and improved safety.

This project combines **Embedded Systems, IoT, Edge AI (TinyML), and Web Technologies** to create a scalable and reliable monitoring platform.

---

## 🚀 Key Features

* 🧠 **Real-time Health Monitoring**

  * Heart rate tracking
  * Body/environment temperature monitoring

* 🌫️ **Environmental Monitoring**

  * Toxic gas detection (MQ sensors)
  * Fire detection using flame sensor

* 📍 **Location Tracking**

  * GPS-based outdoor positioning

* 📡 **Long-Range Communication**

  * LoRa (SX1278) for communication in low-signal environments

* 🤖 **Edge AI (TinyML)**

  * Fall detection
  * Activity recognition (walking, running, still)
  * Air quality classification (SAFE / SMOKE / TOXIC)
  * Anomaly detection

* 🚨 **Emergency Alert System**

  * Manual panic button
  * Automatic alerts on threshold breach

* 📊 **Real-Time Dashboard**

  * Live sensor data visualization
  * Map-based firefighter tracking
  * Alert highlighting and notifications

---

## 🏗️ System Architecture

### 1. Firefighter Node (Wearable Device)

* ESP32 microcontroller
* Sensors:

  * DHT22 (temperature)
  * MQ-2, MQ-9, MQ-135 (gas detection)
  * Pulse sensor (heart rate)
  * MPU6050 (motion tracking)
  * Flame sensor
  * GPS (NEO-6M)
* LoRa module (SX1278)

### 2. Communication Layer

* LoRa-based wireless communication
* Long-range (1+ km)
* Low power consumption
* Star topology (nodes → gateway)

### 3. Backend & Dashboard

* Node.js backend with Express
* Real-time communication via Socket.io
* Data storage:

  * JSON
  * CSV
  * SQLite
* Frontend dashboard:

  * Live monitoring
  * Interactive map (Leaflet.js)
  * Graphs (Chart.js)

---

## 📡 Data Flow

1. Sensors collect real-time data
2. ESP32 processes data using TinyML
3. Data is formatted into JSON
4. Transmitted via LoRa
5. Receiver sends data to backend
6. Dashboard updates in real-time

---

## 📦 Example Data Format

```json
{
  "id": 1,
  "temp": 32,
  "mq2": 300,
  "mq9": 200,
  "mq135": 400,
  "lat": 17.12,
  "lon": 78.12,
  "status": "DANGER",
  "fall_status": "FALL",
  "air_quality": "TOXIC",
  "fire_valid": true,
  "activity": "RUNNING",
  "anomaly": false,
  "emergency": true,
  "timestamp": "ISO STRING"
}
```

---

## ⚙️ Technologies Used

### Hardware

* ESP32
* LoRa SX1278
* DHT22 Sensor
* MQ Gas Sensors
* MPU6050 IMU
* Pulse Sensor
* Flame Sensor
* GPS Module (NEO-6M)

### Software

* Arduino IDE
* Node.js + Express
* Socket.io
* Leaflet.js
* Chart.js
* TinyML models

---

## 🧪 Working Logic

* Sensor data is continuously monitored
* Threshold-based logic triggers alerts:

  * High temperature
  * Toxic gas levels
  * Abnormal heart rate
* TinyML models detect:

  * Falls
  * Activity patterns
  * Air quality classification
* Emergency button overrides all logic and sends immediate alert

---

## 📊 Results

* Reliable detection of hazardous conditions
* Consistent alert triggering during unsafe scenarios
* Effective long-range communication using LoRa
* Real-time monitoring with minimal latency

---

## ✅ Advantages

* Low latency (edge processing)
* Long-range communication
* Multi-parameter monitoring
* Real-time alerts
* Scalable multi-node system

---

## ⚠️ Limitations

* GPS does not work indoors
* Sensor accuracy is limited (prototype level)
* Not yet industrial-grade

---

## 🌍 Applications

* Firefighter safety systems
* Disaster response monitoring
* Industrial worker safety
* Hazardous environment monitoring

---

## 🔮 Future Improvements

* Indoor positioning system (IPS)
* Cloud integration (AWS/Firebase)
* Advanced AI prediction models
* Mobile app for monitoring
* Improved sensor accuracy

---

## 👨‍💻 Authors

* **Rahul Nerella (24BEC0071)**
* **Sachith Velavan (24BEC0736)**

---

## 🏫 Institution

Department of Electronics and Communication Engineering
Vellore Institute of Technology (VIT)
Academic Year: 2025–2026

---

## 📜 License

This project is developed for academic purposes. Further enhancements and real-world deployment are encouraged.

---
