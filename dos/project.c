#include <stdio.h>
#include <string.h>
#include "project.h"

static unsigned char read_u8(FILE *fp)
{
    unsigned char val;
    fread(&val, 1, 1, fp);
    return val;
}

static unsigned short read_u16(FILE *fp)
{
    unsigned char buf[2];
    fread(buf, 2, 1, fp);
    return buf[0] | (buf[1] << 8);
}

static signed char read_s8(FILE *fp)
{
    signed char val;
    fread(&val, 1, 1, fp);
    return val;
}

void new_project(struct project *proj)
{
    strcpy(proj->name, "untitled");
    proj->tile_size = 16;
    memset(proj->colors, 0, sizeof proj->colors);
    memset(proj->tiles, 0, sizeof proj->tiles);
    memset(proj->background.tiles, -1, sizeof proj->background.tiles);
    memset(proj->background.palettes, 0, sizeof proj->background.palettes);
}

int load_project_binary(const char *filename, struct project *proj)
{
    FILE *fp;
    int k, y, x;

    fp = fopen(filename, "rb");
    if (!fp) {
        return -1;
    }

    fread(proj->id, 8, 1, fp);
    if (strcmp("crprj01", proj->id)) {
        return -1;
    }

    fread(proj->name, 64, 1, fp);
    proj->tile_size = read_u8(fp);
    proj->num_tiles = read_u16(fp);

    for (k = 0; k < 128; k++) {
        proj->colors[k].r = read_u8(fp);
        proj->colors[k].g = read_u8(fp);
        proj->colors[k].b = read_u8(fp);
    }

    for (k = 0; k < 100; k++) {
        proj->tiles[k].unused = read_u8(fp);
        proj->tiles[k].preview_palette = read_u8(fp);
        fread(proj->tiles[k].pixels, 256, 1, fp);
    }

    for (y = 0; y < 32; y++) {
        for (x = 0; x < 32; x++) {
            proj->background.tiles[y][x] = read_s8(fp);
        }
    }

    for (y = 0; y < 32; y++) {
        for (x = 0; x < 32; x++) {
            proj->background.palettes[y][x] = read_u8(fp);
        }
    }

    fclose(fp);
    return 0;
}
