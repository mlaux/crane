#include <dos.h>
#include "mouse.h"
#include "vga.h"

void init_mouse(void)
{
    union REGS regs;
    regs.x.ax = 0x00;
    int86(0x33, &regs, &regs);

    regs.x.ax = 0x08;
    regs.x.cx = 0;
    regs.x.dx = SCREEN_HEIGHT - 1;
    int86(0x33, &regs, &regs);
}

int poll_mouse(int *x, int *y)
{
    union REGS regs;
    regs.x.ax = 0x03;
    int86(0x33, &regs, &regs);
    *x = regs.x.cx >> 1;
    *y = regs.x.dx;
    return regs.x.bx;
}
