#include "adv_res_internal.h"
#include <akau.h>

/* synth instruments
 *****************************************************************************/
 
static const double _adv_synth_bass[]={
  1.0,
  0.9,
  0.8,
  0.7,
  0.6,
  0.5,
  0.4,
  0.3,
  0.2,
  0.1,
};
#define _adv_synth_bass_gain 8.0
#define _adv_synth_bass_trim 0.2

static const double _adv_synth_lead[]={
  1.0,
  0.2,
  0.5,
  0.1,
  0.2,
  0.0,
  0.1,
};
#define _adv_synth_lead_gain 1.0
#define _adv_synth_lead_trim 0.4

static const double _adv_synth_pad[]={
  1.0,
  0.1,
};
#define _adv_synth_pad_gain 1.0
#define _adv_synth_pad_trim 0.3

/* load sounds
 *****************************************************************************/
 
static int _adv_load_sound_1(const char *name,int soundid) {
  char path[1024];
  int pathc=snprintf(path,sizeof(path),"%s/sound/%s",adv_resmgr.root,name);
  if ((pathc<1)||(pathc>=sizeof(path))) return -1;
  void *pcm=0;
  int pcmc=adv_file_read(&pcm,path);
  if (pcmc<0) return -1;
  if (!pcm) return -1;
  int waveid=akau_wave_load(pcm,pcmc);
  free(pcm);
  if (waveid<0) {
    fprintf(stderr,"%s: failed to load sound effect\n",path);
    return -1;
  }
  if (waveid!=soundid) {
    fprintf(stderr,"%s: did not acquire wave id %d from akau as expected (got %d)\n",path,soundid,waveid);
    return -1;
  }
  akau_wave_set_name(waveid,name,-1);
  return 0;
}
 
int adv_res_load_sounds() {
  if (_adv_load_sound_1("sword",ADV_SOUND_SWORD)<0) return -1;
  if (_adv_load_sound_1("strike",ADV_SOUND_STRIKE)<0) return -1;
  if (_adv_load_sound_1("arrow",ADV_SOUND_ARROW)<0) return -1;
  if (_adv_load_sound_1("gadget",ADV_SOUND_GADGET)<0) return -1;
  if (_adv_load_sound_1("gadgetno",ADV_SOUND_GADGETNO)<0) return -1;
  if (_adv_load_sound_1("gadgetyes",ADV_SOUND_GADGETYES)<0) return -1;
  if (_adv_load_sound_1("revive",ADV_SOUND_REVIVE)<0) return -1;
  if (_adv_load_sound_1("rescue",ADV_SOUND_RESCUE)<0) return -1;
  if (_adv_load_sound_1("changectl",ADV_SOUND_CHANGECTL)<0) return -1;
  if (_adv_load_sound_1("switch",ADV_SOUND_SWITCH)<0) return -1;
  if (_adv_load_sound_1("hat",ADV_SOUND_HAT)<0) return -1;
  if (_adv_load_sound_1("kick",ADV_SOUND_KICK)<0) return -1;
  if (_adv_load_sound_1("ride",ADV_SOUND_RIDE)<0) return -1;
  if (_adv_load_sound_1("snare",ADV_SOUND_SNARE)<0) return -1;
  return 0;
}

/* play sound
 *****************************************************************************/
 
int adv_sound(int soundid) {
  akau_wave_play(soundid,0x80,0,0);
  return 0;
}

int adv_song(int songid) {
  if ((songid<0)||(songid>=adv_resmgr.songc)) return 0;
  if (songid==adv_resmgr.songid) return 0;
  akau_song_play(adv_resmgr.songv[songid]);
  adv_resmgr.songid=songid;
  return 0;
}

/* load one song
 *****************************************************************************/
 
static int adv_resmgr_load_song_1(const char *path,int songid) {
  if (!path||(songid<0)||(songid>99)) return -1;
  if (songid>=adv_resmgr.songa) {
    int na=songid+10;
    void *nv=realloc(adv_resmgr.songv,sizeof(void*)*na);
    if (!nv) return -1;
    adv_resmgr.songv=nv;
    adv_resmgr.songa=na;
  }
  while (adv_resmgr.songc<=songid) adv_resmgr.songv[adv_resmgr.songc++]=0;
  char *src=0;
  int srcc=adv_file_read(&src,path);
  if ((srcc<0)||!src) return -1;
  struct akau_song *song=0;
  int err=akau_song_compile(&song,src,srcc,path);
  free(src);
  if (err<0) return err;
  if (adv_resmgr.songv[songid]) akau_song_del(adv_resmgr.songv[songid]);
  adv_resmgr.songv[songid]=song;
  return 0;
}

/* load songs
 *****************************************************************************/
 
int adv_res_load_songs() {

  int waveid;
  #define SYNTH(tag) if ((waveid=akau_synthesize_and_load( \
    _adv_synth_##tag,sizeof(_adv_synth_##tag)/sizeof(double),_adv_synth_##tag##_gain,_adv_synth_##tag##_trim \
  ))<0) return -1; \
  if (akau_wave_set_name(waveid,#tag,-1)<0) return -1;
  SYNTH(bass)
  SYNTH(lead)
  SYNTH(pad)
  #undef SYNTH
  
  char subpath[1024];
  int subpathc=snprintf(subpath,sizeof(subpath),"%s/song/",adv_resmgr.root);
  if ((subpathc<1)||(subpathc>=sizeof(subpath))) return -1;
  DIR *dir=opendir(subpath);
  if (!dir) return -1;
  struct dirent *de;
  while (de=readdir(dir)) {
    const char *base=de->d_name;
    if (base[0]=='.') continue;
    int songid=0,basec=0;
    while (base[basec]) {
      int digit=base[basec++]-'0';
      if ((digit<0)||(digit>9)) { songid=-1; break; }
      if (songid>INT_MAX/10) { songid=-1; break; } songid*=10;
      if (songid>INT_MAX-digit) { songid=-1; break; } songid+=digit;
    }
    if ((songid<0)||(songid>99)) {
      fprintf(stderr,"Warning: ignoring song file '%s'; name must be a decimal integer 0..99\n",base);
      continue;
    }
    if (subpathc>=sizeof(subpath)-basec) continue;
    memcpy(subpath+subpathc,base,basec+1);
    adv_resmgr_load_song_1(subpath,songid);
  }
  closedir(dir);
  
  return 0;
}
