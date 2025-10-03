#include <dos.h>
#include <conio.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "font.h"

#define VIDEO_INT 0x10
#define INPUT_STATUS_1 0x03da
#define VRETRACE 0x08
#define SEQ_ADDR 0x03c4
#define SEQ_REG_RESET 0
#define SEQ_REG_CLK_MODE 1
#define SEQ_REG_MAP_MASK 2
#define SEQ_REG_CHARMAP_SELECT 3
#define SEQ_REG_MEM_MODE 4
#define SEQ_REG_READ_MAP 4

#define GC_INDEX 0x03ce
#define GC_READ_MAP 4

#define CRTC_INDEX 0x03d4
#define MISC_OUTPUT 0x03c2
#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240

#define CURSOR_WIDTH 8
#define CURSOR_HEIGHT 8

#define CRTC_V_TOTAL 0x06
#define CRTC_OVERFLOWS 0x07
#define CRTC_MAX_SCAN_LINE 0x09
#define CRTC_START_ADDR_HI 0x0c
#define CRTC_START_ADDR_LO 0x0d
#define CRTC_V_RETRACE_START 0x10
#define CRTC_V_RETRACE_END 0x11
#define CRTC_V_DISPLAY_END 0x12
#define CRTC_UNDERLINE_LOC 0x14
#define CRTC_V_BLANK_START 0x15
#define CRTC_V_BLANK_END 0x16
#define CRTC_MODE_CONTROL 0x17

#define DAC_READ_INDEX 0x3c7
#define DAC_WRITE_INDEX 0x3c8
#define DAC_DATA 0x3c9

// to get rid of syntax errors in vscode
#ifndef __WATCOMC__
#define far
union REGS {
    struct {
        unsigned short ax, bx, cx, dx;
    } x;
    struct {
        unsigned char al, ah, bl, bh, cl, ch, dl, dh;
    } h;
};
#endif

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

// 00 means transparent so not using it for black
// 01-0f are my UI colors
// 10-1f are stock VGA grays (10 = black, 1f = white)
// 20-37 is a "fully saturated" rainbow (blue to blue)
// 38-4f is a lighter rainbow
// 50-67 is an even lighter rainbow
// 68-7f is a dark rainbow
// 80-ff are SNES palettes 0-7, converted from RGB565 to RGB666
//         (red and blue scaled by 2)

static const unsigned char ui_colors[] = {
    14, 14, 28, // this is stock color 0x80
    8, 8, 16,   // 0xc8
    20, 20, 28, // 0x98
};

#define NUM_UI_COLORS (sizeof ui_colors / (3 * sizeof ui_colors[0]))
#define FIRST_UI_COLOR 0x1
#define BACKGROUND_COLOR 0x1
#define CONTENT_COLOR 0x2
#define HIGHLIGHT_COLOR 0x3

#define FIRST_SNES_COLOR 0x80

// 16x16 checkerboard for testing
// unsigned char cursor_sprite[CURSOR_WIDTH * CURSOR_HEIGHT] = {
//     B, W, B, W, B, W, B, W, B, W, B, W, B, W, B, W,
//     W, B, W, B, W, B, W, B, W, B, W, B, W, B, W, B,
//     B, W, B, W, B, W, B, W, B, W, B, W, B, W, B, W,
//     W, B, W, B, W, B, W, B, W, B, W, B, W, B, W, B,
//     B, W, B, W, B, W, B, W, B, W, B, W, B, W, B, W,
//     W, B, W, B, W, B, W, B, W, B, W, B, W, B, W, B,
//     B, W, B, W, B, W, B, W, B, W, B, W, B, W, B, W,
//     W, B, W, B, W, B, W, B, W, B, W, B, W, B, W, B,
//     B, W, B, W, B, W, B, W, B, W, B, W, B, W, B, W,
//     W, B, W, B, W, B, W, B, W, B, W, B, W, B, W, B,
//     B, W, B, W, B, W, B, W, B, W, B, W, B, W, B, W,
//     W, B, W, B, W, B, W, B, W, B, W, B, W, B, W, B,
//     B, W, B, W, B, W, B, W, B, W, B, W, B, W, B, W,
//     W, B, W, B, W, B, W, B, W, B, W, B, W, B, W, B,
//     B, W, B, W, B, W, B, W, B, W, B, W, B, W, B, W,
//     W, B, W, B, W, B, W, B, W, B, W, B, W, B, W, B,
// };

