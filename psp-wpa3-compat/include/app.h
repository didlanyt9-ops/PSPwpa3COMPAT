#ifndef WPA3COMPAT_APP_H
#define WPA3COMPAT_APP_H

#define APP_TITLE        "WPA3 Compat"
#define APP_VERSION      "1.0"
#define PROFILE_NAME     "WPA3Compat"
#define CONFIG_DIR       "ms0:/PSP/GAME/WPA3COMPAT"
#define CONFIG_PATH      CONFIG_DIR "/config.ini"

#define SSID_MAX         32
#define PSK_MAX          63

/* XMB "WPA-PSK (AES)" — the only PSP cipher that WPA3 transition APs still speak. */
#define NETPARAM_SECURE_WPA_AES  4

typedef struct {
	char ssid[SSID_MAX + 1];
	char psk[PSK_MAX + 1];
	int slot;
	int connected;
	char ip[16];
	char last_error[96];
} AppState;

#endif
