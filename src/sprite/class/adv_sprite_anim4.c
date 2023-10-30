#include "adv.h"
#include "sprite/adv_sprite.h"
#include "sprite/adv_sprclass.h"

struct adv_sprite_anim4 {
  struct adv_sprite hdr;
  int animcounter;
};

#define SPR ((struct adv_sprite_anim4*)spr)

#define ANIM_FRAME_TIME 4

/* update
 *****************************************************************************/
 
static int _adv_anim4_update(struct adv_sprite *spr) {
  if (--(SPR->animcounter)<=0) {
    SPR->animcounter=ANIM_FRAME_TIME;
    spr->tileid++;
    if (!(spr->tileid&3)) return adv_sprite_kill(spr);
  }
  return 0;
}

/* init
 *****************************************************************************/
 
static int _adv_anim4_init(struct adv_sprite *spr) {
  SPR->animcounter=ANIM_FRAME_TIME;
  spr->depth=100;
  return 0;
}

/* type definition
 *****************************************************************************/
 
const struct adv_sprclass adv_sprclass_anim4={
  .name="anim4",
  .objlen=sizeof(struct adv_sprite_anim4),
  .init=_adv_anim4_init,
  .update=_adv_anim4_update,
};
