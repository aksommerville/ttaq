#include "adv.h"
#include "sprite/adv_sprite.h"
#include "sprite/adv_sprclass.h"
#include "game/adv_map.h"
#include "game/adv_game_progress.h"
#include "res/adv_res.h"

struct adv_sprite_sword {
  struct adv_sprite hdr;
  int done_screwing_around;
};

#define SPR ((struct adv_sprite_sword*)spr)

int adv_sprite_prize_launch(struct adv_sprite *spr,int dx,int dy);

/* update
 *****************************************************************************/
 
static int _adv_sword_update(struct adv_sprite *spr) {

  /* Choose our physical dimensions based on direction facing. */
  int x,y,w,h,dx=0,dy=0;
  switch (spr->tileid&3) {
    case 0: {
        x=spr->x-(ADV_TILE_W>>1)+19;
        y=spr->y-(ADV_TILE_H>>1);
        w=3;
        h=ADV_TILE_H-12;
        dy=1;
      } break;
    case 1: {
        x=spr->x-(ADV_TILE_W>>1)+10;
        y=spr->y-(ADV_TILE_H>>1)+12;
        w=3;
        h=ADV_TILE_H-12;
        dy=-1;
      } break;
    case 2: if (spr->flop) {
        x=spr->x-(ADV_TILE_W>>1);
        y=spr->y-(ADV_TILE_H>>1)+18;
        w=ADV_TILE_W-12;
        h=3;
        dx=1;
      } else {
        x=spr->x-(ADV_TILE_W>>1)+12;
        y=spr->y-(ADV_TILE_H>>1)+18;
        w=ADV_TILE_W-12;
        h=3;
        dx=-1;
      } break;
  }
  
  /* Look for any monsters to kill. */
  int i; for (i=0;i<adv_sprgrp_fragile->sprc;i++) {
    struct adv_sprite *qspr=adv_sprgrp_fragile->sprv[i];
    if (qspr->x-(ADV_TILE_W>>1)>=x+w) continue;
    if (qspr->y-(ADV_TILE_H>>1)>=y+h) continue;
    if (qspr->x+(ADV_TILE_W>>1)<=x) continue;
    if (qspr->y+(ADV_TILE_H>>1)<=y) continue;
    if (adv_sprite_hurt(qspr)<0) return -1;
  }
  
  /* Launch prizes when we hit them. */
  for (i=0;i<adv_sprgrp_prize->sprc;i++) {
    struct adv_sprite *qspr=adv_sprgrp_prize->sprv[i];
    if (qspr->x-(ADV_TILE_W>>1)>=x+w) continue;
    if (qspr->y-(ADV_TILE_H>>1)>=y+h) continue;
    if (qspr->x+(ADV_TILE_W>>1)<=x) continue;
    if (qspr->y+(ADV_TILE_H>>1)<=y) continue;
    if (adv_sprite_prize_launch(qspr,dx,dy)<0) return -1;
  }
  
  /* Activate SWORDSCREW switches. */
  if (!SPR->done_screwing_around) {
    SPR->done_screwing_around=1;
    int col=(x+(w>>1))/ADV_TILE_W,row=(y+(h>>1))/ADV_TILE_H;
    for (i=0;i<adv_map->switchc;i++) {
      // Don't bother with a sound effect here, since the sword's sound effect is enough.
      struct adv_switch *sw=adv_map->switchv+i;
      if (sw->method!=ADV_SWITCH_METHOD_SWORDSCREW) continue;
      if (sw->col!=col) continue;
      if (sw->row!=row) continue;
      if (adv_global_set(sw->globalid,adv_global_get(sw->globalid)?0:1)<0) return -1;
    }
  }
  
  return 0;
}

/* type definition
 *****************************************************************************/
 
const struct adv_sprclass adv_sprclass_sword={
  .name="sword",
  .objlen=sizeof(struct adv_sprite_sword),
  .update=_adv_sword_update,
};
