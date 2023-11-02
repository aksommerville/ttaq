#include "ttaq_synth_internal.h"

/* Cleanup.
 */

void ttaq_synth_cleanup(struct ttaq_synth *synth) {
  if (synth->voicev) free(synth->voicev);
  int i=TTAQ_SYNTH_SONG_LIMIT;
  while (i-->0) ttaq_song_del(synth->songv[i]);
  memset(synth,0,sizeof(struct ttaq_synth));
}

/* Init.
 */
 
int ttaq_synth_init(struct ttaq_synth *synth,int rate,int chanc) {

  if ((rate<200)||(rate>200000)) return -1;
  if ((chanc<1)||(chanc>8)) return -1;
  
  memset(synth,0,sizeof(struct ttaq_synth));
  if (!(synth->voicev=malloc(sizeof(struct ttaq_voice)*TTAQ_SYNTH_VOICE_LIMIT))) return -1;
  
  synth->rate=rate;
  synth->chanc=chanc;
  
  return 0;
}

/* Load song.
 */
 
int ttaq_synth_load_song(struct ttaq_synth *synth,int songid,const void *v,int c) {
  if ((songid<0)||(songid>=TTAQ_SYNTH_SONG_LIMIT)) return 0;
  struct ttaq_song *song=ttaq_song_new(v,c,synth->rate,songid);
  if (!song) return 0;
  ttaq_song_del(synth->songv[songid]);
  synth->songv[songid]=song;
  return 0;
}

/* New voice.
 */
 
struct ttaq_voice *ttaq_voice_new(struct ttaq_synth *synth) {
  if (synth->voicec<TTAQ_SYNTH_VOICE_LIMIT) {
    struct ttaq_voice *voice=synth->voicev+synth->voicec++;
    memset(voice,0,sizeof(struct ttaq_voice));
    voice->defunct=1;
    return voice;
  }
  struct ttaq_voice *voice=synth->voicev;
  int i=TTAQ_SYNTH_VOICE_LIMIT;
  for (;i-->0;voice++) {
    if (voice->defunct) return voice;
  }
  return 0;
}

/* Drop defunct voices.
 */
 
static void ttaq_synth_drop_defunct_voices(struct ttaq_synth *synth) {
  int i=synth->voicec;
  struct ttaq_voice *voice=synth->voicev+i-1;
  for (;i-->0;voice--) {
    if (voice->defunct||!voice->update) {
      synth->voicec--;
      memmove(voice,voice+1,sizeof(struct ttaq_voice)*(synth->voicec-i));
    }
  }
}

/* Generate an uninterrupted portion of the signal.
 * Voices operate floating-point, and output is integers.
 */
 
static void ttaq_synth_update_f(float *v,int c,struct ttaq_synth *synth) {
  memset(v,0,sizeof(float)*c);
  struct ttaq_voice *voice=synth->voicev;
  int i=synth->voicec;
  for (;i-->0;voice++) {
    if (voice->autorelease>0) {
      if ((voice->autorelease-=c)<=0) {
        ttaq_voice_release(voice);
      }
    }
    if (voice->update) {
      voice->update(v,c,synth,voice);
    } else {
      voice->defunct=1;
    }
  }
}

static void ttaq_synth_quantize(int16_t *dst,const float *src,int c,struct ttaq_synth *synth) {
  for (;c-->0;dst++,src++) {
    if (*src<=-1.0f) *dst=-32768;
    else if (*src>=1.0f) *dst=32767;
    else *dst=(*src)*32768.0f;
  }
}

static void ttaq_synth_generate_signal(int16_t *v,int c,struct ttaq_synth *synth) {
  while (c>=TTAQ_SYNTH_BUFFER_SIZE) {
    ttaq_synth_update_f(synth->buffer,TTAQ_SYNTH_BUFFER_SIZE,synth);
    ttaq_synth_quantize(v,synth->buffer,c,synth);
    v+=TTAQ_SYNTH_BUFFER_SIZE;
    c-=TTAQ_SYNTH_BUFFER_SIZE;
  }
  if (c>0) {
    ttaq_synth_update_f(synth->buffer,c,synth);
    ttaq_synth_quantize(v,synth->buffer,c,synth);
  }
}

