#include "matrix.h"
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "ws2812.pio.h"

static PIO pio = pio0;
static uint sm = 0;
static uint32_t led_buffer[MATRIX_NUM_LEDS];

// BitDogLab 5x5 Serpentine Matrix Mapping: [Row][Col] -> Linear Index (0..24)
static const int LED_MATRIX[5][5] = {
    {24, 23, 22, 21, 20},  // Row 0 (Top)
    {15, 16, 17, 18, 19},  // Row 1
    {14, 13, 12, 11, 10},  // Row 2
    { 5,  6,  7,  8,  9},  // Row 3
    { 0,  1,  2,  3,  4}   // Row 4 (Bottom)
};

static inline uint32_t urgb_u32(uint8_t r, uint8_t g, uint8_t b) {
    return ((uint32_t)(g) << 16) |
           ((uint32_t)(r) << 8)  |
           (uint32_t)(b);
}

void matrix_init(void) {
    uint offset = pio_add_program(pio, &ws2812_program);
    ws2812_program_init(pio, sm, offset, MATRIX_PIN, 800000, false);
    matrix_clear();
    matrix_show();
}

void matrix_clear(void) {
    memset(led_buffer, 0, sizeof(led_buffer));
}

void matrix_show(void) {
    for (int i = 0; i < MATRIX_NUM_LEDS; i++) {
        pio_sm_put_blocking(pio, sm, led_buffer[i] << 8u);
    }
    sleep_us(50); // Latch pulse
}

void matrix_set_pixel(int row, int col, uint8_t r, uint8_t g, uint8_t b) {
    if (row < 0 || row >= 5 || col < 0 || col >= 5) return;
    int idx = LED_MATRIX[row][col];
    if (idx >= 0 && idx < MATRIX_NUM_LEDS) {
        led_buffer[idx] = urgb_u32(r, g, b);
    }
}

void matrix_set_all(uint8_t r, uint8_t g, uint8_t b) {
    uint32_t c = urgb_u32(r, g, b);
    for (int i = 0; i < MATRIX_NUM_LEDS; i++) {
        led_buffer[i] = c;
    }
    matrix_show();
}

static void draw_pattern(const uint8_t pat[5][5], uint8_t r, uint8_t g, uint8_t b) {
    matrix_clear();
    for (int row = 0; row < 5; row++) {
        for (int col = 0; col < 5; col++) {
            if (pat[row][col]) {
                matrix_set_pixel(row, col, r, g, b);
            }
        }
    }
    matrix_show();
}

