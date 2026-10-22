#include "ssd1306.h"
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"

// 128x64 mono framebuffer (1024 bytes)
static uint8_t fb[SSD1306_WIDTH * SSD1306_HEIGHT / 8];

// Basic 8x8 ASCII font (characters 32 ' ' to 126 '~')
static const uint8_t font8x8[95][8] = {
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, // 32 ' '
    {0x18,0x3C,0x3C,0x18,0x18,0x00,0x18,0x00}, // 33 '!'
    {0x66,0x66,0x24,0x00,0x00,0x00,0x00,0x00}, // 34 '"'
    {0x6C,0x6C,0xFE,0x6C,0xFE,0x6C,0x6C,0x00}, // 35 '#'
    {0x18,0x3E,0x60,0x3C,0x06,0x7C,0x18,0x00}, // 36 '$'
    {0x00,0x66,0xA6,0xD4,0x2B,0x65,0x66,0x00}, // 37 '%'
    {0x38,0x6C,0x38,0x76,0xDC,0xCC,0x76,0x00}, // 38 '&'
    {0x30,0x30,0x10,0x20,0x00,0x00,0x00,0x00}, // 39 '''
    {0x0C,0x18,0x30,0x30,0x30,0x18,0x0C,0x00}, // 40 '('
    {0x30,0x18,0x0C,0x0C,0x0C,0x18,0x30,0x00}, // 41 ')'
    {0x00,0x66,0x3C,0xFF,0x3C,0x66,0x00,0x00}, // 42 '*'
    {0x00,0x18,0x18,0x7E,0x18,0x18,0x00,0x00}, // 43 '+'
    {0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x30}, // 44 ','
    {0x00,0x00,0x00,0x7E,0x00,0x00,0x00,0x00}, // 45 '-'
    {0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x00}, // 46 '.'
    {0x06,0x0C,0x18,0x30,0x60,0xC0,0x80,0x00}, // 47 '/'
    {0x3C,0x66,0x6E,0x76,0x66,0x66,0x3C,0x00}, // 48 '0'
    {0x18,0x38,0x18,0x18,0x18,0x18,0x7E,0x00}, // 49 '1'
    {0x3C,0x66,0x06,0x0C,0x18,0x30,0x7E,0x00}, // 50 '2'
    {0x3C,0x66,0x06,0x1C,0x06,0x66,0x3C,0x00}, // 51 '3'
    {0x0C,0x1C,0x3C,0x6C,0xFE,0x0C,0x0C,0x00}, // 52 '4'
    {0x7E,0x60,0x7C,0x06,0x06,0x66,0x3C,0x00}, // 53 '5'
    {0x1C,0x30,0x60,0x7C,0x66,0x66,0x3C,0x00}, // 54 '6'
    {0x7E,0x06,0x0C,0x18,0x30,0x30,0x30,0x00}, // 55 '7'
    {0x3C,0x66,0x66,0x3C,0x66,0x66,0x3C,0x00}, // 56 '8'
    {0x3C,0x66,0x66,0x3E,0x06,0x0C,0x38,0x00}, // 57 '9'
    {0x00,0x18,0x18,0x00,0x18,0x18,0x00,0x00}, // 58 ':'
    {0x00,0x18,0x18,0x00,0x18,0x18,0x30,0x00}, // 59 ';'
    {0x0C,0x18,0x30,0x60,0x30,0x18,0x0C,0x00}, // 60 '<'
    {0x00,0x00,0x7E,0x00,0x7E,0x00,0x00,0x00}, // 61 '='
    {0x30,0x18,0x0C,0x06,0x0C,0x18,0x30,0x00}, // 62 '>'
    {0x3C,0x66,0x0C,0x18,0x18,0x00,0x18,0x00}, // 63 '?'
    {0x3C,0x66,0x6E,0x6A,0x6E,0x60,0x3C,0x00}, // 64 '@'
    {0x18,0x3C,0x66,0x66,0x7E,0x66,0x66,0x00}, // 65 'A'
    {0x7C,0x66,0x66,0x7C,0x66,0x66,0x7C,0x00}, // 66 'B'
    {0x3C,0x66,0x60,0x60,0x60,0x66,0x3C,0x00}, // 67 'C'
    {0x78,0x6C,0x66,0x66,0x66,0x6C,0x78,0x00}, // 68 'D'
    {0x7E,0x60,0x60,0x7C,0x60,0x60,0x7E,0x00}, // 69 'E'
    {0x7E,0x60,0x60,0x7C,0x60,0x60,0x60,0x00}, // 70 'F'
    {0x3C,0x66,0x60,0x6E,0x66,0x66,0x3C,0x00}, // 71 'G'
    {0x66,0x66,0x66,0x7E,0x66,0x66,0x66,0x00}, // 72 'H'
    {0x3C,0x18,0x18,0x18,0x18,0x18,0x3C,0x00}, // 73 'I'
    {0x0E,0x06,0x06,0x06,0x66,0x66,0x3C,0x00}, // 74 'J'
    {0x66,0x6C,0x78,0x70,0x78,0x6C,0x66,0x00}, // 75 'K'
    {0x60,0x60,0x60,0x60,0x60,0x60,0x7E,0x00}, // 76 'L'
    {0x63,0x77,0x7F,0x6B,0x63,0x63,0x63,0x00}, // 77 'M'
    {0x66,0x76,0x7E,0x7E,0x6E,0x66,0x66,0x00}, // 78 'N'
    {0x3C,0x66,0x66,0x66,0x66,0x66,0x3C,0x00}, // 79 'O'
    {0x7C,0x66,0x66,0x7C,0x60,0x60,0x60,0x00}, // 80 'P'
    {0x3C,0x66,0x66,0x66,0x6A,0x6C,0x36,0x00}, // 81 'Q'
    {0x7C,0x66,0x66,0x7C,0x6C,0x66,0x66,0x00}, // 82 'R'
    {0x3C,0x66,0x60,0x3C,0x06,0x66,0x3C,0x00}, // 83 'S'
    {0x7E,0x18,0x18,0x18,0x18,0x18,0x18,0x00}, // 84 'T'
    {0x66,0x66,0x66,0x66,0x66,0x66,0x3C,0x00}, // 85 'U'
    {0x66,0x66,0x66,0x66,0x66,0x3C,0x18,0x00}, // 86 'V'
    {0x63,0x63,0x63,0x6B,0x7F,0x77,0x63,0x00}, // 87 'W'
    {0x66,0x66,0x3C,0x18,0x3C,0x66,0x66,0x00}, // 88 'X'
    {0x66,0x66,0x66,0x3C,0x18,0x18,0x18,0x00}, // 89 'Y'
    {0x7E,0x06,0x0C,0x18,0x30,0x60,0x7E,0x00}, // 90 'Z'
    {0x3C,0x30,0x30,0x30,0x30,0x30,0x3C,0x00}, // 91 '['
    {0xC0,0x60,0x30,0x18,0x0C,0x06,0x02,0x00}, // 92 '\'
    {0x3C,0x0C,0x0C,0x0C,0x0C,0x0C,0x3C,0x00}, // 93 ']'
    {0x10,0x38,0x6C,0xC6,0x00,0x00,0x00,0x00}, // 94 '^'
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF}, // 95 '_'
    {0x30,0x30,0x18,0x00,0x00,0x00,0x00,0x00}, // 96 '`'
    {0x00,0x00,0x3C,0x06,0x3E,0x66,0x3E,0x00}, // 97 'a'
    {0x60,0x60,0x7C,0x66,0x66,0x66,0x7C,0x00}, // 98 'b'
    {0x00,0x00,0x3C,0x66,0x60,0x66,0x3C,0x00}, // 99 'c'
    {0x06,0x06,0x3E,0x66,0x66,0x66,0x3E,0x00}, // 100 'd'
    {0x00,0x00,0x3C,0x66,0x7E,0x60,0x3C,0x00}, // 101 'e'
    {0x1C,0x30,0x7C,0x30,0x30,0x30,0x30,0x00}, // 102 'f'
    {0x00,0x00,0x3E,0x66,0x66,0x3E,0x06,0x3C}, // 103 'g'
    {0x60,0x60,0x7C,0x66,0x66,0x66,0x66,0x00}, // 104 'h'
    {0x18,0x00,0x38,0x18,0x18,0x18,0x3C,0x00}, // 105 'i'
    {0x06,0x00,0x06,0x06,0x06,0x66,0x66,0x3C}, // 106 'j'
    {0x60,0x60,0x66,0x6C,0x78,0x6C,0x66,0x00}, // 107 'k'
    {0x38,0x18,0x18,0x18,0x18,0x18,0x3C,0x00}, // 108 'l'
    {0x00,0x00,0x66,0x7F,0x7F,0x6B,0x63,0x00}, // 109 'm'
    {0x00,0x00,0x7C,0x66,0x66,0x66,0x66,0x00}, // 110 'n'
    {0x00,0x00,0x3C,0x66,0x66,0x66,0x3C,0x00}, // 111 'o'
    {0x00,0x00,0x7C,0x66,0x66,0x7C,0x60,0x60}, // 112 'p'
    {0x00,0x00,0x3E,0x66,0x66,0x3E,0x06,0x07}, // 113 'q'
    {0x00,0x00,0x7C,0x66,0x60,0x60,0x60,0x00}, // 114 'r'
    {0x00,0x00,0x3E,0x60,0x3C,0x06,0x7C,0x00}, // 115 's'
    {0x30,0x30,0x7C,0x30,0x30,0x34,0x18,0x00}, // 116 't'
    {0x00,0x00,0x66,0x66,0x66,0x66,0x3E,0x00}, // 117 'u'
    {0x00,0x00,0x66,0x66,0x66,0x3C,0x18,0x00}, // 118 'v'
    {0x00,0x00,0x63,0x6B,0x7F,0x3E,0x36,0x00}, // 119 'w'
    {0x00,0x00,0x66,0x3C,0x18,0x3C,0x66,0x00}, // 120 'x'
    {0x00,0x00,0x66,0x66,0x66,0x3E,0x06,0x3C}, // 121 'y'
    {0x00,0x00,0x7E,0x0C,0x18,0x30,0x7E,0x00}, // 122 'z'
    {0x0E,0x18,0x18,0x70,0x18,0x18,0x0E,0x00}, // 123 '{'
    {0x18,0x18,0x18,0x00,0x18,0x18,0x18,0x00}, // 124 '|'
    {0x70,0x18,0x18,0x0E,0x18,0x18,0x70,0x00}, // 125 '}'
    {0x76,0xDC,0x00,0x00,0x00,0x00,0x00,0x00}  // 126 '~'
};

