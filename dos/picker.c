#include <stdio.h>
#include <conio.h>
#include <i86.h>
#include "vga.h"
#include "mouse.h"
#include "palette.h"
#include "cursor.h"
#include "project.h"
#include "picker.h"

#define HS_GRID_X PICKER_X
#define HS_GRID_Y PICKER_Y
#define HS_GRID_W 16
#define HS_GRID_H 14
#define HS_CELL_SIZE 8

#define L_SLIDER_X (PICKER_X + 136)
#define L_SLIDER_Y (PICKER_Y + 8)
#define L_SLIDER_STEPS 16
#define L_CELL_SIZE 8

#define PREVIEW_X PICKER_X
#define PREVIEW_Y (PICKER_Y + 120)
#define PREVIEW_W 16
#define PREVIEW_H 24

#define INFO_X (PICKER_X + 40)
#define INFO_Y (PICKER_Y + 120)

extern int cursor_x, cursor_y;

static int cur_r = 0;
static int cur_g = 0;
static int cur_b = 0;
static int cur_h = 0;
static int cur_s = 50;
static int cur_l = 50;

static struct rgb saved_palette[240];

static void save_palette(void)
{
    int k;
    for (k = 0x10; k < 0x100; k++) {
        int index = k - 0x10;
        get_palette(k, 
            &saved_palette[index].r, 
            &saved_palette[index].g, 
            &saved_palette[index].b
        );
    }
}

static void restore_palette(void)
{
    int k;
    for (k = 0x10; k < 0x100; k++) {
        int index = k - 0x10;
        set_palette(k, 
            saved_palette[index].r, 
            saved_palette[index].g, 
            saved_palette[index].b
        );
    }
}

static struct rgb hsl_to_rgb(int h, int s, int l)
{
    struct rgb result;
    long c, x, m;
    long r1, g1, b1;
    int h_sector;
    int use_h;

    if (s == 0) {
        result.r = result.g = result.b = (l * 63) / 100;
        return result;
    }

    if (l < 50) {
        c = (l * s * 2) / 100;
    } else {
        c = ((100 - l) * s * 2) / 100;
    }

    h_sector = h / 60;

    if (h_sector & 1) {
        use_h = (h % 60);
    } else {
        use_h = (60 - (h % 60));
    }

    x = (c * (60 - use_h)) / 60;
    m = l - c / 2;

    switch (h_sector) {
        case 0: r1 = c; g1 = x; b1 = 0; break;
        case 1: r1 = x; g1 = c; b1 = 0; break;
        case 2: r1 = 0; g1 = c; b1 = x; break;
        case 3: r1 = 0; g1 = x; b1 = c; break;
        case 4: r1 = x; g1 = 0; b1 = c; break;
        default: r1 = c; g1 = 0; b1 = x; break;
    }

    result.r = ((r1 + m) * 63) / 100;
    result.g = ((g1 + m) * 63) / 100;
    result.b = ((b1 + m) * 63) / 100;

    return result;
}

static void rgb_to_hsl(struct rgb color, int *h, int *s, int *l)
{
    int r = color.r;
    int g = color.g;
    int b = color.b;
    int max_val, min_val, delta;

    max_val = r;
    if (g > max_val) max_val = g;
    if (b > max_val) max_val = b;

    min_val = r;
    if (g < min_val) min_val = g;
    if (b < min_val) min_val = b;

    *l = ((max_val + min_val) * 100) / (2 * 63);

    delta = max_val - min_val;
    if (delta == 0) {
        *h = 0;
        *s = 0;
        return;
    }

    if (*l < 50) {
        *s = (delta * 100) / (max_val + min_val);
    } else {
        *s = (delta * 100) / (2 * 63 - max_val - min_val);
    }

    if (max_val == r) {
        *h = ((g - b) * 60) / delta;
        if (*h < 0) *h += 360;
    } else if (max_val == g) {
        *h = ((b - r) * 60) / delta + 120;
    } else {
        *h = ((r - g) * 60) / delta + 240;
    }
}