/* Outer update for mono signals.
 * Interleave song events.
 * This gets called exactly once for each outer (update).
 */
 
static void ttaq_synth_update_mono(int16_t *v,int c,struct ttaq_synth *synth) {
  while (c>0) {
    int updc=c;
    
    if (synth->song) {
      int err;
      struct ttaq_song_event event;
      while (!(err=ttaq_song_update(&event,synth->song))) {
        ttaq_synth_event(synth,&event);
      }
      if (err<0) {
        ttaq_synth_play_song(synth,-1);
      } else if (err<updc) {
        updc=err;
      }
      if (synth->song) {
        ttaq_song_advance(synth->song,updc);
      }
    }
    
    ttaq_synth_generate_signal(v,updc,synth);
    v+=updc;
    c-=updc;
  }
  ttaq_synth_drop_defunct_voices(synth);
}

/* Expand mono to multi-channel.
 */
 
static void ttaq_synth_expand_stereo(int16_t *v,int framec) {
  int16_t *dst=v+(framec<<1);
  int16_t *src=v+framec;
  while (framec-->0) {
    int16_t sample=*(--src);
    dst-=2;
    dst[0]=dst[1]=sample;
  }
}

static void ttaq_synth_expand_multi(int16_t *v,int framec,int chanc) {
  int16_t *dst=v+framec*chanc;
  int16_t *src=v+framec;
  while (framec-->0) {
    int16_t sample=*(--src);
    int i=chanc; while (i-->0) *(--dst)=sample;
  }
}

/* Update.
 */

void ttaq_synth_update(int16_t *v,int c,struct ttaq_synth *synth) {
  switch (synth->chanc) {
    case 1: ttaq_synth_update_mono(v,c,synth); break;
    case 2: ttaq_synth_update_mono(v,c>>1,synth); ttaq_synth_expand_stereo(v,c>>1); break;
    default: {
        int framec=c/synth->chanc;
        ttaq_synth_update_mono(v,framec,synth);
        ttaq_synth_expand_multi(v,framec,synth->chanc);
      } break;
  }
}

/* Play song.
 */
 
void ttaq_synth_play_song(struct ttaq_synth *synth,int songid) {
  if (synth->songid==songid) return;
  struct ttaq_song *song=0;
  if ((songid>=0)&&(songid<TTAQ_SYNTH_SONG_LIMIT)) song=synth->songv[songid];
  synth->songid=songid;
  synth->song=song;
  ttaq_synth_release_all(synth);
  if (!synth->song) return;
  ttaq_song_reset(synth->song);
}

/* Release all notes.
 */
 
void ttaq_synth_release_all(struct ttaq_synth *synth) {
  struct ttaq_voice *voice=synth->voicev;
  int i=synth->voicec;
  for (;i-->0;voice++) ttaq_voice_release(voice);
}

void ttaq_synth_silence_all(struct ttaq_synth *synth) {
  synth->voicec=0;
}

/* Apply song event.
 */
 
void ttaq_synth_event(struct ttaq_synth *synth,const struct ttaq_song_event *event) {
  //fprintf(stderr,"%s rate=%f ttl=%d soundid=%d\n",__func__,event->rate,event->ttl,event->soundid);//TODO
  if (event->soundid<0) return; // invalid. shouldn't happen, but we could drop invalids in the stream if something fails during decode.
  if (event->rate>0.0f) { // tuned sustainable note
    ttaq_synth_play_note(synth,event->soundid,event->rate,event->ttl);
  } else { // fire-and-forget pcm note
    ttaq_synth_play_sound(synth,event->soundid);
  }
}
