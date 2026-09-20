#ifndef WPA3COMPAT_NET_H
#define WPA3COMPAT_NET_H

#include "app.h"

int net_init(void);
void net_term(void);

int net_wlan_ready(void);
int net_find_profile_slot(void);
int net_write_transition_profile(AppState *state);
int net_connect(AppState *state);
void net_disconnect(AppState *state);
int net_poll(AppState *state);

const char *net_state_name(int state);

#endif