static void generate_hs_grid(int l)
{
    int h, s, k, h_scaled, s_scaled;
    struct rgb color;

    k = 0;
    for (s = 0; s < HS_GRID_H; s++) {
        s_scaled = ((HS_GRID_H - 1 - s) * 100) / (HS_GRID_H - 1);
        for (h = 0; h < HS_GRID_W; h++) {
            h_scaled = (h * 360) / (HS_GRID_W - 1);
            color = hsl_to_rgb(h_scaled, s_scaled, l);
            set_palette(0x20 + k, color.r, color.g, color.b);
            k++;
        }
    }
}

static void draw_hs_grid_px(int x, int y)
{
    static const unsigned char bayer[2][2] = {
        { 0, 2 },
        { 3, 1 },
    };
    int total_w = HS_GRID_W * HS_CELL_SIZE;
    int total_h = HS_GRID_H * HS_CELL_SIZE;

    int grid_x_4 = (x * (HS_GRID_W * 4 - 1)) / (total_w - 1);
    int grid_y_4 = (y * (HS_GRID_H * 4 - 1)) / (total_h - 1);

    int h_cell = grid_x_4 >> 2;
    int s_cell = grid_y_4 >> 2;
    int h_frac = grid_x_4 & 3;
    int s_frac = grid_y_4 & 3;

    int threshold = bayer[y & 1][x & 1];

    int h_use = h_cell;
    int s_use = s_cell;
    unsigned char color;

    if (h_frac > threshold && h_cell < HS_GRID_W - 1) {
        h_use = h_cell + 1;
    }

    if (s_frac > threshold && s_cell < HS_GRID_H - 1) {
        s_use = s_cell + 1;
    }

    color = 0x20 + s_use * HS_GRID_W + h_use;
    put_pixel(HS_GRID_X + x, HS_GRID_Y + y, color);
}

static void draw_hs_grid(void)
{
    int total_w = HS_GRID_W * HS_CELL_SIZE;
    int total_h = HS_GRID_H * HS_CELL_SIZE;

    int x, y;
    for (y = 0; y < total_h; y++) {
        for (x = 0; x < total_w; x++) {
            draw_hs_grid_px(x, y);
        }
    }
}

static void generate_lightness_slider(int h, int s)
{
    int l;

    for (l = 0; l < L_SLIDER_STEPS; l++) {
        int l_scaled = (l * 100) / (L_SLIDER_STEPS - 1);
        struct rgb color = hsl_to_rgb(h, s, l_scaled);
        set_palette(0x10 + l, color.r, color.g, color.b);
    }
}

static void draw_lightness_slider(void)
{
    static const unsigned char bayer[2][2] = {
        { 0, 2 },
        { 3, 1 },
    };
    int x, y;
    int total_h = L_SLIDER_STEPS * L_CELL_SIZE;

    for (y = 0; y < total_h; y++) {
        int grid_y_16 = (y * (L_SLIDER_STEPS * 4 - 1)) / (total_h - 1);
        int l_cell = grid_y_16 >> 2;
        int l_frac = grid_y_16 & 3;

        for (x = 0; x < L_CELL_SIZE; x++) {
            int threshold = bayer[y & 1][x & 1];
            int l_use = l_cell;
            unsigned char color;

            if (l_frac > threshold && l_cell < L_SLIDER_STEPS - 1) {
                l_use = l_cell + 1;
            }

            color = 0x10 + l_use;
            put_pixel(L_SLIDER_X + x, L_SLIDER_Y + y, color);
        }
    }
}

static void update_preview_color(void)
{
    set_palette(COLOR_PICKER_SELECTED_VALUE, cur_r, cur_g, cur_b);
}

static unsigned int get_snes_bgr555(unsigned int r, unsigned int g, unsigned int b)
{
    unsigned int r5 = r >> 1;
    unsigned int g5 = g >> 1;
    unsigned int b5 = b >> 1;
    return (b5 << 10) | (g5 << 5) | r5;
}

static void draw_color_info(void)
{
    fill_rect(INFO_X, INFO_Y, 88, 24, CONTENT_COLOR);

    drawf(INFO_X, INFO_Y, "HSL: %d,%d,%d", cur_h, cur_s, cur_l);
    drawf(INFO_X, INFO_Y + 8, "RGB: %d,%d,%d", cur_r, cur_g, cur_b);
    drawf(INFO_X, INFO_Y + 16, "SNES: $%04x", get_snes_bgr555(cur_r, cur_g, cur_b));
}

