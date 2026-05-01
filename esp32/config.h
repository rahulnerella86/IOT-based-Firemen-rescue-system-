/* ═══════════════════════════════════════════════════════════════════════════
   FIREFIGHTER MONITORING SYSTEM — ESP32 Receiver Configuration
   ═══════════════════════════════════════════════════════════════════════════ */
#ifndef CONFIG_H
#define CONFIG_H

// ── Node Identity ─────────────────────────────────────────────────────────
// The receiver doesn't have its own node ID; it forwards the transmitter's ID.

// ── WiFi Channel (ESP-NOW uses WiFi channel, no actual WiFi connection) ───
#define ESPNOW_CHANNEL          1
#define ESPNOW_ENCRYPT          false

// ── Serial Output ─────────────────────────────────────────────────────────
#define SERIAL_BAUD_RATE        115200

// ── TinyML Model Configuration ────────────────────────────────────────────

// Tensor Arena — shared memory for all model inference
// Must be large enough for the biggest model's intermediate buffers
#define TENSOR_ARENA_SIZE       (32 * 1024)  // 32 KB

// Activity Classifier
#define ACTIVITY_WINDOW_SIZE    30   // Number of IMU timesteps per inference
#define ACTIVITY_NUM_AXES       6    // ax, ay, az, gx, gy, gz
#define ACTIVITY_NUM_CLASSES    4    // STANDING, WALKING, RUNNING, CRAWLING
#define ACTIVITY_CONFIDENCE_MIN 0.55f

// Activity class labels (must match training order)
static const char* ACTIVITY_LABELS[] = {
  "STANDING",
  "WALKING",
  "RUNNING",
  "CRAWLING"
};

// Fall Detection
#define FALL_WINDOW_SIZE        20   // Shorter window for faster response
#define FALL_NUM_AXES           6
#define FALL_THRESHOLD          0.70f  // Confidence threshold for fall detection

// Anomaly Detection
#define ANOMALY_INPUT_SIZE      8    // temp, mq2, mq9, mq135, ax, ay, az, magnitude
#define ANOMALY_THRESHOLD       0.65f  // Reconstruction error threshold

// Fire Validation
#define FIRE_INPUT_SIZE         5    // temp, mq2, mq9, mq135, humidity
#define FIRE_THRESHOLD          0.60f

// Air Quality Classification
#define AIR_QUALITY_INPUT_SIZE  3    // mq2, mq9, mq135
#define AIR_QUALITY_NUM_CLASSES 3    // CLEAN, MODERATE, TOXIC

static const char* AIR_QUALITY_LABELS[] = {
  "CLEAN",
  "MODERATE",
  "TOXIC"
};

// ── Sensor Normalization Ranges ───────────────────────────────────────────
// These must match the ranges used during model training

// Temperature (°C)
#define TEMP_MIN    0.0f
#define TEMP_MAX    120.0f

// Gas Sensors (analog 0–4095 → mapped to PPM approximately)
#define GAS_MIN     0.0f
#define GAS_MAX     1000.0f

// IMU Accelerometer (g-force, ±16g typical for MPU6050)
#define ACCEL_MIN   -16.0f
#define ACCEL_MAX   16.0f

// IMU Gyroscope (°/s, ±2000 typical for MPU6050)
#define GYRO_MIN    -2000.0f
#define GYRO_MAX    2000.0f

// Humidity (%)
#define HUMIDITY_MIN 0.0f
#define HUMIDITY_MAX 100.0f

// ── Timing ────────────────────────────────────────────────────────────────
#define INFERENCE_INTERVAL_MS   500    // Run inference every 500ms
#define HEARTBEAT_INTERVAL_MS   10000  // Print heartbeat every 10s
#define DATA_TIMEOUT_MS         30000  // Mark node offline after 30s silence

// ── Status Determination Thresholds ───────────────────────────────────────
// These rule-based thresholds complement the TinyML models

#define TEMP_DANGER_THRESHOLD   55.0f   // °C
#define TEMP_WARNING_THRESHOLD  40.0f   // °C

#define MQ2_DANGER_THRESHOLD    600     // PPM (smoke)
#define MQ2_WARNING_THRESHOLD   350     // PPM

#define MQ9_DANGER_THRESHOLD    500     // PPM (CO)
#define MQ9_WARNING_THRESHOLD   250     // PPM

#define MQ135_DANGER_THRESHOLD  700     // PPM (air quality)
#define MQ135_WARNING_THRESHOLD 400     // PPM

// ── ESP-NOW Data Packet Structure ─────────────────────────────────────────
// This struct must be identical on both transmitter and receiver

typedef struct __attribute__((packed)) {
  uint8_t   node_id;        // Firefighter node identifier
  float     temperature;    // °C from DHT22
  float     humidity;       // % from DHT22
  uint16_t  mq2;            // Smoke sensor ADC/PPM
  uint16_t  mq9;            // CO sensor ADC/PPM
  uint16_t  mq135;          // Air quality sensor ADC/PPM
  float     accel_x;        // m/s² from MPU6050
  float     accel_y;
  float     accel_z;
  float     gyro_x;         // °/s from MPU6050
  float     gyro_y;
  float     gyro_z;
  float     latitude;       // From NEO-6M GPS
  float     longitude;
  uint32_t  timestamp_ms;   // millis() on transmitter
} SensorPacket;

// ── LED / Debug Indicators ────────────────────────────────────────────────
#define LED_PIN         2     // Built-in LED on most ESP32 DevKits
#define LED_BLINK_MS    50    // Quick blink on data receive

#endif // CONFIG_H
