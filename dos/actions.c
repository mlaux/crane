#include <string.h>
#include "compat.h"
#include "actions.h"
#include "crane.h"
#include "editor.h"
#include "picker.h"
#include "export.h"
#include "dialog.h"
#include "mouse.h"
#include "cursor.h"
#include "vga.h"
#include "palette.h"
#include "icons.h"

#define TIMER_TICKS ((volatile unsigned long far *)0x0040006cL)
#define SCROLL_DELAY 2

static void button_new_tile(struct project *);
static void button_save(struct project *);
static void button_export_palettes(struct project *);
static void button_export_tiles(struct project *);
static void button_export_background(struct project *);
static void button_scroll_up(struct project *);
static void button_scroll_down(struct project *);
static void button_scroll_left(struct project *);
static void button_scroll_right(struct project *);
static void button_decrement_palette(struct project *);
static void button_increment_palette(struct project *);
static void button_tile_library_up(struct project *);
static void button_tile_library_down(struct project *);
static void empty_func(struct project *proj) { }

static struct button tool_buttons[] = {
    { 7, 206, 8, 8, ICON_PLUS, button_new_tile },
    { 17, 206, 8, 8, ICON_MINUS, empty_func },
    { 27, 206, 8, 8, ICON_SCROLL_DOWN, button_tile_library_down },
    { 37, 206, 8, 8, ICON_SCROLL_UP, button_tile_library_up },

    { 7, 216, 8, 8, ICON_SAVE, button_save },
    { 17, 216, 8, 8, -1, button_export_palettes},
    { 27, 216, 8, 8, -1, button_export_tiles },
    { 37, 216, 8, 8, -1, button_export_background },

    { 312, 12, 8, 8, ICON_SCROLL_UP, button_scroll_up },
    { 312, 22, 8, 8, ICON_SCROLL_DOWN, button_scroll_down },
    { 290, 0, 8, 8, ICON_SCROLL_LEFT, button_scroll_left },
    { 300, 0, 8, 8, ICON_SCROLL_RIGHT, button_scroll_right },
};

static struct button status_bar_buttons[] = {
    { 158, 232, 8, 8, ICON_SCROLL_DOWN, button_decrement_palette },
    { 168, 232, 8, 8, ICON_SCROLL_UP, button_increment_palette },
};

static void draw_button_array(struct button *buttons, int num_buttons)
{
    int k;

    for (k = 0; k < num_buttons; k++) {
        fill_rect(buttons[k].x, buttons[k].y, buttons[k].w, buttons[k].h, BUTTON_COLOR);
        if (buttons[k].icon != -1) {
            draw_sprite(icons[buttons[k].icon], buttons[k].x, buttons[k].y, buttons[k].w, buttons[k].h);
        }
    }
}

void draw_tool_buttons(void)
{
    draw_button_array(tool_buttons, sizeof tool_buttons / sizeof tool_buttons[0]);
}

void draw_status_bar_buttons(void)
{
    draw_button_array(status_bar_buttons, sizeof status_bar_buttons / sizeof status_bar_buttons[0]);
}

static void check_button_array(struct button *buttons, int num_buttons, struct project *proj, int x, int y)
{
    int k;
    for (k = 0; k < num_buttons; k++) {
        if (rect_contains(buttons[k].x, buttons[k].y, buttons[k].w, buttons[k].h, x, y)) {
            buttons[k].on_click(proj);
            while (poll_mouse(&x, &y) & 1);
            return;
        }
    }
}

void handle_button_clicks(struct project *proj, int x, int y)
{
    check_button_array(tool_buttons, sizeof tool_buttons / sizeof tool_buttons[0], proj, x, y);
    check_button_array(status_bar_buttons, sizeof status_bar_buttons / sizeof status_bar_buttons[0], proj, x, y);
}

void handle_background_clicks(struct project *proj, int x, int y)
{
    if (selected_tile < 0 || selected_tile >= proj->num_tiles) {
        return;
    }

    if (rect_contains(56, 8, 256, 224, x, y)) {
        int tile_size = proj->tile_size;
        int bg_x = bg_scroll_x + ((x - 56) / tile_size);
        int bg_y = bg_scroll_y + ((y - 8) / tile_size);

        if (bg_x >= 0 && bg_x < 32 && bg_y >= 0 && bg_y < 32) {
            int screen_x = bg_x - bg_scroll_x;
            int screen_y = bg_y - bg_scroll_y;

            proj->background.tiles[bg_y][bg_x] = selected_tile;
            proj->background.palettes[bg_y][bg_x] = proj->tiles[selected_tile].preview_palette;

            hide_cursor();
            draw_bg_tile(proj, 56, 8, screen_x, screen_y, tile_size);
            show_cursor();
        }
    }
}