void matrix_set_expression(const char *expr) {
    if (strcmp(expr, "happy") == 0) {
        // Smiling mouth (green/yellow)
        const uint8_t pat[5][5] = {
            {0, 0, 0, 0, 0},
            {0, 0, 0, 0, 0},
            {1, 0, 0, 0, 1},
            {0, 1, 1, 1, 0},
            {0, 0, 0, 0, 0}
        };
        draw_pattern(pat, 40, 60, 0);

    } else if (strcmp(expr, "sad") == 0) {
        // Frowning mouth (blue)
        const uint8_t pat[5][5] = {
            {0, 0, 0, 0, 0},
            {0, 0, 0, 0, 0},
            {0, 1, 1, 1, 0},
            {1, 0, 0, 0, 1},
            {0, 0, 0, 0, 0}
        };
        draw_pattern(pat, 0, 20, 80);

    } else if (strcmp(expr, "angry") == 0) {
        // Jagged mouth (red)
        const uint8_t pat[5][5] = {
            {0, 0, 0, 0, 0},
            {0, 0, 0, 0, 0},
            {1, 0, 1, 0, 1},
            {0, 1, 0, 1, 0},
            {0, 0, 0, 0, 0}
        };
        draw_pattern(pat, 90, 0, 0);

    } else if (strcmp(expr, "love") == 0) {
        // 5x5 Full Heart (magenta/red)
        const uint8_t pat[5][5] = {
            {0, 1, 0, 1, 0},
            {1, 1, 1, 1, 1},
            {1, 1, 1, 1, 1},
            {0, 1, 1, 1, 0},
            {0, 0, 1, 0, 0}
        };
        draw_pattern(pat, 80, 0, 40);

    } else if (strcmp(expr, "surprised") == 0) {
        // Open 'O' mouth (orange)
        const uint8_t pat[5][5] = {
            {0, 0, 0, 0, 0},
            {0, 1, 1, 1, 0},
            {0, 1, 0, 1, 0},
            {0, 1, 1, 1, 0},
            {0, 0, 0, 0, 0}
        };
        draw_pattern(pat, 70, 35, 0);

    } else if (strcmp(expr, "wink") == 0) {
        // Playful smirk
        const uint8_t pat[5][5] = {
            {0, 0, 0, 0, 0},
            {0, 0, 0, 0, 0},
            {0, 0, 0, 1, 1},
            {0, 1, 1, 0, 0},
            {0, 0, 0, 0, 0}
        };
        draw_pattern(pat, 50, 50, 0);

    } else if (strcmp(expr, "sleepy") == 0) {
        // "Z" pattern (soft cyan)
        const uint8_t pat[5][5] = {
            {0, 1, 1, 1, 0},
            {0, 0, 0, 1, 0},
            {0, 0, 1, 0, 0},
            {0, 1, 0, 0, 0},
            {0, 1, 1, 1, 0}
        };
        draw_pattern(pat, 0, 30, 40);

    } else if (strcmp(expr, "confused") == 0) {
        // Question mark "?" (purple)
        const uint8_t pat[5][5] = {
            {0, 1, 1, 0, 0},
            {0, 0, 0, 1, 0},
            {0, 0, 1, 0, 0},
            {0, 0, 0, 0, 0},
            {0, 0, 1, 0, 0}
        };
        draw_pattern(pat, 40, 0, 70);

    } else if (strcmp(expr, "thinking") == 0) {
        // Thinking 3 dots (cyan)
        const uint8_t pat[5][5] = {
            {0, 0, 0, 0, 0},
            {0, 0, 0, 0, 0},
            {0, 0, 0, 0, 0},
            {1, 0, 1, 0, 1},
            {0, 0, 0, 0, 0}
        };
        draw_pattern(pat, 0, 50, 50);

    } else {
        // "normal" - Neutral line mouth (soft white)
        const uint8_t pat[5][5] = {
            {0, 0, 0, 0, 0},
            {0, 0, 0, 0, 0},
            {0, 0, 0, 0, 0},
            {0, 1, 1, 1, 0},
            {0, 0, 0, 0, 0}
        };
        draw_pattern(pat, 25, 35, 30);
    }
}

// =============================================================
// Standby / Idle Light Show (5 distinct animation styles)
// =============================================================

// Color wheel helper (0..255) -> (r, g, b)
static void wheel(uint8_t pos, uint8_t *r, uint8_t *g, uint8_t *b, uint8_t max_brightness) {
    pos = 255 - pos;
    if (pos < 85) {
        *r = (uint8_t)(((255 - pos * 3) * max_brightness) / 255);
        *g = 0;
        *b = (uint8_t)(((pos * 3) * max_brightness) / 255);
    } else if (pos < 170) {
        pos -= 85;
        *r = 0;
        *g = (uint8_t)(((pos * 3) * max_brightness) / 255);
        *b = (uint8_t)(((255 - pos * 3) * max_brightness) / 255);
    } else {
        pos -= 170;
        *r = (uint8_t)(((pos * 3) * max_brightness) / 255);
        *g = (uint8_t)(((255 - pos * 3) * max_brightness) / 255);
        *b = 0;
    }
}

// -------------------------------------------------------------
// Style 0: Rainbow Chromatic Wave
// -------------------------------------------------------------
void matrix_effect_wave(uint32_t step) {
    matrix_clear();
    for (int r = 0; r < 5; r++) {
        for (int c = 0; c < 5; c++) {
            uint8_t hue = (uint8_t)((r * 25 + c * 25 + step * 8) & 0xFF);
            uint8_t red, green, blue;
            wheel(hue, &red, &green, &blue, 45);
            matrix_set_pixel(r, c, red, green, blue);
        }
    }
    matrix_show();
}

// -------------------------------------------------------------
// Style 1: Sparkles / Fireflies (Smooth glittering lights)
// -------------------------------------------------------------
void matrix_effect_fireflies(uint32_t step) {
    matrix_clear();
    // 4 fireflies dancing with independent phases
    for (int i = 0; i < 4; i++) {
        float t = (float)step * 0.15f + (float)i * 1.8f;
        int r = (int)(2.0f + 2.0f * sinf(t * 0.7f));
        int c = (int)(2.0f + 2.0f * cosf(t * 0.9f));
        if (r < 0) r = 0; if (r > 4) r = 4;
        if (c < 0) c = 0; if (c > 4) c = 4;

        float glow = (sinf(t * 2.0f) + 1.0f) * 0.5f; // 0.0 .. 1.0
        uint8_t red, green, blue;
        switch (i) {
            case 0: // Golden amber
                red = (uint8_t)(60 * glow); green = (uint8_t)(40 * glow); blue = 0;
                break;
            case 1: // Cyan sparkle
                red = 0; green = (uint8_t)(50 * glow); blue = (uint8_t)(60 * glow);
                break;
            case 2: // Magical violet
                red = (uint8_t)(55 * glow); green = 0; blue = (uint8_t)(60 * glow);
                break;
            default: // Lime green
                red = (uint8_t)(20 * glow); green = (uint8_t)(60 * glow); blue = (uint8_t)(10 * glow);
                break;
        }
        matrix_set_pixel(r, c, red, green, blue);
    }
    matrix_show();
}

