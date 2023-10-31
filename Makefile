all:
.SILENT:

#------------------------------------------------------------------------------
# Configuration.

PRECMD=echo "  $(@F)" ; mkdir -p "$(@D)" ;

GAME:=out/ttaq

CC:=gcc -c -MMD -O2 -Isrc -Werror -Wimplicit -I/opt/vc/include
AS:=gcc -xassembler-with-cpp -c -O2
LD:=gcc -L/opt/vc/lib
LDPOST:=-lpig-static -lpipng-static -llinput-static -lGLESv2 -lz -lakau-static -lasound

# Hello user!
# Please pick the appropriate configuration and enable it like so:
#   ifeq (old pi with bcm,old pi with bcm) # <-- same string twice = enabled

ifeq (old pi with bcm,)
  OPT_ENABLE:=bcm alsa evdev
  CC:=gcc -c -MMD -O2 -Isrc -Werror -Wimplicit -I/opt/vc/include $(foreach U,$(OPT_ENABLE),-DUSE_$U)
  AS:=gcc -xassembler-with-cpp -c -O2
  LD:=gcc -L/opt/vc/lib
  LDPOST:=-lGLESv2 -lz -lasound
  
else ifeq (linux desktop,linux desktop)
  OPT_ENABLE:=glx pulse evdev
  CC:=gcc -c -MMD -O3 -Isrc -Werror -Wimplicit $(foreach U,$(OPT_ENABLE),-DUSE_$U)
  AS:=gcc -xassembler-with-cpp -c -O3
  LD:=gcc
  LDPOST:=-lm -lz -lpulse-simple -lGL -lGLX -lX11
  
else ifeq (linux guiless,)
  OPT_ENABLE:=drm alsa evdev
  CC:=gcc -c -MMD -O3 -Isrc -Werror -Wimplicit -I/usr/include/libdrm $(foreach U,$(OPT_ENABLE),-DUSE_$U)
  AS:=gcc -xassembler-with-cpp -c -O3
  LD:=gcc
  LDPOST:=-lm -lz -lasound -lGL -ldrm
  
else
  $(error Please select configuration. Edit Makefile)
endif

#------------------------------------------------------------------------------
# Build programs.

CFILES:=$(shell find src -name '*.c')
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
