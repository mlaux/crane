#include <dos.h>
#include <conio.h>
#include <string.h>

#include "compat.h"
#include "vga.h"

extern unsigned char far *vga;

void set_mode(unsigned char mode)
{
    union REGS regs;
    regs.h.ah = 0x00;
    regs.h.al = mode;
    int86(VIDEO_INT, &regs, &regs);
}

// http://www.mcamafia.de/pdf/ibm_vgaxga_trm2.pdf
void set_mode_x(void)
{
    unsigned char v;

    set_mode(0x13);

    // xxxx CH4 OE EM x
    // CH4 1 - two low-order memory address bits select the map accessed.
    // OE 1 - system addresses sequentially access data within a bit map,
    // and the maps are accessed according to the value in the Map Mask register
    // EM 0 - disable memory above 64k
    outp(SEQ_ADDR, SEQ_REG_MEM_MODE);
    outp(SEQ_ADDR + 1, 0x06);

    // xxxxxxSA
    // 2-7 "don't care"
    // 1 Synchronous Reset (active low)
    // 0 Asynchronous Reset (active low)
    // so 0x01 is async reset disable, sync reset enable
    outp(SEQ_ADDR, SEQ_REG_RESET);
    outp(SEQ_ADDR + 1, 0x01);

    // | VSP  | HSP  |    xx    |  CS  |  CS  | ERAM | IOS  |
    // 11100011
    // * VSP 1 means negative vertical sync polarity
    // * HSP 1 means negative horizontal sync polarity
    // these sync polarities tell the monitor to adjust the gain of the
    // vertical section so modes aren't different visual heights depending
    // on the # of lines
    // * "don't care" bits are 10 instead of 00 for some reason
    // * CS 00 "Selects 25.175 MHz clock for 640/320 Horizontal PELs"
    // * ERAM 1 enables the RAM
    // * IOS 1 makes CRTC 3d4 and input status 1 3da (as opposed to 3b4 and 3ba)
    outp(MISC_OUTPUT, 0xe3);

    // 0b11 means both resets turned off
    outp(SEQ_ADDR, SEQ_REG_RESET);
    outp(SEQ_ADDR + 1, 0x03);

    // also contains write protect bit, read current value and then write back
    // with it cleared. need this because vtotal and overflows are in the range
    // of protected registers (0-7)
    outp(CRTC_INDEX, CRTC_V_RETRACE_END);
    v = inp(CRTC_INDEX + 1);
    outp(CRTC_INDEX + 1, v & 0x7f);

    // The Vertical Total register (bits 7−0) contains the 8 low-order bits
    // of a 10-bit vertical total. The value for the vertical total is the
    // number of horizontal raster scans on the display, including vertical
    // retrace, minus 2. This value determines the period of the vertical
    // retrace signal.
    outp(CRTC_INDEX, CRTC_V_TOTAL);
    outp(CRTC_INDEX + 1, 0x0d);

    // this is a crazy register that has extra bits of 5 different registers
    // VRS9 VDE9 VT9 LC8 VBS8 VRS8 VDE8 VT8
    // 0x3e = 0b00111110
    // VT9 = 1, VT8 = 0, so actual vertical total is:
    //   0b1000001101 + 2 (from above) = 527
    // LC8 = 1
    // VBS8 = 1
    // VRS8 = 1
    // VDE9 = 0, VDE8 = 1
    outp(CRTC_INDEX, CRTC_OVERFLOWS);
    outp(CRTC_INDEX + 1, 0x3e);

    // Maximum Scan Line Register
    // DSC LC9 VBS9 MSL MSL MSL MSL MSL
    // 01000001
    // DSC 0 - no line doubling? when 1, 200-scan-line video data is converted
    //   to 400-scan-line output
    // LC9 1 - bit 9 of Line Compare
    // VBS9 0 - bit 9 of Vertical Blanking Start
    // MSL 1 - number of scan lines per character row.
    //   The value of this field is the maximum row scan number minus 1.
    //   so for this, 2 lines per character row
    outp(CRTC_INDEX, CRTC_MAX_SCAN_LINE);
    outp(CRTC_INDEX + 1, 0x41);

    // 8 low order bits of the start position of the vertical retrace signal
    // we have 0x1ea (extra bit in overflow register), so retrace starts at
    // y=490 lines
    outp(CRTC_INDEX, CRTC_V_RETRACE_START);
    outp(CRTC_INDEX + 1, 0xea);

    // PR S5R EVI CVI VRE VRE VRE VRE
    // PR 1 - protect bit for registers 0-7
    // S5R 0 - "allows use of the VGA chip with 15 kHz displays" - look into this
    // EVI 1 - disable vertical retrace interrupt (active low)
    // CVI 0 - clear vertical interrupt (active low)
    // VRE 0xc... 
    // "number of lines is added to the value in the Start Vertical Retrace
    // register. The 4 low-order bits of the result are the 4-bit value 
    // programmed."
    outp(CRTC_INDEX, CRTC_V_RETRACE_END);
    outp(CRTC_INDEX + 1, 0xac);

    // The 10-bit value is equal to the total number of scan lines minus 1
    // extra two bits are in the overflow register
    // 0x1df + 1 = 480 lines
    outp(CRTC_INDEX, CRTC_V_DISPLAY_END);
    outp(CRTC_INDEX + 1, 0xdf);

    // turn off dword mode... not sure
    outp(CRTC_INDEX, CRTC_UNDERLINE_LOC);
    outp(CRTC_INDEX + 1, 0x00);

    // 0x1e7 + 1 = 488 lines
    outp(CRTC_INDEX, CRTC_V_BLANK_START);
    outp(CRTC_INDEX + 1, 0xe7);

    // not exactly sure about the wording here, but:
    // "the number of lines is added to the value in the Start Vertical
    // Blanking register minus 1. The 8 low-order bits of the result are the
    // 8-bit value programmed."
    // (0x1e6 + n) & 0xff = 6 ???
    outp(CRTC_INDEX, CRTC_V_BLANK_END);
    outp(CRTC_INDEX + 1, 0x06);

    // RST WB ADW - CB2 HRS SRC CMS0
    // 1110 0011
    // "byte mode", others are defaults i think
    outp(CRTC_INDEX, CRTC_MODE_CONTROL);
    outp(CRTC_INDEX + 1, 0xe3);

    // xxxx3210. 1 in a spot means that memory map is enabled
    outp(SEQ_ADDR, SEQ_REG_MAP_MASK);
    outp(SEQ_ADDR + 1, 0x0f);

    _fmemset(vga, 0x80, 0x8000);
    // _fmemset(&vga[0x8000], 0x60, 0x8000);
}

void wait_vblank(void)
{
    while(inp(INPUT_STATUS_1) & VRETRACE);
    while(!(inp(INPUT_STATUS_1) & VRETRACE));
}
