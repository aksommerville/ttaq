#include "adv_res_internal.h"
#include "audio/ttaq_audio.h"

#if 0
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
#endif

/* load one song
 *****************************************************************************/
 
static int adv_resmgr_load_song_1(const char *path,int songid) {
  if (!path||(songid<0)||(songid>99)) return -1;
  char *src=0;
  int srcc=adv_file_read(&src,path);
  if ((srcc<0)||!src) return -1;
  int err=ttaq_audio_load_song(songid,src,srcc);
  free(src);
  if (err<0) {
    fprintf(stderr,"%s: Error loading song.\n",path);
    return -1;
  }
  return 0;
}

/* load songs
 *****************************************************************************/
 
int adv_res_load_songs() {
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
