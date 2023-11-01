#include "adv.h"
#include "game/adv_game_progress.h"
#include "sprite/adv_sprite.h"
#include "sprite/adv_sprclass.h"
#include "sprite/adv_sprdef.h"
#include "sprite/class/adv_sprite_hero.h"
#include "res/adv_res.h"

struct adv_sprite_princess {
  struct adv_sprite hdr;
  int kill_me;
};

#define SPR ((struct adv_sprite_princess*)spr)

/* update
 *****************************************************************************/
 
static int _adv_princess_update(struct adv_sprite *spr) {
  if (SPR->kill_me) return adv_sprite_kill(spr);
  int left=spr->x-ADV_TILE_W;
  int right=spr->x+ADV_TILE_W;
  int top=spr->y-ADV_TILE_H;
  int bottom=spr->y+ADV_TILE_H;
  int i; for (i=0;i<adv_sprgrp_hero->sprc;i++) {
    struct adv_sprite *hero=adv_sprgrp_hero->sprv[i];
    if (((struct adv_sprite_hero*)hero)->ghost) continue;
    if (((struct adv_sprite_hero*)hero)->offscreen) continue;
    if (hero->x<=left) continue;
    if (hero->x>=right) continue;
    if (hero->y<=top) continue;
    if (hero->y>=bottom) continue;
    ttaq_audio_play_sound(ADV_SOUND_RESCUE);
    if (spr->sprdef) {
      if (adv_global_set(spr->sprdef->argv[0],ADV_PRINCESS_RESCUED)<0) return -1;
    }
    adv_sprite_kill(spr);
    return 0;
  }
  return 0;
}

/* hurt
 *****************************************************************************/
 
static int _adv_princess_hurt(struct adv_sprite *spr) {
  if (spr->sprdef) {
    if (adv_global_set(spr->sprdef->argv[0],ADV_PRINCESS_DEAD)<0) return -1;
  }
  return adv_sprite_hurt_default(spr);
}

/* class definition
 *****************************************************************************/
 
static int _adv_princess_init(struct adv_sprite *spr) {
  if (adv_sprgrp_insert(adv_sprgrp_princess,spr)<0) return -1;
  if (adv_sprgrp_insert(adv_sprgrp_fragile,spr)<0) return -1;
  if (adv_sprgrp_insert(adv_sprgrp_latchable,spr)<0) return -1;
  return 0;
}

static int _adv_princess_postinit(struct adv_sprite *spr) {
  if (spr->sprdef) {
    if (adv_global_get(spr->sprdef->argv[0])!=ADV_PRINCESS_AFIELD) {
      adv_sprgrp_remove(adv_sprgrp_visible,spr);
      SPR->kill_me=1;
    }
  }
  return 0;
}
 
const struct adv_sprclass adv_sprclass_princess={
  .name="princess",
  .objlen=sizeof(struct adv_sprite_princess),
  .init=_adv_princess_init,
  .postinit=_adv_princess_postinit,
  .hurt=_adv_princess_hurt,
  .update=_adv_princess_update,
  .arg0name="global",
};
