CC=wcl
CFLAGS=-0 -ms -w4 -ox
TARGET=crane.exe
PICKER=picker.exe

all: $(TARGET) $(PICKER)

$(TARGET): crane.c font.c vga_control.c palette.c vga.c mouse.c cursor.c project.c
	$(CC) $(CFLAGS) crane.c font.c vga_control.c palette.c vga.c mouse.c cursor.c project.c -fe=$(TARGET)

$(PICKER): picker.c vga_control.c vga.c mouse.c font.c cursor.c
	$(CC) $(CFLAGS) picker.c vga_control.c vga.c mouse.c font.c cursor.c -fe=$(PICKER)

clean:
	rm -f *.exe *.o *.err
