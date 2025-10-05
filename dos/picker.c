#include <stdio.h>
#include <conio.h>
#include <i86.h>
#include "vga.h"
#include "mouse.h"
#include "cursor.h"

#define HS_GRID_X 20
#define HS_GRID_Y 20
#define HS_GRID_W 16
#define HS_GRID_H 14
#define HS_CELL_SIZE 8

#define L_SLIDER_X 160
#define L_SLIDER_Y 20
#define L_SLIDER_STEPS 16
#define L_CELL_SIZE 8

#define PREVIEW_X 20
#define PREVIEW_Y 140
#define PREVIEW_W 40
#define PREVIEW_H 40

#define INFO_X 70
#define INFO_Y 140

static int cur_r = 0;
static int cur_g = 0;
static int cur_b = 0;
static int cur_h = 0;
static int cur_s = 50;
static int cur_l = 50;

struct rgb {
    unsigned char r, g, b;
};

void set_palette_entry(unsigned char index, unsigned char r, unsigned char g, unsigned char b) {
    outp(DAC_WRITE_INDEX, index);
    outp(DAC_DATA, r);
    outp(DAC_DATA, g);
    outp(DAC_DATA, b);
}

struct rgb hsl_to_rgb(int h, int s, int l) {
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

void generate_hs_grid(int l) {
    int h, s, k, h_scaled, s_scaled;
    struct rgb color;

    k = 0;
    for (s = 0; s < HS_GRID_H; s++) {
        s_scaled = ((HS_GRID_H - 1 - s) * 100) / (HS_GRID_H - 1);
        for (h = 0; h < HS_GRID_W; h++) {
            h_scaled = (h * 360) / (HS_GRID_W - 1);
            color = hsl_to_rgb(h_scaled, s_scaled, l);
            set_palette_entry(0x20 + k, color.r, color.g, color.b);
            k++;
        }
    }
}

void draw_hs_grid_dithered(void) {
    static const unsigned char bayer[4][4] = {
        { 0,  8,  2, 10},
        {12,  4, 14,  6},
        { 3, 11,  1,  9},
        {15,  7, 13,  5}
    };
    int x, y;
    int total_w = HS_GRID_W * HS_CELL_SIZE;
    int total_h = HS_GRID_H * HS_CELL_SIZE;

    for (y = 0; y < total_h; y++) {
        for (x = 0; x < total_w; x++) {
            // this is kind of dumb because there aren't enough pixels in the picker
            // to take advantage of all the bayer patterns, but scale it up anyway
            // (x * 255) / 127 = 0-255
            // (y * 223) / 111 = 0-223
            int grid_x_16 = (x * (HS_GRID_W * 16 - 1)) / (total_w - 1);
            int grid_y_16 = (y * (HS_GRID_H * 16 - 1)) / (total_h - 1);

            int h_cell = grid_x_16 >> 4;
            int s_cell = grid_y_16 >> 4;
            int h_frac = grid_x_16 & 15;
            int s_frac = grid_y_16 & 15;

            int bayer_x = x & 3;
            int bayer_y = y & 3;
            int threshold = bayer[bayer_y][bayer_x];

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
    }
}

void generate_lightness_slider(int h, int s) {
    int l;

    for (l = 0; l < L_SLIDER_STEPS; l++) {
        int l_scaled = (l * 100) / (L_SLIDER_STEPS - 1);
        struct rgb color = hsl_to_rgb(h, s, l_scaled);
        set_palette_entry(0x10 + l, color.r, color.g, color.b);
    }
}

void draw_lightness_slider_dithered(void) {
    static const unsigned char bayer[4][4] = {
        { 0,  8,  2, 10},
        {12,  4, 14,  6},
        { 3, 11,  1,  9},
        {15,  7, 13,  5}
    };
    int x, y;
    int total_h = L_SLIDER_STEPS * L_CELL_SIZE;

    for (y = 0; y < total_h; y++) {
        int grid_y_16 = (y * (L_SLIDER_STEPS * 16 - 1)) / (total_h - 1);
        int l_cell = grid_y_16 >> 4;
        int l_frac = grid_y_16 & 15;

        for (x = 0; x < L_CELL_SIZE; x++) {
            int bayer_x = x & 3;
            int bayer_y = y & 3;
            int threshold = bayer[bayer_y][bayer_x];
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

void update_preview_color(void) {
    set_palette_entry(0x01, cur_r, cur_g, cur_b);
}

unsigned int get_snes_bgr555(void) {
    unsigned int r5 = cur_r >> 1;
    unsigned int g5 = cur_g >> 1;
    unsigned int b5 = cur_b >> 1;
    return (b5 << 10) | (g5 << 5) | r5;
}

void draw_color_info(void) {
    fill_rect(INFO_X, INFO_Y, 80, 24, 0);

    drawf(INFO_X, INFO_Y, "HSL: %d,%d,%d", cur_h, cur_s, cur_l);
    drawf(INFO_X, INFO_Y + 8, "RGB: %d,%d,%d", cur_r, cur_g, cur_b);
    drawf(INFO_X, INFO_Y + 16, "SNES: $%04x", get_snes_bgr555());
}

void handle_hs_click(int mouse_x, int mouse_y)
{

    long mouse_x_rel = mouse_x - HS_GRID_X;
    long mouse_y_rel = mouse_y - HS_GRID_Y;

    int grid_x = mouse_x_rel / HS_CELL_SIZE;
    int grid_y = mouse_y_rel / HS_CELL_SIZE;
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

void handle_l_click(int mouse_y)
{               
    int slider_y = (mouse_y - L_SLIDER_Y) / L_CELL_SIZE;
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

extern int cursor_x, cursor_y;

int main(void) {
    int mouse_x, mouse_y, buttons;
    int k;
    struct rgb initial_color;

    set_mode_x();
    init_mouse();

    /* initialize current RGB from initial HSL */
    initial_color = hsl_to_rgb(cur_h, cur_s, cur_l);
    cur_r = initial_color.r;
    cur_g = initial_color.g;
    cur_b = initial_color.b;

    /* generate initial palettes */
    generate_hs_grid(cur_l);
    generate_lightness_slider(cur_h, cur_s);
    update_preview_color();

    /* clear screen */
    fill_rect(0, 0, 320, 240, 0);

    /* draw HS rectangle */
    draw_hs_grid_dithered();

    /* draw lightness slider */
    draw_lightness_slider_dithered();

    /* draw preview */
    fill_rect(PREVIEW_X, PREVIEW_Y, PREVIEW_W, PREVIEW_H, 0x01);

    /* draw color info */
    draw_color_info();

    save_cursor_background();
    /* event loop */
    while (1) {
        buttons = poll_mouse(&mouse_x, &mouse_y);
        wait_vblank();
        if (mouse_x != cursor_x || mouse_y != cursor_y) {
            restore_cursor_background();
            cursor_x = mouse_x;
            cursor_y = mouse_y;
            save_cursor_background();
            draw_cursor();
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

    set_mode(0x03);
    return 0;
}
