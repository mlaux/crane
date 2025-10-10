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
#include "dialog.h"
#include "crane.h"
#include "editor.h"
#include "actions.h"

extern unsigned char far *vga;

char current_filename[13] = "UNTITLED.DAT";

int bg_scroll_x;
int bg_scroll_y;
int displayed_palette;
int tile_library_scroll;
int status_x;
int status_y;
int selected_tile = -1;

int rect_contains(int x0, int y0, int w, int h, int x, int y)
{
    return x >= x0 && y >= y0 && x < x0 + w && y < y0 + h;
}

static void update_status_xy_bg(void)
{
    if (rect_contains(56, 8, 256, 224, cursor_x, cursor_y)) {
        status_x = bg_scroll_x + ((cursor_x - 56) >> 4);
        status_y = bg_scroll_y + ((cursor_y - 8) >> 4);
    }
    fill_rect(0, 232, 40, 8, CONTENT_COLOR);
    drawf(4, 233, "(%2d, %2d)", status_x, status_y);
}

static void upload_ui_palette(void)
{
    int k;
    for (k = 0; k < NUM_UI_COLORS; k++) {
        int base = 3 * k;
        set_palette(FIRST_UI_COLOR + k, ui_colors[base], ui_colors[base + 1],
            ui_colors[base + 2]);
    }
}

static void upload_project_palette(struct project *proj)
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
    int x = x0 + 8, k;

    fill_rect(x0, y0, 136, 8, CONTENT_COLOR);
    draw_char(index + '0', x0, y0 + 1);
    for (k = 0; k < 16; k++) {
        fill_rect(x, y0 + 1, 6, 6, base + k);
        x += 8;
    }
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

void redraw_tile_library_tiles(struct project *proj, int mute)
{
    int k;

    for (k = 0; k < 20; k++) {
        int tile_idx = tile_library_scroll + k;
        int tx = 8 + (k & 1) * 20;
        int ty = 8 + (k >> 1) * 20;
        frame_rect(tx - 1, ty - 1, proj->tile_size + 2, proj->tile_size + 2, CONTENT_COLOR);

        if (tile_idx < proj->num_tiles) {
            draw_project_tile(&proj->tiles[tile_idx], tx, ty, proj->tile_size, mute);
            if (tile_idx == selected_tile && !mute) {
                frame_rect(tx - 1, ty - 1, proj->tile_size + 2, proj->tile_size + 2, HIGHLIGHT_COLOR);
            }
        } else {
            fill_rect(tx, ty, proj->tile_size, proj->tile_size, CONTENT_COLOR);
        }
    }
}

void draw_tile_library(struct project *proj, int mute)
{
    draw_window(4, 4, 44, 224);
    redraw_tile_library_tiles(proj, mute);
}

void draw_bg_tile(struct project *proj, int x0, int y0, int x, int y, int tile_size)
{
    static unsigned char translated[256];
    int k;
    int tile_idx = proj->background.tiles[y + bg_scroll_y][x + bg_scroll_x];

    if (tile_idx >= 0) {
        int pal_idx = proj->background.palettes[y + bg_scroll_y][x + bg_scroll_x];
        struct tile *tile = &proj->tiles[tile_idx];
        int base = FIRST_SNES_COLOR + (pal_idx << 4);

        for (k = 0; k < 256; k++) {
            translated[k] = base + tile->pixels[k];
        }

        draw_sprite(translated, x0 + x * tile_size, y0 + y * tile_size, tile_size, tile_size);
    } else {
        // clear out tile
        fill_rect(x0 + x * tile_size, y0 + y * tile_size, tile_size, tile_size, CONTENT_COLOR);
        // small dot in the middle
        fill_rect(
            x0 + x * tile_size + (tile_size >> 1) - 1,
            y0 + y * tile_size + (tile_size >> 1) - 1,
            2, 2,
            HIGHLIGHT_COLOR
        );
    }
}

void draw_bg_row(struct project *proj, int x0, int y0, int row)
{
    int x;
    int tile_size = proj->tile_size;
    int tiles_x = 256 / tile_size;

    for (x = 0; x < tiles_x; x++) {
        draw_bg_tile(proj, x0, y0, x, row, tile_size);
    }
}

void draw_bg_column(struct project *proj, int x0, int y0, int col)
{
    int y;
    int tile_size = proj->tile_size;
    int tiles_y = 224 / tile_size;

    for (y = 0; y < tiles_y; y++) {
        draw_bg_tile(proj, x0, y0, col, y, tile_size);
    }
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
            int tile_idx = proj->background.tiles[y + bg_scroll_y][x + bg_scroll_x];
            if (tile_idx >= 0) {
                int pal_idx = proj->background.palettes[y + bg_scroll_y][x + bg_scroll_x];
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
                    draw_sprite(
                        translated,
                        x0 + x * tile_size,
                        y0 + y * tile_size,
                        tile_size, tile_size
                    );
                }
            } else {
                fill_rect(
                    x0 + x * tile_size,
                    y0 + y * tile_size,
                    tile_size, tile_size,
                    CONTENT_COLOR
                );
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

void draw_status_bar(const char *text)
{
    fill_rect(0, 232, 320, 8, CONTENT_COLOR);
    drawf(4, 233, "(%2d, %2d) %.20s", status_x, status_y, text);

    draw_snes_palette(180, 232, displayed_palette);
    draw_status_bar_buttons();
}

void draw_entire_screen(struct project *proj)
{
    outpw(SEQ_ADDR, (0x0f << 8) | SEQ_REG_MAP_MASK);
    _fmemset(vga, BACKGROUND_COLOR, 0x8000);

    draw_tile_library(proj, 0);

    draw_window(52, 4, 264, 232);
    draw_project_background(proj, 56, 8, 0);

    draw_status_bar(proj->name);
    draw_tool_buttons();

    show_cursor();
}

int main(int argc, char *argv[])
{
    int x, y;
    int buttons = 0;
    int load_failed = 0;
    static struct project proj;
    char message[48];

    new_project(&proj);

    if (argc > 1) {
        strncpy(current_filename, argv[1], 12);
        current_filename[12] = '\0';
        if (load_project_binary(argv[1], &proj) != 0) {
            load_failed = 1;
        }
    }

    set_mode_x();
    init_mouse();
    upload_ui_palette();
    upload_project_palette(&proj);

    draw_entire_screen(&proj);

    if (load_failed) {
        snprintf(message, 32, "Couldn't load project %s", argv[1]);
        modal_info(message);
    }

    while (1) {
        buttons = poll_mouse(&x, &y);
        wait_vblank();
        if (x != cursor_x || y != cursor_y) {
            move_cursor(x, y);
            update_status_xy_bg();
        }

        if (buttons & 1) {
            handle_tile_clicks(&proj, x, y);
            handle_background_clicks(&proj, x, y);
            handle_palette_clicks(&proj, x, y);
            handle_button_clicks(&proj, x, y);
        }

        if (kbhit() && getch() == 27 
                && modal_confirm("Are you sure you want to exit?")) {
            break;
        }
    }

    set_mode(0x03);
    return 0;
}
