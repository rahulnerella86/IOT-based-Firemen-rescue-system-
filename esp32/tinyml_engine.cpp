/* ═══════════════════════════════════════════════════════════════════════════
   TinyML Inference Engine — Implementation
   ═══════════════════════════════════════════════════════════════════════════
   Uses TensorFlow Lite Micro to run all five models. Each model gets its
   own interpreter sharing a common tensor arena.
   ═══════════════════════════════════════════════════════════════════════════ */

#include "tinyml_engine.h"
#include "config.h"

// ── TFLite Micro Includes ─────────────────────────────────────────────────
#include <TensorFlowLite_ESP32.h>
#include "tensorflow/lite/micro/all_ops_resolver.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/schema/schema_generated.h"

// ── Model Data Headers ────────────────────────────────────────────────────
#include "model_activity.h"
#include "model_fall_detection.h"
#include "model_anomaly.h"
#include "model_fire_validation.h"
#include "model_air_quality.h"

// ── Tensor Arena (shared across sequential inference calls) ───────────────
static uint8_t tensor_arena[TENSOR_ARENA_SIZE] __attribute__((aligned(16)));

// ── Model Pointers ────────────────────────────────────────────────────────
static const tflite::Model* mdl_activity    = nullptr;
static const tflite::Model* mdl_fall        = nullptr;
static const tflite::Model* mdl_anomaly     = nullptr;
static const tflite::Model* mdl_fire        = nullptr;
static const tflite::Model* mdl_air_quality = nullptr;

// ── Op Resolver (includes all built-in ops) ───────────────────────────────
static tflite::AllOpsResolver resolver;

// ══════════════════════════════════════════════════════════════════════════
//  Initialization
// ══════════════════════════════════════════════════════════════════════════

bool tinyml_init() {
  Serial.println("  Loading TinyML models...");

  // Verify TFLite FlatBuffer headers
  mdl_activity = tflite::GetModel(model_activity_data);
  if (mdl_activity->version() != TFLITE_SCHEMA_VERSION) {
    Serial.printf("  ✗ Activity model version mismatch: got %d, expected %d\n",
                  mdl_activity->version(), TFLITE_SCHEMA_VERSION);
    return false;
  }
  Serial.printf("  ✓ Activity model loaded (%u bytes)\n", model_activity_data_len);

  mdl_fall = tflite::GetModel(model_fall_detection_data);
  if (mdl_fall->version() != TFLITE_SCHEMA_VERSION) {
    Serial.println("  ✗ Fall detection model version mismatch");
    return false;
  }
  Serial.printf("  ✓ Fall detection model loaded (%u bytes)\n", model_fall_detection_data_len);

  mdl_anomaly = tflite::GetModel(model_anomaly_data);
  if (mdl_anomaly->version() != TFLITE_SCHEMA_VERSION) {
    Serial.println("  ✗ Anomaly model version mismatch");
    return false;
  }
  Serial.printf("  ✓ Anomaly model loaded (%u bytes)\n", model_anomaly_data_len);

  mdl_fire = tflite::GetModel(model_fire_validation_data);
  if (mdl_fire->version() != TFLITE_SCHEMA_VERSION) {
    Serial.println("  ✗ Fire validation model version mismatch");
    return false;
  }
  Serial.printf("  ✓ Fire validation model loaded (%u bytes)\n", model_fire_validation_data_len);

  mdl_air_quality = tflite::GetModel(model_air_quality_data);
  if (mdl_air_quality->version() != TFLITE_SCHEMA_VERSION) {
    Serial.println("  ✗ Air quality model version mismatch");
    return false;
  }
  Serial.printf("  ✓ Air quality model loaded (%u bytes)\n", model_air_quality_data_len);

  Serial.printf("  ✓ Tensor arena: %d bytes allocated\n", TENSOR_ARENA_SIZE);
  return true;
}

// ══════════════════════════════════════════════════════════════════════════
//  Helper: Run a generic model and return the interpreter output tensor
// ══════════════════════════════════════════════════════════════════════════

/**
 * Creates a temporary interpreter, feeds input, runs inference.
 * Caller reads output from the returned interpreter's output tensor.
 * Returns nullptr on failure.
 */
static tflite::MicroInterpreter* runModel(
  const tflite::Model* model,
  const int8_t* inputData,
  size_t inputSize
) {
  // Create interpreter (lives on the stack, re-uses the shared arena)
  static tflite::MicroInterpreter* interpreter = nullptr;

  interpreter = new tflite::MicroInterpreter(model, resolver, tensor_arena, TENSOR_ARENA_SIZE);

  if (interpreter->AllocateTensors() != kTfLiteOk) {
    Serial.println("  ✗ AllocateTensors failed");
    delete interpreter;
    return nullptr;
  }

  // Copy input data into the model's input tensor
  TfLiteTensor* input = interpreter->input(0);
  memcpy(input->data.int8, inputData, inputSize);

  // Run inference
  if (interpreter->Invoke() != kTfLiteOk) {
    Serial.println("  ✗ Invoke failed");
    delete interpreter;
    return nullptr;
  }

  return interpreter;
}

