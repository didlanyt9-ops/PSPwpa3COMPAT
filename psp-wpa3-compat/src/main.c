#include <pspdebug.h>
#include <pspdisplay.h>
#include <pspctrl.h>
#include <pspkernel.h>
#include <stdio.h>
#include <string.h>

#include "app.h"
#include "config.h"
#include "net.h"
#include "ui.h"

PSP_MODULE_INFO("WPA3Compat", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);
PSP_HEAP_SIZE_KB(2048);

static int g_running = 1;

static int exit_callback(int arg1, int arg2, void *common)
{
	(void)arg1;
	(void)arg2;
	(void)common;
	g_running = 0;
	return 0;
}

static int callback_thread(SceSize args, void *argp)
{
	int cbid;
	(void)args;
	(void)argp;
	cbid = sceKernelCreateCallback("Exit Callback", exit_callback, NULL);
	sceKernelRegisterExitCallback(cbid);
	sceKernelSleepThreadCB();
	return 0;
}

static int setup_callbacks(void)
{
	int thid = sceKernelCreateThread("update_thread", callback_thread, 0x11, 0xFA0, 0, 0);
	if (thid >= 0) {
		sceKernelStartThread(thid, 0, 0);
	}
	return thid;
}

static void mask_psk(const char *psk, char *out, int outlen)
{
	int n = (int)strlen(psk);
	int i;
	if (outlen <= 0) {
		return;
	}
	if (n == 0) {
		strncpy(out, "(empty)", outlen - 1);
		out[outlen - 1] = 0;
		return;
	}
	for (i = 0; i < n && i < outlen - 1; i++) {
		out[i] = '*';
	}
	out[i] = 0;
}

static void draw(const AppState *state, int cursor, int ap_state)
{
	char hidden[PSK_MAX + 1];

	mask_psk(state->psk, hidden, sizeof(hidden));
	ui_clear();

	pspDebugScreenSetTextColor(0x00FFFFAA);
	pspDebugScreenPrintf("  %s  v%s\n", APP_TITLE, APP_VERSION);
	pspDebugScreenSetTextColor(0x00AAAAAA);
	pspDebugScreenPrintf("  WPA3-Personal SAE is not on PSP silicon.\n");
	pspDebugScreenPrintf("  This client joins WPA3 transition APs as WPA2-AES.\n\n");

	pspDebugScreenSetTextColor(0x00FFFFFF);
	pspDebugScreenPrintf("  SSID : %s\n", state->ssid[0] ? state->ssid : "(not set)");
	pspDebugScreenPrintf("  Pass : %s\n", hidden);
	pspDebugScreenPrintf("  Slot : %s%d\n", state->slot ? "" : "-", state->slot);
	pspDebugScreenPrintf("  Link : %s", net_state_name(ap_state < 0 ? 0 : ap_state));
	if (state->ip[0]) {
		pspDebugScreenPrintf("  %s", state->ip);
	}
	pspDebugScreenPrintf("\n");
	pspDebugScreenSetTextColor(0x0055DDFF);
	pspDebugScreenPrintf("  %s\n\n", state->last_error[0] ? state->last_error : "Ready.");

	pspDebugScreenSetTextColor(0x00FFFFFF);
	pspDebugScreenPrintf("  %s Set SSID\n", cursor == 0 ? ">" : " ");
	pspDebugScreenPrintf("  %s Set passphrase (WPA2/WPA3 PSK)\n", cursor == 1 ? ">" : " ");
	pspDebugScreenPrintf("  %s Connect (WPA3 transition)\n", cursor == 2 ? ">" : " ");
	pspDebugScreenPrintf("  %s Disconnect\n", cursor == 3 ? ">" : " ");
	pspDebugScreenPrintf("  %s Save config\n", cursor == 4 ? ">" : " ");
	pspDebugScreenPrintf("  %s Exit\n\n", cursor == 5 ? ">" : " ");

	pspDebugScreenSetTextColor(0x00777777);
	pspDebugScreenPrintf("  D-Pad + X. WLAN switch on. CFW/HEN required.\n");
	pspDebugScreenPrintf("  WPA3-only routers (SAE required) will fail.\n");
}

int main(int argc, char *argv[])
{
	AppState state;
	int cursor = 0;
	int ap_state = 0;

	(void)argc;
	(void)argv;

	setup_callbacks();
	ui_init();
	memset(&state, 0, sizeof(state));
	config_load(&state);
	snprintf(state.last_error, sizeof(state.last_error),
		 "Loaded. Use a mixed WPA2/WPA3 AP, not WPA3-only.");

	while (g_running) {
		unsigned int btn = ui_buttons();
		ap_state = net_poll(&state);

		if (btn & PSP_CTRL_UP) {
			cursor = (cursor + 5) % 6;
		} else if (btn & PSP_CTRL_DOWN) {
			cursor = (cursor + 1) % 6;
		} else if (btn & PSP_CTRL_CROSS) {
			if (cursor == 0) {
				char tmp[SSID_MAX + 1];
				strncpy(tmp, state.ssid, SSID_MAX);
				if (ui_osk("SSID", state.ssid, tmp, SSID_MAX, 0) > 0) {
					strncpy(state.ssid, tmp, SSID_MAX);
					state.ssid[SSID_MAX] = 0;
				}
			} else if (cursor == 1) {
				char tmp[PSK_MAX + 1];
				tmp[0] = 0;
				if (ui_osk("Passphrase", "", tmp, PSK_MAX, 1) > 0) {
					strncpy(state.psk, tmp, PSK_MAX);
					state.psk[PSK_MAX] = 0;
				}
			} else if (cursor == 2) {
				net_connect(&state);
			} else if (cursor == 3) {
				net_disconnect(&state);
			} else if (cursor == 4) {
				config_save(&state);
				snprintf(state.last_error, sizeof(state.last_error),
					 "Saved to %s", CONFIG_PATH);
			} else if (cursor == 5) {
				g_running = 0;
			}
		} else if (btn & PSP_CTRL_CIRCLE) {
			g_running = 0;
		}

		draw(&state, cursor, ap_state);
		sceDisplayWaitVblankStart();
	}

	net_term();
	sceKernelExitGame();
	return 0;
}
