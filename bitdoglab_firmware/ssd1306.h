#ifndef SSD1306_H
#define SSD1306_H

#include <stdint.h>
#include <stdbool.h>
#include "hardware/i2c.h"

// SSD1306 Display dimensions
#define SSD1306_WIDTH   128
#define SSD1306_HEIGHT  64

// BitDogLab I2C Pins for OLED
#define SSD1306_I2C_PORT i2c1
#define SSD1306_SDA_PIN  14
#define SSD1306_SCL_PIN  15
#define SSD1306_I2C_ADDR 0x3C

// Eye positioning constants (exact same layout as original Emo-bot)
#define LEFT_EYE_X   32
#define RIGHT_EYE_X  96
#define EYE_Y        32
#define EYE_R        18
#define PUPIL_R       7

// Initialization & display control
void ssd1306_init(void);
void ssd1306_clear(void);
void ssd1306_show(void);
void ssd1306_set_orientation(bool inverted);

// Primitive drawing
void ssd1306_draw_pixel(int x, int y, bool color);
void ssd1306_draw_line(int x0, int y0, int x1, int y1, bool color);
void ssd1306_draw_rect(int x, int y, int w, int h, bool color);
void ssd1306_fill_rect(int x, int y, int w, int h, bool color);
void ssd1306_draw_circle(int xm, int ym, int r, bool color);
void ssd1306_fill_circle(int xm, int ym, int r, bool color);
void ssd1306_fill_triangle(int x0, int y0, int x1, int y1, int x2, int y2, bool color);

// Text
void ssd1306_draw_char(int x, int y, char c, bool color);
void ssd1306_draw_string(int x, int y, const char *str, bool color);
void ssd1306_draw_char_2x(int x, int y, char c, bool color);
void ssd1306_draw_string_2x(int x, int y, const char *str, bool color);

// Standby "HELLO!" animated frame
void ssd1306_draw_hello_frame(int x_offset);

// Expression functions (The 10 Emo-Bot expressions)
void ssd1306_expr_normal(void);
void ssd1306_expr_happy(void);
void ssd1306_expr_sad(void);
void ssd1306_expr_angry(void);
void ssd1306_expr_surprised(void);
void ssd1306_expr_wink(void);
void ssd1306_expr_sleepy(void);
void ssd1306_expr_love(void);
void ssd1306_expr_confused(void);
void ssd1306_expr_thinking(void);
void ssd1306_smooth_blink(void);

// Message scrolling
void ssd1306_scroll_text(const char *text);

#endif // SSD1306_H
