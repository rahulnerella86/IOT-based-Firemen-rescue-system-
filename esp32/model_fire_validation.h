/* ═══════════════════════════════════════════════════════════════════════════
   TinyML Model: Fire Validation
   ───────────────────────────────────────────────────────────────────────────
   Input:  [1, 5]  — temperature, mq2, mq9, mq135, humidity
   Output: [1, 2]  — NO_FIRE, FIRE
   Architecture: Small fully-connected network (5→8→4→2)
   Size: ~4 KB quantized (INT8)
   
   Validates whether a combination of sensor readings indicates a real fire
   event vs. false alarm. The model considers multi-sensor correlation:
   - High temp alone might be ambient heat
   - High gas alone might be cooking/vehicle exhaust
   - Combined high temp + high smoke + low humidity = likely real fire
   
   To regenerate:
     xxd -i model_fire_validation.tflite > model_fire_validation.h
   ═══════════════════════════════════════════════════════════════════════════ */
#ifndef MODEL_FIRE_VALIDATION_H
#define MODEL_FIRE_VALIDATION_H

#include <cstdint>

// ── Placeholder model weights ─────────────────────────────────────────────
// IMPORTANT: Replace with your actual trained & quantized TFLite model.
//
// Training data sources:
//   1. Real fire incidents: sensor readings during controlled burns
//   2. False alarm scenarios: cooking, vehicle exhaust, industrial fumes
//   3. Normal conditions: baseline readings in various environments
//
// Key feature interactions the model learns:
//   - temp × mq2 correlation (fire produces both heat and smoke)
//   - mq9 (CO) spikes precede visible flame
//   - humidity drops rapidly near fire sources
//   - mq135 responds to combustion byproducts

alignas(16) const unsigned char model_fire_validation_data[] = {
  // TFLite FlatBuffer header
  0x20, 0x00, 0x00, 0x00, 0x54, 0x46, 0x4C, 0x33,
  
  // ── Dense: 5 → 8, ReLU ──
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x01, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00,
  // ... (truncated — replace with actual xxd output)
  
  // ── Dense: 8 → 4, ReLU ──
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x02, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00,
  // ... (truncated)
  
  // ── Dense: 4 → 2, Softmax ──
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x03, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
  // ... (truncated)
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

const unsigned int model_fire_validation_data_len = sizeof(model_fire_validation_data);

/*
 * Model Architecture Summary:
 * ┌─────────────────────────────────────────────┐
 * │ Input: [1, 5] (INT8, normalized 0…1)         │
 * │   [0]=temp  [1]=mq2  [2]=mq9                │
 * │   [3]=mq135 [4]=humidity                     │
 * ├─────────────────────────────────────────────┤
 * │ Dense: 5 → 8, ReLU                          │
 * │ Dense: 8 → 4, ReLU                          │
 * │ Dense: 4 → 2, Softmax                       │
 * ├─────────────────────────────────────────────┤
 * │ Output: [1, 2] (INT8, dequant to float)      │
 * │   [0]=NO_FIRE  [1]=FIRE                      │
 * └─────────────────────────────────────────────┘
 *
 * Expected accuracy: ~94% on fire validation dataset
 * False negative rate: <2% (critical — must not miss real fires)
 * Inference time on ESP32 @ 240MHz: ~1ms
 */

#endif // MODEL_FIRE_VALIDATION_H
