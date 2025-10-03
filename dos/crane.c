#include <dos.h>
#include <conio.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "font.h"
#include "compat.h"
#include "palette.h"
#include "vga.h"

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240

#define CURSOR_WIDTH 8
#define CURSOR_HEIGHT 8

unsigned char far *vga = (unsigned char far *) 0xa0000000L;

unsigned char cursor_buffer[CURSOR_WIDTH * CURSOR_HEIGHT];
int cursor_x = SCREEN_WIDTH / 2;
int cursor_y = SCREEN_HEIGHT / 2;
int cursor_visible = 0;
unsigned int visible_page = 0;
unsigned int active_page = 0x8000;

#define B 0x10
#define W 0x1f

static const unsigned char cursor_sprite[CURSOR_WIDTH * CURSOR_HEIGHT] = {
    W, W, W, W, W, W, W, W,
    W, B, B, B, B, B, W, 0,
    W, B, B, B, B, W, 0, 0,
    W, B, B, B, W, 0, 0, 0,
    W, B, B, W, 0, 0, 0, 0,
    W, B, W, 0, 0, 0, 0, 0,
    W, W, 0, 0, 0, 0, 0, 0,
    W, 0, 0, 0, 0, 0, 0, 0,
};

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

void wait_vblank(void)
{
    while(inp(INPUT_STATUS_1) & VRETRACE);
    while(!(inp(INPUT_STATUS_1) & VRETRACE));
}

void put_pixel(int x, int y, unsigned char color)
{
    unsigned int offset;
    if (x < 0 || x >= SCREEN_WIDTH || y < 0 || y >= SCREEN_HEIGHT) {
        return;
    }
    // outp(SEQ_ADDR, SEQ_REG_MAP_MASK);
    // outp(SEQ_ADDR + 1, 1 << (x & 3));
    outpw(SEQ_ADDR, 1 << ((x & 3) + 8) | SEQ_REG_MAP_MASK);
    offset = active_page + (y << 6) + (y << 4) + (x >> 2);
    vga[offset] = color;
}

void horizontal_line(int x0, int x1, int y, unsigned char color)
{
    int first_byte = x0 >> 2;
    int last_byte = x1 >> 2;
    int left_mask = 0x0f << (x0 & 3);
    int right_mask = 0x0f >> (3 - (x1 & 3));
    int offset = active_page + (y << 6) + (y << 4) + first_byte;
    int x;

    if (first_byte == last_byte) {
        outpw(SEQ_ADDR, ((left_mask & right_mask) << 8) | SEQ_REG_MAP_MASK);
        vga[offset] = color;
    } else {
        outpw(SEQ_ADDR, (left_mask << 8) | SEQ_REG_MAP_MASK);
        vga[offset++] = color;
        outpw(SEQ_ADDR, 0x0f00 | SEQ_REG_MAP_MASK);
        for (x = first_byte + 1; x < last_byte; x++) {
            vga[offset++] = color;
        }
        outpw(SEQ_ADDR, (right_mask << 8) | SEQ_REG_MAP_MASK);
        vga[offset] = color;
    }
}

void vertical_line(int x, int y0, int y1, unsigned char color)
{
    int x_plane = 1 << (x & 3);
    int y, offset;

    outpw(SEQ_ADDR, x_plane << 8 | SEQ_REG_MAP_MASK);
    for (y = y0; y < y1; y++) {
        offset = active_page + (y << 6) + (y << 4) + (x >> 2);
        vga[offset] = color;
    }
}

void drawf(int x, int y, const char *fmt, ...)
{
    char buf[256];
    va_list args;
    va_start(args, fmt);
    vsprintf(buf, fmt, args);
    va_end(args);
    draw_string(buf, x, y);
}

void frame_rect(int x0, int y0, int width, int height, unsigned char color)
{
    int x1 = x0 + width - 1;
    int y1 = y0 + height - 1;

    vertical_line(x0, y0, y1, color);
    vertical_line(x1, y0, y1, color);

    horizontal_line(x0, x1, y0, color);
    horizontal_line(x0, x1, y1, color);
}

