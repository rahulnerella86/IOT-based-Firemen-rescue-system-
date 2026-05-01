/* ═══════════════════════════════════════════════════════════════════════════
   TinyML Inference Engine — Header
   ═══════════════════════════════════════════════════════════════════════════
   Unified wrapper around TensorFlow Lite Micro for running all five
   TinyML models on the ESP32 receiver:
     1. Activity Classifier       [1,30,6] → [1,4]
     2. Fall Detector              [1,20,6] → [1,2]
     3. Anomaly Detector (AE)      [1,8]   → [1,8]
     4. Fire Validator             [1,5]   → [1,2]
     5. Air Quality Classifier     [1,3]   → [1,3]
   ═══════════════════════════════════════════════════════════════════════════ */
#ifndef TINYML_ENGINE_H
#define TINYML_ENGINE_H

#include <Arduino.h>
#include "config.h"

// ── Result Structures ─────────────────────────────────────────────────────

struct ActivityResult {
  int    classIndex;         // 0=STANDING, 1=WALKING, 2=RUNNING, 3=CRAWLING
  float  confidence;         // 0.0 – 1.0
  const char* label;         // Human-readable label
};

struct FallResult {
  bool   isFall;             // true if fall detected
  float  confidence;         // Confidence of the FALL class
};

struct AnomalyResult {
  bool   isAnomaly;          // true if reconstruction error > threshold
  float  reconstructionError;// Mean Squared Error between input and output
};

struct FireResult {
  bool   isFire;             // true if fire event validated
  float  confidence;         // Confidence of the FIRE class
};

struct AirQualityResult {
  int    classIndex;         // 0=CLEAN, 1=MODERATE, 2=TOXIC
  float  confidence;         // Confidence of the winning class
  const char* label;         // Human-readable label
};

// ── Combined Inference Output ─────────────────────────────────────────────
struct InferenceResults {
  ActivityResult   activity;
  FallResult       fall;
  AnomalyResult    anomaly;
  FireResult       fire;
  AirQualityResult airQuality;
  unsigned long    inferenceTimeMs;  // Total time for all models
};

// ── Public API ────────────────────────────────────────────────────────────

/**
 * Initialize all TinyML models. Call once in setup().
 * Returns true if all models loaded successfully.
 */
bool tinyml_init();

/**
 * Run activity classification on a window of IMU data.
 * @param imuBuffer  Pointer to float array [ACTIVITY_WINDOW_SIZE][6]
 *                   Each row: {ax, ay, az, gx, gy, gz}
 */
ActivityResult tinyml_classify_activity(const float* imuBuffer);

/**
 * Run fall detection on a window of IMU data.
 * @param imuBuffer  Pointer to float array [FALL_WINDOW_SIZE][6]
 */
FallResult tinyml_detect_fall(const float* imuBuffer);

/**
 * Run anomaly detection on sensor features.
 * @param features   Array of 8 floats: {temp, mq2, mq9, mq135, ax, ay, az, magnitude}
 */
AnomalyResult tinyml_detect_anomaly(const float* features);

/**
 * Run fire validation on environmental sensors.
 * @param features   Array of 5 floats: {temp, mq2, mq9, mq135, humidity}
 */
FireResult tinyml_validate_fire(const float* features);

/**
 * Run air quality classification on gas sensors.
 * @param features   Array of 3 floats: {mq2, mq9, mq135}
 */
AirQualityResult tinyml_classify_air_quality(const float* features);

/**
 * Run ALL models and return combined results.
 * This is the main entry point called from the receiver loop.
 *
 * @param imuActivityBuf  Float array [ACTIVITY_WINDOW_SIZE * 6] for activity
 * @param imuFallBuf      Float array [FALL_WINDOW_SIZE * 6] for fall detection
 * @param sensorFeatures  Float array [8] for anomaly detection
 * @param fireFeatures    Float array [5] for fire validation
 * @param gasFeatures     Float array [3] for air quality
 */
InferenceResults tinyml_run_all(
  const float* imuActivityBuf,
  const float* imuFallBuf,
  const float* sensorFeatures,
  const float* fireFeatures,
  const float* gasFeatures
);

/**
 * Normalize a raw sensor value to 0.0–1.0 range.
 */
inline float normalize(float value, float minVal, float maxVal) {
  float norm = (value - minVal) / (maxVal - minVal);
  return constrain(norm, 0.0f, 1.0f);
}

/**
 * Convert a normalized float (0.0–1.0) to INT8 (-128 to 127).
 */
inline int8_t float_to_int8(float value) {
  return (int8_t)(value * 255.0f - 128.0f);
}

#endif // TINYML_ENGINE_H
