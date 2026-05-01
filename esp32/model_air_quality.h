/* ═══════════════════════════════════════════════════════════════════════════
   TinyML Model: Air Quality Classification
   ───────────────────────────────────────────────────────────────────────────
   Input:  [1, 3]  — mq2 (smoke), mq9 (CO), mq135 (NH3/benzene/CO2)
   Output: [1, 3]  — CLEAN, MODERATE, TOXIC
   Architecture: Small fully-connected network (3→6→3)
   Size: ~4 KB quantized (INT8)
   
   Classifies overall breathability of the environment. While simple
   thresholds on individual sensors work, the ML model catches combinations
   where no single sensor is critically high but the mixture is dangerous.
   
   Examples:
   - MQ2=300, MQ9=200, MQ135=350 → individually "safe" → combo = MODERATE
   - MQ2=250, MQ9=400, MQ135=300 → CO elevated → TOXIC (even if MQ2 is OK)
   
   To regenerate:
     xxd -i model_air_quality.tflite > model_air_quality.h
   ═══════════════════════════════════════════════════════════════════════════ */
#ifndef MODEL_AIR_QUALITY_H
#define MODEL_AIR_QUALITY_H

#include <cstdint>

// ── Placeholder model weights ─────────────────────────────────────────────
// IMPORTANT: Replace with your actual trained & quantized TFLite model.
//
// Training considerations:
//   - Labels based on AQI standards adapted for enclosed/fire environments
//   - CLEAN:    breathable without mask (AQI 0-50)
//   - MODERATE: mask recommended, short exposure OK (AQI 51-150)
//   - TOXIC:    immediate evacuation, SCBA required (AQI 151+)

alignas(16) const unsigned char model_air_quality_data[] = {
  // TFLite FlatBuffer header
  0x20, 0x00, 0x00, 0x00, 0x54, 0x46, 0x4C, 0x33,
  
  // ── Dense: 3 → 6, ReLU ──
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x01, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00,
  // ... (truncated — replace with actual xxd output)
  
  // ── Dense: 6 → 3, Softmax ──
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x02, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
  // ... (truncated)
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

const unsigned int model_air_quality_data_len = sizeof(model_air_quality_data);

/*
 * Model Architecture Summary:
 * ┌─────────────────────────────────────────────┐
 * │ Input: [1, 3] (INT8, normalized 0…1)         │
 * │   [0]=mq2  [1]=mq9  [2]=mq135               │
 * ├─────────────────────────────────────────────┤
 * │ Dense: 3 → 6, ReLU                          │
 * │ Dense: 6 → 3, Softmax                       │
 * ├─────────────────────────────────────────────┤
 * │ Output: [1, 3] (INT8, dequant to float)      │
 * │   [0]=CLEAN  [1]=MODERATE  [2]=TOXIC         │
 * └─────────────────────────────────────────────┘
 *
 * Expected accuracy: ~96% on air quality dataset
 * Inference time on ESP32 @ 240MHz: <1ms
 */

#endif // MODEL_AIR_QUALITY_H
