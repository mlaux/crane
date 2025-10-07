#ifndef _CURSOR_H
#define _CURSOR_H

#define CURSOR_WIDTH 8
#define CURSOR_HEIGHT 8

extern int cursor_x;
extern int cursor_y;
extern int cursor_visible;

void save_background(int x0, int y0, int w, int h, unsigned char *buffer);

void restore_background(int x0, int y0, int w, int h, unsigned char *buffer);

void save_cursor_background(void);

void restore_cursor_background(void);

void draw_cursor(void);

void move_cursor(int x, int y);

#endif
