#include "adv.h"
#include "sprite/adv_sprite.h"
#include "sprite/adv_sprclass.h"
#include "game/adv_map.h"
#include "res/adv_res.h"

struct adv_sprite_gadget {
  struct adv_sprite hdr;
  int returning;
  int retracting;
  int x0,y0; // initial position
  struct adv_sprgrp *grp_latched;
  struct adv_sprgrp *grp_chain;
  struct adv_sprgrp *grp_owner;
  struct adv_sprite *knife;
};

#define SPR ((struct adv_sprite_gadget*)spr)

#define FORWARD_SPEED 6
#define BACKWARD_SPEED 4
#define RETRACTION_SPEED 8

/* update
 *****************************************************************************/

static int _adv_gadget_update(struct adv_sprite *spr) {

  if (!SPR->x0&&!SPR->y0) {
    SPR->x0=spr->x;
    SPR->y0=spr->y;
  }

  /* Boundary and vector. */
  int x,y,w,h,dx,dy;
  switch (spr->tileid&3) {
    case 0: x=21; y=0; w=11; h=8; dx=0; dy=1; break;
    case 1: x=0; y=24; w=11; h=8; dx=0; dy=-1; break;
    case 2: if (spr->flop) { 
              x=0; y=16; w=8; h=11; dx=1; dy=0;
            } else {
              x=24; y=15; w=8; h=11; dx=-1; dy=0;
            } break;
  }
  x+=spr->x-(ADV_TILE_W>>1);
  y+=spr->y-(ADV_TILE_H>>1);
  if (SPR->returning) {
    dx=-dx*BACKWARD_SPEED;
    dy=-dy*BACKWARD_SPEED;
  } else if (SPR->retracting) {
    dx*=RETRACTION_SPEED;
    dy*=RETRACTION_SPEED;
  } else {
    dx*=FORWARD_SPEED;
    dy*=FORWARD_SPEED;
  }
  
  /* Retraction. */
  if (SPR->retracting) {
    if (SPR->grp_owner->sprc<1) return adv_sprite_kill(spr);
    int restore=SPR->grp_owner->sprv[0]->collide_map_negative;
    SPR->grp_owner->sprv[0]->collide_map_negative=0;
    int move=adv_sprite_move(SPR->grp_owner->sprv[0],dx,dy,0);
    SPR->grp_owner->sprv[0]->collide_map_negative=restore;
    if (move<1) return adv_sprite_kill(spr);
    if (dx>0) SPR->x0+=move; 
    else if (dx<0) SPR->x0-=move;
    else if (dy>0) SPR->y0+=move;
    else if (dy<0) SPR->y0-=move;
    goto _adjust_chain_;
  }
  
  /* Movement. */
  spr->x+=dx; spr->y+=dy;
  x+=dx; y+=dy;
  if ((x+w<=0)||(y+h<=0)||(x>=ADV_SCREEN_W)||(y>=ADV_SCREEN_H)) return adv_sprite_kill(spr);
  if (SPR->returning) {
    if (((dx<0)&&(spr->x<=SPR->x0))||
      ((dx>0)&&(spr->x>=SPR->x0))||
      ((dy<0)&&(spr->y<=SPR->y0))||
      ((dy>0)&&(spr->y>=SPR->y0))
    ) {
      if (SPR->grp_latched->sprc>0) {
        if (adv_sprite_hurt(SPR->grp_latched->sprv[0])<0) return -1;
      }
      return adv_sprite_kill(spr);
    }
  }
  int col; for (col=x/ADV_TILE_W;col<=(x+w-1)/ADV_TILE_W;col++) {
    int row; for (row=y/ADV_TILE_H;row<=(y+h-1)/ADV_TILE_H;row++) {
      switch (adv_map_cell_is_solid(adv_map,col,row,1,0)) {
        case 1: { // solid, unlatchable
            ttaq_audio_play_sound(ADV_SOUND_GADGETNO);
            SPR->returning=1;
          } return 0;
        case 2: { // solid, latchable
            ttaq_audio_play_sound(ADV_SOUND_GADGETYES);
            SPR->retracting=1;
          } return 0;
      }
    }
  }
  int i; 
  for (i=0;i<SPR->grp_latched->sprc;i++) {
    SPR->grp_latched->sprv[i]->x=spr->x;
    SPR->grp_latched->sprv[i]->y=spr->y;
  }
  for (i=0;i<adv_sprgrp_latchable->sprc;i++) {
    struct adv_sprite *qspr=adv_sprgrp_latchable->sprv[i];
    if (qspr->x-(ADV_TILE_W>>1)>=x+w) continue;
    if (qspr->y-(ADV_TILE_H>>1)>=y+h) continue;
    if (qspr->x+(ADV_TILE_W>>1)<=x) continue;
    if (qspr->y+(ADV_TILE_H>>1)<=y) continue;
    if (adv_sprgrp_has(SPR->grp_latched,qspr)) continue;
    if (SPR->returning) return adv_sprite_kill(spr); // hit another latchable sprite back-handed
    if (adv_sprgrp_insert(SPR->grp_latched,qspr)<0) return -1;
    if (adv_sprgrp_has(adv_sprgrp_fragile,qspr)) {
      if (SPR->grp_owner->sprc>0) SPR->grp_owner->sprv[0]->tileid+=0x20;
      if (!SPR->knife) {
        if (adv_sprite_instantiate(&SPR->knife,0)<0) return -1;
        if (adv_sprite_ref(SPR->knife)<0) { SPR->knife=0; return -1; }
        SPR->knife->tileid=spr->tileid+0x3a;
        SPR->knife->flop=spr->flop;
        SPR->knife->depth=spr->depth;
        SPR->knife->x=SPR->x0;
        SPR->knife->y=SPR->y0;
      }
    }
    ttaq_audio_play_sound(ADV_SOUND_GADGETYES);
    SPR->returning=1;
    return 0;
  }
  for (i=0;i<adv_sprgrp_solid->sprc;i++) {
    struct adv_sprite *qspr=adv_sprgrp_solid->sprv[i];
    if (qspr->x-(ADV_TILE_W>>1)>=x+w) continue;
    if (qspr->y-(ADV_TILE_H>>1)>=y+h) continue;
    if (qspr->x+(ADV_TILE_W>>1)<=x) continue;
    if (qspr->y+(ADV_TILE_H>>1)<=y) continue;
    if (adv_sprgrp_has(SPR->grp_latched,qspr)) continue;
    if (SPR->returning) return adv_sprite_kill(spr);
    ttaq_audio_play_sound(ADV_SOUND_GADGETNO);
    SPR->returning=1;
    return 0;
  }
  
  /* Chain. */
 _adjust_chain_:
  if (dx) {
    int chainlen=SPR->x0-spr->x; if (chainlen<0) chainlen=-chainlen;
    int linkc=(chainlen+ADV_TILE_W-1)/ADV_TILE_W;
    while (SPR->grp_chain->sprc<linkc) {
      struct adv_sprite *chain=0;
      if (adv_sprite_instantiate(&chain,0)<0) return -1;
      chain->tileid=0x1f;
      chain->depth=spr->depth-1;
      if (adv_sprgrp_insert(SPR->grp_chain,chain)<0) { adv_sprite_kill(chain); return -1; }
      adv_sprgrp_remove(adv_sprgrp_visible,chain); // force depth sorting
      adv_sprgrp_insert(adv_sprgrp_visible,chain);
    }
    while (SPR->grp_chain->sprc>linkc) if (adv_sprite_kill(SPR->grp_chain->sprv[linkc])<0) return -1;
    int x=spr->x;
    for (i=0;i<linkc;i++) {
      if (spr->flop) x-=ADV_TILE_W; else x+=ADV_TILE_W;
      SPR->grp_chain->sprv[i]->x=x;
      SPR->grp_chain->sprv[i]->y=spr->y;
    }
  } else {
    int chainlen=SPR->y0-spr->y; if (chainlen<0) chainlen=-chainlen;
    int linkc=(chainlen+ADV_TILE_H-1)/ADV_TILE_H;
    while (SPR->grp_chain->sprc<linkc) {
      struct adv_sprite *chain=0;
      if (adv_sprite_instantiate(&chain,0)<0) return -1;
      chain->tileid=0x0f;
      chain->depth=spr->depth-1;
      if (spr->tileid&1) chain->flop=1;
      if (adv_sprgrp_insert(SPR->grp_chain,chain)<0) { adv_sprite_kill(chain); return -1; }
      adv_sprgrp_remove(adv_sprgrp_visible,chain); // force depth sorting
      adv_sprgrp_insert(adv_sprgrp_visible,chain);
    }
    while (SPR->grp_chain->sprc>linkc) if (adv_sprite_kill(SPR->grp_chain->sprv[linkc])<0) return -1;
    int y=spr->y;
    for (i=0;i<linkc;i++) {
      if (spr->tileid&1) y+=ADV_TILE_W; else y-=ADV_TILE_W;
      SPR->grp_chain->sprv[i]->y=y;
      SPR->grp_chain->sprv[i]->x=spr->x;
    }
  }

  return 0;
}

