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
#include "picker.h"
#include "export.h"

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

void draw_project_tile(struct tile *tile, int x, int y, int tile_size, int mute)
{
    static unsigned char translated[256];
    int base = FIRST_SNES_COLOR + (tile->preview_palette << 4);
    int k;
    for (k = 0; k < 256; k++) {
        translated[k] = base + tile->pixels[k];
    }
    if (mute) {
        fill_rect(x, y, tile_size, tile_size, HIGHLIGHT_COLOR);
    } else {
        draw_sprite(translated, x, y, tile_size, tile_size);
    }
}

void draw_tile_library(struct project *proj, int mute)
{
    int k;
    draw_window(4, 4, 44, 224);
    for (k = 0; k < proj->num_tiles && k < 20; k++) {
        int tx = 8 + (k & 1) * 20;
        int ty = 8 + (k >> 1) * 20;
        draw_project_tile(&proj->tiles[k], tx, ty, proj->tile_size, mute);
    }
    fill_rect(8, 208, 8, 8, HIGHLIGHT_COLOR);
    fill_rect(18, 208, 8, 8, BUTTON_COLOR);
    fill_rect(28, 208, 8, 8, BUTTON_COLOR);
    fill_rect(38, 208, 8, 8, BUTTON_COLOR);
    fill_rect(8, 218, 8, 8, BUTTON_COLOR);
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
    int window_size = (tile_size << 3) + 8;
    int x0 = (320 - window_size) >> 1;
    int y0 = (240 - window_size) >> 1;

    restore_background(x0 - 1, y0 - 1, window_size + 1, window_size + 1, bg_buffer);
}

void draw_project_background(struct project *proj, int x0, int y0, int mute)
{
    static unsigned char translated[256];
    int x, y, k;
    int tile_size = proj->tile_size;
    int tiles_x = 256 / tile_size;
    int tiles_y = 224 / tile_size;

    for (y = 0; y < tiles_y && y < 32; y++) {
        for (x = 0; x < tiles_x && x < 32; x++) {
            int tile_idx = proj->background.tiles[y + 18][x];
            if (tile_idx >= 0) {
                int pal_idx = proj->background.palettes[y + 18][x];
                struct tile *tile = &proj->tiles[tile_idx];
                int base = FIRST_SNES_COLOR + (pal_idx << 4);

                for (k = 0; k < 256; k++) {
                    translated[k] = base + tile->pixels[k];
                }

                if (mute) {
                    fill_rect(
                        x0 + x * tile_size,
                        y0 + y * tile_size,
                        tile_size, tile_size, 
                        HIGHLIGHT_COLOR
                    );
                } else {
                    draw_sprite(translated, x0 + x * tile_size, y0 + y * tile_size, tile_size, tile_size);
                }
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

int rect_contains(int x0, int y0, int w, int h, int x, int y)
{
    return x >= x0 && y >= y0 && x < x0 + w && y < y0 + h;
}

void draw_status_bar(const char *text)
{
    fill_rect(0, 232, 320, 8, CONTENT_COLOR);
    drawf(4, 233, "(%3d, %3d) %.20s", cursor_x, cursor_y, text);

    // active palette
    draw_char('0', 180, 233);
    draw_snes_palette(188, 233, 0);
}

void draw_entire_screen(struct project *proj)
{
    outpw(SEQ_ADDR, (0x0f << 8) | SEQ_REG_MAP_MASK);
    _fmemset(vga, BACKGROUND_COLOR, 0x8000);

    // tile library
    draw_tile_library(proj, 0);

    // main background editor area
    draw_window(52, 4, 264, 232);
    draw_project_background(proj, 56, 8, 0);

    fill_rect(8, 208, 8, 8, HIGHLIGHT_COLOR);

    // status bar
    draw_status_bar(proj->name);

    save_cursor_background();
    draw_cursor();
}

int main(int argc, char *argv[])
{
    int x, y, k;
    int buttons = 0;
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
    wait_vblank();
    upload_project_palette(&proj);

    draw_entire_screen(&proj);

    while (!kbhit() || getch() != 27) {
        buttons = poll_mouse(&x, &y);
        wait_vblank();
        fill_rect(0, 232, 55, 8, CONTENT_COLOR);
        drawf(4, 233, "(%3d, %3d)", cursor_x, cursor_y);
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
                    break;
                }
            }

            if (rect_contains(8, 208, 8, 8, x, y)) {
                struct rgb color;
                restore_cursor_background();
                draw_tile_library(&proj, 1);
                draw_window(52, 4, 264, 232);
                draw_project_background(&proj, 56, 8, 1);
                fill_rect(0, 232, 320, 8, CONTENT_COLOR);
                draw_window(PICKER_X - 4, PICKER_Y - 4, PICKER_WIDTH + 8, PICKER_HEIGHT + 8);
                color_picker(&color);
                draw_entire_screen(&proj);
            }

            if (rect_contains(18, 208, 8, 8, x, y)) {
                save_project_binary("PROJECT.DAT", &proj);
                draw_status_bar("Saved");
                while (poll_mouse(&x, &y) & 1);
            }

            if (rect_contains(28, 208, 8, 8, x, y)) {
                export_palettes(&proj, "EXPORT.PAL");
                while (poll_mouse(&x, &y) & 1);
            }

            if (rect_contains(38, 208, 8, 8, x, y)) {
                export_tiles(&proj, "EXPORT.4BP");
                while (poll_mouse(&x, &y) & 1);
            }

            if (rect_contains(8, 218, 8, 8, x, y)) {
                export_background(&proj, "EXPORT.MAP", 0);
                while (poll_mouse(&x, &y) & 1);
            }
        }
    }
exit_loop:

    set_mode(0x03);
    return 0;
}
