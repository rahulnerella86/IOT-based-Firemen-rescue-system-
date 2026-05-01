/* ═══════════════════════════════════════════════════════════════════════════
   TinyML Model: Anomaly Detection (Autoencoder)
   ───────────────────────────────────────────────────────────────────────────
   Input:  [1, 8]  — temp, mq2, mq9, mq135, accel_x, accel_y, accel_z, magnitude
   Output: [1, 8]  — reconstructed input (anomaly = high reconstruction error)
   Architecture: Fully-connected autoencoder (8→4→2→4→8)
   Size: ~6 KB quantized (INT8)
   
   Anomaly detection uses an autoencoder: if the reconstruction error exceeds
   a threshold, the input is flagged as anomalous. This catches unusual
   combinations of sensor readings that don't fit any single threshold.
   
   To regenerate:
     xxd -i model_anomaly.tflite > model_anomaly.h
   ═══════════════════════════════════════════════════════════════════════════ */
#ifndef MODEL_ANOMALY_H
#define MODEL_ANOMALY_H

#include <cstdint>

// ── Placeholder model weights ─────────────────────────────────────────────
// IMPORTANT: Replace with your actual trained & quantized TFLite model.
//
// Training approach:
//   1. Train ONLY on "normal" operating data (healthy firefighter, no fire)
//   2. The autoencoder learns to reconstruct normal patterns
//   3. Anomalous readings (equipment failure, sensor drift, unusual combos)
//      produce high reconstruction error → flagged as anomaly
//   4. Threshold tuned on validation set (see ANOMALY_THRESHOLD in config.h)

alignas(16) const unsigned char model_anomaly_data[] = {
  // TFLite FlatBuffer header
  0x20, 0x00, 0x00, 0x00, 0x54, 0x46, 0x4C, 0x33,
  
  // ── Encoder ──
  // Dense: 8 → 4, ReLU
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x01, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00,
  // Dense: 4 → 2, ReLU (bottleneck)
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x02, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
  
  // ── Decoder ──
  // Dense: 2 → 4, ReLU
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x03, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00,
  // Dense: 4 → 8, Sigmoid
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x04, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00,
  // ... (truncated — replace with actual xxd output)
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

const unsigned int model_anomaly_data_len = sizeof(model_anomaly_data);

/*
 * Model Architecture Summary:
 * ┌─────────────────────────────────────────────┐
 * │ Input: [1, 8] (INT8, normalized 0…1)         │
 * ├─────────────────────────────────────────────┤
 * │ ENCODER:                                     │
 * │   Dense: 8 → 4, ReLU                        │
 * │   Dense: 4 → 2, ReLU  (latent space)        │
 * │ DECODER:                                     │
 * │   Dense: 2 → 4, ReLU                        │
 * │   Dense: 4 → 8, Sigmoid                     │
 * ├─────────────────────────────────────────────┤
 * │ Output: [1, 8] (reconstructed input)         │
 * │ Anomaly = MSE(input, output) > threshold     │
 * └─────────────────────────────────────────────┘
 *
 * Anomaly scoring:
 *   error = mean((input[i] - output[i])^2)
 *   if error > ANOMALY_THRESHOLD → anomaly = true
 *
 * Inference time on ESP32 @ 240MHz: ~2ms
 */

#endif // MODEL_ANOMALY_H
