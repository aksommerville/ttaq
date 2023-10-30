#include "adv.h"
#include "sprite/adv_sprite.h"
#include "sprite/adv_sprclass.h"
#include "sprite/class/adv_sprite_hero.h"
#include "game/adv_map.h"
#include "res/adv_res.h"

struct adv_sprite_missile {
  struct adv_sprite hdr;
  int animcounter;
  int have_target;
  int ax,ay,bx,by,dx,dy;
  int xweight,yweight,weight;
  int suppress_animation;
};

#define SPR ((struct adv_sprite_missile*)spr)

#define ANIM_FRAME_TIME 5
#define MOVE_SPEED 4

/* update
 *****************************************************************************/
 
static int _adv_missile_update(struct adv_sprite *spr) {

  /* Choose target. */
  if (!SPR->have_target) {
    SPR->have_target=1;
    if (adv_sprgrp_hero->sprc<1) return adv_sprite_kill(spr);
    int targetix;
    struct adv_sprite *target;
    int panic=10; while (--panic>0) {
      targetix=rand()%adv_sprgrp_hero->sprc;
      target=adv_sprgrp_hero->sprv[targetix];
      if (((struct adv_sprite_hero*)target)->offscreen) continue;
    }
    SPR->ax=spr->x; SPR->ay=spr->y;
    SPR->bx=target->x; SPR->by=target->y;
    while ((SPR->ax==SPR->bx)&&(SPR->ay==SPR->by)) {
      SPR->bx+=rand()%100-50;
      SPR->by+=rand()%100-50;
    }
         if (SPR->ax<SPR->bx) { SPR->dx=1; SPR->xweight=SPR->bx-SPR->ax; }
    else if (SPR->ax>SPR->bx) { SPR->dx=-1; SPR->xweight=SPR->ax-SPR->bx; }
    else { SPR->dx=0; SPR->xweight=0; }
         if (SPR->ay<SPR->by) { SPR->dy=1; SPR->yweight=SPR->ay-SPR->by; }
    else if (SPR->ay>SPR->by) { SPR->dy=-1; SPR->yweight=SPR->by-SPR->ay; }
    else { SPR->dy=0; SPR->yweight=0; }
  }
  
  /* Advance towards target. */
  int repc; for (repc=0;repc<MOVE_SPEED;repc++) {
    if (SPR->dx&&(SPR->weight>SPR->xweight>>1)) { spr->x+=SPR->dx; SPR->weight+=SPR->yweight; }
    else if (SPR->dy&&(SPR->weight<SPR->yweight>>1)) { spr->y+=SPR->dy; SPR->weight+=SPR->xweight; }
    else {
      spr->x+=SPR->dx;
      spr->y+=SPR->dy;
      SPR->weight+=SPR->xweight+SPR->yweight;
    }
  }
  if ((spr->y<-ADV_TILE_H)||(spr->x<-ADV_TILE_W)||(spr->x>ADV_SCREEN_W+ADV_TILE_W)||(spr->y>ADV_SCREEN_H+ADV_TILE_H)) return adv_sprite_kill(spr);
  
  /* Check collisions.
   * We don't use adv_sprgrp_hazard for this, because missiles should have a very small boundary.
   */
  int left=spr->x-(ADV_TILE_W>>1);
  int right=spr->x+(ADV_TILE_W>>1);
  int top=spr->y-(ADV_TILE_H>>1);
  int bottom=spr->y+(ADV_TILE_H>>1);
  int i; for (i=0;i<adv_sprgrp_hero->sprc;i++) {
    struct adv_sprite *qspr=adv_sprgrp_hero->sprv[i];
    if (((struct adv_sprite_hero*)qspr)->offscreen) continue;
    if (((struct adv_sprite_hero*)qspr)->ghost) continue;
    if (qspr->x<left) continue;
    if (qspr->x>right) continue;
    if (qspr->y<top) continue;
    if (qspr->y>bottom) continue;
    adv_sprite_kill(spr);
    return adv_sprite_hurt(qspr);
  }
  
  int col=spr->x/ADV_TILE_W,row=spr->y/ADV_TILE_H;
  if (adv_map_cell_is_solid(adv_map,col,row,1,0)) return adv_sprite_kill(spr);

  /* Animate. */
  if (!SPR->suppress_animation) {
    if (--(SPR->animcounter)<=0) {
      SPR->animcounter=ANIM_FRAME_TIME;
      spr->tileid^=0x10;
    }
  }
  
  return 0;
}

/* class definition
 *****************************************************************************/
 
static int _adv_missile_init(struct adv_sprite *spr) {
  //if (adv_sprgrp_insert(adv_sprgrp_hazard,spr)<0) return -1;
  spr->depth=50;
  return 0;
}

const struct adv_sprclass adv_sprclass_missile={
  .name="missile",
  .objlen=sizeof(struct adv_sprite_missile),
  .init=_adv_missile_init,
  .update=_adv_missile_update,
};
