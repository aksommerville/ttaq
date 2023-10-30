#include "adv.h"
#include "sprite/adv_sprite.h"
#include "sprite/adv_sprclass.h"
#include "res/adv_res.h"
#include "game/adv_map.h"

struct adv_sprite_livestock {
  struct adv_sprite hdr;
  int dx;
  int animcounter;
  int befuddle; // counts down after a move fails before changing direction (prevents back-to-back flop swaps when trapped).
};

#define SPR ((struct adv_sprite_livestock*)spr)

#define ANIM_FRAME_TIME 10
#define SPEED 1
#define BEFUDDLE_TIME_MIN 30 // Randomizing the befuddle time helps break up synchronization when you have lots of livestock.
#define BEFUDDLE_TIME_MAX 60

/* update
 *****************************************************************************/
 
static int _adv_livestock_update(struct adv_sprite *spr) {
  if (!SPR->dx) {
    if ((SPR->dx=(rand()&1)?1:-1)>0) spr->flop=1;
  }

  /* Animate. */
  if (--(SPR->animcounter)<=0) {
    SPR->animcounter=ANIM_FRAME_TIME;
    spr->tileid^=0x10;
  }
  
  /* Move */
  if (SPR->befuddle>0) {
    if (!--(SPR->befuddle)) {
      SPR->dx=-SPR->dx;
      spr->flop=spr->flop?0:1;
    }
  } else if (!adv_sprite_move(spr,SPR->dx*SPEED,0,0)) {
    SPR->befuddle=BEFUDDLE_TIME_MIN+rand()%(BEFUDDLE_TIME_MAX-BEFUDDLE_TIME_MIN+1);
  }
  
  return 0;
}

/* class definition
 *****************************************************************************/
 
static int _adv_livestock_init(struct adv_sprite *spr) {
  if (adv_sprgrp_insert(adv_sprgrp_fragile,spr)<0) return -1;
  if (adv_sprgrp_insert(adv_sprgrp_latchable,spr)<0) return -1;
  return 0;
}
 
const struct adv_sprclass adv_sprclass_livestock={
  .name="livestock",
  .objlen=sizeof(struct adv_sprite_livestock),
  .update=_adv_livestock_update,
  .init=_adv_livestock_init,
  .collide_edges=1,
  .collide_map_positive=1,
  .collide_map_negative=1,
  .collide_solids=1,
};
