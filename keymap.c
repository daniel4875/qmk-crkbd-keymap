#include QMK_KEYBOARD_H

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
	[0] = LAYOUT_split_3x6_3(KC_TAB, KC_Q, KC_W, KC_E, KC_R, KC_T, KC_Y, KC_U, KC_I, KC_O, KC_P, KC_BSPC, KC_ESC, KC_A, KC_S, KC_D, KC_F, KC_G, KC_H, KC_J, KC_K, KC_L, KC_SCLN, KC_ENT, KC_LSFT, KC_Z, KC_X, KC_C, KC_V, KC_B, KC_N, KC_M, KC_COMM, KC_DOT, KC_SLSH, KC_RSFT, KC_LGUI, MO(1), KC_LCTL, KC_SPC, MO(2), KC_LALT),
	[1] = LAYOUT_split_3x6_3(KC_TAB, KC_1, KC_2, KC_3, KC_4, KC_5, KC_6, KC_7, KC_8, KC_9, KC_0, KC_DEL, KC_ESC, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_LEFT, KC_DOWN, KC_UP, KC_RGHT, KC_NO, KC_INS, KC_LSFT, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_HOME, KC_END, KC_PGUP, KC_PGDN, KC_NO, KC_RSFT, KC_LGUI, KC_TRNS, KC_LCTL, KC_SPC, MO(3), KC_LALT),
	[2] = LAYOUT_split_3x6_3(KC_TAB, KC_EXLM, KC_AT, KC_HASH, KC_DLR, KC_PERC, KC_CIRC, KC_AMPR, KC_ASTR, KC_LPRN, KC_RPRN, KC_NO, KC_ESC, KC_NO, KC_NO, KC_NO, KC_NUBS, KC_GRV, KC_MINS, KC_EQL, KC_LBRC, KC_RBRC, KC_BSLS, KC_QUOT, KC_LSFT, KC_NO, KC_NO, KC_NO, S(KC_NUBS), KC_TILD, KC_UNDS, KC_PLUS, KC_LCBR, KC_RCBR, KC_PIPE, KC_DQUO, KC_LGUI, MO(3), KC_LCTL, KC_SPC, KC_TRNS, KC_LALT),
	[3] = LAYOUT_split_3x6_3(KC_F1, KC_F2, KC_F3, KC_F4, KC_F5, KC_F6, KC_F7, KC_F8, KC_F9, KC_F10, KC_F11, KC_F12, QK_BOOT, KC_NO, KC_NO, KC_NO, KC_NO, KC_BRIU, KC_VOLU, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_BRID, KC_VOLD, KC_MUTE, KC_NO, KC_NO, KC_NO, KC_NO, KC_LGUI, KC_TRNS, KC_LCTL, KC_SPC, KC_TRNS, KC_LALT)
};

#ifdef OLED_ENABLE

// Animation parameters
#define MATRIX_ANIM_FRAME_DURATION 10 // frame duration in ms
#define MATRIX_SPAWN_CHAR_PERCENT 60 // percentage chance of a new character spawning in a given frame
#define MATRIX_REMOVE_CHAR_PERCENT 20 // percentage chance of a character being removed from top of column in a given frame

// RNG parameters
#define RAND_ADD 53
#define RAND_MUL 181
#define RAND_MOD 167

// RNG variables
uint16_t random_value = 157;

// Animation variables
uint32_t matrix_anim_timer = 0;

// Matrix variables
uint8_t next_bottom_of_col[5] = {0};
uint8_t top_of_col[5] = {0};

static uint8_t generate_random_number(uint8_t max_num) {
    // Generate next value in sequence to use as pseudo-random number
    random_value = ((random_value * RAND_MUL) + RAND_ADD) % RAND_MOD;

    // Get first 8 bits of one of the system clocks, TCNT1
    uint8_t clockbyte = TCNT1 % 256;

    // Combine pseudo-random number with clock byte to make a theoretically even-more-random number,
    // then limit to range of 0 to max_num
    return (random_value ^ clockbyte) % max_num;
}

static char generate_random_char(void) {
    static const char matrix_chars[] = "!#$%&()0123456789<>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[]abcdefghijklmnopqrstuvwxyz{}";
    return matrix_chars[generate_random_number(strlen(matrix_chars))];
}

static bool col_without_chars_exists(void) {
    bool exists = false;
    for (uint8_t col = 0; col < 5; col++) {
        if (next_bottom_of_col[col] == 0) {
            exists = true;
        }
    }
    return exists;
}

static bool col_with_chars_without_spaces_at_top_exists(void) {
    bool exists = false;
    for (uint8_t col = 0; col < 5; col++) {
        if ((next_bottom_of_col[col] > 0) && (top_of_col[col] == 0)) {
            exists = true;
        }
    }
    return exists;
}

// Choose random column of OLED that doesn't contain any falling chars
// Note: OLED can fit 5 chars across width, so we have 5 columns
static uint8_t choose_random_col_without_chars(void) {
    uint8_t available_cols[5] = {0};
    uint8_t num_cols_available = 0;
    for (uint8_t col = 0; col < 5; col++) {
        if (next_bottom_of_col[col] == 0) {
            available_cols[num_cols_available] = col;
            num_cols_available++;
        }
    }
    uint8_t rand_num = generate_random_number(num_cols_available);
    return available_cols[rand_num];
}

