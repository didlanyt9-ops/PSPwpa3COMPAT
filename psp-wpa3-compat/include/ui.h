#ifndef WPA3COMPAT_UI_H
#define WPA3COMPAT_UI_H

#include "app.h"

void ui_init(void);
void ui_clear(void);
int ui_osk(const char *title, const char *initial, char *out, int maxlen, int password);
unsigned int ui_buttons(void);

#endif
