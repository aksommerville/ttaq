#include "adv.h"
#include "sprite/adv_sprite.h"
#include "sprite/adv_sprdef.h"
#include "sprite/adv_sprclass.h"
#include "sprite/class/adv_sprite_hero.h"
#include "res/adv_res.h"
#include "game/adv_map.h"

// Cheating a bit.
struct adv_sprite_missile {
  struct adv_sprite hdr;
  int animcounter;
  int have_target;
  int ax,ay,bx,by,dx,dy;
  int xweight,yweight,weight;
  int suppress_animation;
};
extern const struct adv_sprclass adv_sprclass_missile;

struct adv_sprite_pitcher {
  struct adv_sprite hdr;
  struct adv_sprgrp *ball;
  int pitch_in_progress;
  int windup; // Counter after creating ball before we can throw it. Throwing too fast may cause tendonitis.
};

#define SPR ((struct adv_sprite_pitcher*)spr)

#define WINDUP_TIME 30

/* Find our batter and try to wang him in the head. That'll teach him.
 *****************************************************************************/
 
static int _adv_batter_looks_ready(struct adv_sprite *spr) {
  int col=spr->x/ADV_TILE_W;
  if (col<0) col=0; else if (col>=ADV_MAP_W) col=ADV_MAP_W-1;
  int bottom=ADV_SCREEN_H;
  int row; for (row=spr->y/ADV_TILE_H;row<ADV_MAP_H;row++) {
    if (adv_map_cell_is_solid(adv_map,col,row,1,0)) {
      bottom=row*ADV_TILE_H;
      break;
    }
  }
  int top=spr->y;
  int left=spr->x-(ADV_TILE_W>>1);
  int right=left+ADV_TILE_W;
  int i; for (i=0;i<adv_sprgrp_hero->sprc;i++) {
    struct adv_sprite *qspr=adv_sprgrp_hero->sprv[i];
    if (qspr->y<top) continue;
    if (qspr->y>bottom) continue;
    if (qspr->x<=left) continue;
    if (qspr->x>=right) continue;
    return 1;
  }
  return 0;
}

/* update
 *****************************************************************************/
 
static int _adv_pitcher_update(struct adv_sprite *spr) {
  if (SPR->pitch_in_progress) {
    // Other code here need not worry about the extra stuff that has to happen at pitch end. Just kill the ball.
    if (SPR->ball->sprc<1) { SPR->pitch_in_progress=0; spr->tileid-=0x10; return 0; }
  } else {
    struct adv_sprite *ball=0;
    if (SPR->ball->sprc<1) {
      if (adv_sprite_instantiate(&ball,&adv_sprclass_missile)<0) return -1;
      if (adv_sprgrp_insert(SPR->ball,ball)<0) { adv_sprite_kill(ball); return -1; }
      ((struct adv_sprite_missile*)ball)->dy=0;
      ((struct adv_sprite_missile*)ball)->have_target=1;
      ((struct adv_sprite_missile*)ball)->suppress_animation=1;
      ball->tileid=0xa4+(rand()%5)*0x10;
      SPR->windup=WINDUP_TIME;
    } else {
      ball=SPR->ball->sprv[0];
    }
    ball->x=spr->x+13;
    ball->y=spr->y-1;
    ball->depth=spr->depth+1;
    if (SPR->windup>0) {
      SPR->windup--;
    } else if (_adv_batter_looks_ready(spr)) {
      // let 'er rip
      ((struct adv_sprite_missile*)ball)->dy=1;
      ball->x=spr->x;
      ball->y=spr->y+(ADV_TILE_H>>1);
      SPR->pitch_in_progress=1;
      spr->tileid+=0x10;
    }
  }
  return 0;
}

/* setup
 *****************************************************************************/
 
static int _adv_pitcher_init(struct adv_sprite *spr) {
  if (adv_sprgrp_new(&SPR->ball)<0) return -1;
  if (adv_sprgrp_insert(adv_sprgrp_fragile,spr)<0) return -1;
  if (adv_sprgrp_insert(adv_sprgrp_hazard,spr)<0) return -1;
  if (adv_sprgrp_insert(adv_sprgrp_latchable,spr)<0) return -1;
  return 0;
}

static int _adv_pitcher_postinit(struct adv_sprite *spr) {
  return 0;
}

static void _adv_pitcher_del(struct adv_sprite *spr) {
  if (SPR->ball&&(SPR->ball->sprc==1)&&!((struct adv_sprite_missile*)SPR->ball->sprv[0])->dy) adv_sprgrp_kill(SPR->ball);
  else adv_sprgrp_clear(SPR->ball);
  adv_sprgrp_del(SPR->ball);
}

const struct adv_sprclass adv_sprclass_pitcher={
  .name="pitcher",
  .objlen=sizeof(struct adv_sprite_pitcher),
  .init=_adv_pitcher_init,
  .postinit=_adv_pitcher_postinit,
  .del=_adv_pitcher_del,
  .update=_adv_pitcher_update,
};