void fill_rect(int x0, int y0, int width, int height, unsigned char color)
{
    int y;
    for (y = y0; y < y0 + height; y++) {
        horizontal_line(x0, x0 + width - 1, y, color);
    }
}

unsigned char get_pixel(int x, int y) {
    unsigned int offset;
    if(x < 0 || x >= SCREEN_WIDTH || y < 0 || y >= SCREEN_HEIGHT)
        return 0;
    offset = y * (SCREEN_WIDTH >> 2) + (x >> 2);
    outp(GC_INDEX, GC_READ_MAP);
    outp(GC_INDEX + 1, x & 3);
    return vga[offset];
}

void save_cursor_background(void) {
    int x, y, plane;
    int start_plane = cursor_x & 3;
    int offset, out_offset = 0;
    int bytes_per_row = (CURSOR_WIDTH >> 2) + (start_plane != 0);

    for(plane = 0; plane < 4; plane++) {
        outp(GC_INDEX, GC_READ_MAP);
        outp(GC_INDEX + 1, plane);

        offset = cursor_y * (SCREEN_WIDTH >> 2) + (cursor_x >> 2);
        for(y = 0; y < CURSOR_HEIGHT; y++) {
            for(x = 0; x < bytes_per_row; x++) {
                int sprite_x = (x << 2) + plane - start_plane;
                if(sprite_x >= 0 && sprite_x < CURSOR_WIDTH) {
                    cursor_buffer[out_offset++] = vga[offset + x];
                }
            }
            offset += SCREEN_WIDTH >> 2;
        }
    }
}

void restore_cursor_background(void) {
    int x, y, plane;
    int start_plane = cursor_x & 3;
    int offset, in_offset = 0;
    int bytes_per_row = (CURSOR_WIDTH >> 2) + (start_plane != 0);

    for(plane = 0; plane < 4; plane++) {
        outp(SEQ_ADDR, SEQ_REG_MAP_MASK);
        outp(SEQ_ADDR + 1, 1 << plane);

        offset = cursor_y * (SCREEN_WIDTH >> 2) + (cursor_x >> 2);
        for(y = 0; y < CURSOR_HEIGHT; y++) {
            for(x = 0; x < bytes_per_row; x++) {
                int sprite_x = (x << 2) + plane - start_plane;
                if(sprite_x >= 0 && sprite_x < CURSOR_WIDTH) {
                    vga[offset + x] = cursor_buffer[in_offset++];
                }
            }
            offset += SCREEN_WIDTH >> 2;
        }
    }
}

void draw_sprite(const unsigned char *data, int sx, int sy, int width, int height)
{
    int x, y, plane;
    int start_plane = sx & 3;
    unsigned int offset, in_offset, start_offset;
    int bytes_per_row = (width >> 2) + (start_plane != 0);

    start_offset = active_page + sy * (SCREEN_WIDTH >> 2) + (sx >> 2);

    for (plane = 0; plane < 4; plane++) {
        outpw(SEQ_ADDR, 1 << (plane + 8) | SEQ_REG_MAP_MASK);

        offset = start_offset;
        in_offset = 0;
        for (y = 0; y < height; y++) {
            for (x = 0; x < bytes_per_row; x++) {
                int sprite_x = (x << 2) + plane - start_plane;
                if (sprite_x >= 0 && sprite_x < width && sx + sprite_x < SCREEN_WIDTH) {
                    int use = in_offset + sprite_x;
                    if (data[use] != 0) {
                        vga[offset + x] = data[use];
                    }
                }
            }
            in_offset += width;
            offset += SCREEN_WIDTH >> 2;
        }
    }
}

void draw_sprite_aligned_16x16(const unsigned char *data, int sx, int sy)
{
    int y, plane;
    unsigned int offset, start_offset;
    const unsigned char *in_ptr;

    start_offset = active_page + sy * (SCREEN_WIDTH >> 2) + (sx >> 2);

    for (plane = 0; plane < 4; plane++) {
        outpw(SEQ_ADDR, 1 << (plane + 8) | SEQ_REG_MAP_MASK);

        in_ptr = data + plane;
        offset = start_offset;
        for (y = 0; y < 16; y++) {
            unsigned int row_offset = offset;
            int x;
            for (x = 0; x < 4; x++) {
                if (*in_ptr != 0) {
                    vga[row_offset] = *in_ptr;
                }
                in_ptr += 4;
                row_offset++;
            }
            offset += SCREEN_WIDTH >> 2;
        }
    }
}