// ══════════════════════════════════════════════════════════════════════════
//  Activity Classifier
// ══════════════════════════════════════════════════════════════════════════

ActivityResult tinyml_classify_activity(const float* imuBuffer) {
  ActivityResult result = {0, 0.0f, "STANDING"};

  // Normalize and quantize: float → INT8
  const int totalInputs = ACTIVITY_WINDOW_SIZE * ACTIVITY_NUM_AXES;
  int8_t quantized[totalInputs];

  for (int i = 0; i < ACTIVITY_WINDOW_SIZE; i++) {
    for (int j = 0; j < ACTIVITY_NUM_AXES; j++) {
      int idx = i * ACTIVITY_NUM_AXES + j;
      float raw = imuBuffer[idx];
      float norm;
      if (j < 3) {
        // Accelerometer axes (0,1,2)
        norm = normalize(raw, ACCEL_MIN, ACCEL_MAX);
      } else {
        // Gyroscope axes (3,4,5)
        norm = normalize(raw, GYRO_MIN, GYRO_MAX);
      }
      quantized[idx] = float_to_int8(norm);
    }
  }

  // Run inference
  tflite::MicroInterpreter* interpreter = runModel(mdl_activity, quantized, sizeof(quantized));
  if (!interpreter) return result;

  // Read output: [1, 4] INT8 softmax
  TfLiteTensor* output = interpreter->output(0);
  float scores[ACTIVITY_NUM_CLASSES];

  // Dequantize output
  float scale = output->params.scale;
  int   zp    = output->params.zero_point;

  float maxScore = -1e9f;
  for (int i = 0; i < ACTIVITY_NUM_CLASSES; i++) {
    scores[i] = (output->data.int8[i] - zp) * scale;
    if (scores[i] > maxScore) {
      maxScore = scores[i];
      result.classIndex = i;
    }
  }

  result.confidence = maxScore;
  result.label = ACTIVITY_LABELS[result.classIndex];

  delete interpreter;
  return result;
}

// ══════════════════════════════════════════════════════════════════════════
//  Fall Detector
// ══════════════════════════════════════════════════════════════════════════

FallResult tinyml_detect_fall(const float* imuBuffer) {
  FallResult result = {false, 0.0f};

  const int totalInputs = FALL_WINDOW_SIZE * FALL_NUM_AXES;
  int8_t quantized[totalInputs];

  for (int i = 0; i < FALL_WINDOW_SIZE; i++) {
    for (int j = 0; j < FALL_NUM_AXES; j++) {
      int idx = i * FALL_NUM_AXES + j;
      float raw = imuBuffer[idx];
      float norm = (j < 3)
        ? normalize(raw, ACCEL_MIN, ACCEL_MAX)
        : normalize(raw, GYRO_MIN, GYRO_MAX);
      quantized[idx] = float_to_int8(norm);
    }
  }

  tflite::MicroInterpreter* interpreter = runModel(mdl_fall, quantized, sizeof(quantized));
  if (!interpreter) return result;

  TfLiteTensor* output = interpreter->output(0);
  float scale = output->params.scale;
  int   zp    = output->params.zero_point;

  // Output: [0]=STABLE, [1]=FALL
  float fallScore = (output->data.int8[1] - zp) * scale;
  result.confidence = fallScore;
  result.isFall = (fallScore >= FALL_THRESHOLD);

  delete interpreter;
  return result;
}

// ══════════════════════════════════════════════════════════════════════════
//  Anomaly Detector (Autoencoder)
// ══════════════════════════════════════════════════════════════════════════

