#include <dos.h>
#include <conio.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "vga.h"
#include "font.h"
#include "compat.h"
#include "palette.h"
#include "mouse.h"
#include "cursor.h"
#include "project.h"

extern unsigned char far *vga;

/*
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
*/

void upload_ui_palette(void)
{
    int k;
    for (k = 0; k < NUM_UI_COLORS; k++) {
        int base = 3 * k;
        set_palette(FIRST_UI_COLOR + k, ui_colors[base], ui_colors[base + 1],
            ui_colors[base + 2]);
    }
}

void upload_project_palette(struct project *proj)
{
    int k;
    for (k = 0; k < NUM_COLORS; k++) {
        set_palette(FIRST_SNES_COLOR + k, proj->colors[k].r, proj->colors[k].g,
            proj->colors[k].b);
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

void draw_project_tile(struct tile *tile, int x, int y, int tile_size)
{
    static unsigned char translated[256];
    int base = FIRST_SNES_COLOR + (tile->preview_palette << 4);
    int k;
    for (k = 0; k < 256; k++) {
        translated[k] = base + tile->pixels[k];
    }
    draw_sprite(translated, x, y, tile_size, tile_size);
}

void draw_tile_library(struct project *proj)
{
    int k;
    draw_window(4, 4, 44, 224);
    for (k = 0; k < proj->num_tiles && k < 20; k++) {
        int tx = 8 + (k & 1) * 20;
        int ty = 8 + (k >> 1) * 20;
        draw_project_tile(&proj->tiles[k], tx, ty, proj->tile_size);
    }
}

void draw_tile_editor(struct tile *tile, int tile_size, unsigned char *bg_buffer)
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

void close_tile_editor(int tile_size, unsigned char *bg_buffer)
{
    int window_size = tile_size * 8 + 8;
    int x0 = (320 - window_size) / 2;
    int y0 = (240 - window_size) / 2;

    restore_background(x0 - 1, y0 - 1, window_size + 1, window_size + 1, bg_buffer);
}

void draw_project_background(struct project *proj, int x0, int y0)
{
    static unsigned char translated[256];
    int x, y, k;
    int tile_size = proj->tile_size;
    int tiles_x = 256 / tile_size;
    int tiles_y = 224 / tile_size;

    for (y = 0; y < tiles_y && y < 32; y++) {
        for (x = 0; x < tiles_x && x < 32; x++) {
            int tile_idx = proj->background.tiles[y + 0][x];
            if (tile_idx >= 0) {
                int pal_idx = proj->background.palettes[y + 0][x];
                struct tile *tile = &proj->tiles[tile_idx];
                int base = FIRST_SNES_COLOR + (pal_idx << 4);

                for (k = 0; k < 256; k++) {
                    translated[k] = base + tile->pixels[k];
                }

                draw_sprite(translated, x0 + x * tile_size, y0 + y * tile_size, tile_size, tile_size);
            } else {
                fill_rect(
                    x0 + x * tile_size + (tile_size >> 1) - 1, 
                    y0 + y * tile_size + (tile_size >> 1) - 1, 
                    2, 2, 
                    HIGHLIGHT_COLOR
                );
            }
        }
    }
}

int main(int argc, char *argv[])
{
    int x, y, k;
    static struct project proj;
    static unsigned char editor_bg_buffer[137 * 137];

    new_project(&proj);

    if (argc > 1 && load_project_binary(argv[1], &proj) != 0) {
        printf("Failed to load project\n");
        printf("Press any key to continue...\n");
        getch();
    }

    set_mode_x();
    init_mouse();
    wait_vblank();
    upload_ui_palette();
    _fmemset(vga, BACKGROUND_COLOR, 0x8000);
    wait_vblank();
    upload_project_palette(&proj);

    // tile library
    draw_tile_library(&proj);

    // main background editor area
    draw_window(52, 4, 264, 232);
    draw_project_background(&proj, 56, 8);

    // status bar
    fill_rect(0, 232, 320, 8, CONTENT_COLOR);
    drawf(4, 233, "(%d, %d) %s", cursor_x, cursor_y, proj.name);

    // active palette
    draw_char('0', 180, 233);
    draw_snes_palette(188, 233, 0);

    save_cursor_background();

    while (!kbhit() || getch() != 27) {
        int buttons = poll_mouse(&x, &y);
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

        if (buttons & 1) {
            for (k = 0; k < proj.num_tiles && k < 20; k++) {
                int tx = 8 + (k & 1) * 20;
                int ty = 8 + (k >> 1) * 20;
                if (x >= tx && x < tx + 16 && y >= ty && y < ty + 16) {
                    restore_cursor_background();
                    draw_tile_editor(&proj.tiles[k], proj.tile_size, editor_bg_buffer);
                    while (poll_mouse(&x, &y) & 1);
                    while (!(poll_mouse(&x, &y) & 1) && !kbhit());
                    if (kbhit() && getch() == 27) {
                        goto exit_loop;
                    }
                    close_tile_editor(proj.tile_size, editor_bg_buffer);
                    save_cursor_background();
                    draw_cursor();
                    break;
                }
            }
        }
    }
exit_loop:

    set_mode(0x03);
    return 0;
}
