#ifndef _CRANE_H
#define _CRANE_H

#include "project.h"

extern int bg_scroll_x;
extern int bg_scroll_y;
extern int status_x;
extern int status_y;
extern char current_filename[13];

int rect_contains(int x0, int y0, int w, int h, int x, int y);
void draw_snes_palette(int x0, int y0, int index);
void draw_project_tile(struct tile *tile, int x, int y, int tile_size, int mute);
void draw_tile_library(struct project *proj, int mute);
void draw_project_background(struct project *proj, int x0, int y0, int mute);
void draw_bg_row(struct project *proj, int x0, int y0, int row);
void draw_bg_column(struct project *proj, int x0, int y0, int col);
void draw_status_bar(const char *text);
void draw_entire_screen(struct project *proj);

#endif
