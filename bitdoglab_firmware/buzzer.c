#include "buzzer.h"
#include <string.h>
#include "hardware/pwm.h"
#include "hardware/clocks.h"

static uint slice_num;
static uint chan_num;

void buzzer_init(void) {
    gpio_set_function(BUZZER_PIN, GPIO_FUNC_PWM);
    slice_num = pwm_gpio_to_slice_num(BUZZER_PIN);
    chan_num = pwm_gpio_to_channel(BUZZER_PIN);
    pwm_set_enabled(slice_num, false);
}

void buzzer_stop(void) {
    pwm_set_enabled(slice_num, false);
    pwm_set_chan_level(slice_num, chan_num, 0);
}

void buzzer_tone_volume(uint frequency_hz, uint duration_ms, uint8_t duty_percent) {
    if (frequency_hz == 0 || duty_percent == 0) {
        buzzer_stop();
        if (duration_ms > 0) sleep_ms(duration_ms);
        return;
    }

    if (duty_percent > 50) duty_percent = 50;

    uint32_t clock_hz = clock_get_hz(clk_sys);
    float clkdiv = 125.0f; // 125 MHz / 125 = 1 MHz timer
    uint32_t wrap = (1000000 / frequency_hz) - 1;
    if (wrap > 65535) wrap = 65535;

    pwm_set_clkdiv(slice_num, clkdiv);
    pwm_set_wrap(slice_num, (uint16_t)wrap);

    uint32_t level = ((wrap + 1) * duty_percent) / 100;
    if (level < 1) level = 1;
    pwm_set_chan_level(slice_num, chan_num, (uint16_t)level);
    pwm_set_enabled(slice_num, true);

    sleep_ms(duration_ms);
    buzzer_stop();
    sleep_ms(15); // Small silence gap between notes
}

void buzzer_tone(uint frequency_hz, uint duration_ms) {
    buzzer_tone_volume(frequency_hz, duration_ms, 50); // 50% duty (normal full volume)
}

void buzzer_play_emotion(const char *expr) {
    if (strcmp(expr, "happy") == 0) {
        // Joyful ascending arpeggio: C5, E5, G5, C6
        buzzer_tone(523, 60);
        buzzer_tone(659, 60);
        buzzer_tone(784, 60);
        buzzer_tone(1046, 120);

    } else if (strcmp(expr, "sad") == 0) {
        // Descending melancholy tone: G4, E4, C4
        buzzer_tone(392, 120);
        buzzer_tone(330, 140);
        buzzer_tone(261, 200);

    } else if (strcmp(expr, "angry") == 0) {
        // Low robotic buzz
        buzzer_tone(160, 90);
        buzzer_tone(130, 140);

    } else if (strcmp(expr, "love") == 0) {
        // Sweet melodic chime: A5, C6, E6
        buzzer_tone(880, 80);
        buzzer_tone(1046, 80);
        buzzer_tone(1318, 140);

    } else if (strcmp(expr, "surprised") == 0) {
        // Quick high frequency double chirp
        buzzer_tone(1500, 50);
        buzzer_tone(2200, 90);

    } else if (strcmp(expr, "wink") == 0) {
        // Playful double blip
        buzzer_tone(1200, 40);
        buzzer_tone(1800, 60);

    } else if (strcmp(expr, "sleepy") == 0) {
        // Soft low fading notes
        buzzer_tone(330, 100);
        buzzer_tone(220, 160);

    } else if (strcmp(expr, "confused") == 0) {
        // Rising and falling question tone
        buzzer_tone(550, 70);
        buzzer_tone(850, 90);
        buzzer_tone(700, 70);

    } else if (strcmp(expr, "thinking") == 0) {
        // Soft robotic ticks
        buzzer_tone(1400, 30);
        buzzer_tone(0, 50);
        buzzer_tone(1400, 30);

    } else {
        // "normal" - Subtle polite greeting
        buzzer_tone(880, 50);
        buzzer_tone(1175, 70);
    }
}

void buzzer_play_notification(void) {
    buzzer_tone(1046, 50);
    buzzer_tone(1318, 80);
}

void buzzer_play_ping(void) {
    buzzer_tone(2093, 80);
}

// Sons de balbucio robótico ocioso ("falante" em volume meio baixo, ~8% duty cycle)
void buzzer_play_mumble(int variation) {
    const uint8_t low_vol = 8; // ~8% duty cycle (volume meio baixo, nítido e suave)
    switch (variation % 5) {
        case 0:
            // "Hmm-hum" (curious rising murmur)
            buzzer_tone_volume(440, 35, low_vol);
            buzzer_tone_volume(587, 45, low_vol);
            buzzer_tone_volume(520, 50, low_vol);
            break;
        case 1:
            // "Bibip-bop" (cheerful little chatter)
            buzzer_tone_volume(740, 30, low_vol);
            buzzer_tone_volume(880, 35, low_vol);
            buzzer_tone_volume(660, 45, low_vol);
            break;
        case 2:
            // "Uuu-wa?" (puzzled soft query)
            buzzer_tone_volume(400, 40, low_vol);
            buzzer_tone_volume(600, 50, low_vol);
            buzzer_tone_volume(480, 50, low_vol);
            break;
        case 3:
            // "Pip-pip-peep" (playful staccato ticks)
            buzzer_tone_volume(988, 25, low_vol);
            buzzer_tone_volume(1175, 30, low_vol);
            buzzer_tone_volume(1318, 40, low_vol);
            break;
        case 4:
            // "Ooh-hmm" (cozy gentle sigh)
            buzzer_tone_volume(520, 45, low_vol);
            buzzer_tone_volume(415, 60, low_vol);
            buzzer_tone_volume(330, 80, low_vol);
            break;
    }
}

// Cheerful wake-up chime when exiting standby mode
void buzzer_play_wakeup(void) {
    buzzer_tone_volume(880, 50, 20);
    buzzer_tone_volume(1175, 50, 20);
    buzzer_tone_volume(1568, 90, 25);
}

