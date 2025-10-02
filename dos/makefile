CC=wcl
CFLAGS=-0 -ms -w4 -ox
TARGET=crane.exe

$(TARGET): crane.c font.c
	$(CC) $(CFLAGS) crane.c font.c -fe=$(TARGET)

clean:
	rm -f *.exe *.obj *.err
