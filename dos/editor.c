#include <conio.h>
#include "editor.h"
#include "vga.h"
#include "palette.h"
#include "mouse.h"
#include "cursor.h"
#include "dialog.h"
#include "crane.h"

static void draw_tile_pixels(struct tile *tile, int tile_size)
{
    int window_size = tile_size * 8 + 8;
    int x0 = (320 - window_size) / 2;
    int y0 = (240 - window_size) / 2;
    int px, py, k;
    int base = FIRST_SNES_COLOR + (tile->preview_palette << 4);

    k = 0;
    for (py = 0; py < tile_size; py++) {
        for (px = 0; px < tile_size; px++) {
            int color = base + tile->pixels[k];
            fill_rect(x0 + 4 + px * 8, y0 + 4 + py * 8, 8, 8, color);
            k++;
        }
    }
}

static void draw_tile_editor(struct tile *tile, int tile_size, unsigned char *bg_buffer)
{
    int window_size = tile_size * 8 + 8;
    int x0 = (320 - window_size) / 2;
    int y0 = (240 - window_size) / 2;

    save_background(x0 - 1, y0 - 1, window_size + 1, window_size + 1, bg_buffer);
    draw_window(x0, y0, window_size, window_size);
    draw_tile_pixels(tile, tile_size);
}

static void draw_color_selection(int color_index)
{
    int palette_x = 188 + color_index * 8;
    int palette_y = 233;

    frame_rect(palette_x - 1, palette_y - 1, 8, 8, 0x0f);
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

static void handle_pixel_click(struct tile *tile, int tile_size, int mouse_x, int mouse_y, int current_color)
{
    int window_size = tile_size * 8 + 8;
    int x0 = (320 - window_size) >> 1;
    int y0 = (240 - window_size) >> 1;
    int px = (mouse_x - x0 - 4) >> 3;
    int py = (mouse_y - y0 - 4) >> 3;

    if (px >= 0 && px < tile_size && py >= 0 && py < tile_size) {
        int base = FIRST_SNES_COLOR + (tile->preview_palette << 4);
        int pixel_index = py * tile_size + px;

        tile->pixels[pixel_index] = current_color;
        fill_rect(x0 + 4 + px * 8, y0 + 4 + py * 8, 8, 8, base + current_color);
    }
}

void tile_editor(struct tile *tile, unsigned char tile_size)
{
    int x, y, buttons;
    int current_color = 0;
    int old_displayed_palette = displayed_palette;

    displayed_palette = tile->preview_palette;

    hide_cursor();
    draw_tile_editor(tile, tile_size, dialog_bg_buffer);
    draw_snes_palette(180, 232, displayed_palette);
    draw_color_selection(current_color);
    show_cursor();

    while (poll_mouse(&x, &y) & 1);

    while (1) {
        buttons = poll_mouse(&x, &y);
        wait_vblank();

        if (x != cursor_x || y != cursor_y) {
            move_cursor(x, y);
        }

        if (kbhit() && getch() == 27) {
            break;
        }

        if (buttons & 1) {
            const int palette_y = 233;
            const int palette_x_start = 188;
            int handled = 0;

            if (y >= palette_y && y < palette_y + 6) {
                int k;
                for (k = 0; k < 16; k++) {
                    int color_x = palette_x_start + k * 8;
                    if (x >= color_x && x < color_x + 6) {
                        int old_color = current_color;
                        current_color = k;
                        hide_cursor();
                        frame_rect(palette_x_start + old_color * 8 - 1, palette_y - 1, 8, 8, CONTENT_COLOR);
                        draw_color_selection(current_color);
                        show_cursor();
                        handled = 1;
                        break;
                    }
                }
            }

            // palette up/down buttons. TODO break this out separately
            if (!handled && rect_contains(158, 232, 8, 8, x, y)) {
                if (displayed_palette > 0) {
                    displayed_palette--;
                    tile->preview_palette = displayed_palette;
                    hide_cursor();
                    draw_snes_palette(180, 232, displayed_palette);
                    draw_color_selection(current_color);
                    draw_tile_pixels(tile, tile_size);
                    show_cursor();
                    handled = 1;
                }
            }

            if (!handled && rect_contains(168, 232, 8, 8, x, y)) {
                if (displayed_palette < 7) {
                    displayed_palette++;
                    tile->preview_palette = displayed_palette;
                    hide_cursor();
                    draw_snes_palette(180, 232, displayed_palette);
                    draw_color_selection(current_color);
                    draw_tile_pixels(tile, tile_size);
                    show_cursor();
                    handled = 1;
                }
            }

            if (handled) {
                // only wait for mouse up if it was a button action
                while (poll_mouse(&x, &y) & 1);
            } else if (editor_contains(x, y, tile_size)) {
                // allow dragging for pixel drawing
                hide_cursor();
                handle_pixel_click(tile, tile_size, x, y, current_color);
                show_cursor();
            }
        }
    }

    displayed_palette = old_displayed_palette;

    hide_cursor();
    close_tile_editor(tile_size, dialog_bg_buffer);
    draw_snes_palette(180, 232, displayed_palette);
    show_cursor();
}
