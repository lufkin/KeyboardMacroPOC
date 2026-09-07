/*
 * Pico USB Keyboard Macro Injector - Native TinyUSB Implementation
 * 
 * Minimal firmware for Raspberry Pi Pico WH as a USB HID keyboard.
 * Single button on GP15 triggers a two-line test macro with controlled pacing.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "pico/stdlib.h"
#include "pico/unique_id.h"

#include "bsp/board.h"
#include "tusb.h"

/* ===== Configuration ===== */
#define BUTTON_PIN 15              /* GP15 */
#define DEBOUNCE_MS 30
#define KEY_PRESS_MS 25            /* Hold key down for 25 ms */
#define KEY_RELEASE_MS 75          /* Hold key up for 75 ms (total 100 ms per char) */
#define ENTER_PRESS_MS 25          /* Hold Enter down for 25 ms */
#define LINE_WAIT_MS 1000          /* Pause 1000 ms after Enter release */

/* Macro text to send */
static const char *macro_text = "/echo Pico macro test";
static const int macro_line_count = 2;

/* ===== State Machine ===== */
typedef enum {
    STATE_IDLE,
    STATE_CHAR_PRESS,
    STATE_CHAR_RELEASE,
    STATE_ENTER_PRESS,
    STATE_ENTER_RELEASE,
    STATE_LINE_WAIT
} macro_state_t;

static macro_state_t macro_state = STATE_IDLE;
static int current_line = 0;
static size_t current_char_index = 0;
static uint32_t step_start_ms = 0;
static bool button_was_pressed = false;

/* Keycode and modifier for current character */
static uint8_t current_modifier = 0;
static uint8_t current_keycode = 0;

/* US ASCII to USB HID Keycode mapping */
typedef struct {
    char ascii;
    uint8_t keycode;
    uint8_t modifier;  /* 0 for no shift, 0x02 for left shift */
} key_map_entry_t;

static const key_map_entry_t key_map[] = {
    {'a', 0x04, 0x00}, {'b', 0x05, 0x00}, {'c', 0x06, 0x00}, {'d', 0x07, 0x00},
    {'e', 0x08, 0x00}, {'f', 0x09, 0x00}, {'g', 0x0A, 0x00}, {'h', 0x0B, 0x00},
    {'i', 0x0C, 0x00}, {'j', 0x0D, 0x00}, {'k', 0x0E, 0x00}, {'l', 0x0F, 0x00},
    {'m', 0x10, 0x00}, {'n', 0x11, 0x00}, {'o', 0x12, 0x00}, {'p', 0x13, 0x00},
    {'q', 0x14, 0x00}, {'r', 0x15, 0x00}, {'s', 0x16, 0x00}, {'t', 0x17, 0x00},
    {'u', 0x18, 0x00}, {'v', 0x19, 0x00}, {'w', 0x1A, 0x00}, {'x', 0x1B, 0x00},
    {'y', 0x1C, 0x00}, {'z', 0x1D, 0x00},
    {'0', 0x27, 0x00}, {'1', 0x1E, 0x00}, {'2', 0x1F, 0x00}, {'3', 0x20, 0x00},
    {'4', 0x21, 0x00}, {'5', 0x22, 0x00}, {'6', 0x23, 0x00}, {'7', 0x24, 0x00},
    {'8', 0x25, 0x00}, {'9', 0x26, 0x00},
    {' ', 0x2C, 0x00}, {'-', 0x2D, 0x00}, {'=', 0x2E, 0x00},
    {'[', 0x2F, 0x00}, {']', 0x30, 0x00}, {'\\', 0x31, 0x00},
    {';', 0x33, 0x00}, {'\'', 0x34, 0x00}, {'`', 0x35, 0x00},
    {',', 0x36, 0x00}, {'.', 0x37, 0x00}, {'/', 0x38, 0x00},
    {0, 0, 0}  /* End marker */
};

static uint8_t ascii_to_hid(char c, uint8_t *out_modifier) {
    bool is_upper = false;
    if (c >= 'A' && c <= 'Z') {
        is_upper = true;
        c = c - 'A' + 'a';
    }

    for (int i = 0; key_map[i].ascii != 0; i++) {
        if (key_map[i].ascii == c) {
            *out_modifier = is_upper ? 0x02 : key_map[i].modifier;
            return key_map[i].keycode;
        }
    }
    *out_modifier = 0;
    return 0;  /* No mapping */
}

