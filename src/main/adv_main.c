#include "adv.h"
#include "game/adv_game_progress.h"
#include "video/adv_video.h"
#include "input/adv_input.h"
#include "sprite/adv_sprite.h"
#include "sprite/adv_sprclass.h"
#include "sprite/class/adv_sprite_hero.h"
#include "res/adv_res.h"
#include <linux/input.h>
#include <signal.h>
#include <sys/time.h>
#include <time.h>

extern int adv_main_menu_begin();
extern int adv_main_menu_update();

/* signals
 *****************************************************************************/

static volatile int adv_sigc=0;

static void adv_rcvsig(int sigid) {
  switch (sigid) {
    case SIGINT: case SIGTERM: case SIGQUIT: {
        if (++adv_sigc>=3) { fprintf(stderr,"Too many pending signals. Abort.\n"); exit(1); }
      } break;
  }
}

/* time
 *****************************************************************************/

static int64_t adv_get_time() {
  struct timeval tv={0};
  gettimeofday(&tv,0);
  return (int64_t)tv.tv_sec*1000000+tv.tv_usec;
}

/* init
 *****************************************************************************/

static int adv_init(int argc,char **argv) {
  int err;

  srand(time(0));
  signal(SIGINT,adv_rcvsig);
  signal(SIGTERM,adv_rcvsig);
  signal(SIGQUIT,adv_rcvsig);
  
  //TODO if (akau_init(44100)<0) return -1;

  if ((err=adv_sprgrp_init())<0) return err;
  if ((err=adv_video_init())<0) return err;
  if ((err=adv_res_init((argc>=1)?argv[0]:""))<0) return err;
  if ((err=adv_input_init())<0) return err;
  
  if ((err=adv_input_map_useraction(KEY_ESC,ADV_USERACTION_QUIT))<0) return err;
  if ((err=adv_input_map_useraction(KEY_P,ADV_USERACTION_PAUSE))<0) return err;
  
  if (adv_game_load(0)<0) fprintf(stderr,"Failed to load saved game.\n");
  if (adv_main_menu_begin()<0) return -1;

  return 0;
}

/* quit
 *****************************************************************************/

static void adv_quit() {
  adv_input_quit();
  adv_res_quit();
  //TODO akau_quit();
  adv_video_quit();
  adv_sprgrp_quit();
}

/* update game
 *****************************************************************************/
 
static int _adv_update_game() {
  int err,i;
   
  if (adv_sprgrp_update) for (i=0;i<adv_sprgrp_update->sprc;i++) {
    struct adv_sprite *spr=adv_sprgrp_update->sprv[i];
    if (spr->sprclass&&spr->sprclass->update) {
      if ((err=spr->sprclass->update(spr))<0) return err;
    }
  }
  
  /* Check for map changes. */
  int change_to_map=0;
  for (i=0;i<adv_sprgrp_hero->sprc;i++) {
    struct adv_sprite *spr=adv_sprgrp_hero->sprv[i];
    if (spr->sprclass!=&adv_sprclass_hero) return -1;
    struct adv_sprite_hero *SPR=(struct adv_sprite_hero*)spr;
    if (!SPR->offscreen) { change_to_map=0; break; }
    if (!SPR->offscreen_target) { change_to_map=0; break; }
    if (!change_to_map) change_to_map=SPR->offscreen_target;
    else if (SPR->offscreen_target!=change_to_map) { change_to_map=0; break; }
  }
  if (change_to_map) {
    if (adv_res_load_map(change_to_map)<0) return -1;
  }
  
  return 0;
}

/* update
 * Sleep and return >0 if game in progress.
 *****************************************************************************/

static int adv_update() {
  int err;
  if (adv_sigc) return 0;
  if ((err=adv_input_update())<0) return err;
  if (adv_input_quit_requested()) return 0;
  
  if (adv_video_is_main_menu()) {
    if ((err=adv_main_menu_update())<0) return err;
  } else if (!adv_input_get_pause()) {
    if ((err=_adv_update_game())<0) return err;
  }
  
  if ((err=adv_video_update())<0) return err;
  return 1;
}

/* main
 *****************************************************************************/

int main(int argc,char **argv) {
  int err=0;
  int64_t starttime=0,endtime=0,framec=0;

  if ((err=adv_init(argc,argv))<0) goto _done_;

  starttime=adv_get_time();
  while ((err=adv_update())>0) framec++;
  
  if (!err&&(framec>0)) {
    endtime=adv_get_time();
    if (endtime>starttime) {
      int useconds=(endtime-starttime)%1000000;
      int seconds=(endtime-starttime)/1000000;
      int minutes=seconds/60; seconds%=60;
      int hours=minutes/60; minutes%=60;
      printf("Play time %d:%02d:%02d.%06d\n",hours,minutes,seconds,useconds);
      printf("Average frame rate %.2f Hz\n",(framec*1000000.0)/(endtime-starttime));
    }
  }
  
 _done_:
  if (err>=0) adv_game_save(0);
  adv_quit();
  if (err<0) {
    fprintf(stderr,"Fatal error #%d\n",err);
    return 1;
  }
  return 0;
}
