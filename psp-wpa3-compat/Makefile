TARGET = wpa3compat
OBJS = src/main.o src/ui.o src/net.o src/config.o

INCDIR = include
CFLAGS = -O2 -G0 -Wall -Wextra -fno-strict-aliasing
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = $(CFLAGS)

LIBDIR =
LDFLAGS =
LIBS = -lpspwlan -lpspnet -lpspnet_inet -lpspnet_apctl -lpsputility -lpspgu

EXTRA_TARGETS = EBOOT.PBP
PSP_EBOOT_TITLE = WPA3 Compat
PSP_EBOOT_ICON = assets/ICON0.PNG

PSPSDK = $(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak
