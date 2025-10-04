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

static int approx_h = 0;  // 0-15
static int approx_s = 13; // 0-13
static int approx_l = 7;  // 0-15
static int current_r = 0; // 0-63
static int current_g = 0; // 0-63
static int current_b = 0; // 0-63
static int exact_h_360 = 0;
static int exact_s_100 = 86;
static int exact_l_100 = 50;

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
    int c, x, m;
    int r1, g1, b1;
    int h_sector, h_frac;

    if (s == 0) {
        result.r = result.g = result.b = (l * 63) / 15;
        return result;
    }

    c = (l < 8) ? (l * s * 2) / 15 : ((15 - l) * s * 2) / 15;

    h_sector = (h * 6) / 16;
    h_frac = (h * 6) % 16;

    if (h_sector & 1) {
        x = (c * (16 - h_frac)) / 16;
    } else {
        x = (c * h_frac) / 16;
    }

    m = l - c / 2;

    switch (h_sector) {
        case 0: r1 = c; g1 = x; b1 = 0; break;
        case 1: r1 = x; g1 = c; b1 = 0; break;
        case 2: r1 = 0; g1 = c; b1 = x; break;
        case 3: r1 = 0; g1 = x; b1 = c; break;
        case 4: r1 = x; g1 = 0; b1 = c; break;
        default: r1 = c; g1 = 0; b1 = x; break;
    }

    result.r = ((r1 + m) * 63) / 15;
    result.g = ((g1 + m) * 63) / 15;
    result.b = ((b1 + m) * 63) / 15;

    return result;
}

struct rgb hsl_360_to_rgb(int h, int s, int l) {
    struct rgb result;
    long c, x, m;
    long r1, g1, b1;
    int h_sector;

    if (s == 0) {
        result.r = result.g = result.b = (l * 63) / 100;
        return result;
    }

    c = (l < 50) ? (l * s * 2) / 100 : ((100 - l) * s * 2) / 100;

    h_sector = h / 60;

    x = (c * (60 - ((h_sector & 1) ? (h % 60) : (60 - (h % 60))))) / 60;

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
    int h, s, k, s_scaled;

    k = 0;
    for (s = 0; s < HS_GRID_H; s++) {
        s_scaled = (s * 15) / 13;
        for (h = 0; h < HS_GRID_W; h++) {
            struct rgb color = hsl_to_rgb(h, s_scaled, l);
            set_palette_entry(0x20 + k, color.r, color.g, color.b);
            k++;
        }
    }
}

void generate_lightness_slider(int h, int s) {
    int l;

    for (l = 0; l < L_SLIDER_STEPS; l++) {
        struct rgb color = hsl_to_rgb(h, s, l);
        set_palette_entry(0x10 + l, color.r, color.g, color.b);
    }
}

void update_preview_color(void) {
    set_palette_entry(0x01, current_r, current_g, current_b);
}

unsigned int get_snes_bgr555(void) {
    unsigned int r5 = current_r >> 1;
    unsigned int g5 = current_g >> 1;
    unsigned int b5 = current_b >> 1;
    return (b5 << 10) | (g5 << 5) | r5;
}

void draw_color_info(void) {
    fill_rect(INFO_X, INFO_Y, 120, 32, 0);

    drawf(INFO_X, INFO_Y, "HSL: %d,%d,%d", approx_h, approx_s, approx_l);
    drawf(INFO_X, INFO_Y + 8, "HSL: %d,%d,%d", exact_h_360, exact_s_100, exact_l_100);
    drawf(INFO_X, INFO_Y + 16, "RGB: %d,%d,%d", current_r, current_g, current_b);
    drawf(INFO_X, INFO_Y + 24, "SNES: $%04x", get_snes_bgr555());
}

extern int cursor_x, cursor_y;

int main(void) {
    int mouse_x, mouse_y, buttons;
    int k;
    struct rgb initial_color;

    set_mode_x();
    init_mouse();

    /* initialize current RGB from initial HSL */
    exact_h_360 = (approx_h * 360) / 16;
    exact_s_100 = (approx_s * 100) / 13;
    exact_l_100 = (approx_l * 100) / 15;
    initial_color = hsl_360_to_rgb(exact_h_360, exact_s_100, exact_l_100);
    current_r = initial_color.r;
    current_g = initial_color.g;
    current_b = initial_color.b;

    /* generate initial palettes */
    generate_hs_grid(approx_l);
    generate_lightness_slider(approx_h, approx_s);
    update_preview_color();

    /* clear screen */
    fill_rect(0, 0, 320, 240, 0);

    /* draw HS rectangle */
    for (k = 0; k < HS_GRID_H * HS_GRID_W; k++) {
        int grid_x = k % HS_GRID_W;
        int grid_y = k / HS_GRID_W;
        int x = HS_GRID_X + grid_x * HS_CELL_SIZE;
        int y = HS_GRID_Y + grid_y * HS_CELL_SIZE;
        unsigned char color = 0x20 + k;
        fill_rect(x, y, HS_CELL_SIZE, HS_CELL_SIZE, color);
    }

    /* draw lightness slider */
    for (k = 0; k < L_SLIDER_STEPS; k++) {
        int y = L_SLIDER_Y + k * L_CELL_SIZE;
        unsigned char color = 0x10 + k;
        fill_rect(L_SLIDER_X, y, L_CELL_SIZE, L_CELL_SIZE, color);
    }

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
            if (mouse_x >= HS_GRID_X && mouse_x < HS_GRID_X + HS_GRID_W * HS_CELL_SIZE &&
                mouse_y >= HS_GRID_Y && mouse_y < HS_GRID_Y + HS_GRID_H * HS_CELL_SIZE) {
                long mouse_x_rel = mouse_x - HS_GRID_X;
                long mouse_y_rel = mouse_y - HS_GRID_Y;

                int grid_x = mouse_x_rel / HS_CELL_SIZE;
                int grid_y = mouse_y_rel / HS_CELL_SIZE;
                struct rgb color;

                approx_h = grid_x;
                approx_s = (grid_y * 15) / 13;

                exact_h_360 = (int) ((mouse_x_rel * 360L) / (HS_GRID_W * HS_CELL_SIZE));
                exact_s_100 = (int) ((mouse_y_rel * 100L) / (HS_GRID_H * HS_CELL_SIZE));

                color = hsl_360_to_rgb(exact_h_360, exact_s_100, exact_l_100);
                current_r = color.r;
                current_g = color.g;
                current_b = color.b;

                update_preview_color();
                draw_color_info();
                generate_lightness_slider(approx_h, approx_s);

                for (k = 0; k < L_SLIDER_STEPS; k++) {
                    int y = L_SLIDER_Y + k * L_CELL_SIZE;
                    unsigned char c = 0x10 + k;
                    fill_rect(L_SLIDER_X, y, L_CELL_SIZE, L_CELL_SIZE, c);
                }
            }

            if (mouse_x >= L_SLIDER_X && mouse_x < L_SLIDER_X + L_CELL_SIZE &&
                mouse_y >= L_SLIDER_Y && mouse_y < L_SLIDER_Y + L_SLIDER_STEPS * L_CELL_SIZE) {
                int slider_y = (mouse_y - L_SLIDER_Y) / L_CELL_SIZE;
                struct rgb color;

                approx_l = slider_y;

                exact_l_100 = ((mouse_y - L_SLIDER_Y) * 100) / (L_SLIDER_STEPS * L_CELL_SIZE);

                color = hsl_360_to_rgb(exact_h_360, exact_s_100, exact_l_100);
                current_r = color.r;
                current_g = color.g;
                current_b = color.b;

                update_preview_color();
                draw_color_info();
                generate_hs_grid(approx_l);

                for (k = 0; k < HS_GRID_H * HS_GRID_W; k++) {
                    int grid_x = k % HS_GRID_W;
                    int grid_y = k / HS_GRID_W;
                    int x = HS_GRID_X + grid_x * HS_CELL_SIZE;
                    int y = HS_GRID_Y + grid_y * HS_CELL_SIZE;
                    unsigned char c = 0x20 + k;
                    fill_rect(x, y, HS_CELL_SIZE, HS_CELL_SIZE, c);
                }
            }
        }

        if (kbhit() && getch() == 27)
            break;
    }

    set_mode(0x03);
    return 0;
}