/* setup
 *****************************************************************************/

static int _adv_gadget_init(struct adv_sprite *spr) {
  if (adv_sprgrp_new(&SPR->grp_latched)<0) return -1;
  if (adv_sprgrp_new(&SPR->grp_chain)<0) return -1;
  if (adv_sprgrp_new(&SPR->grp_owner)<0) return -1;
  return 0;
}

static void _adv_gadget_del(struct adv_sprite *spr) {
  adv_sprgrp_clear(SPR->grp_latched);
  adv_sprgrp_del(SPR->grp_latched);
  adv_sprgrp_kill(SPR->grp_chain);
  adv_sprgrp_del(SPR->grp_chain);
  adv_sprgrp_clear(SPR->grp_owner);
  adv_sprgrp_del(SPR->grp_owner);
  adv_sprite_kill(SPR->knife);
}

const struct adv_sprclass adv_sprclass_gadget={
  .name="gadget",
  .objlen=sizeof(struct adv_sprite_gadget),
  .init=_adv_gadget_init,
  .del=_adv_gadget_del,
  .update=_adv_gadget_update,
};

int adv_sprite_gadget_set_owner(struct adv_sprite *spr,struct adv_sprite *owner) {
  if (!spr||!owner||(spr->sprclass!=&adv_sprclass_gadget)) return -1;
  if (adv_sprgrp_clear(SPR->grp_owner)<0) return -1;
  return adv_sprgrp_insert(SPR->grp_owner,owner);
}
