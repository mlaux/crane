#ifndef ACTIONS_H
#define ACTIONS_H

#include "project.h"

struct button {
    int x, y, w, h;
    int icon;
    void (*on_click)(struct project *);
};

void invoke_color_picker(struct project *proj);
void handle_button_clicks(struct project *proj, int x, int y);
void handle_tile_clicks(struct project *proj, int x, int y);
void handle_background_clicks(struct project *proj, int x, int y);
void draw_buttons(void);

#endif
