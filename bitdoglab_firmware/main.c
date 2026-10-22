#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"

#include "ssd1306.h"
#include "matrix.h"
#include "rgb_led.h"
#include "buzzer.h"

// Buttons on BitDogLab (Active LOW with internal pull-up)
#define BTN_A_PIN 5
#define BTN_B_PIN 6

static const char *EMOTIONS[] = {
    "normal", "happy", "sad", "angry", "surprised",
    "love", "confused", "thinking", "wink", "sleepy", "wave"
};
#define NUM_EMOTIONS (sizeof(EMOTIONS) / sizeof(EMOTIONS[0]))

static int current_emotion_idx = 0;
static char current_expression[32] = "normal";

// Timers for auto-blink
static uint32_t last_blink_time = 0;
static uint32_t blink_interval_ms = 5000;

// Button debounce timers
static uint32_t last_btn_a_time = 0;
static uint32_t last_btn_b_time = 0;

// Standby Timeout: 3 minutes of inactivity (180,000 ms)
#define STANDBY_TIMEOUT_MS (3 * 60 * 1000)

// Standby Style Duration: 60 seconds (60,000 ms) per light show style
#define STANDBY_PHASE_DURATION_MS (60 * 1000)

// Buzzer "Falante" Standby Interval: Alterna a cada 2 minutos (120.000 ms)
#define STANDBY_MUMBLE_INTERVAL_MS (2 * 60 * 1000)

static uint32_t last_activity_time = 0;
static bool is_in_standby = false;
static bool normal_display_flipped = false;

// Standby animation cycle variables
static int standby_phase = 0; // 0: Normal, 1: Happy, 2: Confused, 3: Thinking, 4: Blink, 5: "HELLO!" scroll
static uint32_t standby_phase_start = 0;
static uint32_t last_matrix_frame_time = 0;
static uint32_t matrix_frame_step = 0;
static uint32_t last_standby_mumble_time = 0;
static int standby_mumble_variation = 0;
static uint32_t last_standby_blink_time = 0;

// Persistent Wave Mode (activated via Button A, only exits when Button A is pressed again)
static bool is_wave_mode = false;

static int hello_scroll_x = -100;
static uint32_t last_hello_step_time = 0;

void enter_standby_mode(void);
void wake_up_from_standby(void);
void switch_standby_phase(int next_phase);
void update_standby_cycle(uint32_t now);

void set_robot_expression(const char *expr, bool play_sound) {
    char clean_expr[32];
    int len = strlen(expr);
    if (len >= sizeof(clean_expr)) len = sizeof(clean_expr) - 1;
    for (int i = 0; i < len; i++) {
        clean_expr[i] = tolower(expr[i]);
    }
    clean_expr[len] = '\0';

    // Remove any trailing whitespace / CR / LF
    while (len > 0 && (clean_expr[len - 1] == '\r' || clean_expr[len - 1] == '\n' || clean_expr[len - 1] == ' ')) {
        clean_expr[--len] = '\0';
    }

    strncpy(current_expression, clean_expr, sizeof(current_expression) - 1);

    // 0. Modo Onda Arco-Íris persistente
    if (strcmp(clean_expr, "wave") == 0) {
        is_wave_mode = true;
        ssd1306_expr_happy();
        rgb_led_set(0, 1, 1); // Ciano
        if (play_sound) {
            buzzer_tone(523, 60);
            buzzer_tone(659, 60);
            buzzer_tone(784, 90);
        }
        return;
    } else {
        is_wave_mode = false;
    }

    // 1. Update OLED
    if (strcmp(clean_expr, "happy") == 0) ssd1306_expr_happy();
    else if (strcmp(clean_expr, "sad") == 0) ssd1306_expr_sad();
    else if (strcmp(clean_expr, "angry") == 0) ssd1306_expr_angry();
    else if (strcmp(clean_expr, "surprised") == 0) ssd1306_expr_surprised();
    else if (strcmp(clean_expr, "love") == 0) ssd1306_expr_love();
    else if (strcmp(clean_expr, "confused") == 0) ssd1306_expr_confused();
    else if (strcmp(clean_expr, "thinking") == 0) ssd1306_expr_thinking();
    else if (strcmp(clean_expr, "wink") == 0) ssd1306_expr_wink();
    else if (strcmp(clean_expr, "sleepy") == 0) ssd1306_expr_sleepy();
    else if (strcmp(clean_expr, "blink") == 0) ssd1306_smooth_blink();
    else ssd1306_expr_normal();

    // 2. Update 5x5 Matrix (mouth / icon)
    matrix_set_expression(clean_expr);

    // 3. Update RGB LED
    rgb_led_set_expression(clean_expr);

    // 4. Play sound effect
    if (play_sound && strcmp(clean_expr, "blink") != 0) {
        buzzer_play_emotion(clean_expr);
    }
}

