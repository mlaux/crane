#ifndef _PICKER_H
#define _PICKER_H

#define PICKER_WIDTH 144
#define PICKER_HEIGHT 144

#define PICKER_X ((SCREEN_WIDTH / 2) - (PICKER_WIDTH / 2))
#define PICKER_Y ((SCREEN_HEIGHT / 2) - (PICKER_HEIGHT / 2))

void color_picker(struct rgb *color);

#endif