static bool send_press(uint8_t modifier, uint8_t keycode) {
    if (!tud_hid_ready()) {
        return false;
    }
    uint8_t keys[6] = { keycode, 0, 0, 0, 0, 0 };
    return tud_hid_keyboard_report(0, modifier, keys);
}

static bool send_release(void) {
    if (!tud_hid_ready()) {
        return false;
    }
    return tud_hid_keyboard_report(0, 0, NULL);
}

/* ===== Main Macro Sender State Machine ===== */
static void macro_process(void) {
    uint32_t now_ms = board_millis();

    switch (macro_state) {
        case STATE_IDLE:
            break;

        case STATE_CHAR_PRESS: {
            if (current_char_index < strlen(macro_text)) {
                char c = macro_text[current_char_index];
                current_keycode = ascii_to_hid(c, &current_modifier);

                if (current_keycode != 0) {
                    if (send_press(current_modifier, current_keycode)) {
                        step_start_ms = now_ms;
                        macro_state = STATE_CHAR_RELEASE;
                    }
                } else {
                    /* Skip unmapped characters */
                    current_char_index++;
                }
            } else {
                /* Finished typing line characters -> Send Enter */
                macro_state = STATE_ENTER_PRESS;
            }
            break;
        }

        case STATE_CHAR_RELEASE: {
            if (now_ms - step_start_ms >= KEY_PRESS_MS) {
                if (send_release()) {
                    current_char_index++;
                    step_start_ms = now_ms;
                    macro_state = (current_char_index < strlen(macro_text))
                                  ? STATE_CHAR_PRESS
                                  : STATE_ENTER_PRESS;
                }
            }
            break;
        }

        case STATE_ENTER_PRESS: {
            if (now_ms - step_start_ms >= KEY_RELEASE_MS) {
                if (send_press(0, 0x28)) {  /* 0x28 = Enter */
                    step_start_ms = now_ms;
                    macro_state = STATE_ENTER_RELEASE;
                }
            }
            break;
        }

        case STATE_ENTER_RELEASE: {
            if (now_ms - step_start_ms >= ENTER_PRESS_MS) {
                if (send_release()) {
                    step_start_ms = now_ms;
                    current_line++;
                    if (current_line < macro_line_count) {
                        macro_state = STATE_LINE_WAIT;
                    } else {
                        /* Macro finished */
                        macro_state = STATE_IDLE;
                        current_line = 0;
                        current_char_index = 0;
                    }
                }
            }
            break;
        }

        case STATE_LINE_WAIT: {
            if (now_ms - step_start_ms >= LINE_WAIT_MS) {
                current_char_index = 0;
                macro_state = STATE_CHAR_PRESS;
            }
            break;
        }

        default:
            macro_state = STATE_IDLE;
            break;
    }
}

/* ===== Button Handling ===== */
static void button_check(void) {
    bool button_pressed = !gpio_get(BUTTON_PIN);  /* Active low */

    if (button_pressed && !button_was_pressed) {
        /* Rising edge: debounce and start macro */
        sleep_ms(DEBOUNCE_MS);
        if (!gpio_get(BUTTON_PIN) && macro_state == STATE_IDLE) {
            macro_state = STATE_CHAR_PRESS;
            current_line = 0;
            current_char_index = 0;
            step_start_ms = board_millis();
        }
        button_was_pressed = true;
    } else if (!button_pressed && button_was_pressed) {
        button_was_pressed = false;
    }
}

/* ===== TinyUSB Callbacks ===== */

void tud_hid_report_complete_cb(uint8_t instance, uint8_t const *report, uint16_t len) {
    /* Report sent; macro state machine will send next key on timer */
}

uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id,
                                hid_report_type_t report_type, uint8_t *buffer,
                                uint16_t reqlen) {
    /* Not used for keyboard */
    return 0;
}

void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id,
                            hid_report_type_t report_type, uint8_t const *buffer,
                            uint16_t buflen) {
    /* LED state can be handled here if needed */
}

/* ===== Main ===== */
int main(void) {
    board_init();
    stdio_init_all();

    /* Initialize button as input with pull-up */
    gpio_init(BUTTON_PIN);
    gpio_set_dir(BUTTON_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_PIN);

    /* Initialize TinyUSB */
    tusb_init();

    printf("Pico USB Keyboard Macro Injector\n");
    printf("Button on GP15, two-line test macro\n");

    /* Main loop */
    while (1) {
        tud_task();  /* TinyUSB device task */
        button_check();
        macro_process();
    }

    return 0;
}
