#ifndef RGB_LED_H
#define RGB_LED_H

#include <stdint.h>
#include <stdbool.h>

#define LED_RED_PIN   13
#define LED_GREEN_PIN 11
#define LED_BLUE_PIN  12

void rgb_led_init(void);
void rgb_led_set(bool r, bool g, bool b);
void rgb_led_set_expression(const char *expr);

#endif // RGB_LED_H
