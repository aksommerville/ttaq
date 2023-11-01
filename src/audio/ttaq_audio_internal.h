#ifndef TTAQ_AUDIO_INTERNAL_H
#define TTAQ_AUDIO_INTERNAL_H

#include "ttaq_audio.h"
#include "ttaq_synth.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#if USE_pulse
  #include "opt/pulse/pulse.h"
  #define HAVE_AUDIO_DRIVER 1
#endif
#if USE_alsa
  #include "opt/alsa/alsapcm.h"
  #define HAVE_AUDIO_DRIVER 2
#endif
#if !HAVE_AUDIO_DRIVER
  // It's OK to build with no audio support.
#endif

extern struct ttaq_audio {
  #if USE_pulse
    struct pulse *pulse;
  #elif USE_alsa
    struct alsapcm *alsa;
  #endif
  int rate,chanc;
  struct ttaq_synth synth;
} ttaq_audio;

#endif
