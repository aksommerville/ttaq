#include "ttaq_audio_internal.h"

struct ttaq_audio ttaq_audio={0};

/* Init.
 */

int ttaq_audio_init() {
  if (ttaq_audio.rate) return -1;
  memset(&ttaq_audio,0,sizeof(struct ttaq_audio));
  
  #if USE_pulse
    if (ttaq_audio.pulse=pulse_new(...)) {//TODO
      ttaq_audio.rate=pulse_get_rate(ttaq_audio.pulse);
      ttaq_audio.chanc=pulse_get_chanc(ttaq_audio.pulse);
    } else {
      fprintf(stderr,"WARNING: Failed to initialize PulseAudio. Will proceed without sound.\n");
    }
    
  #elif USE_alsa
    struct alsapcm_delegate delegate={
      .userdata=&ttaq_audio.synth,
      .pcm_out=(void*)ttaq_synth_update,
    };
    if (ttaq_audio.alsa=alsapcm_new(&delegate,0)) {
      ttaq_audio.rate=alsapcm_get_rate(ttaq_audio.alsa);
      ttaq_audio.chanc=alsapcm_get_chanc(ttaq_audio.alsa);
    } else {
      fprintf(stderr,"WARNING: Failed to initialize ALSA. Will proceed without sound.\n");
    }
  #endif
  
  if (!ttaq_audio.rate) return 0;
  if (ttaq_synth_init(&ttaq_audio.synth,ttaq_audio.rate,ttaq_audio.chanc)<0) {
    fprintf(stderr,"Failed to initialize synthesizer.\n");
    return -2;
  }
  
  return 0;
}

/* Quit.
 */
 
void ttaq_audio_quit() {
  #if USE_pulse
    pulse_del(ttaq_audio.pulse);
  #elif USE_alsa
    alsapcm_del(ttaq_audio.alsa);
  #endif
  ttaq_synth_cleanup(&ttaq_audio.synth);
  memset(&ttaq_audio,0,sizeof(struct ttaq_audio));
}

/* Update.
 */

int ttaq_audio_update() {
  if (!ttaq_audio.rate) return 0;
  #if USE_pulse
    if (pulse_update(ttaq_audio.pulse)<0) return -1;
  #elif USE_alsa
    if (alsapcm_update(ttaq_audio.alsa)<0) return -1;
  #endif
  return 0;
}

/* Play/stop.
 * When stopping, also poke the mutex to ensure callback is not running by the time we return.
 */
 
int ttaq_audio_play(int play) {
  if (!ttaq_audio.rate) return 0;
  #if USE_pulse
    pulse_play(ttaq_audio.pulse,play);
  #elif USE_alsa
    alsapcm_set_running(ttaq_audio.alsa,play);
  #endif
  if (!play) {
    if (ttaq_audio_lock()>=0) ttaq_audio_unlock();
  }
  return 0;
}

/* Play sound effect.
 */

int ttaq_audio_play_sound(int soundid) {
  if (!ttaq_audio.rate) return 0;
  if (ttaq_audio_lock()>=0) {
    ttaq_synth_play_sound(&ttaq_audio.synth,soundid);
    ttaq_audio_unlock();
  }
  return 0;
}

/* Play song.
 */
 
int ttaq_audio_play_song(int songid) {
  if (!ttaq_audio.rate) return 0;
  if (songid==ttaq_audio.synth.songid) return 0;
  if (ttaq_audio_lock()>=0) {
    ttaq_synth_play_song(&ttaq_audio.synth,songid);
    ttaq_audio_unlock();
  }
  return 0;
}

/* Lock.
 */

int ttaq_audio_lock() {
  #if USE_pulse
    return pulse_lock(ttaq_audio.pulse);
  #elif USE_alsa
    return alsapcm_lock(ttaq_audio.alsa);
  #endif
  return 0;
}

void ttaq_audio_unlock() {
  #if USE_pulse
    pulse_unlock(ttaq_audio.pulse);
  #elif USE_alsa
    alsapcm_unlock(ttaq_audio.alsa);
  #endif
}

/* Load data.
 */
 
int ttaq_audio_load_song(int songid,const void *src,int srcc) {
  if (!ttaq_audio.rate) return 0;
  return ttaq_synth_load_song(&ttaq_audio.synth,songid,src,srcc);
}