void process_serial_command(const char *cmd) {
    char reply[64] = "OK";

    if (strcmp(cmd, "PING") == 0) {
        buzzer_play_ping();
        set_robot_expression("surprised", false);
        sleep_ms(300);
        set_robot_expression("normal", false);
        printf("PONG:AI_ROBOT_ONLINE\n");
        return;

    } else if (strcmp(cmd, "STATUS") == 0) {
        printf("STATUS:expr=%s,mode=%s\n", current_expression, is_wave_mode ? "WAVE" : (is_in_standby ? "STANDBY" : "USB"));
        return;

    } else if (strcmp(cmd, "WAVE") == 0) {
        set_robot_expression("wave", true);
        printf("OK:WAVE\n");
        return;

    } else if (strcmp(cmd, "STANDBY") == 0 || strcmp(cmd, "IDLE") == 0) {
        enter_standby_mode();
        printf("OK:STANDBY\n");
        return;

    } else if (strncmp(cmd, "EXPR:", 5) == 0) {
        const char *expr = cmd + 5;
        set_robot_expression(expr, true);
        printf("OK:EXPR:%s\n", expr);
        return;

    } else if (strncmp(cmd, "MSG:", 4) == 0) {
        const char *msg = cmd + 4;
        buzzer_play_notification();
        ssd1306_scroll_text(msg);
        set_robot_expression(current_expression, false);
        printf("OK:MSG\n");
        return;

    } else if (strncmp(cmd, "ALL:", 4) == 0) {
        // Format: ALL:emotion:message
        const char *body = cmd + 4;
        const char *colon = strchr(body, ':');
        if (colon != NULL) {
            char expr[32];
            int expr_len = colon - body;
            if (expr_len >= sizeof(expr)) expr_len = sizeof(expr) - 1;
            strncpy(expr, body, expr_len);
            expr[expr_len] = '\0';
            const char *msg = colon + 1;

            set_robot_expression(expr, true);
            sleep_ms(400);
            ssd1306_scroll_text(msg);
            set_robot_expression(expr, false);
            printf("OK:ALL\n");
            return;
        } else {
            set_robot_expression(body, true);
            printf("OK:ALL\n");
            return;
        }

    } else if (strcmp(cmd, "BLINK") == 0) {
        ssd1306_smooth_blink();
        set_robot_expression(current_expression, false);
        printf("OK:BLINK\n");
        return;

    } else if (strcmp(cmd, "FLIP") == 0 || strcmp(cmd, "ROTATE") == 0) {
        normal_display_flipped = !normal_display_flipped;
        if (!is_in_standby) {
            ssd1306_set_orientation(normal_display_flipped);
            set_robot_expression(current_expression, false);
        }
        printf("OK:FLIP:%d\n", normal_display_flipped ? 1 : 0);
        return;

    } else if (strcmp(cmd, "MUMBLE") == 0) {
        buzzer_play_mumble(standby_mumble_variation);
        printf("OK:MUMBLE:%d\n", standby_mumble_variation);
        standby_mumble_variation = (standby_mumble_variation + 1) % 5;
        return;

    } else {
        printf("ERR:Unknown command\n");
    }
}

// =============================================================
// Standby / Idle Mode Implementation
// =============================================================
void switch_standby_phase(int next_phase) {
    standby_phase = next_phase;
    standby_phase_start = to_ms_since_boot(get_absolute_time());
    last_standby_blink_time = standby_phase_start;

    switch (standby_phase) {
        case 0:
            // 1. Normal (Estilo 0 - Onda Arco-Íris na Matriz)
            ssd1306_expr_normal();
            rgb_led_set(0, 1, 0); // Soft green
            break;
        case 1:
            // 2. Feliz (Estilo 1 - Vagalumes Cintilantes)
            ssd1306_expr_happy();
            rgb_led_set(1, 1, 0); // Yellow
            break;
        case 2:
            // 3. Confuso (Estilo 2 - Espiral Giratória)
            ssd1306_expr_confused();
            rgb_led_set(0, 1, 1); // Cyan
            break;
        case 3:
            // 4. Pensando (Estilo 3 - Equalizador Saltitante)
            ssd1306_expr_thinking();
            rgb_led_set(0, 0, 1); // Blue
            break;
        case 4:
            // 5. Piscando (Estilo 4 - Coração Respirando)
            ssd1306_smooth_blink();
            rgb_led_set(1, 0, 1); // Magenta
            break;
        case 5:
            // 6. Mensagem "HELLO!" deslizando no display para a direita
            hello_scroll_x = -100; // Inicia fora da tela à esquerda
            last_hello_step_time = standby_phase_start;
            rgb_led_set(1, 1, 1); // White
            break;
    }
}

