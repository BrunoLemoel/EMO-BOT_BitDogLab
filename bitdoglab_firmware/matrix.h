#ifndef MATRIX_H
#define MATRIX_H

#include <stdint.h>
#include <stdbool.h>

#define MATRIX_PIN       7
#define MATRIX_NUM_LEDS  25

// Initialization
void matrix_init(void);

// Pixel control
void matrix_clear(void);
void matrix_show(void);
void matrix_set_pixel(int row, int col, uint8_t r, uint8_t g, uint8_t b);
void matrix_set_all(uint8_t r, uint8_t g, uint8_t b);

// Emotion animations on the 5x5 matrix
void matrix_set_expression(const char *expr);

// Standby / Idle Light Show (5 distinct animation styles)
void matrix_effect_wave(uint32_t step);            // Style 0: Rainbow chromatic wave
void matrix_effect_fireflies(uint32_t step);       // Style 1: Sparkles / fireflies with smooth fade
void matrix_effect_spiral(uint32_t step);          // Style 2: Rotating spiral orbit
void matrix_effect_equalizer(uint32_t step);       // Style 3: Bouncing audio equalizer bars
void matrix_effect_breathing_heart(uint32_t step); // Style 4: Breathing pulse heart
void matrix_play_idle_style(int style, uint32_t step);

#endif // MATRIX_H
