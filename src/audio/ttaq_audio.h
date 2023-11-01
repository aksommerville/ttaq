/* ttaq_audio.h
 * Coordinator for the PCM-out driver and synthesizer.
 */
 
#ifndef TTAQ_AUDIO_H
#define TTAQ_AUDIO_H

int ttaq_audio_init();
void ttaq_audio_quit();

int ttaq_audio_update();

/* System is stopped initially.
 * While stopped, it's safe to load resources and whatever; the callback can't be running.
 */
int ttaq_audio_play(int play);

// No need to lock.
int ttaq_audio_play_sound(int soundid);
int ttaq_audio_play_song(int songid);

/* Locking is only necessary when loading resources.
 */
int ttaq_audio_lock();
void ttaq_audio_unlock();

int ttaq_audio_load_song(int songid,const void *src,int srcc);

#endif
