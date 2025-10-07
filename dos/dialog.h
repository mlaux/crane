#ifndef _DIALOG_H
#define _DIALOG_H

#define DIALOG_BG_BUFFER_SIZE (20 * 1024)

extern unsigned char dialog_bg_buffer[DIALOG_BG_BUFFER_SIZE];

int modal_text_input(const char *prompt, char *value, int value_len);
int modal_confirm(const char *message);
void modal_info(const char *message);

#endif
