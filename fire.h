#ifndef FIRE_H
#define FIRE_H

#include <stdint.h>

typedef struct {
    int D;
    uint8_t learnShift;  // e.g., 1 => η=1/2
    uint8_t bitWidth;    // 8 or 16
    int32_t *accum;      // length D
    int16_t *deltas;     // length D
} FIREState;

// For initializing FIREState
void FIRE_init(FIREState* state, int D, uint8_t learnShift, uint8_t w, int32_t* accum, int16_t* deltas);

// For predicting next sample
void FIRE_predict(FIREState* s, const int16_t* prev_sample, int16_t* out_pred);

// For training/updating FIRE state
void FIRE_train(FIREState* s, const int16_t* prev_sample, const int16_t* x, const int16_t* err);

#endif