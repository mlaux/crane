#ifndef _VGA_H
#define _VGA_H

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

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240

void set_mode(unsigned char);

void set_mode_x(void);

void wait_vblank(void);

void put_pixel(int x, int y, unsigned char color);

unsigned char get_pixel(int x, int y);

void horizontal_line(int x0, int x1, int y, unsigned char color);

void vertical_line(int x, int y0, int y1, unsigned char color);

void frame_rect(int x0, int y0, int width, int height, unsigned char color);

void fill_rect(int x0, int y0, int width, int height, unsigned char color);

void draw_window(int x, int y, int w, int h);

void drawf(int x, int y, const char *fmt, ...);

void drawf_clear(int x, int y, int bgcolor, const char *fmt, ...);

void draw_sprite(const unsigned char *data, int sx, int sy, int width, int height);

void draw_sprite_aligned_16x16(const unsigned char *data, int sx, int sy);

void draw_char(unsigned char uch, int x, int y);

void draw_string(const char *str, int x, int y);

#endif
