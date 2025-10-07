#include <conio.h>
#include "compat.h"
#include "cursor.h"
#include "palette.h"
#include "vga.h"

extern unsigned char far *vga;

int cursor_x = SCREEN_WIDTH / 2;
int cursor_y = SCREEN_HEIGHT / 2;
int cursor_visible = 0;

static unsigned char cursor_buffer[CURSOR_WIDTH * CURSOR_HEIGHT];

static const unsigned char cursor_fish[CURSOR_WIDTH * CURSOR_HEIGHT] = {
    CURSOR_FISH_OUTLINE, CURSOR_FISH_OUTLINE, CURSOR_FISH_OUTLINE, CURSOR_FISH_OUTLINE, 0, 0, 0, 0,
    CURSOR_FISH_OUTLINE, CURSOR_FISH_BODY, B, CURSOR_FISH_BODY, CURSOR_FISH_OUTLINE, 0, 0, 0,
    CURSOR_FISH_OUTLINE, CURSOR_FISH_BODY, CURSOR_FISH_BODY, CURSOR_FISH_BODY, CURSOR_FISH_BODY, CURSOR_FISH_OUTLINE, 0, 0,
    CURSOR_FISH_SHADOW, CURSOR_FISH_BODY, CURSOR_FISH_BODY, CURSOR_FISH_BODY, CURSOR_FISH_BODY, CURSOR_FISH_OUTLINE, 0, 0,
    0, CURSOR_FISH_SHADOW, CURSOR_FISH_BODY, CURSOR_FISH_BODY, CURSOR_FISH_BODY, CURSOR_FISH_OUTLINE, CURSOR_FISH_OUTLINE, CURSOR_FISH_OUTLINE,
    0, 0, CURSOR_FISH_SHADOW, CURSOR_FISH_OUTLINE, CURSOR_FISH_OUTLINE, CURSOR_FISH_OUTLINE, CURSOR_FISH_SHADOW, 0,
    0, 0, 0, 0, CURSOR_FISH_OUTLINE, CURSOR_FISH_SHADOW, 0, 0,
    0, 0, 0, 0, CURSOR_FISH_OUTLINE, 0, 0, 0,
};

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

void save_background(int x0, int y0, int w, int h, unsigned char *buffer)
{
    int x, y, plane;
    int start_plane = x0 & 3;
    int offset, start_offset, out_offset = 0;
    int bytes_per_row = (w >> 2) + (start_plane != 0);

    start_offset = y0 * (SCREEN_WIDTH >> 2) + (x0 >> 2);

    for (plane = 0; plane < 4; plane++) {
        outpw(GC_INDEX, plane << 8 | GC_READ_MAP);

        offset = start_offset;
        for (y = 0; y < h; y++) {
            for (x = 0; x < bytes_per_row; x++) {
                int sprite_x = (x << 2) + plane - start_plane;
                if (sprite_x >= 0 && sprite_x < w) {
                    buffer[out_offset++] = vga[offset + x];
                }
            }
            offset += SCREEN_WIDTH >> 2;
        }
    }
}

void restore_background(int x0, int y0, int w, int h, unsigned char *buffer)
{
    int x, y, plane;
    int start_plane = x0 & 3;
    int offset, start_offset, in_offset = 0;
    int bytes_per_row = (w >> 2) + (start_plane != 0);

    start_offset = y0 * (SCREEN_WIDTH >> 2) + (x0 >> 2);

    for (plane = 0; plane < 4; plane++) {
        outpw(SEQ_ADDR, 1 << (plane + 8) | SEQ_REG_MAP_MASK);

        offset = start_offset;
        for (y = 0; y < h; y++) {
            for (x = 0; x < bytes_per_row; x++) {
                int sprite_x = (x << 2) + plane - start_plane;
                if (sprite_x >= 0 && sprite_x < w) {
                    vga[offset + x] = buffer[in_offset++];
                }
            }
            offset += SCREEN_WIDTH >> 2;
        }
    }
}

void save_cursor_background(void)
{
    save_background(cursor_x, cursor_y, CURSOR_WIDTH, CURSOR_HEIGHT, cursor_buffer);
}

void restore_cursor_background(void)
{
    restore_background(cursor_x, cursor_y, CURSOR_WIDTH, CURSOR_HEIGHT, cursor_buffer);
}

void draw_cursor(void)
{
    draw_sprite(cursor_fish, cursor_x, cursor_y, CURSOR_WIDTH, CURSOR_HEIGHT);
}

void move_cursor(int x, int y)
{
    restore_cursor_background();
    cursor_x = x;
    cursor_y = y;
    save_cursor_background();
    draw_cursor();
}