void enter_standby_mode(void) {
    if (!is_in_standby) {
        is_in_standby = true;
        matrix_frame_step = 0;
        last_matrix_frame_time = 0;
        uint32_t now = to_ms_since_boot(get_absolute_time());
        last_standby_mumble_time = now;
        standby_mumble_variation = 0;
        // Garante a orientação padrão correta e legível (HELLO, un-mirrored e de pé)
        ssd1306_set_orientation(false);
        switch_standby_phase(0);
        // Primeiro balbucio em volume meio baixo ao iniciar o modo standby
        buzzer_play_mumble(standby_mumble_variation);
        standby_mumble_variation = (standby_mumble_variation + 1) % 5;
    }
}

void wake_up_from_standby(void) {
    if (is_in_standby) {
        is_in_standby = false;
        is_wave_mode = false;
        standby_mumble_variation = 0;
        uint32_t now = to_ms_since_boot(get_absolute_time());
        last_activity_time = now;
        last_blink_time = now;
        // Restaura a orientação da operação normal
        ssd1306_set_orientation(normal_display_flipped);
        buzzer_play_wakeup();
        set_robot_expression("normal", false);
    }
}

void update_standby_cycle(uint32_t now) {
    if (!is_in_standby) return;

    // A. Animar a matriz de LEDs 5x5 a cada 70ms (~14 FPS) com os 5 estilos
    if (now - last_matrix_frame_time >= 70) {
        last_matrix_frame_time = now;
        int style = (standby_phase < 5) ? standby_phase : (int)((matrix_frame_step / 25) % 5);
        matrix_play_idle_style(style, matrix_frame_step);
        matrix_frame_step++;
    }

    // B. Buzzer "Falante" em Volume Meio Baixo:
    // Alterna os padrões de balbucio robótico a cada 2 minutos (120.000 ms)
    if (now - last_standby_mumble_time >= STANDBY_MUMBLE_INTERVAL_MS) {
        last_standby_mumble_time = now;
        buzzer_play_mumble(standby_mumble_variation);
        standby_mumble_variation = (standby_mumble_variation + 1) % 5;
    }

    // C. Atualizar o display e a progressão de fases
    if (standby_phase == 5) {
        // Mensagem "HELLO!" deslizando para a direita até sumir
        if (now - last_hello_step_time >= 22) { // taxa suave de scroll
            last_hello_step_time = now;
            hello_scroll_x += 3; // Desloca para a direita
            ssd1306_draw_hello_frame(hello_scroll_x);

            // Largura do texto: 6 chars * 16px = 96px. Quando hello_scroll_x > 130, sumiu completamente à direita!
            if (hello_scroll_x > 130) {
                switch_standby_phase(0); // Reinicia o ciclo na expressão normal
            }
        }
    } else {
        // Expressões e 5 Estilos de Jogo de Luzes na Matriz 5x5:
        // Alterna entre os estilos a cada 60 segundos (STANDBY_PHASE_DURATION_MS)
        if (now - standby_phase_start >= STANDBY_PHASE_DURATION_MS) {
            switch_standby_phase((standby_phase + 1) % 6);
        } else {
            // Pisca suavemente os olhos a cada 7 segundos para manter o robô vivo
            if (standby_phase != 4 && now - last_standby_blink_time >= 7000) {
                last_standby_blink_time = now;
                ssd1306_smooth_blink();
                switch (standby_phase) {
                    case 0: ssd1306_expr_normal(); break;
                    case 1: ssd1306_expr_happy(); break;
                    case 2: ssd1306_expr_confused(); break;
                    case 3: ssd1306_expr_thinking(); break;
                }
            }
        }
    }
}

