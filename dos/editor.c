#include "editor.h"
#include "vga.h"
#include "palette.h"
#include "mouse.h"
#include "cursor.h"
#include "dialog.h"

static void draw_tile_editor(struct tile *tile, int tile_size, unsigned char *bg_buffer)
{
    int window_size = tile_size * 8 + 8;
    int x0 = (320 - window_size) / 2;
    int y0 = (240 - window_size) / 2;
    int px, py, k;
    int base = FIRST_SNES_COLOR + (tile->preview_palette << 4);

    save_background(x0 - 1, y0 - 1, window_size + 1, window_size + 1, bg_buffer);
    draw_window(x0, y0, window_size, window_size);

    k = 0;
    for (py = 0; py < tile_size; py++) {
        for (px = 0; px < tile_size; px++) {
            int color = base + tile->pixels[k];
            fill_rect(x0 + 4 + px * 8, y0 + 4 + py * 8, 8, 8, color);
            k++;
        }
    }
}

int editor_contains(int x, int y, unsigned char tile_size)
{
    int window_size = tile_size * 8 + 8;
    int x0 = (320 - window_size) / 2;
    int y0 = (240 - window_size) / 2;
    return x >= x0 && y >= y0 && x < x0 + window_size && y < y0 + window_size;
}

static void close_tile_editor(int tile_size, unsigned char *bg_buffer)
{
    int window_size = (tile_size << 3) + 8;
    int x0 = (320 - window_size) >> 1;
    int y0 = (240 - window_size) >> 1;

    restore_background(x0 - 1, y0 - 1, window_size + 1, window_size + 1, bg_buffer);
}

void tile_editor(struct tile *tile, unsigned char tile_size)
{
    int x, y;

    hide_cursor();
    draw_tile_editor(tile, tile_size, dialog_bg_buffer);
    show_cursor();

    while (poll_mouse(&x, &y) & 1);

    while (!(poll_mouse(&x, &y) & 1)) {
        wait_vblank();
        if (x != cursor_x || y != cursor_y) {
            move_cursor(x, y);
        }
    }

    hide_cursor();
    close_tile_editor(tile_size, dialog_bg_buffer);
    show_cursor();

    while (poll_mouse(&x, &y) & 1);
}
