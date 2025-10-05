#include <stdio.h>
#include "project.h"
#include "export.h"

int main(int argc, char *argv[]) {
    static struct project proj;
    const char *input_file = "project.dat";
    const char *base_name = "test";
    char pal_file[64], tiles_file[64], map_file[64];

    if (argc > 1) {
        input_file = argv[1];
    }
    if (argc > 2) {
        base_name = argv[2];
    }

    sprintf(pal_file, "%s.pal", base_name);
    sprintf(tiles_file, "%s.4bp", base_name);
    sprintf(map_file, "%s.map", base_name);

    printf("loading project: %s\n", input_file);
    if (load_project_binary(input_file, &proj) < 0) {
        printf("error loading project\n");
        return 1;
    }

    printf("project: %s\n", proj.name);
    printf("tile size: %dx%d\n", proj.tile_size, proj.tile_size);
    printf("num tiles: %d\n", proj.num_tiles);

    printf("\nexporting palettes to %s...\n", pal_file);
    if (export_palettes(&proj, pal_file) < 0) {
        printf("error exporting palettes\n");
        return 1;
    }
    printf("done\n");

    printf("\nexporting tiles to %s...\n", tiles_file);
    if (export_tiles(&proj, tiles_file) < 0) {
        printf("error exporting tiles\n");
        return 1;
    }
    printf("done\n");

    printf("\nexporting background to %s...\n", map_file);
    if (export_background(&proj, map_file, 0) < 0) {
        printf("error exporting background\n");
        return 1;
    }
    printf("done\n");

    printf("\nall exports complete!\n");
    return 0;
}
