#include <string.h>
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

static void button_color_picker(struct project *);
static void button_save(struct project *);
static void button_export_palettes(struct project *);
static void button_export_tiles(struct project *);
static void button_export_background(struct project *);
static void button_scroll_up(struct project *);
static void button_scroll_down(struct project *);
static void button_scroll_left(struct project *);
static void button_scroll_right(struct project *);

static struct button tool_buttons[] = {
    { 7, 208, 8, 8, button_color_picker },
    { 17, 208, 8, 8, button_save },
    { 27, 208, 8, 8, button_export_palettes },
    { 37, 208, 8, 8, button_export_tiles },
    { 7, 218, 8, 8, button_export_background },

    { 312, 12, 8, 8, button_scroll_up },
    { 312, 22, 8, 8, button_scroll_down },
    { 290, 0, 8, 8, button_scroll_left },
    { 300, 0, 8, 8, button_scroll_right },
};

void draw_buttons(void)
{
    int k;

    for (k = 0; k < sizeof(tool_buttons) / sizeof(tool_buttons[0]); k++) {
        fill_rect(tool_buttons[k].x, tool_buttons[k].y, tool_buttons[k].w, tool_buttons[k].h, BUTTON_COLOR);
    }
}

void handle_button_clicks(struct project *proj, int x, int y)
{
    int k;
    for (k = 0; k < sizeof(tool_buttons) / sizeof(tool_buttons[0]); k++) {
        if (rect_contains(tool_buttons[k].x, tool_buttons[k].y, tool_buttons[k].w, tool_buttons[k].h, x, y)) {
            tool_buttons[k].on_click(proj);
            while (poll_mouse(&x, &y) & 1);
            return;
        }
    }
}

void handle_tile_clicks(struct project *proj, int x, int y)
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
    struct rgb color = { 51, 1, 34 };
    int x, y;
    hide_cursor();

    draw_tile_library(proj, 1);
    draw_window(52, 4, 264, 232);
    draw_project_background(proj, 56, 8, 1);
    fill_rect(0, 232, 320, 8, CONTENT_COLOR);

    draw_window(PICKER_X - 4, PICKER_Y - 4, PICKER_WIDTH + 8, PICKER_HEIGHT + 8);
    color_picker(&color);

    draw_entire_screen(proj);
    while (poll_mouse(&x, &y) & 1);
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

static void refresh_bg(struct project *proj)
{
    hide_cursor();
    draw_project_background(proj, 56, 8, 0);
    show_cursor();
}

static void button_scroll_up(struct project *proj)
{
    if (bg_scroll_y > 0) {
        bg_scroll_y--;
        refresh_bg(proj);
    }
}

static void button_scroll_down(struct project *proj)
{
    if (bg_scroll_y < 18) {
        bg_scroll_y++;
        refresh_bg(proj);
    }
}

static void button_scroll_left(struct project *proj)
{
    if (bg_scroll_x > 0) {
        bg_scroll_x--;
        refresh_bg(proj);
    }
}

static void button_scroll_right(struct project *proj)
{
    if (bg_scroll_x < 16) {
        bg_scroll_x++;
        refresh_bg(proj);
    }
}
