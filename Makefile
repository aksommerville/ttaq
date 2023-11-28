all:
.SILENT:

#------------------------------------------------------------------------------
# Configuration.

PRECMD=echo "  $(@F)" ; mkdir -p "$(@D)" ;

GAME:=out/ttaq

ifndef BUILDCONFIG
  UNAMENM:=$(shell uname -nm)
  UNAMES:=$(shell uname -s)
  ifeq ($(UNAMENM),nuc x86_64)
    BUILDCONFIG:=linux-desktop
  else ifeq ($(UNAMENM),vcs x86_64) # TODO confirm
    BUILDCONFIG:=linux-guiless
  else ifeq ($(UNAMENM),raspberrypi armv7) # TODO confirm
    BUILDCONFIG:=linux-guiless
  else ifeq ($(UNAMENM),raspberrypi armv6l) # TODO confirm
    BUILDCONFIG:=linux-raspi1
  else ifeq ($(UNAMES),Linux)
    BUILDCONFIG:=linux-desktop
  else ifeq ($(UNAMES),Darwin)
    $(error MacOS not supported)
  else ifneq (,$(strip $(filter MINGW%,$(UNAMES))))
    $(error MS Windows not supported)
  else
    $(error Unable to guess host configuration. Please set BUILDCONFIG or edit Makefile)
  endif
endif

# "linux-raspi1": Specific to early Raspberry Pi. Newer ones should use "linux-guiless". (if Linux DRM is available)
ifeq ($(BUILDCONFIG),linux-raspi1)
  OPT_ENABLE:=bcm alsa evdev
  CC:=gcc -c -MMD -O2 -Isrc -Werror -Wimplicit -I/opt/vc/include $(foreach U,$(OPT_ENABLE),-DUSE_$U) -DTTAQ_GLSL_VERSION=100
  AS:=gcc -xassembler-with-cpp -c -O2
  LD:=gcc -L/opt/vc/lib
  LDPOST:=-lm -lz -lpthread -lGLESv2 -lEGL -lbcm_host

# "linux-desktop": GLX for video and keyboard events. A typical choice.
else ifeq ($(BUILDCONFIG),linux-desktop)
  OPT_ENABLE:=glx alsa evdev
  CC:=gcc -c -MMD -O3 -Isrc -Werror -Wimplicit $(foreach U,$(OPT_ENABLE),-DUSE_$U) -DTTAQ_GLSL_VERSION=120
  AS:=gcc -xassembler-with-cpp -c -O3
  LD:=gcc
  LDPOST:=-lm -lz -lpthread -lGL -lGLX -lX11

# "linux-guiless": DRM. Won't run with an X server. Good for newer Raspberry Pi, and bespoke game consoles.
else ifeq ($(BUILDCONFIG),linux-guiless)
  OPT_ENABLE:=drm alsa evdev
  CC:=gcc -c -MMD -O3 -Isrc -Werror -Wimplicit -I/usr/include/libdrm $(foreach U,$(OPT_ENABLE),-DUSE_$U) -DTTAQ_GLSL_VERSION=120
  AS:=gcc -xassembler-with-cpp -c -O3
  LD:=gcc
  LDPOST:=-lm -lz -lpthread -lGL -ldrm -lEGL -lgbm
  # DRM device is usually card0, but on the Pi 4 for some reason it's always card1. card0 is the default.
  ifeq ($(shell uname -n),raspberrypi)
    CC+=-DTTAQ_DRM_DEVICE=\"/dev/dri/card1\"
  endif
  
else
  $(error BUILDCONFIG $(BUILDCONFIG) not defined. Edit Makefile)
endif

#------------------------------------------------------------------------------
# Build programs.

CFILES:=$(shell find src -name '*.c')
CFILES:=$(filter-out src/opt/%,$(CFILES)) $(filter $(addprefix src/opt/,$(addsuffix /%,$(OPT_ENABLE))),$(CFILES))
OFILES_ALL:=$(patsubst src/%.c,mid/%.o,$(CFILES))
-include $(OFILES_ALL:.o=.d)
mid/%.o:src/%.c;$(PRECMD) $(CC) -o $@ $<

SHADERSRC:=$(wildcard src/shaders/*.essl)
SHADERDST:=$(patsubst src/%.essl,mid/%.o,$(SHADERSRC))
SHADERGLUE:=$(SHADERDST:.o=.s)
mid/shaders/%.o:mid/shaders/%.s src/shaders/%.essl;$(PRECMD) $(AS) -o $@ $<
OFILES_ALL+=$(SHADERDST)
mid/shaders/%.s:;$(PRECMD) etc/tool/mkshaderglue $@
all:$(SHADERGLUE)

ifneq (,$(strip $(GAME)))
  all:$(GAME)
  OFILES_GAME:=$(filter-out mid/editor/%,$(OFILES_ALL))
  $(GAME):$(OFILES_GAME);$(PRECMD) $(LD) -o $@ $^ $(LDPOST)
  run:$(GAME);mkdir -p ~/.ttaq ; cp etc/input ~/.ttaq/input ; $(GAME)
else
  run:;echo "Not building game" ; exit 1
endif

#------------------------------------------------------------------------------
# Special targets.

clean:;rm -rf mid out editor/*.pyc

edit:;python editor/adv_editor.py

INSTALLPFX:=/usr/local/
SUDO:=sudo
install:$(GAME); \
  $(SUDO) rm -rf $(INSTALLPFX)bin/ttaq $(INSTALLPFX)share/ttaq || exit 1 ; \
  $(SUDO) cp $(GAME) $(INSTALLPFX)bin/ttaq || exit 1 ; \
  $(SUDO) mkdir -p $(INSTALLPFX)share/ttaq || exit 1 ; \
  $(SUDO) cp -r src/data $(INSTALLPFX)share/ttaq/data || exit 1 ; \
  mkdir -p ~/.ttaq || exit 1 ; \
  touch ~/.ttaq/saved-game || exit 1 ; \
  cp etc/input ~/.ttaq/input || exit 1 ; \
  echo "Installed successfully. Enter 'ttaq' anywhere to play."
  
uninstall:; \
  $(SUDO) rm -rf $(INSTALLPFX)bin/ttaq $(INSTALLPFX)share/ttaq || exit 1 ; \
  rm -rf ~/.ttaq || exit 1
