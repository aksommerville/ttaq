#include "adv.h"
#include "sprite/adv_sprite.h"
#include "sprite/adv_sprclass.h"

struct adv_sprite_bonfire {
  struct adv_sprite hdr;
  int animcounter;
};

#define SPR ((struct adv_sprite_bonfire*)spr)

#define ANIM_FRAME_TIME 7

/* update
 *****************************************************************************/
 
static int _adv_bonfire_update(struct adv_sprite *spr) {
  if (--(SPR->animcounter)<=0) {
    SPR->animcounter=ANIM_FRAME_TIME;
    spr->tileid^=0x10;
  }
  return 0;
}

/* class definition
 *****************************************************************************/
 
static int _adv_bonfire_init(struct adv_sprite *spr) {
  if (adv_sprgrp_insert(adv_sprgrp_hazard,spr)<0) return -1;
  return 0;
}
 
const struct adv_sprclass adv_sprclass_bonfire={
  .name="bonfire",
  .objlen=sizeof(struct adv_sprite_bonfire),
  .update=_adv_bonfire_update,
  .init=_adv_bonfire_init,
};
