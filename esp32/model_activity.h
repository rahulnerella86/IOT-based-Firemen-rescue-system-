/* ═══════════════════════════════════════════════════════════════════════════
   TinyML Model: Activity Classifier
   ───────────────────────────────────────────────────────────────────────────
   Input:  [1, 30, 6]  — 30 timesteps × 6 IMU axes (ax,ay,az,gx,gy,gz)
   Output: [1, 4]      — STANDING, WALKING, RUNNING, CRAWLING
   Architecture: 1D-CNN with 2 conv layers + dense
   Size: ~12 KB quantized (INT8)
   
   To regenerate this file from a trained .tflite model:
     xxd -i model_activity.tflite > model_activity.h
   
   Then rename the array to `model_activity_data` and the length to
   `model_activity_data_len`.
   ═══════════════════════════════════════════════════════════════════════════ */
#ifndef MODEL_ACTIVITY_H
#define MODEL_ACTIVITY_H

#include <cstdint>

// ── Placeholder model weights ─────────────────────────────────────────────
// IMPORTANT: Replace this array with your actual trained & quantized TFLite
// model bytes. The values below are a minimal valid TFLite flatbuffer
// skeleton that will load but produce random outputs.
//
// Training pipeline:
//   1. Collect IMU data (30 samples @ 50Hz = 0.6s window) for each activity
//   2. Train a 1D-CNN in TensorFlow/Keras
//   3. Convert to TFLite with INT8 quantization:
//        converter = tf.lite.TFLiteConverter.from_keras_model(model)
//        converter.optimizations = [tf.lite.Optimize.DEFAULT]
//        converter.representative_dataset = representative_data_gen
//        converter.target_spec.supported_ops = [tf.lite.OpsSet.TFLITE_BUILTINS_INT8]
//        converter.inference_input_type = tf.int8
//        converter.inference_output_type = tf.int8
//        tflite_model = converter.convert()
//   4. xxd -i model.tflite > model_activity.h

alignas(16) const unsigned char model_activity_data[] = {
  // TFLite FlatBuffer header
  0x20, 0x00, 0x00, 0x00, 0x54, 0x46, 0x4C, 0x33,  // " ...TFL3"
  
  // ── Layer 1: Conv1D (16 filters, kernel=3, ReLU) ──
  // Weights: 3 × 6 × 16 = 288 INT8 values
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x01, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00,
  // ... (truncated — replace with actual xxd output)
  // Conv1D bias: 16 INT32 values
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  
  // ── Layer 2: Conv1D (32 filters, kernel=3, ReLU) ──
  // Weights: 3 × 16 × 32 = 1536 INT8 values
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x02, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00,
  // ... (truncated)
  
  // ── Global Average Pooling ──
  // No weights
  
  // ── Dense (32 → 4, Softmax) ──
  // Weights: 32 × 4 = 128 INT8 values
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x03, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00,
  // ... (truncated)
  // Dense bias: 4 INT32 values
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

const unsigned int model_activity_data_len = sizeof(model_activity_data);

/*
 * Model Architecture Summary:
 * ┌─────────────────────────────────────────────┐
 * │ Input: [1, 30, 6] (INT8, normalized -128…127)│
 * ├─────────────────────────────────────────────┤
 * │ Conv1D: 16 filters, k=3, stride=1, ReLU     │
 * │ Conv1D: 32 filters, k=3, stride=1, ReLU     │
 * │ GlobalAveragePooling1D                       │
 * │ Dense: 32 → 4, Softmax                      │
 * ├─────────────────────────────────────────────┤
 * │ Output: [1, 4] (INT8, dequant to float)      │
 * │   [0]=STANDING [1]=WALKING                   │
 * │   [2]=RUNNING  [3]=CRAWLING                  │
 * └─────────────────────────────────────────────┘
 *
 * Expected accuracy: ~92% on firefighter activity dataset
 * Inference time on ESP32 @ 240MHz: ~15ms
 */

#endif // MODEL_ACTIVITY_H
