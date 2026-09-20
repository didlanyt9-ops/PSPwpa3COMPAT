# PSP WPA3 Compat

Homebrew client that lets a PSP join a **WPA3 transition** (mixed WPA2/WPA3) network.

Sony never added WPA3-Personal (SAE + PMF) to the Marvell 88W8686 stack. This EBOOT writes a **WPA-PSK AES** infrastructure profile using your WPA3 passphrase and connects through `sceNetApctl`. That is the same cipher WPA3 transition access points still offer to legacy stations. A WPA3-**only** AP will refuse the PSP.

## Install

Copy `EBOOT.PBP` to:

`ms0:/PSP/GAME/WPA3COMPAT/EBOOT.PBP`

Needs CFW or HEN. Turn the WLAN switch on.

## Use

1. Set SSID
2. Set the same passphrase as the WPA2/WPA3 network
3. Connect (WPA3 transition)
4. On the router, enable **WPA2/WPA3** or **WPA3 transition**, not WPA3-only

SSID and passphrase are stored in `ms0:/PSP/GAME/WPA3COMPAT/config.ini`.

## Build

Needs [pspdev](https://github.com/pspdev/pspdev).

```sh
python3 tools/make_icon.py
make
```

GitHub Actions also produces `EBOOT.PBP` as an artifact.

## Limits

- No SAE handshake and no 802.11w (PMF)
- Ad-hoc / infrastructure games still use the firmware Wi-Fi stack
- Overwrites or reuses a saved connection named `WPA3Compat` in slots 1–10
