#ifndef _PALETTE_H
#define _PALETTE_H

// 00 means transparent so not using it for black
// 01-0f are my UI colors
// 10-1f are stock VGA grays (10 = black, 1f = white)
// 20-37 is a "fully saturated" rainbow (blue to blue)
// 38-4f is a lighter rainbow
// 50-67 is an even lighter rainbow
// 68-7f is a dark rainbow
// 80-ff are SNES palettes 0-7, converted from RGB565 to RGB666
//         (red and blue scaled by 2)

#define NUM_UI_COLORS 9

#define FIRST_UI_COLOR 0x1
#define BACKGROUND_COLOR 0x1
#define CONTENT_COLOR 0x2
#define HIGHLIGHT_COLOR 0x3
#define CURSOR_FISH_BODY 0x4
#define CURSOR_FISH_OUTLINE 0x5
#define CURSOR_FISH_SHADOW 0x6
#define COLOR_PICKER_SELECTED_VALUE 0x7
#define BUTTON_COLOR 0x8
#define BUTTON_HIGHLIGHT 0x9

#define B 0x10
#define W 0x1f

#define FIRST_SNES_COLOR 0x80

extern const unsigned char ui_colors[3 * NUM_UI_COLORS];

void get_palette(
    unsigned char index, 
    unsigned char *r, 
    unsigned char *g, 
    unsigned char *b
);

void set_palette(
    unsigned char index, 
    unsigned char r, 
    unsigned char g, 
    unsigned char b
);

#endif