// -------------------------------------------------------------
// Style 2: Rotating Spiral Orbit (Snake from outside to inside)
// -------------------------------------------------------------
static const int8_t SPIRAL_COORDS[25][2] = {
    // Outer ring (16 LEDs)
    {0,0}, {0,1}, {0,2}, {0,3}, {0,4},
    {1,4}, {2,4}, {3,4}, {4,4},
    {4,3}, {4,2}, {4,1}, {4,0},
    {3,0}, {2,0}, {1,0},
    // Middle ring (8 LEDs)
    {1,1}, {1,2}, {1,3},
    {2,3}, {3,3},
    {3,2}, {3,1},
    {2,1},
    // Center (1 LED)
    {2,2}
};

void matrix_effect_spiral(uint32_t step) {
    matrix_clear();
    int head = (int)(step % 25);
    // Draw a moving comet with 5-pixel tail
    for (int t = 0; t < 5; t++) {
        int idx = (head - t + 25) % 25;
        int r = SPIRAL_COORDS[idx][0];
        int c = SPIRAL_COORDS[idx][1];
        uint8_t bright = (uint8_t)(55 - t * 10);
        // Aqua-to-blue comet
        matrix_set_pixel(r, c, (uint8_t)(bright / 4), (uint8_t)(bright * 3 / 4), bright);
    }
    matrix_show();
}

// -------------------------------------------------------------
// Style 3: Bouncing Equalizer Bars
// -------------------------------------------------------------
void matrix_effect_equalizer(uint32_t step) {
    matrix_clear();
    for (int col = 0; col < 5; col++) {
        float phase = (float)step * 0.35f + (float)col * 1.25f;
        int h = 1 + (int)(2.0f * (1.0f + sinf(phase)));
        if (h > 5) h = 5;

        for (int r = 0; r < h; r++) {
            int row = 4 - r; // from bottom row up
            uint8_t red = 0, green = 0, blue = 0;
            if (r == 0) { // Base
                green = 45; red = 5;
            } else if (r == 1) {
                green = 40; red = 25;
            } else if (r == 2) {
                green = 30; red = 45;
            } else if (r == 3) {
                green = 15; red = 55;
            } else { // Top peak
                red = 60; green = 0;
            }
            matrix_set_pixel(row, col, red, green, blue);
        }
    }
    matrix_show();
}

// -------------------------------------------------------------
// Style 4: Breathing Heart Pulse (Gentle rhythmic glow)
// -------------------------------------------------------------
void matrix_effect_breathing_heart(uint32_t step) {
    static const uint8_t heart_pat[5][5] = {
        {0, 1, 0, 1, 0},
        {1, 1, 1, 1, 1},
        {1, 1, 1, 1, 1},
        {0, 1, 1, 1, 0},
        {0, 0, 1, 0, 0}
    };
    matrix_clear();
    float breath = (sinf((float)step * 0.18f) + 1.0f) * 0.5f; // 0.0 .. 1.0
    uint8_t r = (uint8_t)(15.0f + 65.0f * breath);
    uint8_t b = (uint8_t)(5.0f + 35.0f * breath);

    for (int row = 0; row < 5; row++) {
        for (int col = 0; col < 5; col++) {
            if (heart_pat[row][col]) {
                matrix_set_pixel(row, col, r, 0, b);
            }
        }
    }
    matrix_show();
}

// -------------------------------------------------------------
// Dispatcher
// -------------------------------------------------------------
void matrix_play_idle_style(int style, uint32_t step) {
    switch (style % 5) {
        case 0: matrix_effect_wave(step); break;
        case 1: matrix_effect_fireflies(step); break;
        case 2: matrix_effect_spiral(step); break;
        case 3: matrix_effect_equalizer(step); break;
        case 4: matrix_effect_breathing_heart(step); break;
    }
}

