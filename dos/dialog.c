#include <string.h>
#include <conio.h>

#include "cursor.h"
#include "vga.h"
#include "palette.h"
#include "mouse.h"
#include "dialog.h"

#define DIALOG_WIDTH 200
#define DIALOG_HEIGHT 40

#define DIALOG_X ((SCREEN_WIDTH / 2) - (DIALOG_WIDTH / 2))
#define DIALOG_Y ((SCREEN_HEIGHT / 2) - (DIALOG_HEIGHT / 2))
unsigned char dialog_bg_buffer[DIALOG_BG_BUFFER_SIZE];

int modal_text_input(const char *prompt, char *value, int value_len)
{
    int mouse_x, mouse_y, buttons, frame = 0;
    int cursor_pos = strlen(value);
    int result = 0;
    int button_y = DIALOG_Y + 24;
    int cancel_x = DIALOG_X + DIALOG_WIDTH - 68;
    int ok_x = DIALOG_X + DIALOG_WIDTH - 68 + 10 * 5;
    int text_changed = 0;

    hide_cursor();
    save_background(DIALOG_X - 1, DIALOG_Y - 1, DIALOG_WIDTH + 1, DIALOG_HEIGHT + 1, dialog_bg_buffer);
    draw_window(DIALOG_X, DIALOG_Y, DIALOG_WIDTH, DIALOG_HEIGHT);
    show_cursor();

    while (1) {
        if (kbhit()) {
            int ch = getch();
            if (ch == 27) {
                result = 0;
                break;
            }
            if (ch == '\r' || ch == '\n') {
                result = 1;
                break;
            }
            if (ch == '\b') {
                if (cursor_pos > 0) {
                    cursor_pos--;
                    value[cursor_pos] = '\0';
                    text_changed = 1;
                }
            } else if (ch >= 32 && ch < 127) {
                if (cursor_pos < value_len - 1) {
                    value[cursor_pos] = ch;
                    cursor_pos++;
                    value[cursor_pos] = '\0';
                    text_changed = 1;
                }
            }
        }

        buttons = poll_mouse(&mouse_x, &mouse_y);

        if (buttons & 1) {
            if (mouse_y >= button_y && mouse_y < button_y + 7) {
                if (mouse_x >= cancel_x && mouse_x < cancel_x + 30) {
                    result = 0;
                    break;
                }
                if (mouse_x >= ok_x && mouse_x < ok_x + 10) {
                    result = 1;
                    break;
                }
            }
        }

        wait_vblank();

        if (mouse_x != cursor_x || mouse_y != cursor_y) {
            move_cursor(mouse_x, mouse_y);
        }

        if (text_changed) {
            fill_rect(DIALOG_X + 8, DIALOG_Y + 23, (value_len - 1) * 5 + 1, 9, CONTENT_COLOR);
            text_changed = 0;
        }

        draw_string(prompt, DIALOG_X + 8, DIALOG_Y + 8);
        draw_string(value, DIALOG_X + 8, DIALOG_Y + 24);
        horizontal_line(DIALOG_X + 8, DIALOG_X + 8 + ((value_len - 1) * 5), DIALOG_Y + 32, 0x0f);
        if ((frame % 30) >= 15) {
            vertical_line(DIALOG_X + 8 + 5 * cursor_pos, DIALOG_Y + 23, DIALOG_Y + 31, 0x0f);
        } else {
            vertical_line(DIALOG_X + 8 + 5 * cursor_pos, DIALOG_Y + 23, DIALOG_Y + 31, CONTENT_COLOR);
        }
        draw_string("Cancel    OK", DIALOG_X + DIALOG_WIDTH - 68, DIALOG_Y + 24);

        frame++;
    }

    while (mouse_buttons_down() & 1);

    hide_cursor();
    restore_background(DIALOG_X - 1, DIALOG_Y - 1, DIALOG_WIDTH + 1, DIALOG_HEIGHT + 1, dialog_bg_buffer);
    show_cursor();

    return result;
}

int modal_confirm(const char *message)
{
    int mouse_x, mouse_y, buttons;
    int result = 0;
    int button_y = DIALOG_Y + 24;
    int cancel_x = DIALOG_X + DIALOG_WIDTH - 68;
    int ok_x = DIALOG_X + DIALOG_WIDTH - 68 + 10 * 5;

    hide_cursor();
    save_background(DIALOG_X - 1, DIALOG_Y - 1, DIALOG_WIDTH + 1, DIALOG_HEIGHT + 1, dialog_bg_buffer);
    draw_window(DIALOG_X, DIALOG_Y, DIALOG_WIDTH, DIALOG_HEIGHT);
    draw_string(message, DIALOG_X + 8, DIALOG_Y + 8);
    draw_string("Cancel    OK", cancel_x, button_y);
    show_cursor();

    while (1) {
        if (kbhit()) {
            int ch = getch();
            if (ch == 27) {
                result = 0;
                break;
            }
            if (ch == '\r' || ch == '\n') {
                result = 1;
                break;
            }
        }

        buttons = poll_mouse(&mouse_x, &mouse_y);

        if (buttons & 1) {
            if (mouse_y >= button_y && mouse_y < button_y + 7) {
                if (mouse_x >= cancel_x && mouse_x < cancel_x + 30) {
                    result = 0;
                    break;
                }
                if (mouse_x >= ok_x && mouse_x < ok_x + 10) {
                    result = 1;
                    break;
                }
            }
        }

        wait_vblank();

        if (mouse_x != cursor_x || mouse_y != cursor_y) {
            move_cursor(mouse_x, mouse_y);
        }
    }

    while (mouse_buttons_down() & 1);

    hide_cursor();
    restore_background(DIALOG_X - 1, DIALOG_Y - 1, DIALOG_WIDTH + 1, DIALOG_HEIGHT + 1, dialog_bg_buffer);
    show_cursor();

    return result;
}

