#ifndef _MOUSE_H
#define _MOUSE_H

void init_mouse(void);

int poll_mouse(int *x, int *y);

int mouse_buttons_down(void);

#endif
