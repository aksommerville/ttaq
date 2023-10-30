#include "adv.h"
#include "video/adv_video.h"
#include "input/adv_input.h"
#include "sprite/adv_sprite.h"
#include "sprite/class/adv_sprite_hero.h"
#include "res/adv_res.h"

/* private setup state
 *****************************************************************************/
 
struct adv_player {
  int hero_choice; // sprdefid or zero if not clicked-in
  int ready;
  unsigned char pvinput;
  struct adv_sprite *spr_hero;
  struct adv_sprite *spr_readyl;
  struct adv_sprite *spr_readyr;
};
 
static struct adv_main_menu {
  struct adv_player playerv[1+ADV_PLAYER_LIMIT]; // first is dummy; remainder keyed by player id
  int animcounter;
  int ready_delay;
} adv_main_menu={0};

#define ANIM_FRAME_TIME 15
#define READY_DELAY 60 // After every player is "ready", wait for so long to let someone change his mind.

/* end
 *****************************************************************************/
 
static int adv_main_menu_end() {
  int i;

  adv_input_set_pause(0);
  //adv_input_set_clickin(0); //XXX 'clickin' is now always on
  if (adv_video_end_main_menu()<0) return -1;
  
  /* Assign player ids. Ooh, this is so hacky it's kind of kinky. */
  if (adv_sprgrp_hero->sprc!=ADV_PLAYER_LIMIT) return -1;
  for (i=1;i<=ADV_PLAYER_LIMIT;i++) {
    struct adv_player *player=adv_main_menu.playerv+i;
    if (!player->hero_choice) continue;
    adv_sprite_hero_set_playerid(adv_sprgrp_hero->sprv[player->hero_choice-1],i);
  }

  memset(&adv_main_menu,0,sizeof(struct adv_main_menu));
  return 0;
}

/* begin
 *****************************************************************************/
 
int adv_main_menu_begin() {
  int i;

  adv_input_set_pause(1);
  //adv_input_set_clickin(1);
  if (adv_video_begin_main_menu()<0) return -1;
  
  for (i=1;i<=ADV_PLAYER_LIMIT;i++) {
    struct adv_player *player=adv_main_menu.playerv+i;
    if (adv_sprite_instantiate(&player->spr_hero,0)<0) return -1;
    if (adv_sprite_instantiate(&player->spr_readyl,0)<0) return -1;
    if (adv_sprite_instantiate(&player->spr_readyr,0)<0) return -1;
    player->spr_hero->tileid=0x06;
    player->spr_hero->y=ADV_SCREEN_H>>1;
    player->spr_hero->x=(i*ADV_SCREEN_W)/(ADV_PLAYER_LIMIT+1);
    player->spr_readyl->tileid=0x20;
    player->spr_readyl->y=player->spr_hero->y+(ADV_TILE_H<<1);
    player->spr_readyl->x=player->spr_hero->x-(ADV_TILE_W>>1);
    adv_sprgrp_remove(adv_sprgrp_visible,player->spr_readyl);
    player->spr_readyr->tileid=0x21;
    player->spr_readyr->y=player->spr_hero->y+(ADV_TILE_H<<1);
    player->spr_readyr->x=player->spr_hero->x+(ADV_TILE_W>>1);
    adv_sprgrp_remove(adv_sprgrp_visible,player->spr_readyr);
  }
  
  return 0;
}

/* unused hero sprdefid
 *****************************************************************************/
 
static int adv_main_menu_get_unused_player_sprdefid() {
  int usage[5]={0};
  int i; for (i=1;i<=ADV_PLAYER_LIMIT;i++) {
    if (adv_main_menu.playerv[i].hero_choice<1) continue;
    if (adv_main_menu.playerv[i].hero_choice>4) continue;
    usage[adv_main_menu.playerv[i].hero_choice]=1;
  }
  if (!usage[1]) return 1;
  if (!usage[2]) return 2;
  if (!usage[3]) return 3;
  if (!usage[4]) return 4;
  return 0;
}

