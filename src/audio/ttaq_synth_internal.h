#ifndef TTAQ_SYNTH_INTERNAL_H
#define TTAQ_SYNTH_INTERNAL_H

#include "ttaq_synth.h"
#include <string.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <stdio.h>
#include <math.h>

/* Envelope.
 **********************************************************************/

struct ttaq_env {
  int hold;
  float level;
  float leveld;
  int levelc;
  int stage; // 0,1,2,3=attack,decay,release,finished. Lingers in decay if sustained.
  float level_dec,level_rls,level_fin; // initial level, at start of decay and release stages.
  float leveld_dec,leveld_rls;
  int levelc_dec,levelc_rls;
  int sustain,released;
};

void ttaq_env_init_constant(struct ttaq_env *env,float v);

void ttaq_env_init_level(
  struct ttaq_env *env,
  const struct ttaq_synth *synth,
  uint8_t atkl,
  uint8_t atkt_ms,
  uint8_t rel_decl,
  uint8_t sustain,
  uint16_t rlst_ms
);

void ttaq_env_init_ramp(
  struct ttaq_env *env,
  const struct ttaq_synth *synth,
  float levela,
  uint16_t dur_ms,
  float levelz
);

void ttaq_env_init_ramp_hz(
  struct ttaq_env *env,
  const struct ttaq_synth *synth,
  float pitcha,
  uint16_t dur_ms,
  float pitchz
);

void ttaq_env_init_generic(
  struct ttaq_env *env,
  const struct ttaq_synth *synth,
  float initlevel,
  uint16_t atkt_ms,
  float atklevel,
  uint16_t dect_ms,
  float declevel,
  uint8_t sustain,
  uint16_t rlst_ms,
  float rlslevel
);

void ttaq_env_trim(struct ttaq_env *env,float level);

void ttaq_env_release(struct ttaq_env *env);

static inline int ttaq_env_is_finished(const struct ttaq_env *env) {
  return env->stage>=3;
}

// Only ttaq_env_next() should call this.
void ttaq_env_advance_stage(struct ttaq_env *env);

static inline float ttaq_env_next(struct ttaq_env *env) {
  if (env->hold) return env->level;
  if (env->levelc<=0) {
    ttaq_env_advance_stage(env);
  } else {
    env->level+=env->leveld;
    env->levelc--;
  }
  return env->level;
}

/* Voice.
 *********************************************************************/

struct ttaq_voice {

  float p;
  struct ttaq_env pd; // Main rate in c/frame (should be very small). Usually constant.
  float modp;
  float modrate; // Absolute c/frame, or multiplier against (pd), depends on (update).
  struct ttaq_env modrange; // Amount of FM effect, typically in like 0..5
  struct ttaq_env level; // Output level in 0..1
  int autorelease; // frames. <=0 means not scheduled to autorelease

  /* Required.
   * Add to (v), preserve existing signal.
   */
  void (*update)(float *v,int c,struct ttaq_synth *synth,struct ttaq_voice *voice);
  
  /* (update) or (release) should set, once it's known that (update) will be noop from now on.
   */
  int defunct;
};

/* Returns an unused voice, or null if everything is in use.
 * Caller must set (voice->update) and unset (voice->defunct) to accept it.
 */
struct ttaq_voice *ttaq_voice_new(struct ttaq_synth *synth);

void ttaq_voice_release(struct ttaq_voice *voice);

/* Song.
 * The ttaq_song object is both the data container and the live player.
 ***************************************************************************/
 
struct ttaq_song_event {
  int time; // frames from start of song
  float rate; // normalized (0..1/2)
  float level; // 0..1
  int ttl;
  int soundid;
};

struct ttaq_song {
// Permanent:
  int mainrate;
  struct ttaq_song_event *eventv;
  int eventc,eventa;
  int tempo,frames_per_beat; // only relevant during decode
  int framec;
// Volatile:
  int eventp;
  int time; // frames
};

void ttaq_song_del(struct ttaq_song *song);
struct ttaq_song *ttaq_song_new(const void *v,int c,int mainrate,int songid);

/* Drop all state and start over from the beginning.
 */
void ttaq_song_reset(struct ttaq_song *song);

/* If an event is ready to play, populate (event) and return zero.
 * Otherwise return the duration in frames until the next event.
 */
int ttaq_song_update(struct ttaq_song_event *event,struct ttaq_song *song);

/* Advance our clock by so many frames.
 * Call this after ttaq_song_update() returns positive, with a number no greater than that.
 */
void ttaq_song_advance(struct ttaq_song *song,int framec);

void ttaq_synth_event(struct ttaq_synth *synth,const struct ttaq_song_event *event);

#endif
