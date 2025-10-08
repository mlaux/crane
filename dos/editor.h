#ifndef _EDITOR_H
#define _EDITOR_H

#include "project.h"

void tile_editor(struct tile *tile, unsigned char tile_size);
int editor_contains(int x, int y, unsigned char tile_size);

#endif
