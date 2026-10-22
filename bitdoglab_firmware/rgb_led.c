#include "rgb_led.h"
#include <string.h>
#include "pico/stdlib.h"

void rgb_led_init(void) {
    gpio_init(LED_RED_PIN);
    gpio_set_dir(LED_RED_PIN, GPIO_OUT);
    gpio_put(LED_RED_PIN, 0);

    gpio_init(LED_GREEN_PIN);
    gpio_set_dir(LED_GREEN_PIN, GPIO_OUT);
    gpio_put(LED_GREEN_PIN, 0);

    gpio_init(LED_BLUE_PIN);
    gpio_set_dir(LED_BLUE_PIN, GPIO_OUT);
    gpio_put(LED_BLUE_PIN, 0);
}

void rgb_led_set(bool r, bool g, bool b) {
    gpio_put(LED_RED_PIN, r);
    gpio_put(LED_GREEN_PIN, g);
    gpio_put(LED_BLUE_PIN, b);
}

void rgb_led_set_expression(const char *expr) {
    if (strcmp(expr, "happy") == 0) {
        // Yellow (Red + Green)
        rgb_led_set(1, 1, 0);
    } else if (strcmp(expr, "sad") == 0) {
        // Blue
        rgb_led_set(0, 0, 1);
    } else if (strcmp(expr, "angry") == 0) {
        // Red
        rgb_led_set(1, 0, 0);
    } else if (strcmp(expr, "love") == 0) {
        // Magenta (Red + Blue)
        rgb_led_set(1, 0, 1);
    } else if (strcmp(expr, "surprised") == 0) {
        // Yellow/Amber (Red + Green)
        rgb_led_set(1, 1, 0);
    } else if (strcmp(expr, "confused") == 0) {
        // Cyan (Green + Blue)
        rgb_led_set(0, 1, 1);
    } else if (strcmp(expr, "thinking") == 0) {
        // Cyan (Green + Blue)
        rgb_led_set(0, 1, 1);
    } else if (strcmp(expr, "wink") == 0) {
        // Green
        rgb_led_set(0, 1, 0);
    } else if (strcmp(expr, "sleepy") == 0) {
        // Blue
        rgb_led_set(0, 0, 1);
    } else {
        // "normal" - Green (or Soft Green)
        rgb_led_set(0, 1, 0);
    }
}
