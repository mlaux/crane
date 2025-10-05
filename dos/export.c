#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "export.h"
#include "project.h"

#define COLORS_PER_PALETTE 16

static void write_u16_le(FILE *f, unsigned short val) {
    unsigned char buf[2];
    buf[0] = val & 0xff;
    buf[1] = (val >> 8) & 0xff;
    fwrite(buf, 1, 2, f);
}

static unsigned short rgb_to_bgr555(struct rgb color) {
    unsigned char r = color.r >> 1;
    unsigned char g = color.g >> 1;
    unsigned char b = color.b >> 1;
    return (b << 10) | (g << 5) | r;
}

static int find_last_nonzero_palette(const struct project *proj) {
    int num_palettes = NUM_COLORS / COLORS_PER_PALETTE;
    int k, p;

    for (p = num_palettes - 1; p >= 0; p--) {
        for (k = 0; k < COLORS_PER_PALETTE; k++) {
            struct rgb color = proj->colors[p * COLORS_PER_PALETTE + k];
            if (color.r != 0 || color.g != 0 || color.b != 0) {
                return p;
            }
        }
    }
    return -1;
}

int export_palettes(const struct project *proj, const char *filename) {
    FILE *f = fopen(filename, "wb");
    int k;
    int last_palette;
    int num_colors;

    if (!f) {
        return -1;
    }

    last_palette = find_last_nonzero_palette(proj);
    if (last_palette < 0) {
        fclose(f);
        return 0;
    }

    num_colors = (last_palette + 1) * COLORS_PER_PALETTE;

    for (k = 0; k < num_colors; k++) {
        unsigned short bgr = rgb_to_bgr555(proj->colors[k]);
        write_u16_le(f, bgr);
    }

    fclose(f);
    return 0;
}

static void convert_tile_8x8(const unsigned char *pixels, unsigned char *out) {
    int row, bit;
    int out_idx = 0;

    for (row = 0; row < 8; row++) {
        for (bit = 0; bit < 2; bit++) {
            unsigned char byte = 0;
            int col;
            for (col = 0; col < 8; col++) {
                unsigned char pix = pixels[row * 8 + col];
                if (pix & (1 << bit)) {
                    byte |= (1 << (7 - col));
                }
            }
            out[out_idx++] = byte;
        }
    }

    for (row = 0; row < 8; row++) {
        for (bit = 2; bit < 4; bit++) {
            unsigned char byte = 0;
            int col;
            for (col = 0; col < 8; col++) {
                unsigned char pix = pixels[row * 8 + col];
                if (pix & (1 << bit)) {
                    byte |= (1 << (7 - col));
                }
            }
            out[out_idx++] = byte;
        }
    }
}

static void convert_subtile(const unsigned char *pixels, int start_row, int start_col, unsigned char *out) {
    unsigned char subtile[64];
    int row, col;

    for (row = 0; row < 8; row++) {
        for (col = 0; col < 8; col++) {
            subtile[row * 8 + col] = pixels[(start_row + row) * 16 + (start_col + col)];
        }
    }

    convert_tile_8x8(subtile, out);
}

static void write_tile_16x16(const unsigned char *pixels, unsigned char *buf, int tile_index) {
    unsigned char tl[32], tr[32], bl[32], br[32];
    int base_offset = tile_index % 8 * 64 + tile_index / 8 * 0x400;
    int k;

    convert_subtile(pixels, 0, 0, tl);
    convert_subtile(pixels, 0, 8, tr);
    convert_subtile(pixels, 8, 0, bl);
    convert_subtile(pixels, 8, 8, br);

    for (k = 0; k < 32; k++) {
        buf[base_offset + k] = tl[k];
        buf[base_offset + 32 + k] = tr[k];
        buf[base_offset + 0x200 + k] = bl[k];
        buf[base_offset + 0x200 + 32 + k] = br[k];
    }
}

int export_tiles(const struct project *proj, const char *filename) {
    FILE *f = fopen(filename, "wb");
    int tile_data_len;
    unsigned char *buf;
    unsigned char blank_tile[256];
    int k;

    if (!f) {
        return -1;
    }

    if (proj->tile_size == 16) {
        int num_tiles_with_blank = proj->num_tiles + 1;
        tile_data_len = ((32 * 4 * num_tiles_with_blank + 0x3ff) / 0x400) * 0x400;
    } else {
        tile_data_len = 32 * (proj->num_tiles + 1);
    }

    buf = (unsigned char *)calloc(tile_data_len, 1);
    if (!buf) {
        fclose(f);
        return -1;
    }

    if (proj->tile_size == 16) {
        memset(blank_tile, 0, 256);
        for (k = 0; k < proj->num_tiles; k++) {
            write_tile_16x16(proj->tiles[k].pixels, buf, k);
        }
        write_tile_16x16(blank_tile, buf, proj->num_tiles);
    } else {
        for (k = 0; k < proj->num_tiles; k++) {
            convert_tile_8x8(proj->tiles[k].pixels, buf + k * 32);
        }
        memset(buf + proj->num_tiles * 32, 0, 32);
    }

    fwrite(buf, 1, tile_data_len, f);
    free(buf);
    fclose(f);
    return 0;
}

int export_background(const struct project *proj, const char *filename, int palette_offset) {
    FILE *f = fopen(filename, "wb");
    int blank_tile_index = proj->num_tiles;
    int x, y;

    if (!f) {
        return -1;
    }

    for (y = 0; y < 32; y++) {
        for (x = 0; x < 32; x++) {
            int tile_num = proj->background.tiles[y][x];
            int palette_num = 0;
            int tile_num_8x8;
            unsigned short entry;

            if (tile_num == -1) {
                tile_num = blank_tile_index;
            } else {
                palette_num = proj->background.palettes[y][x] + palette_offset;
            }

            tile_num_8x8 = tile_num;
            if (proj->tile_size == 16) {
                tile_num_8x8 = 2 * (tile_num % 8) + 32 * (tile_num / 8);
            }

            entry = (palette_num << 10) | tile_num_8x8;
            write_u16_le(f, entry);
        }
    }

    fclose(f);
    return 0;
}
