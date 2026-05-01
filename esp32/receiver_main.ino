/* ═══════════════════════════════════════════════════════════════════════════
   FIREFIGHTER MONITORING SYSTEM — ESP32 Receiver (TinyML Processing Hub)
   ═══════════════════════════════════════════════════════════════════════════
   Receives raw sensor data from transmitter nodes via ESP-NOW, runs five
   TinyML models for real-time inference, and outputs processed JSON over
   Serial to the Node.js dashboard server.

   Upload to Arduino IDE:
     Board  → ESP32 Dev Module
     Speed  → 115200
     
   Required Libraries (install via Arduino Library Manager):
     - TensorFlowLite_ESP32
   ═══════════════════════════════════════════════════════════════════════════ */

#include <WiFi.h>
#include <esp_now.h>
#include <ArduinoJson.h>
#include "config.h"
#include "tinyml_engine.h"

// ── IMU Ring Buffers ──────────────────────────────────────────────────────
// We maintain two sliding windows for the time-series models:
//   - Activity window: 30 timesteps × 6 axes
//   - Fall window:     20 timesteps × 6 axes

static float imuActivityBuffer[ACTIVITY_WINDOW_SIZE * ACTIVITY_NUM_AXES];
static int   imuActivityIndex = 0;
static bool  imuActivityFull  = false;

static float imuFallBuffer[FALL_WINDOW_SIZE * FALL_NUM_AXES];
static int   imuFallIndex = 0;
static bool  imuFallFull  = false;

// ── Latest Received Packet ────────────────────────────────────────────────
static SensorPacket latestPacket;
static volatile bool newDataReady = false;
static unsigned long lastDataTime = 0;

// ── Timing ────────────────────────────────────────────────────────────────
static unsigned long lastInferenceTime = 0;
static unsigned long lastHeartbeatTime = 0;
static unsigned long packetCount       = 0;

// ══════════════════════════════════════════════════════════════════════════
//  ESP-NOW Receive Callback
// ══════════════════════════════════════════════════════════════════════════

void onDataReceived(const uint8_t *mac, const uint8_t *data, int len) {
  if (len != sizeof(SensorPacket)) {
    Serial.printf("⚠ Bad packet size: got %d, expected %d\n", len, sizeof(SensorPacket));
    return;
  }

  memcpy(&latestPacket, data, sizeof(SensorPacket));
  newDataReady = true;
  lastDataTime = millis();
  packetCount++;

  // Quick LED blink on receive
  digitalWrite(LED_PIN, HIGH);
}

// ══════════════════════════════════════════════════════════════════════════
//  IMU Buffer Management
// ══════════════════════════════════════════════════════════════════════════

void pushIMUSample(float ax, float ay, float az,
                   float gx, float gy, float gz) {
  // ── Activity buffer (30-sample window) ──
  int aOffset = imuActivityIndex * ACTIVITY_NUM_AXES;
  imuActivityBuffer[aOffset + 0] = ax;
  imuActivityBuffer[aOffset + 1] = ay;
  imuActivityBuffer[aOffset + 2] = az;
  imuActivityBuffer[aOffset + 3] = gx;
  imuActivityBuffer[aOffset + 4] = gy;
  imuActivityBuffer[aOffset + 5] = gz;

  imuActivityIndex++;
  if (imuActivityIndex >= ACTIVITY_WINDOW_SIZE) {
    imuActivityIndex = 0;
    imuActivityFull = true;
  }

  // ── Fall buffer (20-sample window) ──
  int fOffset = imuFallIndex * FALL_NUM_AXES;
  imuFallBuffer[fOffset + 0] = ax;
  imuFallBuffer[fOffset + 1] = ay;
  imuFallBuffer[fOffset + 2] = az;
  imuFallBuffer[fOffset + 3] = gx;
  imuFallBuffer[fOffset + 4] = gy;
  imuFallBuffer[fOffset + 5] = gz;

  imuFallIndex++;
  if (imuFallIndex >= FALL_WINDOW_SIZE) {
    imuFallIndex = 0;
    imuFallFull = true;
  }
}

// ══════════════════════════════════════════════════════════════════════════
//  Determine Overall Status
// ══════════════════════════════════════════════════════════════════════════

