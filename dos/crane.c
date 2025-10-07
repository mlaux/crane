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

extern unsigned char far *vga;

static char current_filename[13] = "UNTITLED.DAT";

struct button {
    int x, y, w, h;
    void (*on_click)(struct project *);
};

static void button_color_picker(struct project *);
static void button_save(struct project *);
static void button_export_palettes(struct project *);
static void button_export_tiles(struct project *);
static void button_export_background(struct project *);

static struct button buttons[] = {
    { 7, 208, 8, 8, button_color_picker },
    { 17, 208, 8, 8, button_save },
    { 27, 208, 8, 8, button_export_palettes },
    { 37, 208, 8, 8, button_export_tiles },
    { 7, 218, 8, 8, button_export_background }
};

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
    int x = x0, k;
    for (k = 0; k < 16; k++) {
        fill_rect(x, y0, 6, 6, base + k);
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

void draw_tile_library(struct project *proj, int mute)
{
    int k;
    draw_window(4, 4, 44, 224);
    for (k = 0; k < proj->num_tiles && k < 20; k++) {
        int tx = 8 + (k & 1) * 20;
        int ty = 8 + (k >> 1) * 20;
        draw_project_tile(&proj->tiles[k], tx, ty, proj->tile_size, mute);
    }
}

static void draw_buttons(void)
{
    int k;

    for (k = 0; k < sizeof(buttons) / sizeof(buttons[0]); k++) {
        fill_rect(buttons[k].x, buttons[k].y, buttons[k].w, buttons[k].h, BUTTON_COLOR);
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

int editor_contains(int x, int y, unsigned char tile_size)
{
    int window_size = tile_size * 8 + 8;
    int x0 = (320 - window_size) / 2;
    int y0 = (240 - window_size) / 2;
    return x >= x0 && y >= y0 && x < x0 + window_size && y < y0 + window_size;
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
                    draw_sprite(
                        translated, 
                        x0 + x * tile_size, 
                        y0 + y * tile_size, 
                        tile_size, tile_size
                    );
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
    draw_buttons();

    // main background editor area
    draw_window(52, 4, 264, 232);
    draw_project_background(proj, 56, 8, 0);

    // status bar
    draw_status_bar(proj->name);

    save_cursor_background();
    draw_cursor();
}

void tile_editor(struct tile *tile, unsigned char tile_size)
{
    static unsigned char editor_bg_buffer[137 * 137];
    int x, y;

    draw_tile_editor(tile, tile_size, editor_bg_buffer);

    while (!(poll_mouse(&x, &y) & 1)) {
        wait_vblank();
        fill_rect(0, 232, 55, 8, CONTENT_COLOR);
        drawf(4, 233, "(%3d, %3d)", cursor_x, cursor_y);
        if (x != cursor_x || y != cursor_y) {
            move_cursor(x, y);
        }
    }

    close_tile_editor(tile_size, editor_bg_buffer);

    if (editor_contains(x, y, tile_size)) {
        // bc last time it was saved, it was with the editor below it, so when
        // the mouse is moved after the editor is closed, a little piece of 
        // the editor is drawn. resave with editor gone. this is still buggy
        // if the cursor is partially within and partially outside the editor. 
        // the color picker sidesteps this issue because it redraws the entire
        // screen when it closes.

        // not really sure what the solution is other than redrawing the tiles
        // intersecting the dialog (which is probably ok)
        save_cursor_background();
        draw_cursor();
    }

    // wait for mouse up
    while (poll_mouse(&x, &y) & 1);
}

void invoke_color_picker(struct project *proj)
{
    struct rgb color = { 51, 1, 34 };
    int x, y;
    restore_cursor_background();

    // draw "muted" (all solid color) version of tiles and background, because
    // it needs pretty much the entire palette for the color picker
    draw_tile_library(proj, 1);
    draw_window(52, 4, 264, 232);
    draw_project_background(proj, 56, 8, 1);
    fill_rect(0, 232, 320, 8, CONTENT_COLOR);

    draw_window(PICKER_X - 4, PICKER_Y - 4, PICKER_WIDTH + 8, PICKER_HEIGHT + 8);
    color_picker(&color);

    draw_entire_screen(proj);
    while (poll_mouse(&x, &y) & 1);
}

static void handle_button_clicks(struct project *proj, int x, int y)
{
    int k;
    for (k = 0; k < sizeof(buttons) / sizeof(buttons[0]); k++) {
        if (rect_contains(buttons[k].x, buttons[k].y, buttons[k].w, buttons[k].h, x, y)) {
            buttons[k].on_click(proj);
            while (poll_mouse(&x, &y) & 1);
            return;
        }
    }
}

static void handle_tile_clicks(struct project *proj, int x, int y)
{
    int k;
    for (k = 0; k < proj->num_tiles && k < 20; k++) {
        int tx = 8 + (k & 1) * 20;
        int ty = 8 + (k >> 1) * 20;
        if (x >= tx && x < tx + 16 && y >= ty && y < ty + 16) {
            tile_editor(&proj->tiles[k], proj->tile_size);
            break;
        }
    }
}

static void button_color_picker(struct project *proj)
{
    invoke_color_picker(proj);
}

static void button_save(struct project *proj)
{
    char filename[13];
    int result;
    strcpy(filename, current_filename);

    result = modal_text_input("Save project as", filename, 13);
    if (result) {
        save_project_binary(filename, proj);
        strcpy(current_filename, filename);
    }
    draw_entire_screen(proj);
    draw_status_bar(result ? "Saved" : "Cancelled");
}

static void button_export_palettes(struct project *proj)
{
    char filename[13];
    int result;
    strcpy(filename, "EXPORT.PAL");

    result = modal_text_input("Enter filename for palette export", filename, 13);
    if (result) {
        export_palettes(proj, filename);
    }
    draw_entire_screen(proj);
    draw_status_bar(result ? "Exported palettes" : "Cancelled");
}

static void button_export_tiles(struct project *proj)
{
    char filename[13];
    int result;
    strcpy(filename, "EXPORT.4BP");

    result = modal_text_input("Export tiles as", filename, 13);
    if (result) {
        export_tiles(proj, filename);
    }
    draw_entire_screen(proj);
    draw_status_bar(result ? "Exported tiles" : "Cancelled");
}

static void button_export_background(struct project *proj)
{
    char filename[13];
    int result;
    strcpy(filename, "EXPORT.MAP");

    result = modal_text_input("Export background as", filename, 13);
    if (result) {
        export_background(proj, filename, 0);
    }
    draw_entire_screen(proj);
    draw_status_bar(result ? "Exported background" : "Cancelled");
}

int main(int argc, char *argv[])
{
    int x, y;
    int buttons = 0;
    int load_failed = 0;
    static struct project proj;

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
        modal_info("Failed to load project");
        draw_entire_screen(&proj);
    }

    while (!kbhit() || getch() != 27) {
        buttons = poll_mouse(&x, &y);
        wait_vblank();
        fill_rect(0, 232, 55, 8, CONTENT_COLOR);
        drawf(4, 233, "(%3d, %3d)", cursor_x, cursor_y);
        if (x != cursor_x || y != cursor_y) {
            move_cursor(x, y);
        }

        if (buttons & 1) {
            handle_tile_clicks(&proj, x, y);
            handle_button_clicks(&proj, x, y);
        }
    }

    set_mode(0x03);
    return 0;
}
