/* ═══════════════════════════════════════════════════════════════════════════
   TinyML Model: Fall Detection
   ───────────────────────────────────────────────────────────────────────────
   Input:  [1, 20, 6]  — 20 timesteps × 6 IMU axes (ax,ay,az,gx,gy,gz)
   Output: [1, 2]      — STABLE, FALL
   Architecture: 1D-CNN optimized for low-latency fall detection
   Size: ~8 KB quantized (INT8)
   
   Fall detection requires a shorter window (20 samples @ 50Hz = 0.4s) for
   faster response time — critical for firefighter safety.
   
   To regenerate:
     xxd -i model_fall_detection.tflite > model_fall_detection.h
   ═══════════════════════════════════════════════════════════════════════════ */
#ifndef MODEL_FALL_DETECTION_H
#define MODEL_FALL_DETECTION_H

#include <cstdint>

// ── Placeholder model weights ─────────────────────────────────────────────
// IMPORTANT: Replace with your actual trained & quantized TFLite model.
//
// Training approach:
//   1. Dataset: Simulated falls + normal activities from MPU6050
//   2. Features: Raw accel (ax,ay,az) + gyro (gx,gy,gz) at 50Hz
//   3. Window: 20 timesteps (0.4 seconds) with 50% overlap
//   4. Key features the model learns:
//      - Sudden acceleration spike (free-fall → impact)
//      - Post-impact stillness
//      - Orientation change (gyro integration)
//   5. Architecture: Lightweight 1D-CNN for real-time inference

alignas(16) const unsigned char model_fall_detection_data[] = {
  // TFLite FlatBuffer header
  0x20, 0x00, 0x00, 0x00, 0x54, 0x46, 0x4C, 0x33,
  
  // ── Layer 1: Conv1D (8 filters, kernel=5, ReLU) ──
  // Wider kernel to capture the fall impact signature
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x01, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00,
  // ... (truncated — replace with actual xxd output)
  
  // ── Layer 2: Conv1D (16 filters, kernel=3, ReLU) ──
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x02, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00,
  // ... (truncated)
  
  // ── MaxPooling1D (pool=2) ──
  // No weights
  
  // ── Dense (16*4 → 2, Softmax) ──
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x03, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
  // ... (truncated)
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

const unsigned int model_fall_detection_data_len = sizeof(model_fall_detection_data);

/*
 * Model Architecture Summary:
 * ┌─────────────────────────────────────────────┐
 * │ Input: [1, 20, 6] (INT8, normalized)         │
 * ├─────────────────────────────────────────────┤
 * │ Conv1D: 8 filters, k=5, stride=1, ReLU      │
 * │ Conv1D: 16 filters, k=3, stride=1, ReLU     │
 * │ MaxPooling1D: pool_size=2                    │
 * │ Flatten                                      │
 * │ Dense: → 2, Softmax                          │
 * ├─────────────────────────────────────────────┤
 * │ Output: [1, 2] (INT8, dequant to float)      │
 * │   [0]=STABLE  [1]=FALL                       │
 * └─────────────────────────────────────────────┘
 *
 * Expected accuracy: ~95% (fall detection is safety-critical)
 * False positive rate: <3% (important to avoid false alarms)
 * Inference time on ESP32 @ 240MHz: ~8ms
 */

#endif // MODEL_FALL_DETECTION_H
