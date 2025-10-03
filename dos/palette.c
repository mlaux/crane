#include <conio.h>

#include "palette.h"
#include "vga.h"

const unsigned char ui_colors[] = {
    0x0e, 0x0e, 0x1c, // BACKGROUND_COLOR this is stock color 0x80 but moved to $1
    0x08, 0x08, 0x10, // CONTENT_COLOR 0xc8
    0x14, 0x14, 0x1c, // HIGHLIGHT_COLOR 0x98
    0x3c, 0x2c, 0x24, // CURSOR_FISH_BODY
    0x3a, 0x20, 0x10, // CURSOR_FISH_OUTLINE
    0x30, 0x16, 0x06, // CURSOR_FISH_SHADOW
};

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
