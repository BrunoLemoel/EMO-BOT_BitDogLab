#ifndef BUZZER_H
#define BUZZER_H

#include <stdint.h>
#include "pico/stdlib.h"

#define BUZZER_PIN 21 // Buzzer A on BitDogLab

void buzzer_init(void);
void buzzer_tone(uint frequency_hz, uint duration_ms);
void buzzer_tone_volume(uint frequency_hz, uint duration_ms, uint8_t duty_percent);
void buzzer_stop(void);
void buzzer_play_emotion(const char *expr);
void buzzer_play_notification(void);
void buzzer_play_ping(void);
void buzzer_play_mumble(int variation);
void buzzer_play_wakeup(void);

#endif // BUZZER_H
