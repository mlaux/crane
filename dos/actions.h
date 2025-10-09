#ifndef _ACTIONS_H
#define _ACTIONS_H

#include "project.h"

struct button {
    int x, y, w, h;
    int icon;
    void (*on_click)(struct project *);
};

void handle_button_clicks(struct project *proj, int x, int y);
void handle_tile_clicks(struct project *proj, int x, int y);
void handle_background_clicks(struct project *proj, int x, int y);
void handle_palette_clicks(struct project *proj, int x, int y);
void draw_tool_buttons(void);
void draw_status_bar_buttons(void);

#endif