void draw_cursor(void)
{
    draw_sprite(cursor_sprite, cursor_x, cursor_y, CURSOR_WIDTH, CURSOR_HEIGHT);
}

extern const struct bitmap_font font;

void draw_char(unsigned char uch, int x, int y)
{
    unsigned short char_index;
    const unsigned char *glyph_data;
    int k;

    if (uch >= font.Chars)
        return;

    char_index = font.Index[uch];
    glyph_data = &font.Bitmap[char_index * font.Height];

    for (k = 0; k < font.Height; k++) {
        unsigned char row = glyph_data[k];
        int bit;
        for (bit = 7; bit >= 3; bit--) {
            if (row & (1 << bit)) {
                put_pixel(x + (7 - bit), y + k, 0x0f);
            }
        }
    }
}

void draw_string(const char *str, int x, int y)
{
    int offset = 0;
    while (*str) {
        draw_char(*str, x + offset, y);
        offset += font.Width;
        str++;
    }
}

void init_mouse(void)
{
    union REGS regs;
    regs.x.ax = 0x00;
    int86(0x33, &regs, &regs);

    regs.x.ax = 0x08;
    regs.x.cx = 0;
    regs.x.dx = SCREEN_HEIGHT - 1;
    int86(0x33, &regs, &regs);
}

int poll_mouse(int *x, int *y)
{
    union REGS regs;
    regs.x.ax = 0x03;
    int86(0x33, &regs, &regs);
    *x = regs.x.cx >> 1;
    *y = regs.x.dx;
    return regs.x.bx;
}

unsigned char r, g, b;
void flip_pages(void)
{
    unsigned int temp;

    temp = visible_page;
    visible_page = active_page;
    active_page = temp;

    while (inp(INPUT_STATUS_1) & VRETRACE);
    // now not in vblank
    _disable();
    outp(CRTC_INDEX, CRTC_START_ADDR_HI);
    outp(CRTC_INDEX + 1, (visible_page >> 8) & 0xff);
    outp(CRTC_INDEX, CRTC_START_ADDR_LO);
    outp(CRTC_INDEX + 1, visible_page & 0xff);
    _enable();

    while (!(inp(INPUT_STATUS_1) & VRETRACE));
    // now in vblank
    //while (inp(INPUT_STATUS_1) & VRETRACE);
}

void upload_ui_palette(void)
{
    int k;
    for (k = 0; k < NUM_UI_COLORS; k++) {
        int base = 3 * k;
        set_palette(FIRST_UI_COLOR + k, ui_colors[base], ui_colors[base + 1], 
            ui_colors[base + 2]);
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

int main(void) {
    int k, x, y;

    set_mode_x();
    init_mouse();
    wait_vblank();
    upload_ui_palette();

    while (!kbhit() || getch() != 27) {
        poll_mouse(&cursor_x, &cursor_y);

        outp(SEQ_ADDR, SEQ_REG_MAP_MASK);
        outp(SEQ_ADDR + 1, 0x0f);
        _fmemset(&vga[active_page], BACKGROUND_COLOR, 19200);

        for (k = 0; k < 8; k++) {
            draw_char(k + '0', 8, 8 + (k * 8));
            draw_snes_palette(14, 8 + (k * 8), k);
        }

        for (y = 0; y < 8; y++) { 
            for (x = 0; x < 8; x++) {
                draw_sprite_aligned_16x16(example_tile, 8 + x * 16, 80 + y * 16);
            }
        }

        fill_rect(0, 232, 320, 8, CONTENT_COLOR);
        //draw_window(4, 72, 316, 168);
        drawf(4, 233, "(%d, %d)", cursor_x, cursor_y);

        //fill_rect(100, 100, 50, 50, 0x98);
        //fill_rect(101, 101, 50, 50, 0xc8);

        // do other drawing here
        draw_cursor();

        flip_pages();
    }

    set_mode(0x03);
    return 0;
}
