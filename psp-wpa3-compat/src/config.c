#include <pspiofilemgr.h>
#include <stdio.h>
#include <string.h>

#include "config.h"

static void trim_crlf(char *s)
{
	int n = (int)strlen(s);
	while (n > 0 && (s[n - 1] == '\n' || s[n - 1] == '\r')) {
		s[--n] = 0;
	}
}

void config_load(AppState *state)
{
	FILE *f;
	char line[160];

	state->ssid[0] = 0;
	state->psk[0] = 0;
	state->slot = 0;
	state->connected = 0;
	state->ip[0] = 0;
	state->last_error[0] = 0;

	f = fopen(CONFIG_PATH, "r");
	if (!f) {
		return;
	}

	while (fgets(line, sizeof(line), f)) {
		trim_crlf(line);
		if (!strncmp(line, "ssid=", 5)) {
			strncpy(state->ssid, line + 5, SSID_MAX);
			state->ssid[SSID_MAX] = 0;
		} else if (!strncmp(line, "psk=", 4)) {
			strncpy(state->psk, line + 4, PSK_MAX);
			state->psk[PSK_MAX] = 0;
		}
	}
	fclose(f);
}

void config_save(const AppState *state)
{
	FILE *f;
	SceUID fd;

	sceIoMkdir("ms0:/PSP", 0777);
	sceIoMkdir("ms0:/PSP/GAME", 0777);
	sceIoMkdir(CONFIG_DIR, 0777);

	fd = sceIoOpen(CONFIG_PATH, PSP_O_WRONLY | PSP_O_CREAT | PSP_O_TRUNC, 0777);
	if (fd >= 0) {
		sceIoClose(fd);
	}

	f = fopen(CONFIG_PATH, "w");
	if (!f) {
		return;
	}
	fprintf(f, "ssid=%s\npsk=%s\n", state->ssid, state->psk);
	fclose(f);
}
