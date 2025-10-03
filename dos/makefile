CC=wcl
CFLAGS=-0 -ms -w4 -ox
TARGET=crane.exe

$(TARGET): crane.c font.c vga_init.c palette.c
	$(CC) $(CFLAGS) crane.c font.c vga_init.c palette.c -fe=$(TARGET)

clean:
	rm -f *.exe *.o *.err
