#include <dos.h>
#include <conio.h>
#include <stdlib.h>
#include <string.h>
#include "font.h"
#include "compat.h"
#include "palette.h"
#include "vga.h"
#include "mouse.h"
#include "cursor.h"

extern unsigned char far *vga;

// 16x16 checkerboard for testing
static const unsigned char example_tile[256] = {
    B, W, B, W, B, W, B, W, B, W, B, W, B, W, B, W,
    W, B, W, B, W, B, W, B, W, B, W, B, W, B, W, B,
    B, W, B, W, B, W, B, W, B, W, B, W, B, W, B, W,
    W, B, W, B, W, B, W, B, W, B, W, B, W, B, W, B,
    B, W, B, W, B, W, B, W, B, W, B, W, B, W, B, W,
    W, B, W, B, W, B, W, B, W, B, W, B, W, B, W, B,
    B, W, B, W, B, W, B, W, B, W, B, W, B, W, B, W,
    W, B, W, B, W, B, W, B, W, B, W, B, W, B, W, B,
    B, W, B, W, B, W, B, W, B, W, B, W, B, W, B, W,
    W, B, W, B, W, B, W, B, W, B, W, B, W, B, W, B,
    B, W, B, W, B, W, B, W, B, W, B, W, B, W, B, W,
    W, B, W, B, W, B, W, B, W, B, W, B, W, B, W, B,
    B, W, B, W, B, W, B, W, B, W, B, W, B, W, B, W,
    W, B, W, B, W, B, W, B, W, B, W, B, W, B, W, B,
    B, W, B, W, B, W, B, W, B, W, B, W, B, W, B, W,
    W, B, W, B, W, B, W, B, W, B, W, B, W, B, W, B,
};

unsigned char r, g, b;
void upload_ui_palette(void)
{
    int k;
    for (k = 0; k < NUM_UI_COLORS; k++) {
        int base = 3 * k;
        set_palette(FIRST_UI_COLOR + k, ui_colors[base], ui_colors[base + 1], 
            ui_colors[base + 2]);
    }
}

void draw_snes_palette(int x0, int y0, int index)
{
    int base = FIRST_SNES_COLOR + (index << 4);
    int x = x0, k;
    for (k = 0; k < 16; k++) {
        fill_rect(x, y0, 6, 6, base + k);
        x += 8;
    }
}

void draw_window(int x, int y, int w, int h)
{
    fill_rect(x, y, w, h, CONTENT_COLOR);
    horizontal_line(x - 1, x + w - 2, y - 1, HIGHLIGHT_COLOR);
    vertical_line(x - 1, y - 1, y + h - 2, HIGHLIGHT_COLOR);
}

int main(void) {
    int k, x, y;

    set_mode_x();
    init_mouse();
    wait_vblank();
    upload_ui_palette();

    draw_window(4, 4, 264, 256);

    for (y = 0; y < 14; y++) { 
        for (x = 0; x < 16; x++) {
            draw_sprite_aligned_16x16(example_tile, 8 + x * 16, 8 + y * 16);
        }
    }

    fill_rect(0, 232, 320, 8, CONTENT_COLOR);
    drawf(4, 233, "(%d, %d)", cursor_x, cursor_y);

    draw_char('0', 180, 233);
    draw_snes_palette(188, 233, 0);

    save_cursor_background();
    draw_cursor();

    while (!kbhit() || getch() != 27) {
        poll_mouse(&x, &y);
        wait_vblank();
        fill_rect(0, 232, 55, 8, CONTENT_COLOR);
        drawf(4, 233, "(%d, %d)", cursor_x, cursor_y);
        if (x != cursor_x || y != cursor_y) {
            restore_cursor_background();
            cursor_x = x;
            cursor_y = y;
            save_cursor_background();
            draw_cursor();
        }
    }

    set_mode(0x03);
    return 0;
}
