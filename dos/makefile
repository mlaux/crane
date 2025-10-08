CC=wcl
CFLAGS=-0 -mc -w4 -ox
TARGET=crane.exe
TEST_EXPORT=test_exp.exe
# PICKER=picker.exe

all: $(TARGET) # $(PICKER)

$(TARGET): crane.c font.c vga_ctrl.c palette.c vga.c mouse.c cursor.c project.c picker.c export.c dialog.c editor.c actions.c icons.c
	$(CC) $(CFLAGS) crane.c font.c vga_ctrl.c palette.c vga.c mouse.c cursor.c project.c picker.c export.c dialog.c editor.c actions.c icons.c -fe=$(TARGET)

# $(PICKER): picker.c vga_control.c vga.c mouse.c font.c cursor.c
# 	$(CC) $(CFLAGS) picker.c vga_control.c vga.c mouse.c font.c cursor.c -fe=$(PICKER)

$(TEST_EXPORT): test_export.c project.c export.c
	$(CC) $(CFLAGS) test_export.c project.c export.c -fe=$(TEST_EXPORT)

clean:
	rm -f *.exe *.o *.err