static void ssd1306_send_cmd(uint8_t cmd) {
    uint8_t buf[2] = {0x00, cmd};
    i2c_write_blocking(SSD1306_I2C_PORT, SSD1306_I2C_ADDR, buf, 2, false);
}

void ssd1306_init(void) {
    i2c_init(SSD1306_I2C_PORT, 400 * 1000);
    gpio_set_function(SSD1306_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(SSD1306_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(SSD1306_SDA_PIN);
    gpio_pull_up(SSD1306_SCL_PIN);

    sleep_ms(100);

    // Initialization sequence for 128x64 SSD1306
    ssd1306_send_cmd(0xAE); // Display off
    ssd1306_send_cmd(0xD5); // Set display clock divide
    ssd1306_send_cmd(0x80);
    ssd1306_send_cmd(0xA8); // Set multiplex ratio
    ssd1306_send_cmd(0x3F); // 64MUX
    ssd1306_send_cmd(0xD3); // Set display offset
    ssd1306_send_cmd(0x00);
    ssd1306_send_cmd(0x40); // Set start line
    ssd1306_send_cmd(0x8D); // Charge pump
    ssd1306_send_cmd(0x14); // Enable charge pump
    ssd1306_send_cmd(0x20); // Memory addressing mode
    ssd1306_send_cmd(0x00); // Horizontal addressing mode
    ssd1306_send_cmd(0xA1); // Segment remap (column 127 mapped to SEG0 -> un-mirrored HELLO)
    ssd1306_send_cmd(0xC8); // COM output scan direction (remapped COM[N-1] to COM0 -> upright)
    ssd1306_send_cmd(0xDA); // COM pins hardware configuration
    ssd1306_send_cmd(0x12);
    ssd1306_send_cmd(0x81); // Set contrast control
    ssd1306_send_cmd(0xCF);
    ssd1306_send_cmd(0xD9); // Set pre-charge period
    ssd1306_send_cmd(0xF1);
    ssd1306_send_cmd(0xDB); // Set Vcomh deselect level
    ssd1306_send_cmd(0x40);
    ssd1306_send_cmd(0xA4); // Output follows RAM
    ssd1306_send_cmd(0xA6); // Normal display (not inverted)
    ssd1306_send_cmd(0xAF); // Display on

    ssd1306_clear();
    ssd1306_show();
}

void ssd1306_set_orientation(bool inverted) {
    if (inverted) {
        ssd1306_send_cmd(0xA0); // 180° rotated
        ssd1306_send_cmd(0xC0);
    } else {
        ssd1306_send_cmd(0xA1); // Normal upright orientation (legible HELLO, upright)
        ssd1306_send_cmd(0xC8);
    }
}

void ssd1306_clear(void) {
    memset(fb, 0, sizeof(fb));
}

void ssd1306_show(void) {
    ssd1306_send_cmd(0x21); // Column address
    ssd1306_send_cmd(0);
    ssd1306_send_cmd(SSD1306_WIDTH - 1);

    ssd1306_send_cmd(0x22); // Page address
    ssd1306_send_cmd(0);
    ssd1306_send_cmd((SSD1306_HEIGHT / 8) - 1);

    uint8_t buffer[SSD1306_WIDTH + 1];
    buffer[0] = 0x40; // Data mode

    for (int p = 0; p < SSD1306_HEIGHT / 8; p++) {
        memcpy(&buffer[1], &fb[p * SSD1306_WIDTH], SSD1306_WIDTH);
        i2c_write_blocking(SSD1306_I2C_PORT, SSD1306_I2C_ADDR, buffer, sizeof(buffer), false);
    }
}

void ssd1306_draw_pixel(int x, int y, bool color) {
    if (x < 0 || x >= SSD1306_WIDTH || y < 0 || y >= SSD1306_HEIGHT) return;
    int index = x + (y / 8) * SSD1306_WIDTH;
    if (color) {
        fb[index] |= (1 << (y % 8));
    } else {
        fb[index] &= ~(1 << (y % 8));
    }
}

void ssd1306_draw_line(int x0, int y0, int x1, int y1, bool color) {
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy, e2;

    while (1) {
        ssd1306_draw_pixel(x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

void ssd1306_draw_rect(int x, int y, int w, int h, bool color) {
    for (int i = x; i < x + w; i++) {
        ssd1306_draw_pixel(i, y, color);
        ssd1306_draw_pixel(i, y + h - 1, color);
    }
    for (int j = y; j < y + h; j++) {
        ssd1306_draw_pixel(x, j, color);
        ssd1306_draw_pixel(x + w - 1, j, color);
    }
}

void ssd1306_fill_rect(int x, int y, int w, int h, bool color) {
    for (int j = y; j < y + h; j++) {
        for (int i = x; i < x + w; i++) {
            ssd1306_draw_pixel(i, j, color);
        }
    }
}

void ssd1306_draw_circle(int xm, int ym, int r, bool color) {
    int x = -r, y = 0, err = 2 - 2 * r;
    do {
        ssd1306_draw_pixel(xm - x, ym + y, color);
        ssd1306_draw_pixel(xm - y, ym - x, color);
        ssd1306_draw_pixel(xm + x, ym - y, color);
        ssd1306_draw_pixel(xm + y, ym + x, color);
        r = err;
        if (r <= y) err += ++y * 2 + 1;
        if (r > x || err > y) err += ++x * 2 + 1;
    } while (x < 0);
}

void ssd1306_fill_circle(int xm, int ym, int r, bool color) {
    for (int y = -r; y <= r; y++) {
        for (int x = -r; x <= r; x++) {
            if (x * x + y * y <= r * r) {
                ssd1306_draw_pixel(xm + x, ym + y, color);
            }
        }
    }
}

void ssd1306_fill_triangle(int x0, int y0, int x1, int y1, int x2, int y2, bool color) {
    int minX = x0, maxX = x0, minY = y0, maxY = y0;
    if (x1 < minX) minX = x1; if (x1 > maxX) maxX = x1;
    if (x2 < minX) minX = x2; if (x2 > maxX) maxX = x2;
    if (y1 < minY) minY = y1; if (y1 > maxY) maxY = y1;
    if (y2 < minY) minY = y2; if (y2 > maxY) maxY = y2;

    for (int y = minY; y <= maxY; y++) {
        for (int x = minX; x <= maxX; x++) {
            int w0 = (x1 - x0) * (y - y0) - (y1 - y0) * (x - x0);
            int w1 = (x2 - x1) * (y - y1) - (y2 - y1) * (x - x1);
            int w2 = (x0 - x2) * (y - y2) - (y0 - y2) * (x - x2);
            if ((w0 >= 0 && w1 >= 0 && w2 >= 0) || (w0 <= 0 && w1 <= 0 && w2 <= 0)) {
                ssd1306_draw_pixel(x, y, color);
            }
        }
    }
}

void ssd1306_draw_char(int x, int y, char c, bool color) {
    if (c < 32 || c > 126) c = ' ';
    const uint8_t *glyph = font8x8[c - 32];
    for (int row = 0; row < 8; row++) {
        uint8_t line = glyph[row];
        for (int col = 0; col < 8; col++) {
            if (line & (0x80 >> col)) {
                ssd1306_draw_pixel(x + col, y + row, color);
            }
        }
    }
}

void ssd1306_draw_string(int x, int y, const char *str, bool color) {
    while (*str) {
        ssd1306_draw_char(x, y, *str, color);
        x += 8;
        str++;
    }
}

void ssd1306_draw_char_2x(int x, int y, char c, bool color) {
    if (c < 32 || c > 126) c = ' ';
    const uint8_t *glyph = font8x8[c - 32];
    for (int row = 0; row < 8; row++) {
        uint8_t line = glyph[row];
        for (int col = 0; col < 8; col++) {
            if (line & (0x80 >> col)) {
                ssd1306_fill_rect(x + col * 2, y + row * 2, 2, 2, color);
            }
        }
    }
}

void ssd1306_draw_string_2x(int x, int y, const char *str, bool color) {
    while (*str) {
        ssd1306_draw_char_2x(x, y, *str, color);
        x += 16;
        str++;
    }
}

// ------------------------------------------------------------------
// Eye Helpers
// ------------------------------------------------------------------
static void draw_eye(int cx, int cy, int outerR, int pupilR, int offX, int offY) {
    ssd1306_draw_circle(cx, cy, outerR, true);
    ssd1306_fill_circle(cx + offX, cy + offY, pupilR, true);
    // Specular highlight dot (white pupil reflex)
    ssd1306_fill_circle(cx + offX + 2, cy + offY - 2, 2, false);
}

// ------------------------------------------------------------------
// The 10 Expressions
// ------------------------------------------------------------------
void ssd1306_expr_normal(void) {
    ssd1306_clear();
    draw_eye(LEFT_EYE_X,  EYE_Y, EYE_R, PUPIL_R, 0, 0);
    draw_eye(RIGHT_EYE_X, EYE_Y, EYE_R, PUPIL_R, 0, 0);
    ssd1306_show();
}

void ssd1306_expr_happy(void) {
    ssd1306_clear();
    for (int i = 0; i < 2; i++) {
        int cx = (i == 0) ? LEFT_EYE_X : RIGHT_EYE_X;
        for (int a = 0; a <= 180; a += 3) {
            float r = a * 3.14159f / 180.0f;
            ssd1306_draw_pixel(cx + (int)(EYE_R * cosf(r)),
                               EYE_Y - (int)(EYE_R * sinf(r)), true);
            ssd1306_draw_pixel(cx + (int)((EYE_R-1) * cosf(r)),
                               EYE_Y - (int)((EYE_R-1) * sinf(r)), true);
        }
        ssd1306_fill_circle(cx + EYE_R - 4, EYE_Y + 8, 3, true);
    }
    ssd1306_show();
}

void ssd1306_expr_sad(void) {
    ssd1306_clear();
    for (int i = 0; i < 2; i++) {
        int cx = (i == 0) ? LEFT_EYE_X : RIGHT_EYE_X;
        ssd1306_draw_circle(cx, EYE_Y + 4, EYE_R, true);
        ssd1306_fill_rect(cx - EYE_R - 1, EYE_Y - EYE_R - 1, EYE_R * 2 + 2, EYE_R - 2, false);
        if (i == 0) {
            ssd1306_draw_line(cx - EYE_R, EYE_Y - EYE_R + 2, cx + EYE_R / 2, EYE_Y - EYE_R - 4, true);
        } else {
            ssd1306_draw_line(cx - EYE_R / 2, EYE_Y - EYE_R - 4, cx + EYE_R, EYE_Y - EYE_R + 2, true);
        }
        // Tear drop
        ssd1306_fill_circle(cx + EYE_R - 5, EYE_Y + EYE_R + 4, 2, true);
    }
    ssd1306_show();
}

void ssd1306_expr_angry(void) {
    ssd1306_clear();
    draw_eye(LEFT_EYE_X,  EYE_Y, EYE_R, PUPIL_R,  2, 2);
    draw_eye(RIGHT_EYE_X, EYE_Y, EYE_R, PUPIL_R, -2, 2);
    // Furrowed brow lines
    ssd1306_draw_line(LEFT_EYE_X - EYE_R,  EYE_Y - EYE_R - 5, LEFT_EYE_X + EYE_R,  EYE_Y - EYE_R - 1, true);
    ssd1306_draw_line(LEFT_EYE_X - EYE_R,  EYE_Y - EYE_R - 4, LEFT_EYE_X + EYE_R,  EYE_Y - EYE_R, true);
    ssd1306_draw_line(RIGHT_EYE_X - EYE_R, EYE_Y - EYE_R - 1, RIGHT_EYE_X + EYE_R, EYE_Y - EYE_R - 5, true);
    ssd1306_draw_line(RIGHT_EYE_X - EYE_R, EYE_Y - EYE_R,     RIGHT_EYE_X + EYE_R, EYE_Y - EYE_R - 4, true);
    ssd1306_show();
}

void ssd1306_expr_surprised(void) {
    ssd1306_clear();
    ssd1306_draw_circle(LEFT_EYE_X,  EYE_Y, EYE_R + 4, true);
    ssd1306_draw_circle(RIGHT_EYE_X, EYE_Y, EYE_R + 4, true);
    ssd1306_fill_circle(LEFT_EYE_X,  EYE_Y, PUPIL_R + 2, true);
    ssd1306_fill_circle(RIGHT_EYE_X, EYE_Y, PUPIL_R + 2, true);
    // Raised high eyebrows
    ssd1306_draw_line(LEFT_EYE_X - EYE_R,  EYE_Y - EYE_R - 9, LEFT_EYE_X + EYE_R,  EYE_Y - EYE_R - 9, true);
    ssd1306_draw_line(RIGHT_EYE_X - EYE_R, EYE_Y - EYE_R - 9, RIGHT_EYE_X + EYE_R, EYE_Y - EYE_R - 9, true);
    ssd1306_show();
}

void ssd1306_expr_wink(void) {
    ssd1306_clear();
    draw_eye(LEFT_EYE_X, EYE_Y, EYE_R, PUPIL_R, 0, 0);
    // Right eye is winking (closed line)
    ssd1306_draw_line(RIGHT_EYE_X - EYE_R, EYE_Y - 1, RIGHT_EYE_X + EYE_R, EYE_Y - 1, true);
    ssd1306_draw_line(RIGHT_EYE_X - EYE_R, EYE_Y,     RIGHT_EYE_X + EYE_R, EYE_Y,     true);
    ssd1306_draw_line(RIGHT_EYE_X - EYE_R, EYE_Y + 1, RIGHT_EYE_X + EYE_R, EYE_Y + 1, true);
    ssd1306_show();
}

void ssd1306_expr_sleepy(void) {
    ssd1306_clear();
    for (int i = 0; i < 2; i++) {
        int cx = (i == 0) ? LEFT_EYE_X : RIGHT_EYE_X;
        ssd1306_draw_circle(cx, EYE_Y, EYE_R, true);
        ssd1306_fill_rect(cx - EYE_R - 1, EYE_Y - EYE_R - 1, EYE_R * 2 + 2, (int)(EYE_R * 1.4f), false);
        ssd1306_draw_line(cx - EYE_R, EYE_Y - 3, cx + EYE_R, EYE_Y - 3, true);
        ssd1306_fill_circle(cx, EYE_Y + 5, 4, true);
    }
    ssd1306_draw_string(44, 52, "z z z", true);
    ssd1306_show();
}

void ssd1306_expr_love(void) {
    ssd1306_clear();
    for (int i = 0; i < 2; i++) {
        int cx = (i == 0) ? LEFT_EYE_X : RIGHT_EYE_X;
        ssd1306_fill_circle(cx - 6, EYE_Y - 4, 7, true);
        ssd1306_fill_circle(cx + 6, EYE_Y - 4, 7, true);
        ssd1306_fill_triangle(cx - 13, EYE_Y - 2, cx + 13, EYE_Y - 2, cx, EYE_Y + 12, true);
    }
    ssd1306_show();
}

void ssd1306_expr_confused(void) {
    ssd1306_clear();
    // Left eye smaller
    ssd1306_draw_circle(LEFT_EYE_X, EYE_Y, EYE_R - 4, true);
    ssd1306_fill_circle(LEFT_EYE_X, EYE_Y, PUPIL_R - 2, true);
    // Right eye larger
    ssd1306_draw_circle(RIGHT_EYE_X, EYE_Y, EYE_R + 4, true);
    ssd1306_fill_circle(RIGHT_EYE_X, EYE_Y, PUPIL_R + 2, true);
    // Question mark
    ssd1306_draw_char(60, 48, '?', true);
    ssd1306_show();
}

void ssd1306_expr_thinking(void) {
    ssd1306_clear();
    draw_eye(LEFT_EYE_X,  EYE_Y, EYE_R, PUPIL_R, 5, -5);
    draw_eye(RIGHT_EYE_X, EYE_Y, EYE_R, PUPIL_R, 5, -5);
    ssd1306_draw_line(RIGHT_EYE_X - EYE_R, EYE_Y - EYE_R - 8, RIGHT_EYE_X + EYE_R, EYE_Y - EYE_R - 4, true);
    ssd1306_draw_string(48, 52, ". . .", true);
    ssd1306_show();
}

void ssd1306_smooth_blink(void) {
    for (int h = 0; h <= EYE_R * 2; h += 3) {
        ssd1306_clear();
        for (int i = 0; i < 2; i++) {
            int cx = (i == 0) ? LEFT_EYE_X : RIGHT_EYE_X;
            ssd1306_draw_circle(cx, EYE_Y, EYE_R, true);
            ssd1306_fill_rect(cx - EYE_R, EYE_Y - EYE_R, EYE_R * 2 + 1, h, false);
        }
        ssd1306_show();
        sleep_ms(12);
    }
    sleep_ms(60);
    for (int h = EYE_R * 2; h >= 0; h -= 3) {
        ssd1306_clear();
        for (int i = 0; i < 2; i++) {
            int cx = (i == 0) ? LEFT_EYE_X : RIGHT_EYE_X;
            ssd1306_draw_circle(cx, EYE_Y, EYE_R, true);
            ssd1306_fill_rect(cx - EYE_R, EYE_Y - EYE_R, EYE_R * 2 + 1, h, false);
        }
        ssd1306_show();
        sleep_ms(12);
    }
}

void ssd1306_scroll_text(const char *text) {
    int len = (int)strlen(text);
    int textW = len * 8;
    int startX = SSD1306_WIDTH;
    int endX = -textW;

    for (int x = startX; x > endX; x -= 4) {
        ssd1306_clear();
        // Header
        ssd1306_draw_string(16, 2, "[ EMO BOT ]", true);
        ssd1306_draw_line(0, 12, 127, 12, true);
        // Scrolling message
        ssd1306_draw_string(x, 32, text, true);
        // Footer
        ssd1306_draw_line(0, 52, 127, 52, true);
        ssd1306_draw_string(16, 54, "BitDogLab AI", true);

        ssd1306_show();
        sleep_ms(20);
    }
}

void ssd1306_draw_hello_frame(int x_offset) {
    ssd1306_clear();
    // Header decorative bar
    ssd1306_draw_string(24, 0, "~ EMO BOT ~", true);
    ssd1306_draw_line(0, 10, 127, 10, true);

    // Large 2x text "HELLO!" (width 96px, height 16px) centered vertically
    ssd1306_draw_string_2x(x_offset, 24, "HELLO!", true);

    // Footer decorative bar
    ssd1306_draw_line(0, 52, 127, 52, true);
    ssd1306_draw_string(28, 55, "* Standby *", true);

    ssd1306_show();
}

