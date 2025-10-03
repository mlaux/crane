CC=wcl
CFLAGS=-0 -ms -w4 -ox
TARGET=crane.exe

$(TARGET): crane.c font.c vga_control.c palette.c vga.c mouse.c cursor.c
	$(CC) $(CFLAGS) crane.c font.c vga_control.c palette.c vga.c mouse.c cursor.c -fe=$(TARGET)

clean:
	rm -f *.exe *.o *.err