static int adv_main_menu_cycle_sprdefid(int sprdefid,int d) {
  int i; for (i=0;i<4;i++) {
    sprdefid+=d;
    if (sprdefid<1) sprdefid+=4;
    else if (sprdefid>4) sprdefid-=4;
    int inuse=0,j; for (j=1;j<=ADV_PLAYER_LIMIT;j++) if (adv_main_menu.playerv[j].hero_choice==sprdefid) { inuse=1; break; }
    if (!inuse) return sprdefid;
  }
  return 0;
}

/* update
 *****************************************************************************/
 
int adv_main_menu_update() {
  int i;
  
  if (--(adv_main_menu.animcounter)<=0) {
    adv_main_menu.animcounter=ANIM_FRAME_TIME;
    for (i=1;i<ADV_PLAYER_LIMIT;i++) {
      struct adv_player *player=adv_main_menu.playerv+i;
      player->spr_readyl->tileid^=0x10;
      player->spr_readyr->tileid^=0x10;
    }
  }
  
  for (i=1;i<=ADV_PLAYER_LIMIT;i++) {
    struct adv_player *player=adv_main_menu.playerv+i;
    
    if (adv_input_player_exists(i)) { // player (ie controller) is present
      #define BTNDOWN(tag) ((adv_inputs[i]&ADV_BTNID_##tag)&&!(player->pvinput&ADV_BTNID_##tag))
      #define BTNUP(tag) (!(adv_inputs[i]&ADV_BTNID_##tag)&&(player->pvinput&ADV_BTNID_##tag))
      if (!player->hero_choice) {
        adv_sound(ADV_SOUND_ARROW);
        if (player->hero_choice=adv_main_menu_get_unused_player_sprdefid()) {
          player->spr_hero->tileid=0x02+player->hero_choice-1;
        }
      }
      if (BTNDOWN(UP)) {
        adv_sound(ADV_SOUND_ARROW);
        if (player->ready) {
          player->ready=0;
          adv_sprgrp_remove(adv_sprgrp_visible,player->spr_readyl);
          adv_sprgrp_remove(adv_sprgrp_visible,player->spr_readyr);
        } else {
          player->hero_choice=0;
          player->spr_hero->tileid=0x06;
          adv_input_remove_player(i);
          continue;
        }
      }
      if (BTNDOWN(LEFT)&&!player->ready) {
        adv_sound(ADV_SOUND_ARROW);
        int nsprdefid=adv_main_menu_cycle_sprdefid(player->hero_choice,-1);
        if (nsprdefid) {
          player->hero_choice=nsprdefid;
          player->spr_hero->tileid=0x02+player->hero_choice-1;
        }
      }
      if (BTNDOWN(RIGHT)&&!player->ready) {
        adv_sound(ADV_SOUND_ARROW);
        int nsprdefid=adv_main_menu_cycle_sprdefid(player->hero_choice,1);
        if (nsprdefid) {
          player->hero_choice=nsprdefid;
          player->spr_hero->tileid=0x02+player->hero_choice-1;
        }
      }
      if (BTNDOWN(ACTION)) {
        adv_sound(ADV_SOUND_ARROW);
        if (!player->ready) {
          player->ready=1;
          adv_sprgrp_insert(adv_sprgrp_visible,player->spr_readyl);
          adv_sprgrp_insert(adv_sprgrp_visible,player->spr_readyr);
        }
      }
      
      player->pvinput=adv_inputs[i];
      #undef BTNDOWN
      #undef BTNUP
    } else { // no player -- make sure we're displaying the default
      if (player->hero_choice) {
        player->hero_choice=0;
        player->spr_hero->tileid=0x06;
      }
      if (player->ready) {
        player->ready=0;
        adv_sprgrp_remove(adv_sprgrp_visible,player->spr_readyl);
        adv_sprgrp_remove(adv_sprgrp_visible,player->spr_readyr);
      }
    }
  }
  
  int playerc=0,all_ready=1;
  for (i=1;i<=ADV_PLAYER_LIMIT;i++) {
    struct adv_player *player=adv_main_menu.playerv+i;
    if (!player->hero_choice) continue;
    playerc++;
    if (!player->ready) { all_ready=0; break; }
  }
  if (playerc&&all_ready) {
    if (--(adv_main_menu.ready_delay)<=0) {
      return adv_main_menu_end();
    }
  } else {
    adv_main_menu.ready_delay=READY_DELAY;
  }
  
  return 0;
}