static void handle_hs_click(int mouse_x, int mouse_y)
{
    long mouse_x_rel = mouse_x - HS_GRID_X;
    long mouse_y_rel = mouse_y - HS_GRID_Y;
    struct rgb color;

    cur_h = (int) ((mouse_x_rel * 360L) / (HS_GRID_W * HS_CELL_SIZE));
    cur_s = 100 - (int) ((mouse_y_rel * 100L) / (HS_GRID_H * HS_CELL_SIZE));

    color = hsl_to_rgb(cur_h, cur_s, cur_l);
    cur_r = color.r;
    cur_g = color.g;
    cur_b = color.b;

    update_preview_color();
    draw_color_info();
    generate_lightness_slider(cur_h, cur_s);
}

static void handle_l_click(int mouse_y)
{
    struct rgb color;

    cur_l = ((mouse_y - L_SLIDER_Y) * 100) / (L_SLIDER_STEPS * L_CELL_SIZE);

    color = hsl_to_rgb(cur_h, cur_s, cur_l);
    cur_r = color.r;
    cur_g = color.g;
    cur_b = color.b;

    update_preview_color();
    draw_color_info();
    generate_hs_grid(cur_l);
}

void color_picker(struct rgb *color)
{
    int mouse_x, mouse_y, buttons;
    struct rgb temp_out;

    cur_r = color->r;
    cur_g = color->g;
    cur_b = color->b;
    set_palette(COLOR_PICKER_INITIAL_VALUE, cur_r, cur_g, cur_b);

    rgb_to_hsl(*color, &cur_h, &cur_s, &cur_l);

    /* generate initial palettes */
    save_palette();
    generate_hs_grid(cur_l);
    generate_lightness_slider(cur_h, cur_s);
    update_preview_color();

    draw_hs_grid();
    draw_lightness_slider();

    /* draw preview */
    fill_rect(PREVIEW_X, PREVIEW_Y, PREVIEW_W, PREVIEW_H, COLOR_PICKER_INITIAL_VALUE);
    fill_rect(PREVIEW_X + PREVIEW_W, PREVIEW_Y, PREVIEW_W, PREVIEW_H, COLOR_PICKER_SELECTED_VALUE);

    draw_color_info();

    show_cursor();

    while (1) {
        buttons = poll_mouse(&mouse_x, &mouse_y);
        wait_vblank();
        if (mouse_x != cursor_x || mouse_y != cursor_y) {
            move_cursor(mouse_x, mouse_y);
        }

        if (buttons & 1) {
            if (mouse_x >= HS_GRID_X && mouse_x < HS_GRID_X + HS_GRID_W * HS_CELL_SIZE
                    && mouse_y >= HS_GRID_Y && mouse_y <= HS_GRID_Y + HS_GRID_H * HS_CELL_SIZE) {
                handle_hs_click(mouse_x, mouse_y);
            }

            if (mouse_x >= L_SLIDER_X && mouse_x < L_SLIDER_X + L_CELL_SIZE 
                    && mouse_y >= L_SLIDER_Y && mouse_y <= L_SLIDER_Y + L_SLIDER_STEPS * L_CELL_SIZE) {
                handle_l_click(mouse_y);
            }
        }

        if (kbhit() && getch() == 27)
            break;
    }

    temp_out = hsl_to_rgb(cur_h, cur_s, cur_l);
    color->r = temp_out.r;
    color->g = temp_out.g;
    color->b = temp_out.b;
    restore_palette();
}

int standalone_main(void)
{
    struct rgb color;

    set_mode_x();
    init_mouse();

    /* initialize current RGB from initial HSL */
    color = hsl_to_rgb(cur_h, cur_s, cur_l);
    cur_r = color.r;
    cur_g = color.g;
    cur_b = color.b;

    /* clear screen */
    fill_rect(0, 0, 320, 240, 0);

    color_picker(&color);

    set_mode(0x03);
    return 0;
}
