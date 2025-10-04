#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cJSON.h"

#define MAX_TILES 100
#define MAX_PROJECT_NAME 64
#define NUM_COLORS 128

struct tile {
    unsigned char unused;
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
    char id[8];
    char name[MAX_PROJECT_NAME];
    unsigned char tile_size;
    int num_tiles;
    struct rgb colors[NUM_COLORS];
    struct tile tiles[MAX_TILES];
    struct background background;
};

static void parse_hex_color(const char *hex, struct rgb *color) {
    unsigned int r, g, b;
    if (hex[0] == '#') {
        hex++;
    }
    sscanf(hex, "%2x%2x%2x", &r, &g, &b);
    color->r = r >> 2;
    color->g = g >> 2;
    color->b = b >> 2;
}

static int load_colors(cJSON *colors_array, struct project *proj) {
    int num_colors, k;
    cJSON *color_obj;

    num_colors = cJSON_GetArraySize(colors_array);
    if (num_colors > NUM_COLORS) {
        fprintf(stderr, "Warning: Project has %d colors, truncating to %d\n", num_colors, NUM_COLORS);
        num_colors = NUM_COLORS;
    }

    for (k = 0; k < num_colors; k++) {
        color_obj = cJSON_GetArrayItem(colors_array, k);
        if (color_obj && color_obj->valuestring) {
            parse_hex_color(color_obj->valuestring, &proj->colors[k]);
        } else {
            proj->colors[k].r = 0;
            proj->colors[k].g = 0;
            proj->colors[k].b = 0;
        }
    }

    for (k = num_colors; k < NUM_COLORS; k++) {
        proj->colors[k].r = 0;
        proj->colors[k].g = 0;
        proj->colors[k].b = 0;
    }

    return 0;
}

static int load_tiles(cJSON *tiles_array, struct project *proj) {
    int num_tiles, k, num_pixels, j;
    cJSON *tile_obj, *data_array, *pixel, *palette_obj;

    num_tiles = cJSON_GetArraySize(tiles_array);
    if (num_tiles > MAX_TILES) {
        fprintf(stderr, "Warning: Project has %d tiles, truncating to %d\n", num_tiles, MAX_TILES);
        num_tiles = MAX_TILES;
    }

    for (k = 0; k < num_tiles; k++) {
        tile_obj = cJSON_GetArrayItem(tiles_array, k);
        if (!tile_obj) {
            continue;
        }

        data_array = cJSON_GetObjectItem(tile_obj, "data");
        if (!data_array) {
            continue;
        }

        proj->tiles[k].unused = 0;

        palette_obj = cJSON_GetObjectItem(tile_obj, "palette");
        if (palette_obj) {
            proj->tiles[k].preview_palette = (unsigned char)palette_obj->valueint;
        } else {
            proj->tiles[k].preview_palette = 0;
        }

        num_pixels = cJSON_GetArraySize(data_array);
        if (num_pixels > 256) {
            num_pixels = 256;
        }

        for (j = 0; j < num_pixels; j++) {
            pixel = cJSON_GetArrayItem(data_array, j);
            proj->tiles[k].pixels[j] = (unsigned char)pixel->valueint;
        }

        for (j = num_pixels; j < 256; j++) {
            proj->tiles[k].pixels[j] = 0;
        }
    }

    proj->num_tiles = num_tiles;
    return 0;
}

static int load_background(cJSON *bg_array, cJSON *tiles_array, struct project *proj) {
    int height, width, y, x, idx;
    cJSON *row, *tile_index, *tile_obj, *palette_obj;

    height = cJSON_GetArraySize(bg_array);
    if (height > 32) {
        height = 32;
    }

    for (y = 0; y < height; y++) {
        row = cJSON_GetArrayItem(bg_array, y);
        if (!row) {
            continue;
        }

        width = cJSON_GetArraySize(row);
        if (width > 32) {
            width = 32;
        }

        for (x = 0; x < width; x++) {
            tile_index = cJSON_GetArrayItem(row, x);
            idx = tile_index->valueint;

            proj->background.tiles[y][x] = (signed char)idx;

            if (idx >= 0 && idx < proj->num_tiles) {
                tile_obj = cJSON_GetArrayItem(tiles_array, idx);
                if (tile_obj) {
                    palette_obj = cJSON_GetObjectItem(tile_obj, "palette");
                    if (palette_obj) {
                        proj->background.palettes[y][x] = (unsigned char)palette_obj->valueint;
                    }
                }
            }
        }
    }

    return 0;
}

