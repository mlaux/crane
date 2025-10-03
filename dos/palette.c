#include <conio.h>

#include "palette.h"
#include "vga.h"

const unsigned char ui_colors[] = {
    14, 14, 28, // BACKGROUND_COLOR this is stock color 0x80
    8, 8, 16,   // CONTENT_COLOR 0xc8
    20, 20, 28, // HIGHLIGHT_COLOR 0x98
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