static uint8_t choose_random_col_with_chars_without_spaces_at_top(void) {
    uint8_t available_cols[5] = {0};
    uint8_t num_cols_available = 0;
    for (uint8_t col = 0; col < 5; col++) {
        if ((next_bottom_of_col[col] > 0) && (top_of_col[col] == 0)) {
            available_cols[num_cols_available] = col;
            num_cols_available++;
        }
    }
    uint8_t rand_num = generate_random_number(num_cols_available);
    return available_cols[rand_num];
}

// Render next frame of matrix digital rain animation
static void render_matrix_digital_rain_frame(void) {
    // Add new char to all columns that already have chars in it and that still have space to add chars to bottom
    // Note: OLED can fit 16 chars along height, so if next bottom of col is 16, it is out of range of screen (0 to 15)
    for (uint8_t col = 0; col < 5; col++) {
        uint8_t next_char_pos = next_bottom_of_col[col];
        if ((next_char_pos > 0) && (next_char_pos < 16)) {
            char new_char = generate_random_char();
            oled_set_cursor(col, next_char_pos);
            oled_write_char(new_char, false);
            next_bottom_of_col[col]++;
        }
    }

    // Remove top char (replace top char with space) for all columns that have chars in them and already have some chars replaced with space
    for (uint8_t col = 0; col < 5; col++) {
        uint8_t next_space_pos = top_of_col[col];
        if (next_space_pos > 0) {
            oled_set_cursor(col, next_space_pos);
            oled_write_char(' ', false);
            top_of_col[col]++;
        }
    }

    // Add new char to random column that hasn't already got chars in it (not every frame, only a chance per frame)
    bool should_add_new_char = generate_random_number(100) <= MATRIX_SPAWN_CHAR_PERCENT;
    if (should_add_new_char && col_without_chars_exists()) {
        char new_char = generate_random_char();
        uint8_t col = choose_random_col_without_chars();
        oled_set_cursor(col, 0);
        oled_write_char(new_char, false);
        next_bottom_of_col[col] = 1;
    }

    // Remove char (replace with space) from top of random column that already has chars in it and that hasn't had
    // any chars removed (not every frame, only a chance per frame)
    bool should_remove_char = generate_random_number(100) <= MATRIX_REMOVE_CHAR_PERCENT;
    if (should_remove_char && col_with_chars_without_spaces_at_top_exists()) {
        uint8_t col = choose_random_col_with_chars_without_spaces_at_top();
        oled_set_cursor(col, 0);
        oled_write_char(' ', false);
        top_of_col[col] = 1;
    }

    // Reset columns that have had all chars removed (replaced with spaces)
    for (int col = 0; col < 5; col++) {
        if (top_of_col[col] == 16) {
            next_bottom_of_col[col] = 0;
            top_of_col[col] = 0;
        }
    }
}

static void render_matrix_digital_rain(void) {
    // Run animation
    if (timer_elapsed32(matrix_anim_timer) > MATRIX_ANIM_FRAME_DURATION) {
        // Set timer to updated time
        matrix_anim_timer = timer_read32();

        // Draw next frame
        render_matrix_digital_rain_frame();
    }

    /* Note:
     * timer_read32() -> gives ms it has been since keyboard was powered on
     * timer_elapsed32(timer) -> gives ms since given time "timer"
     */
}

static void render_layer_state(void) {
    oled_write_P(PSTR("LAYER"), false);
    oled_set_cursor(0, 2);

    switch (get_highest_layer(layer_state)) {
        case 0:
            oled_write_ln_P(PSTR("Base"), false);
            break;
        case 1:
            oled_write_ln_P(PSTR("Num"), false);
            break;
        case 2:
            oled_write_ln_P(PSTR("Sym"), false);
            break;
        case 3:
            oled_write_ln_P(PSTR("Func"), false);
            break;
        default:
            oled_write_ln_P(PSTR("Undef"), false);
            break;
    }
}

static void render_caps_state(void) {
    oled_set_cursor(0, 6);
    oled_write_P(PSTR("CAPS"), false);
    oled_set_cursor(0, 8);
    if (host_keyboard_led_state().caps_lock) {
        oled_write_ln_P(PSTR("On"), false);
    } else {
        oled_write_ln_P(PSTR("Off"), false);
    }
}

static void render_wpm(void) {
    oled_set_cursor(0, 12);
    oled_write_P(PSTR("WPM"), false);
    oled_set_cursor(0, 14);
    oled_write_ln(get_u8_str(get_current_wpm(), '0'), false);
}

static void render_status(void) {
    render_layer_state();
    render_caps_state();
    render_wpm();
}

// Set OLED rotation
oled_rotation_t oled_init_user(oled_rotation_t rotation) {
    return OLED_ROTATION_270;
}

// Draw to OLED
bool oled_task_user(void) {
    // Draw to left OLED
    if (is_keyboard_master()) {
        render_status();
    // Draw to right OLED
    } else {
        render_matrix_digital_rain();
    }
    return false;
}

#endif

