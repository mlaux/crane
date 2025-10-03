#ifndef PROJECT_H
#define PROJECT_H

#define MAX_TILES 100
#define MAX_PROJECT_NAME 64
#define NUM_COLORS 128

struct tile {
    unsigned char size;
    unsigned char preview_palette;
    unsigned char pixels[256];
};

struct background {
    signed char tiles[32][32];
    unsigned char palettes[32][32];
};

struct rgb {
    unsigned char r;
    unsigned char g;
    unsigned char b;
};

struct project {
    char name[MAX_PROJECT_NAME];
    unsigned char tile_size;
    int num_tiles;
    struct rgb colors[NUM_COLORS];
    struct tile tiles[MAX_TILES];
    struct background background;
};

int load_project_binary(const char *filename, struct project *proj);

#endif