const char* determineOverallStatus(const SensorPacket &pkt,
                                   const InferenceResults &inf) {
  // Priority: DANGER > WARNING > SAFE
  // Check ML-based alerts first, then rule-based thresholds

  if (inf.fall.isFall) return "DANGER";
  if (inf.fire.isFire) return "DANGER";
  if (inf.anomaly.isAnomaly) return "WARNING";

  // Rule-based checks
  if (pkt.temperature >= TEMP_DANGER_THRESHOLD) return "DANGER";
  if (pkt.mq2 >= MQ2_DANGER_THRESHOLD)         return "DANGER";
  if (pkt.mq9 >= MQ9_DANGER_THRESHOLD)         return "DANGER";
  if (pkt.mq135 >= MQ135_DANGER_THRESHOLD)      return "DANGER";

  if (pkt.temperature >= TEMP_WARNING_THRESHOLD) return "WARNING";
  if (pkt.mq2 >= MQ2_WARNING_THRESHOLD)         return "WARNING";
  if (pkt.mq9 >= MQ9_WARNING_THRESHOLD)         return "WARNING";
  if (pkt.mq135 >= MQ135_WARNING_THRESHOLD)      return "WARNING";

  // Air quality ML check
  if (inf.airQuality.classIndex == 2) return "DANGER";   // TOXIC
  if (inf.airQuality.classIndex == 1) return "WARNING";  // MODERATE

  return "SAFE";
}

bool isEmergency(const char* status, const InferenceResults &inf) {
  // Emergency = fall detected OR (fire confirmed AND danger status)
  if (inf.fall.isFall) return true;
  if (inf.fire.isFire && strcmp(status, "DANGER") == 0) return true;
  return false;
}

// ══════════════════════════════════════════════════════════════════════════
//  Output JSON Over Serial
// ══════════════════════════════════════════════════════════════════════════

void outputJSON(const SensorPacket &pkt, const InferenceResults &inf) {
  const char* status = determineOverallStatus(pkt, inf);
  bool emergency = isEmergency(status, inf);

  // Build JSON using ArduinoJson for safe serialization
  StaticJsonDocument<512> doc;

  doc["id"]           = pkt.node_id;
  doc["temp"]         = serialized(String(pkt.temperature, 1));
  doc["humidity"]     = serialized(String(pkt.humidity, 1));
  doc["mq2"]          = pkt.mq2;
  doc["mq9"]          = pkt.mq9;
  doc["mq135"]        = pkt.mq135;
  doc["lat"]          = serialized(String(pkt.latitude, 6));
  doc["lon"]          = serialized(String(pkt.longitude, 6));
  doc["status"]       = status;
  doc["fall_status"]  = inf.fall.isFall ? "FALL" : "STABLE";
  doc["air_quality"]  = inf.airQuality.label;
  doc["fire_valid"]   = inf.fire.isFire;
  doc["activity"]     = inf.activity.label;
  doc["anomaly"]      = inf.anomaly.isAnomaly;
  doc["emergency"]    = emergency;

  // Serialize and print as a single line
  serializeJson(doc, Serial);
  Serial.println();
}

// ══════════════════════════════════════════════════════════════════════════
//  SETUP
// ══════════════════════════════════════════════════════════════════════════

void setup() {
  Serial.begin(SERIAL_BAUD_RATE);
  delay(500);

  Serial.println();
  Serial.println("╔══════════════════════════════════════════════════╗");
  Serial.println("║  FIREFIGHTER MONITORING — RECEIVER / TinyML HUB ║");
  Serial.println("╚══════════════════════════════════════════════════╝");
  Serial.println();

  // LED
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  // ── Initialize WiFi for ESP-NOW ────────────────────────────────────
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  Serial.printf("  MAC Address: %s\n", WiFi.macAddress().c_str());
  Serial.printf("  WiFi Channel: %d\n", ESPNOW_CHANNEL);

  // ── Initialize ESP-NOW ─────────────────────────────────────────────
  if (esp_now_init() != ESP_OK) {
    Serial.println("  ✗ ESP-NOW initialization FAILED — rebooting in 3s...");
    delay(3000);
    ESP.restart();
  }
  Serial.println("  ✓ ESP-NOW initialized");

  // Register receive callback
  esp_now_register_recv_cb(onDataReceived);
  Serial.println("  ✓ Receive callback registered");

  // ── Initialize TinyML Engine ───────────────────────────────────────
  Serial.println();
  if (!tinyml_init()) {
    Serial.println("  ✗ TinyML initialization FAILED — running without ML");
    Serial.println("    (sensor data will still be forwarded with default labels)");
  } else {
    Serial.println("  ✓ All TinyML models ready");
  }

  // ── Clear buffers ──────────────────────────────────────────────────
  memset(imuActivityBuffer, 0, sizeof(imuActivityBuffer));
  memset(imuFallBuffer, 0, sizeof(imuFallBuffer));

  Serial.println();
  Serial.println("  Waiting for incoming sensor data...");
  Serial.println("  ─────────────────────────────────────────────────");
  Serial.println();
}

