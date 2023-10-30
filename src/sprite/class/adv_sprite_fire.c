#include "adv.h"
#include "sprite/adv_sprite.h"
#include "sprite/adv_sprclass.h"

struct adv_sprite_fire {
  struct adv_sprite hdr;
  int animcounter;
  struct adv_sprite *owner; // weak
};

#define SPR ((struct adv_sprite_fire*)spr)

#define ANIM_FRAME_TIME 4

#define KILL_RADIUS (ADV_TILE_W>>2) // it's actually a square...

/* update
 *****************************************************************************/
 
static int _adv_fire_update(struct adv_sprite *spr) {
  if (SPR->animcounter--<=0) {
    SPR->animcounter=ANIM_FRAME_TIME;
    if (++(spr->tileid)>=0x3f) spr->tileid=0x3c;
  }
  int left=spr->x-KILL_RADIUS-(ADV_TILE_W>>1);
  int top=spr->y-KILL_RADIUS-(ADV_TILE_H>>1);
  int right=spr->x+KILL_RADIUS+(ADV_TILE_W>>1);
  int bottom=spr->y+KILL_RADIUS+(ADV_TILE_H>>1);
  int i; for (i=0;i<adv_sprgrp_fragile->sprc;i++) {
    struct adv_sprite *qspr=adv_sprgrp_fragile->sprv[i];
    if (qspr==SPR->owner) continue;
    if (qspr->x<=left) continue;
    if (qspr->y<=top) continue;
    if (qspr->x>=right) continue;
    if (qspr->y>=bottom) continue;
    if (adv_sprite_hurt(qspr)<0) return -1;
  }
  return 0;
}

/* type definition
 *****************************************************************************/
 
const struct adv_sprclass adv_sprclass_fire={
  .name="fire",
  .objlen=sizeof(struct adv_sprite_fire),
  .update=_adv_fire_update,
};

/* odds, ends
 *****************************************************************************/
 
int adv_sprite_fire_set_owner(struct adv_sprite *spr,struct adv_sprite *owner) {
  if (!spr||(spr->sprclass!=&adv_sprclass_fire)) return -1;
  SPR->owner=owner;
  return 0;
}
