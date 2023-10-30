#include "adv.h"
#include "sprite/adv_sprite.h"
#include "sprite/adv_sprclass.h"
#include "sprite/adv_sprdef.h"
#include "res/adv_res.h"
#include "game/adv_map.h"

struct adv_sprite_knight {
  struct adv_sprite hdr;
  struct adv_sprgrp *weapon;
  int state; // 0:idle, 1:charge, -1:retreat
  int animcounter;
  int x0;
};

#define SPR ((struct adv_sprite_knight*)spr)

#define CHARGE_SPEED    7
#define RETREAT_SPEED   5
#define ANIM_FRAME_TIME 6

/* update
 *****************************************************************************/
 
static int _adv_knight_update(struct adv_sprite *spr) {
  struct adv_sprite *weapon=0;
  if (SPR->weapon->sprc>=1) weapon=SPR->weapon->sprv[0];
  
  /* Animation. */
  if (--(SPR->animcounter)<=0) {
    SPR->animcounter=ANIM_FRAME_TIME;
    if (SPR->state==0) spr->tileid=0xc2;
    else if (spr->tileid==0xc2) spr->tileid=0xd2;
    else if (spr->tileid==0xd2) spr->tileid=0xe2;
    else if (spr->tileid==0xe2) spr->tileid=0xd2;
    if (weapon) {
      if (weapon->tileid==0xa3) weapon->tileid=0xb3;
      else if (weapon->tileid==0xb3) weapon->tileid=0xa3;
    }
  }
  
  /* Move or look for heroes. */
  switch (SPR->state) {
    case 0: {
        int i; for (i=0;i<adv_sprgrp_hero->sprc;i++) {
          struct adv_sprite *qspr=adv_sprgrp_hero->sprv[i];
          if (spr->flop) {
            if (qspr->x>=spr->x) continue;
          } else {
            if (qspr->x<=spr->x) continue;
          }
          if (qspr->y<=spr->y-ADV_TILE_H) continue;
          if (qspr->y>=spr->y+ADV_TILE_H) continue;
          SPR->state=1;
          break;
        }
      } break;
    case 1: {
        if (!adv_sprite_move(spr,spr->flop?-CHARGE_SPEED:CHARGE_SPEED,0,0)) SPR->state=-1;
      } break;
    case -1: {
        spr->collide_solids=0; // only collide solides when moving forward -- eg, the heroes are solid and we want to kill them, not stop at them!
        if (spr->flop) {
          adv_sprite_move(spr,RETREAT_SPEED,0,0);
          if (spr->x>=SPR->x0) { spr->x=SPR->x0; SPR->state=0; }
        } else {
          adv_sprite_move(spr,-RETREAT_SPEED,0,0);
          if (spr->x<=SPR->x0) { spr->x=SPR->x0; SPR->state=0; }
        }
        spr->collide_solids=1;
      } break;
  }
  
  /* Weapon position. */
  if (weapon) {
    weapon->x=spr->x;
    weapon->y=spr->y;
    if (weapon->flop=spr->flop) weapon->x-=ADV_TILE_W;
    else weapon->x+=ADV_TILE_W;
  }
  return 0;
}

/* setup
 *****************************************************************************/
 
static int _adv_knight_init(struct adv_sprite *spr) {
  if (adv_sprgrp_new(&SPR->weapon)<0) return -1;
  if (adv_sprgrp_insert(adv_sprgrp_hazard,spr)<0) return -1;
  return 0;
}

static int _adv_knight_postinit(struct adv_sprite *spr) {
  if (!spr->sprdef) return 0;
  spr->flop=spr->sprdef->argv[0];
  SPR->x0=spr->x;
  struct adv_sprite *weapon=0;
  if (adv_sprite_instantiate(&weapon,0)<0) return -1;
  if (adv_sprgrp_insert(SPR->weapon,weapon)<0) return -1;
  if (adv_sprgrp_insert(adv_sprgrp_hazard,weapon)<0) return -1;
  switch (rand()&3) {
    case 0: weapon->tileid=0xa3; break;
    case 1: weapon->tileid=0xc3; break;
    case 2: weapon->tileid=0xd3; break;
    case 3: weapon->tileid=0xe3; break;
  }
  weapon->x=spr->x;
  weapon->y=spr->y;
  if (weapon->flop=spr->flop) weapon->x-=ADV_TILE_W;
  else weapon->x+=ADV_TILE_W;
  return 0;
}

static void _adv_knight_del(struct adv_sprite *spr) {
  adv_sprgrp_kill(SPR->weapon);
  adv_sprgrp_del(SPR->weapon);
}

const struct adv_sprclass adv_sprclass_knight={
  .name="knight",
  .objlen=sizeof(struct adv_sprite_knight),
  .init=_adv_knight_init,
  .postinit=_adv_knight_postinit,
  .del=_adv_knight_del,
  .update=_adv_knight_update,
  .collide_edges=1,
  .collide_map_positive=1,
  .collide_map_negative=1,
  .collide_solids=1,
  .arg0name="flop",
};
