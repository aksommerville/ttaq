#include "adv.h"
#include "sprite/adv_sprdef.h"
#include "sprite/adv_sprite.h"
#include "sprite/adv_sprclass.h"

struct adv_sprite_phantasm {
  struct adv_sprite hdr;
  int animcounter;
  int dx,dy;
  int missile_time;
  int prevention; // don't doppelgang twice within this interval -- for some attacks, like wizard's spell, it really matters!
};

extern const struct adv_sprclass adv_sprclass_phantasm;

#define SPR ((struct adv_sprite_phantasm*)spr)

#define ANIM_FRAME_TIME 12
#define MISSILE_TIME_MIN 180
#define MISSILE_TIME_MAX 900
#define PREVENTION_TIME 60

/* init
 *****************************************************************************/
 
static int _adv_phantasm_init(struct adv_sprite *spr) {
  if (adv_sprgrp_insert(adv_sprgrp_fragile,spr)<0) return -1;
  if (adv_sprgrp_insert(adv_sprgrp_hazard,spr)<0) return -1;
  SPR->missile_time=MISSILE_TIME_MIN+(rand()%(MISSILE_TIME_MAX-MISSILE_TIME_MIN+1));
  return 0;
}

/* update
 *****************************************************************************/
 
static int _adv_phantasm_update(struct adv_sprite *spr) {
  if (--(SPR->animcounter)<=0) {
    SPR->animcounter=ANIM_FRAME_TIME;
    spr->tileid^=0x10;
  }
  if (SPR->prevention>0) SPR->prevention--;
  if (!SPR->dx&&!SPR->dy) {
    switch (rand()&3) {
      case 0: SPR->dx=-1; break;
      case 1: SPR->dx=1; break;
      case 2: SPR->dy=-1; break;
      case 3: SPR->dy=1; break;
    }
  }
  if (!adv_sprite_move(spr,SPR->dx,SPR->dy,0)) {
    SPR->dx=SPR->dy=0;
  } else if ((SPR->dx&&!((spr->x-(ADV_TILE_W>>1))%ADV_TILE_W))||(SPR->dy&&!((spr->y-(ADV_TILE_H>>1))%ADV_TILE_H))) {
    // When landing on a grid line, allow a 25% chance of changing direction.
    if (!(rand()&3)) SPR->dx=SPR->dy=0;
  }
  if (spr->sprdef&&spr->sprdef->argv[0]) {
    if (--(SPR->missile_time)<=0) {
      SPR->missile_time=MISSILE_TIME_MIN+(rand()%(MISSILE_TIME_MAX-MISSILE_TIME_MIN+1));
      struct adv_sprite *missile=0;
      if (adv_sprite_create(&missile,spr->sprdef->argv[0],spr->x,spr->y)<0) return -1;
    }
  }
  return 0;
}

/* hurt
 *****************************************************************************/
 
static int _adv_phantasm_hurt(struct adv_sprite *spr) {
  if (!spr->sprdefid) return adv_sprite_hurt_default(spr);
  if (SPR->prevention) return 0;
  struct adv_sprite *doppelganger=0;
  if (adv_sprite_create(&doppelganger,spr->sprdefid,spr->x,spr->y)<0) return -1;
  if (doppelganger->sprclass!=&adv_sprclass_phantasm) return 0; // ?
  struct adv_sprite_phantasm *DOPPELGANGER=(struct adv_sprite_phantasm*)doppelganger;
  DOPPELGANGER->dx=-SPR->dx;
  DOPPELGANGER->dy=-SPR->dy;
  SPR->prevention=PREVENTION_TIME;
  DOPPELGANGER->prevention=PREVENTION_TIME;
  return 0;
}

/* class definition
 *****************************************************************************/
 
const struct adv_sprclass adv_sprclass_phantasm={
  .name="phantasm",
  .objlen=sizeof(struct adv_sprite_phantasm),
  .init=_adv_phantasm_init,
  .update=_adv_phantasm_update,
  .hurt=_adv_phantasm_hurt,
  .collide_edges=1,
  .collide_map_positive=1,
  .collide_map_negative=1,
  .arg0name="missile",
};