void set_mode(unsigned char mode)
{
    union REGS regs;
    regs.h.ah = 0x00;
    regs.h.al = mode;
    int86(VIDEO_INT, &regs, &regs);
}

void wait_vblank(void)
{
    while(inp(INPUT_STATUS_1) & VRETRACE);
    while(!(inp(INPUT_STATUS_1) & VRETRACE));
}

// http://www.mcamafia.de/pdf/ibm_vgaxga_trm2.pdf
void set_mode_x(void)
{
    unsigned char v;

    set_mode(0x13);

    // xxxx CH4 OE EM x
    // CH4 1 - two low-order memory address bits select the map accessed.
    // OE 1 - system addresses sequentially access data within a bit map,
    // and the maps are accessed according to the value in the Map Mask register
    // EM 0 - disable memory above 64k
    outp(SEQ_ADDR, SEQ_REG_MEM_MODE);
    outp(SEQ_ADDR + 1, 0x06);

    // xxxxxxSA
    // 2-7 "don't care"
    // 1 Synchronous Reset (active low)
    // 0 Asynchronous Reset (active low)
    // so 0x01 is async reset disable, sync reset enable
    outp(SEQ_ADDR, SEQ_REG_RESET);
    outp(SEQ_ADDR + 1, 0x01);

    // | VSP  | HSP  |    xx    |  CS  |  CS  | ERAM | IOS  |
    // 11100011
    // * VSP 1 means negative vertical sync polarity
    // * HSP 1 means negative horizontal sync polarity
    // these sync polarities tell the monitor to adjust the gain of the
    // vertical section so modes aren't different visual heights depending
    // on the # of lines
    // * "don't care" bits are 10 instead of 00 for some reason
    // * CS 00 "Selects 25.175 MHz clock for 640/320 Horizontal PELs"
    // * ERAM 1 enables the RAM
    // * IOS 1 makes CRTC 3d4 and input status 1 3da (as opposed to 3b4 and 3ba)
    outp(MISC_OUTPUT, 0xe3);

    // 0b11 means both resets turned off
    outp(SEQ_ADDR, SEQ_REG_RESET);
    outp(SEQ_ADDR + 1, 0x03);

    // also contains write protect bit, read current value and then write back
    // with it cleared. need this because vtotal and overflows are in the range
    // of protected registers (0-7)
    outp(CRTC_INDEX, CRTC_V_RETRACE_END);
    v = inp(CRTC_INDEX + 1);
    outp(CRTC_INDEX + 1, v & 0x7f);

    // The Vertical Total register (bits 7−0) contains the 8 low-order bits
    // of a 10-bit vertical total. The value for the vertical total is the
    // number of horizontal raster scans on the display, including vertical
    // retrace, minus 2. This value determines the period of the vertical
    // retrace signal.
    outp(CRTC_INDEX, CRTC_V_TOTAL);
    outp(CRTC_INDEX + 1, 0x0d);

    // this is a crazy register that has extra bits of 5 different registers
    // VRS9 VDE9 VT9 LC8 VBS8 VRS8 VDE8 VT8
    // 0x3e = 0b00111110
    // VT9 = 1, VT8 = 0, so actual vertical total is:
    //   0b1000001101 + 2 (from above) = 527
    // LC8 = 1
    // VBS8 = 1
    // VRS8 = 1
    // VDE9 = 0, VDE8 = 1
    outp(CRTC_INDEX, CRTC_OVERFLOWS);
    outp(CRTC_INDEX + 1, 0x3e);

    // Maximum Scan Line Register
    // DSC LC9 VBS9 MSL MSL MSL MSL MSL
    // 01000001
    // DSC 0 - no line doubling? when 1, 200-scan-line video data is converted
    //   to 400-scan-line output
    // LC9 1 - bit 9 of Line Compare
    // VBS9 0 - bit 9 of Vertical Blanking Start
    // MSL 1 - number of scan lines per character row.
    //   The value of this field is the maximum row scan number minus 1.
    //   so for this, 2 lines per character row
    outp(CRTC_INDEX, CRTC_MAX_SCAN_LINE);
    outp(CRTC_INDEX + 1, 0x41);

    // 8 low order bits of the start position of the vertical retrace signal
    // we have 0x1ea (extra bit in overflow register), so retrace starts at
    // y=490 lines
    outp(CRTC_INDEX, CRTC_V_RETRACE_START);
    outp(CRTC_INDEX + 1, 0xea);

    // PR S5R EVI CVI VRE VRE VRE VRE
    // PR 1 - protect bit for registers 0-7
    // S5R 0 - "allows use of the VGA chip with 15 kHz displays" - look into this
    // EVI 1 - disable vertical retrace interrupt (active low)
    // CVI 0 - clear vertical interrupt (active low)
    // VRE 0xc... 
    // "number of lines is added to the value in the Start Vertical Retrace
    // register. The 4 low-order bits of the result are the 4-bit value 
    // programmed."
    outp(CRTC_INDEX, CRTC_V_RETRACE_END);
    outp(CRTC_INDEX + 1, 0xac);

    // The 10-bit value is equal to the total number of scan lines minus 1
    // extra two bits are in the overflow register
    // 0x1df + 1 = 480 lines
    outp(CRTC_INDEX, CRTC_V_DISPLAY_END);
    outp(CRTC_INDEX + 1, 0xdf);

    // turn off dword mode... not sure
    outp(CRTC_INDEX, CRTC_UNDERLINE_LOC);
    outp(CRTC_INDEX + 1, 0x00);

    // 0x1e7 + 1 = 488 lines
    outp(CRTC_INDEX, CRTC_V_BLANK_START);
    outp(CRTC_INDEX + 1, 0xe7);

    // not exactly sure about the wording here, but:
    // "the number of lines is added to the value in the Start Vertical
    // Blanking register minus 1. The 8 low-order bits of the result are the
    // 8-bit value programmed."
    // (0x1e6 + n) & 0xff = 6 ???
    outp(CRTC_INDEX, CRTC_V_BLANK_END);
    outp(CRTC_INDEX + 1, 0x06);

    // RST WB ADW - CB2 HRS SRC CMS0
    // 1110 0011
    // "byte mode", others are defaults i think
    outp(CRTC_INDEX, CRTC_MODE_CONTROL);
    outp(CRTC_INDEX + 1, 0xe3);

    // xxxx3210. 1 in a spot means that memory map is enabled
    outp(SEQ_ADDR, SEQ_REG_MAP_MASK);
    outp(SEQ_ADDR + 1, 0x0f);

    _fmemset(vga, 0x80, 0x8000);
    // _fmemset(&vga[0x8000], 0x60, 0x8000);
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

void draw_sprite(const unsigned char *data, int sx, int sy, int width, int height)
{
    int x, y, plane;
    int start_plane = sx & 3;
    unsigned int offset, in_offset, start_offset;
    int bytes_per_row = (width >> 2) + (start_plane != 0);

    start_offset = active_page + sy * (SCREEN_WIDTH >> 2) + (sx >> 2);

    for (plane = 0; plane < 4; plane++) {
        outp(SEQ_ADDR, SEQ_REG_MAP_MASK);
        outp(SEQ_ADDR + 1, 1 << plane);

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

void get_palette(
    unsigned char index, 
    unsigned char *r, 
    unsigned char *g, 
    unsigned char *b
) {
    outp(DAC_READ_INDEX, index);
    *r = inp(DAC_DATA);
    *g = inp(DAC_DATA);
    *b = inp(DAC_DATA);
}

void set_palette(
    unsigned char index, 
    unsigned char r, 
    unsigned char g, 
    unsigned char b
) {
    outp(DAC_WRITE_INDEX, index);
    outp(DAC_DATA, r);
    outp(DAC_DATA, g);
    outp(DAC_DATA, b);
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
    int k;

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