void handle_tile_clicks(struct project *proj, int x, int y)
{
    static unsigned long last_click_time = 0;
    static int last_clicked_tile = -1;
    unsigned long current_time = *TIMER_TICKS;
    int k;

    for (k = 0; k < 20; k++) {
        int tile_idx = tile_library_scroll + k;
        int tx, ty;

        if (tile_idx >= proj->num_tiles) {
            break;
        }

        tx = 8 + (k & 1) * 20;
        ty = 8 + (k >> 1) * 20;

        if (x >= tx && x < tx + 16 && y >= ty && y < ty + 16) {
            if (tile_idx == last_clicked_tile && (current_time - last_click_time) < 9) {
                // double click, open tile editor
                int bg_x, bg_y;
                int tile_size = proj->tile_size;
                int tiles_x = 256 / tile_size;
                int tiles_y = 224 / tile_size;

                tile_editor(&proj->tiles[tile_idx], proj->tile_size);
                hide_cursor();
                draw_project_tile(&proj->tiles[tile_idx], tx, ty, proj->tile_size, 0);

                for (bg_y = 0; bg_y < tiles_y; bg_y++) {
                    for (bg_x = 0; bg_x < tiles_x; bg_x++) {
                        int world_x = bg_scroll_x + bg_x;
                        int world_y = bg_scroll_y + bg_y;
                        if (world_x < 32 && world_y < 32) {
                            if (proj->background.tiles[world_y][world_x] == tile_idx) {
                                draw_bg_tile(proj, 56, 8, bg_x, bg_y, tile_size);
                            }
                        }
                    }
                }
                show_cursor();
            } else {
                int j;
                hide_cursor();
                for (j = 0; j < 20; j++) {
                    int unhighlight_tx = 8 + (j & 1) * 20;
                    int unhighlight_ty = 8 + (j >> 1) * 20;
                    frame_rect(unhighlight_tx - 1, unhighlight_ty - 1, 
                        proj->tile_size + 2, proj->tile_size + 2, CONTENT_COLOR);
                }
                selected_tile = tile_idx;
                frame_rect(tx - 1, ty - 1, proj->tile_size + 2, 
                        proj->tile_size + 2, HIGHLIGHT_COLOR);
                show_cursor();
            }
            last_clicked_tile = tile_idx;
            last_click_time = current_time;

            // wait for mouse up
            while (poll_mouse(&x, &y) & 1);
            break;
        }
    }
}

static void button_new_tile(struct project *proj)
{
    int k;
    int tile_idx;

    if (proj->num_tiles >= MAX_TILES) {
        modal_info("Can't create any more tiles (max 100)");
        return;
    }

    tile_idx = proj->num_tiles;
    proj->num_tiles++;

    for (k = 0; k < 256; k++) {
        proj->tiles[tile_idx].pixels[k] = 0;
    }
    proj->tiles[tile_idx].preview_palette = displayed_palette;

    tile_editor(&proj->tiles[tile_idx], proj->tile_size);

    hide_cursor();
    redraw_tile_library_tiles(proj, 0);
    show_cursor();
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
    draw_status_bar(result ? "Saved" : "Cancelled");
}

static void button_export_palettes(struct project *proj)
{
    char filename[13];
    int result;
    strcpy(filename, "EXPORT.PAL");

    result = modal_text_input("Export palette as", filename, 13);
    if (result) {
        export_palettes(proj, filename);
    }
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
    draw_status_bar(result ? "Exported background" : "Cancelled");
}

static void button_scroll_up(struct project *proj)
{
    int tile_size = proj->tile_size;
    unsigned long last_tick = 0;

    while ((mouse_buttons_down() & 1) && bg_scroll_y > 0) {
        unsigned long current_tick = *TIMER_TICKS;
        if (current_tick - last_tick >= SCROLL_DELAY) {
            bg_scroll_y--;
            hide_cursor();
            scroll_bg_down(56, 8, 256, 224, tile_size);
            draw_bg_row(proj, 56, 8, 0);
            show_cursor();
            last_tick = current_tick;
        }
    }
}

