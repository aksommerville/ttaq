#include "adv.h"
#include "adv_game_progress.h"
#include "adv_map.h"
#include "res/adv_res.h"
#include "video/adv_video.h"

struct adv_game_progress adv_game_progress={0};
char *adv_save_game_path=0;

static int adv_game_defer_action=0;

// Kind of hacky. These global IDs are forced zero when encoding.
// If the player saves while standing on a dead-man switch, things won't get too crazy.
static const int adv_no_save_globals[]={
  11,14,15,16,18,19,20,21,22,23,24,25,
0};

/* victory
 *****************************************************************************/
 
#define VICTORY_SPLASH_DURATION 300
 
static int _adv_game_victory=0;
static int _adv_game_victory_defer=0;
 
static int adv_game_victory(int victory) {
  if (victory==_adv_game_victory) return 0;
  if (adv_game_defer_action) { _adv_game_victory_defer=victory; return 0; }
  _adv_game_victory_defer=0;
  switch (_adv_game_victory=victory) {
    case -2: { // rescued when we were murdering
        adv_video_splash(4,VICTORY_SPLASH_DURATION);
      } break;
    case -1: { // murdered when we were rescuing
        adv_video_splash(3,VICTORY_SPLASH_DURATION);
      } break;
    case 1: { // all rescued
        adv_video_splash(2,VICTORY_SPLASH_DURATION);
      } break;
    case 2: { // all murdered
        adv_video_splash(1,VICTORY_SPLASH_DURATION);
      } break;
  }
  return 0;
}

/* value changed
 *****************************************************************************/
 
int adv_global_changed(int globalid) {
  switch (globalid) {
  
    /* Check for victory. */
    case ADV_GLOBAL_PRINCESS_1:
    case ADV_GLOBAL_PRINCESS_2:
    case ADV_GLOBAL_PRINCESS_3:
    case ADV_GLOBAL_PRINCESS_4:
    case ADV_GLOBAL_PRINCESS_5:
    case ADV_GLOBAL_PRINCESS_6:
    case ADV_GLOBAL_PRINCESS_7: {
        int pendingc=0,rescuec=0,murderc=0,i;
        for (i=ADV_GLOBAL_PRINCESS_1;i<=ADV_GLOBAL_PRINCESS_7;i++) switch (adv_game_progress.globals[i]) {
          case 0: pendingc++; break;
          case 1: rescuec++; break;
          case 2: murderc++; break;
        }
        if (rescuec==7) {
          adv_game_victory(1);
        } else if (murderc==7) {
          adv_game_victory(2);
        } else switch (adv_game_progress.globals[globalid]) {
          case 1: if (murderc) {
              adv_game_victory(-2);
            } break;
          case 2: if (rescuec) {
              adv_game_victory(-1);
            } break;
        }
      } break;
      
    /* Reset. */
    case 22: case 23: case 24: case 25: {
        int i,ok=1; for (i=22;i<=25;i++) if (!adv_game_progress.globals[i]) { ok=0; break; }
        if (ok) {
          for (i=0;i<ADV_GLOBAL_COUNT;i++) if (adv_global_set(i,0)<0) return -1;
          printf("Reset game\n");
        }
      } break;
    
  }
  if (adv_map_update_switches(adv_map,1)<0) return -1;
  return 0;
}

/* encode/decode
 *****************************************************************************/
 
int adv_game_progress_encode(void *dst,int dsta,const struct adv_game_progress *src) {
  if (((dsta>0)&&!dst)||!src) return -1;
  int dstc=ADV_GLOBAL_COUNT;
  if (dstc>dsta) return dstc;
  int i; for (i=0;i<ADV_GLOBAL_COUNT;i++) ((unsigned char*)dst)[i]=src->globals[i];
  const int *g=adv_no_save_globals; for (;*g;g++) ((unsigned char*)dst)[*g]=0;
  return dstc;
}
 
int adv_game_progress_decode(struct adv_game_progress *dst,const void *src,int srcc) {
  if (!dst||(srcc<0)||(srcc&&!src)) return -1;
  memset(dst,0,sizeof(struct adv_game_progress));
  int srcp=0; while (1) {
    if (srcp>=srcc) break;
    if (srcp>=ADV_GLOBAL_COUNT) break;
    dst->globals[srcp]=((const unsigned char*)src)[srcp];
    srcp++;
  }
  return srcp;
}

/* path setup
 *****************************************************************************/
 
static int _adv_set_save_path(const char *src) {
  if (adv_save_game_path&&!strcmp(src,adv_save_game_path)) return 0;
  char *npath=strdup(src);
  if (!npath) return -1;
  if (adv_save_game_path) free(adv_save_game_path);
  adv_save_game_path=npath;
  return 0;
}

static int _adv_concat_save_path(const char *a,const char *b) {
  if (!a||!b) return -1;
  int ac=0; while (a[ac]) ac++;
  int bc=0; while (b[bc]) bc++;
  if (!ac) return _adv_set_save_path(b);
  if (!bc) return _adv_set_save_path(a);
  if (a[ac-1]=='/') while (b[0]=='/') { b++; bc--; }
  char *npath=malloc(ac+bc+1);
  if (!npath) return -1;
  memcpy(npath,a,ac);
  memcpy(npath+ac,b,bc);
  npath[ac+bc]=0;
  if (adv_save_game_path) free(adv_save_game_path);
  adv_save_game_path=npath;
  return 0;
}
 
static int _adv_set_default_save_path() {
  const char *HOME=getenv("HOME");
  if (HOME&&HOME[0]) return _adv_concat_save_path(HOME,"/.ttaq/saved-game");
  return _adv_set_save_path("/usr/local/share/ttaq/saved-game");
}

/* save/load
 *****************************************************************************/
 
int adv_game_save(const char *path) {
  if (path) {
    if (_adv_set_save_path(path)<0) return -1;
  } else {
    if (!adv_save_game_path&&(_adv_set_default_save_path()<0)) return -1;
  }
  int srca=ADV_GLOBAL_COUNT,srcc=0;
  void *src=malloc(srca);
  if (!src) return -1;
  while (1) {
    if ((srcc=adv_game_progress_encode(src,srca,&adv_game_progress))<0) { free(src); return -1; }
    if (srcc<=srca) break;
    free(src);
    if (!(src=malloc(srca=srcc))) return -1;
  }
  int err=adv_file_write(adv_save_game_path,src,srcc);
  free(src);
  if (err<0) return err;
  printf("Saved game to '%s'\n",adv_save_game_path);
  return 0;
}

int adv_game_load(const char *path) {
  if (path) {
    if (_adv_set_save_path(path)<0) return -1;
  } else {
    if (!adv_save_game_path&&(_adv_set_default_save_path()<0)) return -1;
  }
  void *src=0;
  int srcc=adv_file_read(&src,adv_save_game_path);
  if (srcc<0) return -1;
  if (!src) return -1;
  struct adv_game_progress tmp={0};
  if (adv_game_progress_decode(&tmp,src,srcc)<0) { free(src); return -1; }
  free(src);
  printf("Loaded saved game from '%s'\n",adv_save_game_path);
  adv_game_defer_action=1;
  int i; for (i=0;i<ADV_GLOBAL_COUNT;i++) adv_global_set(i,tmp.globals[i]);
  adv_game_defer_action=0;
  adv_game_victory(_adv_game_victory_defer);
  return 0;
}