int modal_three_option(
    const char *message,
    const char *option1,
    const char *option2,
    const char *option3
) {
    int mouse_x, mouse_y, buttons;
    int result = 0;
    const int button_y = DIALOG_Y + 24;
    int w1 = 5 * strlen(option1);
    int w2 = 5 * strlen(option2);
    int w3 = 5 * strlen(option3);
    int total_width = w1 + w2 + w3 + 40;
    int start_x = DIALOG_X + (DIALOG_WIDTH - total_width) / 2;
    int opt1_x = start_x;
    int opt2_x = start_x + w1 + 20;
    int opt3_x = start_x + w1 + 20 + w2 + 20;

    hide_cursor();
    save_background(DIALOG_X - 1, DIALOG_Y - 1, DIALOG_WIDTH + 1, DIALOG_HEIGHT + 1, dialog_bg_buffer);
    draw_window(DIALOG_X, DIALOG_Y, DIALOG_WIDTH, DIALOG_HEIGHT);
    draw_string(message, DIALOG_X + 8, DIALOG_Y + 8);
    draw_string(option1, opt1_x, button_y);
    draw_string(option2, opt2_x, button_y);
    draw_string(option3, opt3_x, button_y);
    show_cursor();

    while (1) {
        if (kbhit()) {
            int ch = getch();
            if (ch == 27) {
                result = 0;
                break;
            }
        }

        buttons = poll_mouse(&mouse_x, &mouse_y);

        if (buttons & 1) {
            if (mouse_y >= button_y && mouse_y < button_y + 7) {
                if (mouse_x >= opt1_x && mouse_x < opt1_x + w1) {
                    result = 0;
                    break;
                }
                if (mouse_x >= opt2_x && mouse_x < opt2_x + w2) {
                    result = 1;
                    break;
                }
                if (mouse_x >= opt3_x && mouse_x < opt3_x + w3) {
                    result = 2;
                    break;
                }
            }
        }

        wait_vblank();

        if (mouse_x != cursor_x || mouse_y != cursor_y) {
            move_cursor(mouse_x, mouse_y);
        }
    }

    while (mouse_buttons_down() & 1);

    hide_cursor();
    restore_background(DIALOG_X - 1, DIALOG_Y - 1, DIALOG_WIDTH + 1, DIALOG_HEIGHT + 1, dialog_bg_buffer);
    show_cursor();

    return result;
}

void modal_info(const char *message)
{
    int mouse_x, mouse_y, buttons;
    int button_y = DIALOG_Y + 24;
    int ok_x = DIALOG_X + DIALOG_WIDTH - 18;

    hide_cursor();
    save_background(DIALOG_X - 1, DIALOG_Y - 1, DIALOG_WIDTH + 1, DIALOG_HEIGHT + 1, dialog_bg_buffer);
    draw_window(DIALOG_X, DIALOG_Y, DIALOG_WIDTH, DIALOG_HEIGHT);
    draw_string(message, DIALOG_X + 8, DIALOG_Y + 8);
    draw_string("OK", DIALOG_X + DIALOG_WIDTH - 18, DIALOG_Y + 24);
    show_cursor();

    while (1) {
        if (kbhit()) {
            int ch = getch();
            if (ch == 27 || ch == '\r' || ch == '\n') {
                break;
            }
        }

        buttons = poll_mouse(&mouse_x, &mouse_y);

        if (buttons & 1) {
            if (mouse_y >= button_y && mouse_y < button_y + 7) {
                if (mouse_x >= ok_x && mouse_x < ok_x + 10) {
                    break;
                }
            }
        }

        wait_vblank();

        if (mouse_x != cursor_x || mouse_y != cursor_y) {
            move_cursor(mouse_x, mouse_y);
        }
    }

    while (mouse_buttons_down() & 1);

    hide_cursor();
    restore_background(DIALOG_X - 1, DIALOG_Y - 1, DIALOG_WIDTH + 1, DIALOG_HEIGHT + 1, dialog_bg_buffer);
    show_cursor();
}
