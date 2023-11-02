/* ttaq_synth.h
 * 2023-10-31: I've dropped the old "akau" synthesizer+driver, and now need to write a complete replacement.
 * Going to keep it simple, I guess.
 * This is private to the "ttaq_audio" unit; rest of the app should use ttaq_audio's interface for everything.
 */
 
#ifndef TTAQ_SYNTH_H
#define TTAQ_SYNTH_H

#include <stdint.h>

/* No particular reason for these limits.
 * Just declaring fixed sizes for this to avoid dynamic allocation.
 * Perfectly safe to change.
 */
#define TTAQ_SYNTH_VOICE_LIMIT 32
#define TTAQ_SYNTH_BUFFER_SIZE 512
#define TTAQ_SYNTH_SONG_LIMIT 20

struct ttaq_voice;
struct ttaq_song;

struct ttaq_synth {
  int rate,chanc;
  int songid;
  struct ttaq_voice *voicev;
  int voicec;
  float buffer[TTAQ_SYNTH_BUFFER_SIZE];
  struct ttaq_song *songv[TTAQ_SYNTH_SONG_LIMIT];
  struct ttaq_song *song; // WEAK
};

void ttaq_synth_cleanup(struct ttaq_synth *synth);
int ttaq_synth_init(struct ttaq_synth *synth,int rate,int chanc);

int ttaq_synth_load_song(struct ttaq_synth *synth,int songid,const void *v,int c);

/* This must match our drivers' callback signatures, ttaq_audio will assign it directly.
 */
void ttaq_synth_update(int16_t *v,int c,struct ttaq_synth *synth);

void ttaq_synth_play_sound(struct ttaq_synth *synth,int soundid);
void ttaq_synth_play_note(struct ttaq_synth *synth,int instrumentid,float normrate,int ttl);
void ttaq_synth_play_song(struct ttaq_synth *synth,int songid);

void ttaq_synth_release_all(struct ttaq_synth *synth);
void ttaq_synth_silence_all(struct ttaq_synth *synth);

#endif
