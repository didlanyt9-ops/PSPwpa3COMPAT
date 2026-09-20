#include <pspkernel.h>
#include <pspnet.h>
#include <pspnet_apctl.h>
#include <pspnet_inet.h>
#include <psputility.h>
#include <psputility_netparam.h>
#include <psputility_netmodules.h>
#include <pspwlan.h>
#include <stdio.h>
#include <string.h>

#include "net.h"

static int g_net_ready;

const char *net_state_name(int state)
{
	switch (state) {
	case PSP_NET_APCTL_STATE_DISCONNECTED: return "disconnected";
	case PSP_NET_APCTL_STATE_SCANNING:     return "scanning";
	case PSP_NET_APCTL_STATE_JOINING:      return "joining";
	case PSP_NET_APCTL_STATE_GETTING_IP:   return "getting IP";
	case PSP_NET_APCTL_STATE_GOT_IP:       return "got IP";
	case PSP_NET_APCTL_STATE_EAP_AUTH:     return "EAP auth";
	case PSP_NET_APCTL_STATE_KEY_EXCHANGE: return "key exchange";
	default:                               return "unknown";
	}
}

int net_wlan_ready(void)
{
	return sceWlanDevIsPowerOn() && sceWlanGetSwitchState();
}

int net_init(void)
{
	int err;

	if (g_net_ready) {
		return 0;
	}

	sceUtilityLoadNetModule(PSP_NET_MODULE_COMMON);
	sceUtilityLoadNetModule(PSP_NET_MODULE_INET);

	err = sceNetInit(128 * 1024, 42, 4 * 1024, 42, 4 * 1024);
	if (err < 0) {
		return err;
	}
	err = sceNetInetInit();
	if (err < 0) {
		return err;
	}
	err = sceNetApctlInit(0x8000, 48);
	if (err < 0) {
		return err;
	}

	g_net_ready = 1;
	return 0;
}

void net_term(void)
{
	if (!g_net_ready) {
		return;
	}
	sceNetApctlDisconnect();
	sceNetApctlTerm();
	sceNetInetTerm();
	sceNetTerm();
	sceUtilityUnloadNetModule(PSP_NET_MODULE_INET);
	sceUtilityUnloadNetModule(PSP_NET_MODULE_COMMON);
	g_net_ready = 0;
}

int net_find_profile_slot(void)
{
	int i;
	int empty = 0;
	netData data;

	for (i = 1; i <= 10; i++) {
		if (sceUtilityCheckNetParam(i) < 0) {
			if (!empty) {
				empty = i;
			}
			continue;
		}
		memset(&data, 0, sizeof(data));
		if (sceUtilityGetNetParam(i, PSP_NETPARAM_NAME, &data) == 0) {
			if (!strcmp(data.asString, PROFILE_NAME)) {
				return i;
			}
		}
	}
	return empty ? empty : 10;
}

static int set_str(int param, const char *value)
{
	netData data;
	memset(&data, 0, sizeof(data));
	strncpy(data.asString, value, sizeof(data.asString) - 1);
	return sceUtilitySetNetParam(param, &data);
}

static int set_u32(int param, unsigned int value)
{
	netData data;
	memset(&data, 0, sizeof(data));
	data.asUint = value;
	return sceUtilitySetNetParam(param, &data);
}

int net_write_transition_profile(AppState *state)
{
	int slot = net_find_profile_slot();
	int err;

	if (sceUtilityCheckNetParam(slot) == 0) {
		sceUtilityDeleteNetParam(slot);
	}

	err = sceUtilityCreateNetParam(slot);
	if (err < 0) {
		snprintf(state->last_error, sizeof(state->last_error),
			 "CreateNetParam failed %08X", err);
		return err;
	}

	/* sceUtilitySetNetParam always writes working slot 0; copy it out after. */
	if ((err = set_str(PSP_NETPARAM_NAME, PROFILE_NAME)) < 0) {
		goto fail;
	}
	if ((err = set_str(PSP_NETPARAM_SSID, state->ssid)) < 0) {
		goto fail;
	}
	if ((err = set_u32(PSP_NETPARAM_SECURE, NETPARAM_SECURE_WPA_AES)) < 0) {
		goto fail;
	}
	if ((err = set_str(PSP_NETPARAM_WEPKEY, state->psk)) < 0) {
		goto fail;
	}
	if ((err = set_u32(PSP_NETPARAM_IS_STATIC_IP, 0)) < 0) {
		goto fail;
	}
	if ((err = set_u32(PSP_NETPARAM_MANUAL_DNS, 0)) < 0) {
		goto fail;
	}

	err = sceUtilityCopyNetParam(0, slot);
	if (err < 0) {
		goto fail;
	}

	state->slot = slot;
	snprintf(state->last_error, sizeof(state->last_error),
		 "Wrote WPA2-AES profile in slot %d", slot);
	return 0;

fail:
	snprintf(state->last_error, sizeof(state->last_error),
		 "SetNetParam failed %08X", err);
	return err;
}

int net_connect(AppState *state)
{
	int err;

	if (!net_wlan_ready()) {
		snprintf(state->last_error, sizeof(state->last_error),
			 "WLAN switch is off");
		return -1;
	}
	if (!state->ssid[0] || !state->psk[0]) {
		snprintf(state->last_error, sizeof(state->last_error),
			 "Set SSID and passphrase first");
		return -1;
	}

	err = net_init();
	if (err < 0) {
		snprintf(state->last_error, sizeof(state->last_error),
			 "net_init failed %08X", err);
		return err;
	}

	err = net_write_transition_profile(state);
	if (err < 0) {
		return err;
	}

	sceNetApctlDisconnect();
	sceKernelDelayThread(200 * 1000);

	err = sceNetApctlConnect(state->slot);
	if (err < 0) {
		snprintf(state->last_error, sizeof(state->last_error),
			 "apctl connect failed %08X", err);
		return err;
	}

	snprintf(state->last_error, sizeof(state->last_error),
		 "Connecting via WPA2-AES (WPA3 transition)...");
	state->connected = 0;
	state->ip[0] = 0;
	return 0;
}

void net_disconnect(AppState *state)
{
	sceNetApctlDisconnect();
	state->connected = 0;
	state->ip[0] = 0;
	snprintf(state->last_error, sizeof(state->last_error), "Disconnected");
}

int net_poll(AppState *state)
{
	int st = 0;
	union SceNetApctlInfo info;

	if (!g_net_ready) {
		return PSP_NET_APCTL_STATE_DISCONNECTED;
	}
	if (sceNetApctlGetState(&st) < 0) {
		return -1;
	}

	if (st == PSP_NET_APCTL_STATE_GOT_IP) {
		memset(&info, 0, sizeof(info));
		if (sceNetApctlGetInfo(PSP_NET_APCTL_INFO_IP, &info) == 0) {
			strncpy(state->ip, info.ip, sizeof(state->ip) - 1);
		}
		state->connected = 1;
		snprintf(state->last_error, sizeof(state->last_error),
			 "Online. PSP used WPA2-AES, not SAE.");
	} else if (st == PSP_NET_APCTL_STATE_DISCONNECTED && state->connected) {
		state->connected = 0;
		state->ip[0] = 0;
		snprintf(state->last_error, sizeof(state->last_error),
			 "Dropped. If the AP is WPA3-only (SAE/PMF), PSP cannot join.");
	}

	return st;
}