AnomalyResult tinyml_detect_anomaly(const float* features) {
  AnomalyResult result = {false, 0.0f};

  int8_t quantized[ANOMALY_INPUT_SIZE];

  // Normalize each feature to 0–1 range
  // features[]: {temp, mq2, mq9, mq135, ax, ay, az, magnitude}
  float normRanges[][2] = {
    {TEMP_MIN, TEMP_MAX},       // temp
    {GAS_MIN, GAS_MAX},         // mq2
    {GAS_MIN, GAS_MAX},         // mq9
    {GAS_MIN, GAS_MAX},         // mq135
    {ACCEL_MIN, ACCEL_MAX},     // ax
    {ACCEL_MIN, ACCEL_MAX},     // ay
    {ACCEL_MIN, ACCEL_MAX},     // az
    {0.0f, 32.0f}               // magnitude (sqrt(ax²+ay²+az²), max ~27g)
  };

  float normalizedInput[ANOMALY_INPUT_SIZE];
  for (int i = 0; i < ANOMALY_INPUT_SIZE; i++) {
    normalizedInput[i] = normalize(features[i], normRanges[i][0], normRanges[i][1]);
    quantized[i] = float_to_int8(normalizedInput[i]);
  }

  tflite::MicroInterpreter* interpreter = runModel(mdl_anomaly, quantized, sizeof(quantized));
  if (!interpreter) return result;

  // Autoencoder output: reconstructed input [1, 8]
  TfLiteTensor* output = interpreter->output(0);
  float scale = output->params.scale;
  int   zp    = output->params.zero_point;

  // Calculate Mean Squared Error between input and reconstruction
  float mse = 0.0f;
  for (int i = 0; i < ANOMALY_INPUT_SIZE; i++) {
    float reconstructed = (output->data.int8[i] - zp) * scale;
    float diff = normalizedInput[i] - reconstructed;
    mse += diff * diff;
  }
  mse /= ANOMALY_INPUT_SIZE;

  result.reconstructionError = mse;
  result.isAnomaly = (mse >= ANOMALY_THRESHOLD);

  delete interpreter;
  return result;
}

// ══════════════════════════════════════════════════════════════════════════
//  Fire Validator
// ══════════════════════════════════════════════════════════════════════════

FireResult tinyml_validate_fire(const float* features) {
  FireResult result = {false, 0.0f};

  int8_t quantized[FIRE_INPUT_SIZE];

  // features[]: {temp, mq2, mq9, mq135, humidity}
  float normRanges[][2] = {
    {TEMP_MIN, TEMP_MAX},
    {GAS_MIN, GAS_MAX},
    {GAS_MIN, GAS_MAX},
    {GAS_MIN, GAS_MAX},
    {HUMIDITY_MIN, HUMIDITY_MAX}
  };

  for (int i = 0; i < FIRE_INPUT_SIZE; i++) {
    float norm = normalize(features[i], normRanges[i][0], normRanges[i][1]);
    quantized[i] = float_to_int8(norm);
  }

  tflite::MicroInterpreter* interpreter = runModel(mdl_fire, quantized, sizeof(quantized));
  if (!interpreter) return result;

  TfLiteTensor* output = interpreter->output(0);
  float scale = output->params.scale;
  int   zp    = output->params.zero_point;

  // Output: [0]=NO_FIRE, [1]=FIRE
  float fireScore = (output->data.int8[1] - zp) * scale;
  result.confidence = fireScore;
  result.isFire = (fireScore >= FIRE_THRESHOLD);

  delete interpreter;
  return result;
}

// ══════════════════════════════════════════════════════════════════════════
//  Air Quality Classifier
// ══════════════════════════════════════════════════════════════════════════

AirQualityResult tinyml_classify_air_quality(const float* features) {
  AirQualityResult result = {0, 0.0f, "CLEAN"};

  int8_t quantized[AIR_QUALITY_INPUT_SIZE];

  // features[]: {mq2, mq9, mq135}
  for (int i = 0; i < AIR_QUALITY_INPUT_SIZE; i++) {
    float norm = normalize(features[i], GAS_MIN, GAS_MAX);
    quantized[i] = float_to_int8(norm);
  }

  tflite::MicroInterpreter* interpreter = runModel(mdl_air_quality, quantized, sizeof(quantized));
  if (!interpreter) return result;

  TfLiteTensor* output = interpreter->output(0);
  float scale = output->params.scale;
  int   zp    = output->params.zero_point;

  float maxScore = -1e9f;
  for (int i = 0; i < AIR_QUALITY_NUM_CLASSES; i++) {
    float score = (output->data.int8[i] - zp) * scale;
    if (score > maxScore) {
      maxScore = score;
      result.classIndex = i;
    }
  }

  result.confidence = maxScore;
  result.label = AIR_QUALITY_LABELS[result.classIndex];

  delete interpreter;
  return result;
}

// ══════════════════════════════════════════════════════════════════════════
//  Run All Models
// ══════════════════════════════════════════════════════════════════════════

InferenceResults tinyml_run_all(
  const float* imuActivityBuf,
  const float* imuFallBuf,
  const float* sensorFeatures,
  const float* fireFeatures,
  const float* gasFeatures
) {
  InferenceResults results;
  unsigned long startTime = millis();

  results.activity   = tinyml_classify_activity(imuActivityBuf);
  results.fall       = tinyml_detect_fall(imuFallBuf);
  results.anomaly    = tinyml_detect_anomaly(sensorFeatures);
  results.fire       = tinyml_validate_fire(fireFeatures);
  results.airQuality = tinyml_classify_air_quality(gasFeatures);

  results.inferenceTimeMs = millis() - startTime;

  return results;
}