// ══════════════════════════════════════════════════════════════════════════
//  LOOP
// ══════════════════════════════════════════════════════════════════════════

void loop() {
  unsigned long now = millis();

  // ── Turn off LED after brief blink ──────────────────────────────────
  if (digitalRead(LED_PIN) == HIGH && (now - lastDataTime > LED_BLINK_MS)) {
    digitalWrite(LED_PIN, LOW);
  }

  // ── Process new data when available ─────────────────────────────────
  if (newDataReady) {
    newDataReady = false;

    // Push IMU sample into sliding windows
    pushIMUSample(
      latestPacket.accel_x, latestPacket.accel_y, latestPacket.accel_z,
      latestPacket.gyro_x,  latestPacket.gyro_y,  latestPacket.gyro_z
    );
  }

  // ── Run inference at fixed interval ─────────────────────────────────
  if (now - lastInferenceTime >= INFERENCE_INTERVAL_MS) {
    lastInferenceTime = now;

    // Only run if we have received at least one packet
    if (lastDataTime == 0) return;

    // Check for data timeout (node went offline)
    if (now - lastDataTime > DATA_TIMEOUT_MS) {
      Serial.printf("{\"id\":%d,\"status\":\"OFFLINE\",\"emergency\":false}\n",
                    latestPacket.node_id);
      return;
    }

    // ── Prepare feature arrays for each model ──────────────────────

    // Anomaly features: {temp, mq2, mq9, mq135, ax, ay, az, magnitude}
    float mag = sqrtf(
      latestPacket.accel_x * latestPacket.accel_x +
      latestPacket.accel_y * latestPacket.accel_y +
      latestPacket.accel_z * latestPacket.accel_z
    );
    float sensorFeatures[ANOMALY_INPUT_SIZE] = {
      latestPacket.temperature,
      (float)latestPacket.mq2,
      (float)latestPacket.mq9,
      (float)latestPacket.mq135,
      latestPacket.accel_x,
      latestPacket.accel_y,
      latestPacket.accel_z,
      mag
    };

    // Fire features: {temp, mq2, mq9, mq135, humidity}
    float fireFeatures[FIRE_INPUT_SIZE] = {
      latestPacket.temperature,
      (float)latestPacket.mq2,
      (float)latestPacket.mq9,
      (float)latestPacket.mq135,
      latestPacket.humidity
    };

    // Air quality features: {mq2, mq9, mq135}
    float gasFeatures[AIR_QUALITY_INPUT_SIZE] = {
      (float)latestPacket.mq2,
      (float)latestPacket.mq9,
      (float)latestPacket.mq135
    };

    // ── Run all models ─────────────────────────────────────────────
    InferenceResults results;

    if (imuActivityFull && imuFallFull) {
      // All buffers ready — run complete inference pipeline
      results = tinyml_run_all(
        imuActivityBuffer,
        imuFallBuffer,
        sensorFeatures,
        fireFeatures,
        gasFeatures
      );
    } else {
      // IMU buffers still filling — only run non-time-series models
      results.activity   = {0, 0.0f, "STANDING"};
      results.fall       = {false, 0.0f};
      results.anomaly    = tinyml_detect_anomaly(sensorFeatures);
      results.fire       = tinyml_validate_fire(fireFeatures);
      results.airQuality = tinyml_classify_air_quality(gasFeatures);
      results.inferenceTimeMs = 0;
    }

    // ── Output processed JSON to Serial ────────────────────────────
    outputJSON(latestPacket, results);
  }

  // ── Periodic heartbeat / stats ──────────────────────────────────────
  if (now - lastHeartbeatTime >= HEARTBEAT_INTERVAL_MS) {
    lastHeartbeatTime = now;
    Serial.printf("/* Heartbeat: %lu packets received, uptime %lus, free heap %u bytes */\n",
                  packetCount, now / 1000, ESP.getFreeHeap());
  }
}
