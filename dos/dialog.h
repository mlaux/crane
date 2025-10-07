#ifndef _DIALOG_H
#define _DIALOG_H

int modal_text_input(const char *prompt, char *value, int value_len);
int modal_confirm(const char *message);
void modal_info(const char *message);

#endif
