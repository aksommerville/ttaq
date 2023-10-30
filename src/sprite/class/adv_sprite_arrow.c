#include "adv.h"
#include "sprite/adv_sprite.h"
#include "sprite/adv_sprclass.h"
#include "game/adv_map.h"
#include "res/adv_res.h"

struct adv_sprite_arrow {
  struct adv_sprite hdr;
};

#define SPR ((struct adv_sprite_arrow*)spr)

#define ARROW_SPEED 6

/* update
 *****************************************************************************/
 
static int _adv_arrow_update(struct adv_sprite *spr) {

  /* Move, kill if offscreen. 
   * Also take this opportunity to store the collision area.
   */
  int x,y,w,h;
  switch (spr->tileid&3) {
    case 0: {
        spr->y+=ARROW_SPEED;
        if (spr->y-(ADV_TILE_H>>1)>=ADV_SCREEN_H) return adv_sprite_kill(spr);
        x=spr->x-(ADV_TILE_W>>1)+19;
        y=spr->y-(ADV_TILE_H>>1);
        w=3;
        h=ADV_TILE_H;
      } break;
    case 1: {
        spr->y-=ARROW_SPEED;
        if (spr->y+(ADV_TILE_H>>1)<=0) return adv_sprite_kill(spr);
        x=spr->x-(ADV_TILE_W>>1)+11;
        y=spr->y-(ADV_TILE_H>>1);
        w=3;
        h=ADV_TILE_H;
      } break;
    case 2: {
        if (spr->flop) {
          spr->x+=ARROW_SPEED;
          if (spr->x-(ADV_TILE_W>>1)>=ADV_SCREEN_W) return adv_sprite_kill(spr);
        } else {
          spr->x-=ARROW_SPEED;
          if (spr->x+(ADV_TILE_W>>1)<=0) return adv_sprite_kill(spr);
        }
        x=spr->x-(ADV_TILE_W>>1);
        y=spr->y-(ADV_TILE_H>>1)+20;
        w=ADV_TILE_W;
        h=3;
      } break;
  }
  
  /* Hurt fragile sprites. */
  int i; for (i=0;i<adv_sprgrp_fragile->sprc;i++) {
    struct adv_sprite *qspr=adv_sprgrp_fragile->sprv[i];
    int qx=qspr->x-(ADV_TILE_W>>1);
    int qy=qspr->y-(ADV_TILE_H>>1);
    if (qx>=x+w) continue;
    if (qy>=y+h) continue;
    if (qx+ADV_TILE_W<=x) continue;
    if (qy+ADV_TILE_H<=y) continue;
    if (adv_sprite_hurt(qspr)<0) return -1;
    return adv_sprite_kill(spr);
  }
  
  /* Kill if we hit anything solid. */
  int col; for (col=x/ADV_TILE_W;col<=(x+w-1)/ADV_TILE_W;col++) {
    int row; for (row=y/ADV_TILE_H;row<=(y+h-1)/ADV_TILE_H;row++) {
      if (adv_map_cell_is_solid(adv_map,col,row,1,0)) return adv_sprite_kill(spr);
    }
  }
  
  return 0;
}

/* type definition
 *****************************************************************************/
 
const struct adv_sprclass adv_sprclass_arrow={
  .name="arrow",
  .objlen=sizeof(struct adv_sprite_arrow),
  .update=_adv_arrow_update,
};
