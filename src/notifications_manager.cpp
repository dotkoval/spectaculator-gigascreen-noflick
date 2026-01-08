#include "font.h"
#include <cstdint>
#include <windows.h>
#include <stdio.h>

// 272 = Small border, 320 = Medium, 352 = Large
#define NOTIFICATION_WIDTH 352*2
#define NOTIFICATION_HEIGHT 10
#define NOTIFICATION_OFFSET 0
// notification delay in frames: divide by 50 to get seconds
#define NOTIFICATION_DELAY 200
#define COLOR_SHADOW 0b0000000000000001
#define COLOR_TEXT 0b1111111111011100

// buffer is double-sized to hide the second line in banner message
unsigned short notification_bar[NOTIFICATION_WIDTH * NOTIFICATION_HEIGHT * 2];  
unsigned int notification_delay = 0;
unsigned int play_banner = 0;
unsigned int full_width = 0;
unsigned int view_width = 0;
bool is_initialized = false;
int scroll = 0;

// print a character glyph row by row (1 row = 8 bits)
void print_char(unsigned char c, int x, int y, WORD color) {
    const unsigned width = NOTIFICATION_WIDTH;
    WORD *dst = (WORD *)notification_bar + x + y * width;

    unsigned char *char_addr = (unsigned char *)(FONT_BITMAP + (c - 32) * 8);
    for (int i = 0; i < 8; i++) {
        unsigned char x = char_addr[i];
        WORD *row = dst + i * width;

        if (x & 0b10000000) row[0] = color;
        if (x & 0b01000000) row[1] = color;
        if (x & 0b00100000) row[2] = color;
        if (x & 0b00010000) row[3] = color;
        if (x & 0b00001000) row[4] = color;
        if (x & 0b00000100) row[5] = color;
        if (x & 0b00000010) row[6] = color;
        if (x & 0b00000001) row[7] = color;
    }
}

// print a string character by character
void print_string(char *str, int cursor_x, int cursor_y) {
    while (*str) {
        // print character shadow first (with an offset)
        print_char(*str, cursor_x + 1, cursor_y + 1, COLOR_SHADOW);
        print_char(*str, cursor_x + 1, cursor_y, COLOR_SHADOW);
        // print actual character over
        print_char(*str, cursor_x, cursor_y, COLOR_TEXT);
        cursor_x += 8;
        str++;
    }
}

// clear the notification bar before printing
void notification_bar_clean() {
    // clear buffer
    memset(notification_bar, 0, sizeof(notification_bar));
}

// helper to convert a mode value to a string
const char *mode_to_string(int value) {
    switch (value) {
        case 0:
            return "Disabled";
        case 1:
            return "2-frame";
        case 2:
            return "2- or 3-frame";
        default:
            return "Unknown";
    }
}

// helper to convert a motion parameter value to a string
const char *motion_to_string(int value) {
    switch (value) {
        case 0:
            return " (fullscreen)";
        case 1:
            return " (adaptive)";
        default:
            return "";
    }
}

void notification_init(int f_width, int v_width, int show_banner, char* version_str) {
    full_width = f_width;
    view_width = v_width;

    if (is_initialized)
        return;

    is_initialized = true;
    notification_bar_clean();

    if (show_banner) {
        play_banner = show_banner;
        char str[128];
        int str_len =
            snprintf(str, sizeof(str), "Gigascreen No-Flick initialized (v%s)", version_str);
        // center position
        print_string(str, view_width - str_len * 4, 1);

        str_len = snprintf(str, sizeof(str), "Press Shift+Tab to cycle anti-flicker modes");
        print_string(str, view_width - str_len * 4, NOTIFICATION_HEIGHT + 1);
        notification_delay = NOTIFICATION_DELAY;
    }
}

void notification_update(int mode, float gamma, float ratio, int motion_check) {
    // do not update the notification bar until initialized or while the startup banner is playing
    if (!is_initialized || play_banner > 0)
        return;

    notification_bar_clean();

    // if the notification is already visible, just extend the delay
    notification_delay = notification_delay > NOTIFICATION_HEIGHT
                             ? NOTIFICATION_DELAY - NOTIFICATION_HEIGHT
                             : NOTIFICATION_DELAY;

    // format the plugin status string
    char str[128];
    snprintf(str, sizeof(str), "Anti-flicker: %s%s", 
             mode_to_string(mode),
             mode ? motion_to_string(motion_check) : "");

    print_string(str, 1, 1);

    snprintf(str, sizeof(str), "| Gamma: %1.1f | Ratio: %d%%", gamma, (int)(ratio * 100));
    print_string(str, view_width * 2 - 26 * 8, 1);
}

void notification_draw(unsigned short *dst) {

    // do not draw the notification bar until initialized, or if the bar is hidden
    if (!is_initialized || notification_delay == 0)
        return;

    // reset the banner animation flag once the bar animation is finished
    if (--notification_delay == 0) {
        play_banner = 0;
        scroll = 0;
    }

    // the notification bar offset is always zero (kept for prototyping other scenarios)
    int start_y = NOTIFICATION_OFFSET;

    // y-position animation while sliding down
    if (notification_delay < NOTIFICATION_HEIGHT + NOTIFICATION_OFFSET)
        start_y = notification_delay - NOTIFICATION_HEIGHT;

    // y-position animation while sliding up
    if (notification_delay > NOTIFICATION_DELAY - NOTIFICATION_HEIGHT)
        start_y = NOTIFICATION_DELAY - notification_delay - NOTIFICATION_HEIGHT;

    // animation logic for the startup banner (two-line message)
    if (play_banner && notification_delay == NOTIFICATION_HEIGHT && scroll < NOTIFICATION_HEIGHT) {
        notification_delay++;
        if (++scroll == NOTIFICATION_HEIGHT)
            notification_delay = NOTIFICATION_DELAY - NOTIFICATION_HEIGHT;
    }

    for (int y = start_y; y <= NOTIFICATION_HEIGHT + start_y; y++) {
        if (y < 0)
            continue;

        for (int x = 0; x < NOTIFICATION_WIDTH; x++) {
            // draw bottom line
            if (y == NOTIFICATION_HEIGHT + start_y) {
                dst[full_width * y + x + 0] = COLOR_TEXT;
                continue;
            }

            int pixel_data = notification_bar[NOTIFICATION_WIDTH * (y - start_y + scroll) + x];

            // simulate transparency
            if (pixel_data == 0) {
                pixel_data = dst[full_width * y + x];
                if ((pixel_data & 0b1000010000010000) == 0) {
                    // case A: bright on dark
                    pixel_data |= 0b0100001000001000;

                } else {
                    // case B: dark on bright
                    int p1 = (pixel_data & 0b1111011111011110) >> 1;
                    int p2 = (pixel_data & 0b1110011110011100) >> 2;
                    pixel_data = p1 | p2;
                }
            }
            dst[full_width * y + x + 0] = pixel_data;
        }
    }
}
