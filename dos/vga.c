#include <conio.h>
#include <stdarg.h>
#include <stdio.h>

#include "compat.h"
#include "vga.h"
#include "font.h"

unsigned char far *vga = (unsigned char far *) 0xa0000000L;

extern const struct bitmap_font font;

void put_pixel(int x, int y, unsigned char color)
{
    if (x < 0 || x >= SCREEN_WIDTH || y < 0 || y >= SCREEN_HEIGHT) {
        return;
    }
    outpw(SEQ_ADDR, 1 << ((x & 3) + 8) | SEQ_REG_MAP_MASK);
    vga[(y << 6) + (y << 4) + (x >> 2)] = color;
}

unsigned char get_pixel(int x, int y) {
    if(x < 0 || x >= SCREEN_WIDTH || y < 0 || y >= SCREEN_HEIGHT) {
        return 0;
    }
    outp(GC_INDEX, GC_READ_MAP);
    outp(GC_INDEX + 1, x & 3);
    return vga[(y << 6) + (y << 4) + (x >> 2)];
}

void horizontal_line(int x0, int x1, int y, unsigned char color)
{
    int first_byte = x0 >> 2;
    int last_byte = x1 >> 2;
    int left_mask = 0x0f << (x0 & 3);
    int right_mask = 0x0f >> (3 - (x1 & 3));
    int offset = (y << 6) + (y << 4) + first_byte;
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
    int y;

    outpw(SEQ_ADDR, x_plane << 8 | SEQ_REG_MAP_MASK);
    for (y = y0; y < y1; y++) {
        vga[(y << 6) + (y << 4) + (x >> 2)] = color;
    }
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

void drawf(int x, int y, const char *fmt, ...)
{
    char buf[256];
    va_list args;
    va_start(args, fmt);
    vsprintf(buf, fmt, args);
    va_end(args);
    draw_string(buf, x, y);
}

void draw_sprite(const unsigned char *data, int sx, int sy, int width, int height)
{
    int x, y, plane;
    int start_plane = sx & 3;
    unsigned int offset, in_offset, start_offset;
    int bytes_per_row = (width >> 2) + (start_plane != 0);

    start_offset = sy * (SCREEN_WIDTH >> 2) + (sx >> 2);

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

    start_offset = sy * (SCREEN_WIDTH >> 2) + (sx >> 2);

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
