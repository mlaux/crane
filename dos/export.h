#ifndef EXPORT_H
#define EXPORT_H

#include "project.h"

int export_palettes(const struct project *proj, const char *filename);
int export_tiles(const struct project *proj, const char *filename);
int export_background(const struct project *proj, const char *filename, int palette_offset);

#endif