static int load_json(const char *filename, struct project *proj) {
    FILE *fp;
    long size;
    char *buffer;
    cJSON *root, *name, *tile_size, *colors_array, *tiles_array, *bg_array;
    int y, x;

    fp = fopen(filename, "rb");
    if (!fp) {
        return -1;
    }

    fseek(fp, 0, SEEK_END);
    size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    buffer = malloc(size + 1);
    if (!buffer) {
        fclose(fp);
        return -1;
    }

    fread(buffer, 1, size, fp);
    buffer[size] = '\0';
    fclose(fp);

    printf("Parsing %ld byte JSON file...\n", size);

    root = cJSON_Parse(buffer);
    free(buffer);

    if (!root) {
        fprintf(stderr, "JSON parse error\n");
        return -1;
    }

    strcpy(proj->id, "crprj01");

    name = cJSON_GetObjectItem(root, "name");
    if (name && name->valuestring) {
        strncpy(proj->name, name->valuestring, MAX_PROJECT_NAME - 1);
        proj->name[MAX_PROJECT_NAME - 1] = '\0';
    } else {
        proj->name[0] = '\0';
    }

    tile_size = cJSON_GetObjectItem(root, "tileSize");
    if (tile_size) {
        proj->tile_size = (unsigned char)tile_size->valueint;
    } else {
        proj->tile_size = 16;
    }

    colors_array = cJSON_GetObjectItem(root, "colors");
    if (colors_array) {
        load_colors(colors_array, proj);
    }

    for (y = 0; y < 32; y++) {
        for (x = 0; x < 32; x++) {
            proj->background.tiles[y][x] = -1;
            proj->background.palettes[y][x] = 0;
        }
    }

    tiles_array = cJSON_GetObjectItem(root, "tiles");
    if (tiles_array) {
        load_tiles(tiles_array, proj);
    }

    bg_array = cJSON_GetObjectItem(root, "background");
    if (bg_array && tiles_array) {
        load_background(bg_array, tiles_array, proj);
    }

    cJSON_Delete(root);
    return 0;
}

static void write_u8(FILE *fp, unsigned char val) {
    fwrite(&val, 1, 1, fp);
}

static void write_u16(FILE *fp, unsigned short val) {
    unsigned char buf[2];
    buf[0] = val & 0xff;
    buf[1] = (val >> 8) & 0xff;
    fwrite(buf, 2, 1, fp);
}

static void write_s8(FILE *fp, signed char val) {
    fwrite(&val, 1, 1, fp);
}

static int save_binary(const char *filename, struct project *proj) {
    FILE *fp;
    int k, y, x;

    fp = fopen(filename, "wb");
    if (!fp) {
        return -1;
    }

    fwrite(proj->id, 8, 1, fp);
    fwrite(proj->name, MAX_PROJECT_NAME, 1, fp);
    write_u8(fp, proj->tile_size);
    write_u16(fp, (unsigned short)proj->num_tiles);

    for (k = 0; k < NUM_COLORS; k++) {
        write_u8(fp, proj->colors[k].r);
        write_u8(fp, proj->colors[k].g);
        write_u8(fp, proj->colors[k].b);
    }

    for (k = 0; k < MAX_TILES; k++) {
        write_u8(fp, proj->tiles[k].unused);
        write_u8(fp, proj->tiles[k].preview_palette);
        fwrite(proj->tiles[k].pixels, 256, 1, fp);
    }

    for (y = 0; y < 32; y++) {
        for (x = 0; x < 32; x++) {
            write_s8(fp, proj->background.tiles[y][x]);
        }
    }

    for (y = 0; y < 32; y++) {
        for (x = 0; x < 32; x++) {
            write_u8(fp, proj->background.palettes[y][x]);
        }
    }

    fclose(fp);
    return 0;
}

int main(int argc, char *argv[]) {
    struct project proj;
    char output_filename[256];
    const char *dot;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <input.json>\n", argv[0]);
        fprintf(stderr, "Converts JSON project to binary .dat file\n");
        return 1;
    }

    memset(&proj, 0, sizeof(proj));

    if (load_json(argv[1], &proj) != 0) {
        fprintf(stderr, "Failed to load %s\n", argv[1]);
        return 1;
    }

    printf("Loaded: %s\n", proj.name);
    printf("Tile size: %d\n", proj.tile_size);
    printf("Tiles: %d\n", proj.num_tiles);

    dot = strrchr(argv[1], '.');
    if (dot) {
        strncpy(output_filename, argv[1], dot - argv[1]);
        output_filename[dot - argv[1]] = '\0';
    } else {
        strcpy(output_filename, argv[1]);
    }
    strcat(output_filename, ".dat");

    if (save_binary(output_filename, &proj) != 0) {
        fprintf(stderr, "Failed to write %s\n", output_filename);
        return 1;
    }

    printf("Written: %s (%zu bytes)\n", output_filename, sizeof(struct project));
    return 0;
}