int main(void) {
    // 1. Initialize USB Serial
    stdio_init_all();

    // 2. Initialize OLED Display (GPIO 14 SDA, GPIO 15 SCL)
    ssd1306_init();

    // 3. Initialize 5x5 LED Matrix (GPIO 7)
    matrix_init();

    // 4. Initialize RGB LED (GPIO 13 R, GPIO 11 G, GPIO 12 B)
    rgb_led_init();

    // 5. Initialize Buzzer (GPIO 21 PWM)
    buzzer_init();

    // 6. Initialize Buttons
    gpio_init(BTN_A_PIN);
    gpio_set_dir(BTN_A_PIN, GPIO_IN);
    gpio_pull_up(BTN_A_PIN);

    gpio_init(BTN_B_PIN);
    gpio_set_dir(BTN_B_PIN, GPIO_IN);
    gpio_pull_up(BTN_B_PIN);

    // 7. Splash Screen
    ssd1306_clear();
    ssd1306_draw_string(16, 12, "[ EMO BOT ]", true);
    ssd1306_draw_string(8, 28, "BitDogLab Ready", true);
    ssd1306_draw_string(12, 44, "Listening USB...", true);
    ssd1306_show();

    // Startup Chime & Matrix Flash
    matrix_set_all(20, 20, 40);
    buzzer_tone(523, 70); // C5
    buzzer_tone(659, 70); // E5
    buzzer_tone(784, 70); // G5
    buzzer_tone(1046, 140); // C6
    sleep_ms(600);

    // Initial normal expression
    set_robot_expression("normal", false);
    last_blink_time = to_ms_since_boot(get_absolute_time());
    last_activity_time = last_blink_time;

    char rx_buf[256];
    int rx_pos = 0;

    // Main Control Loop
    while (true) {
        uint32_t now = to_ms_since_boot(get_absolute_time());

        // A. Handle Serial Input (Non-blocking)
        int ch = getchar_timeout_us(0);
        while (ch != PICO_ERROR_TIMEOUT) {
            // Qualquer dado recebido acorda imediatamente o robô
            if (is_in_standby) {
                wake_up_from_standby();
            }
            last_activity_time = now;

            if (ch == '\n' || ch == '\r') {
                if (rx_pos > 0) {
                    rx_buf[rx_pos] = '\0';
                    process_serial_command(rx_buf);
                    rx_pos = 0;
                    last_blink_time = now; // Reset idle blink timer
                }
            } else if (rx_pos < sizeof(rx_buf) - 1) {
                rx_buf[rx_pos++] = (char)ch;
            }
            ch = getchar_timeout_us(0);
        }

        // B. Handle Button A: Cycle Expressions manually (or wake up / toggle wave mode)
        if (!gpio_get(BTN_A_PIN)) {
            if (now - last_btn_a_time > 300) { // 300ms debounce
                last_btn_a_time = now;
                last_activity_time = now;
                if (is_in_standby) {
                    wake_up_from_standby();
                } else if (is_wave_mode) {
                    // Se estiver no modo Onda Arco-Íris, sai apenas ao pressionar o botão A novamente!
                    is_wave_mode = false;
                    current_emotion_idx = 0; // Volta para o início (normal)
                    set_robot_expression(EMOTIONS[current_emotion_idx], true);
                    last_blink_time = now;
                } else {
                    current_emotion_idx = (current_emotion_idx + 1) % NUM_EMOTIONS;
                    set_robot_expression(EMOTIONS[current_emotion_idx], true);
                    last_blink_time = now;
                }
            }
        }

        // C. Handle Button B: Trigger Blink & Pip (or wake up)
        if (!gpio_get(BTN_B_PIN)) {
            if (now - last_btn_b_time > 300) {
                last_btn_b_time = now;
                last_activity_time = now;
                if (is_in_standby) {
                    wake_up_from_standby();
                } else if (is_wave_mode) {
                    // No modo Onda Arco-Íris, o botão B dá piscada e bip sem sair do modo
                    buzzer_tone(1500, 50);
                    ssd1306_smooth_blink();
                    ssd1306_expr_happy();
                    last_blink_time = now;
                } else {
                    buzzer_tone(1500, 50);
                    ssd1306_smooth_blink();
                    set_robot_expression(current_expression, false);
                    last_blink_time = now;
                }
            }
        }

        // D. Gestão dos Modos: Onda Arco-Íris persistente vs Standby vs Normal
        if (is_wave_mode) {
            // Anima a Onda Arco-Íris continuamente na matriz 5x5 a cada 60ms
            if (now - last_matrix_frame_time >= 60) {
                last_matrix_frame_time = now;
                matrix_effect_wave(matrix_frame_step++);
            }
            // Auto-blink periódico nos olhos para manter o robô expressivo
            if (now - last_blink_time > blink_interval_ms) {
                ssd1306_smooth_blink();
                ssd1306_expr_happy();
                last_blink_time = now;
                blink_interval_ms = 4000 + (rand() % 4000);
            }
            // Mantém ativo (impede standby enquanto estiver no modo onda arco-íris)
            last_activity_time = now;
        } else if (!is_in_standby) {
            // Se passar 3 minutos sem nenhum comando ou interação de botão, entra em standby
            if (now - last_activity_time >= STANDBY_TIMEOUT_MS) {
                enter_standby_mode();
            } else {
                // Auto-blink autônomo a cada 4-8s durante operação normal
                if (now - last_blink_time > blink_interval_ms) {
                    ssd1306_smooth_blink();
                    set_robot_expression(current_expression, false);
                    last_blink_time = now;
                    blink_interval_ms = 4000 + (rand() % 4000);
                }
            }
        } else {
            // Robô está em standby "brincando consigo mesmo"
            update_standby_cycle(now);
        }

        sleep_ms(10);
    }

    return 0;
}
