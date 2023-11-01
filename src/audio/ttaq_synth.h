/* ttaq_synth.h
 * 2023-10-31: I've dropped the old "akau" synthesizer+driver, and now need to write a complete replacement.
 * Going to keep it simple, I guess.
 * This is private to the "ttaq_audio" unit; rest of the app should use ttaq_audio's interface for everything.
 */
 
#ifndef TTAQ_SYNTH_H
#define TTAQ_SYNTH_H

#include <stdint.h>

#define TTAQ_SYNTH_VOICE_LIMIT 16
#define TTAQ_SYNTH_BUFFER_SIZE 512

struct ttaq_voice;

struct ttaq_synth {
  int rate,chanc;
  int songid;
  struct ttaq_voice *voicev;
  int voicec;
  float buffer[TTAQ_SYNTH_BUFFER_SIZE];
};

void ttaq_synth_cleanup(struct ttaq_synth *synth);
int ttaq_synth_init(struct ttaq_synth *synth,int rate,int chanc);

int ttaq_synth_load_song(struct ttaq_synth *synth,int songid,const void *v,int c);

/* This must match our drivers' callback signatures, ttaq_audio will assign it directly.
 */
void ttaq_synth_update(int16_t *v,int c,struct ttaq_synth *synth);

void ttaq_synth_play_sound(struct ttaq_synth *synth,int soundid);
void ttaq_synth_play_song(struct ttaq_synth *synth,int songid);

#endif