static void button_scroll_down(struct project *proj)
{
    int tile_size = proj->tile_size;
    int tiles_y = 224 / tile_size;
    unsigned long last_tick = 0;

    while ((mouse_buttons_down() & 1) && bg_scroll_y < 32 - tiles_y) {
        unsigned long current_tick = *TIMER_TICKS;
        if (current_tick - last_tick >= SCROLL_DELAY) {
            bg_scroll_y++;
            hide_cursor();
            scroll_bg_up(56, 8, 256, 224, tile_size);
            draw_bg_row(proj, 56, 8, tiles_y - 1);
            show_cursor();
            last_tick = current_tick;
        }
    }
}

static void button_scroll_left(struct project *proj)
{
    int tile_size = proj->tile_size;
    unsigned long last_tick = 0;

    while ((mouse_buttons_down() & 1) && bg_scroll_x > 0) {
        unsigned long current_tick = *TIMER_TICKS;
        if (current_tick - last_tick >= SCROLL_DELAY) {
            bg_scroll_x--;
            hide_cursor();
            scroll_bg_right(56, 8, 256, 224, tile_size);
            draw_bg_column(proj, 56, 8, 0);
            show_cursor();
            last_tick = current_tick;
        }
    }
}

static void button_scroll_right(struct project *proj)
{
    int tile_size = proj->tile_size;
    int tiles_x = 256 / tile_size;
    unsigned long last_tick = 0;

    while ((mouse_buttons_down() & 1) && bg_scroll_x < 32 - tiles_x) {
        unsigned long current_tick = *TIMER_TICKS;
        if (current_tick - last_tick >= SCROLL_DELAY) {
            bg_scroll_x++;
            hide_cursor();
            scroll_bg_left(56, 8, 256, 224, tile_size);
            draw_bg_column(proj, 56, 8, tiles_x - 1);
            show_cursor();
            last_tick = current_tick;
        }
    }
}

static void button_decrement_palette(struct project *proj)
{
    if (displayed_palette > 0) {
        displayed_palette--;
        draw_snes_palette(180, 232, displayed_palette);
    }
}

static void button_increment_palette(struct project *proj)
{
    if (displayed_palette < 7) {
        displayed_palette++;
        draw_snes_palette(180, 232, displayed_palette);
    }
}

void handle_palette_clicks(struct project *proj, int x, int y)
{
    int k;
    int palette_y = 232;
    int palette_x_start = 188;

    if (y >= palette_y && y < palette_y + 6) {
        for (k = 0; k < 16; k++) {
            int color_x = palette_x_start + k * 8;
            if (x >= color_x && x < color_x + 6) {
                int color_index = displayed_palette * 16 + k;
                struct rgb color = proj->colors[color_index];
                int result;

                hide_cursor();
                draw_tile_library(proj, 1);
                draw_window(52, 4, 264, 232);
                draw_project_background(proj, 56, 8, 1);
                fill_rect(0, 232, 320, 8, CONTENT_COLOR);

                draw_window(PICKER_X - 4, PICKER_Y - 4, PICKER_WIDTH + 8, PICKER_HEIGHT + 8);
                result = color_picker(&color);

                if (result) {
                    proj->colors[color_index] = color;
                    set_palette(FIRST_SNES_COLOR + color_index, color.r, color.g, color.b);
                }

                draw_entire_screen(proj);
                while (poll_mouse(&x, &y) & 1);
                break;
            }
        }
    }
}

static void button_tile_library_up(struct project *proj)
{
    if (tile_library_scroll > 0) {
        tile_library_scroll -= 2;
        if (tile_library_scroll < 0) {
            tile_library_scroll = 0;
        }
        hide_cursor();
        redraw_tile_library_tiles(proj, 0);
        show_cursor();
    }
}

static void button_tile_library_down(struct project *proj)
{
    if (tile_library_scroll + 20 < proj->num_tiles) {
        tile_library_scroll += 2;
        hide_cursor();
        redraw_tile_library_tiles(proj, 0);
        show_cursor();
    }
}
