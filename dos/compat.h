// to get rid of syntax errors in vscode
#ifndef _COMPAT_H
#define _COMPAT_H

#ifndef __WATCOMC__
#define far
union REGS {
    struct {
        unsigned short ax, bx, cx, dx;
    } x;
    struct {
        unsigned char al, ah, bl, bh, cl, ch, dl, dh;
    } h;
};
#endif
#endif
