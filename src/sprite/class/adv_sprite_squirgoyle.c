#include "adv.h"
#include "sprite/adv_sprite.h"
#include "sprite/adv_sprdef.h"
#include "sprite/adv_sprclass.h"
#include "res/adv_res.h"
#include "game/adv_map.h"

struct adv_sprite_squirgoyle {
  struct adv_sprite hdr;
  int state; // 0:idle, 1:blink, 2:burninate
  int suspend; // no state change while counting time
  int decocounter; // timer for blinking
  struct adv_sprgrp *flame;
  int animcounter; // for the flame (we animate it; its class is NULL)
};

#define SPR ((struct adv_sprite_squirgoyle*)spr)

#define SUSPEND_TIME 30
#define BLINK_TIME   10
#define BLINK_INTERVAL_MIN 60
#define BLINK_INTERVAL_MAX 600
#define FLAME_ANIM_TIME 6
#define VICTIM_GRACE 8

/* burnination
 *****************************************************************************/
 
static int _adv_squirgoyle_should_burninate(struct adv_sprite *spr) {
  int top=spr->y-ADV_TILE_H,bottom=spr->y+ADV_TILE_H;
  int i; for (i=0;i<adv_sprgrp_hero->sprc;i++) {
    struct adv_sprite *qspr=adv_sprgrp_hero->sprv[i];
    if (qspr->y<top) continue;
    if (qspr->y>bottom) continue;
    if (spr->flop) {
      if (qspr->x>=spr->x) continue;
    } else {
      if (qspr->x<=spr->x) continue;
    }
    return 1;
  }
  return 0;
}

static int _adv_squirgoyle_survey_wreckage(struct adv_sprite *spr) {
  if (SPR->flame->sprc<1) return 0;
  struct adv_sprite *fire=SPR->flame->sprv[0];
  int left=fire->x-ADV_TILE_W+VICTIM_GRACE;
  int right=fire->x+ADV_TILE_W-VICTIM_GRACE;
  int top=fire->y-ADV_TILE_H+VICTIM_GRACE;
  int bottom=fire->y+ADV_TILE_H-VICTIM_GRACE;
  // Fire will be triggered only by heroes, but it burns anything fragile.
  int i; for (i=0;i<adv_sprgrp_fragile->sprc;i++) {
    struct adv_sprite *qspr=adv_sprgrp_fragile->sprv[i];
    if (qspr->x<=left) continue;
    if (qspr->y<=top) continue;
    if (qspr->x>=right) continue;
    if (qspr->y>=bottom) continue;
    if (adv_sprite_hurt(qspr)<0) return -1;
  }
  return 0;
}

/* update
 *****************************************************************************/
 
static int _adv_squirgoyle_update(struct adv_sprite *spr) {
  if (SPR->flame->sprc>0) {
    if (--(SPR->animcounter)<=0) {
      SPR->animcounter=FLAME_ANIM_TIME;
      int i; for (i=0;i<SPR->flame->sprc;i++) {
        struct adv_sprite *fire=SPR->flame->sprv[i];
        if (fire->tileid==0xd1) fire->tileid=0xe1;
        else if (fire->tileid==0xe1) fire->tileid=0xd1;
      }
    }
  }
  if (SPR->suspend) {
    if (SPR->state==2) {
      if (_adv_squirgoyle_survey_wreckage(spr)<0) return -1;
    }
    SPR->suspend--;
  } else switch (SPR->state) {
  
    case 0: { // idle
        if (_adv_squirgoyle_should_burninate(spr)) {
          SPR->state=2;
          SPR->suspend=SUSPEND_TIME;
          struct adv_sprite *fire=0;
          if (adv_sprite_instantiate(&fire,0)<0) return -1;
          if (adv_sprgrp_insert(SPR->flame,fire)<0) { adv_sprite_kill(fire); return -1; }
          fire->y=spr->y;
          if (spr->flop) {
            fire->x=spr->x-ADV_TILE_W; 
            fire->flop=1;
          } else {
            fire->x=spr->x+ADV_TILE_W;
          }
          fire->tileid=0xd1;
          spr->tileid+=0x20;
        } else if (--(SPR->decocounter)<=0) {
          SPR->state=1;
          SPR->decocounter=BLINK_TIME;
          spr->tileid+=0x10;
        }
      } break;
      
    case 1: { // blink
        if (--(SPR->decocounter)<=0) {
          SPR->decocounter=BLINK_INTERVAL_MIN+rand()%(BLINK_INTERVAL_MAX-BLINK_INTERVAL_MIN+1);
          spr->tileid-=0x10;
          SPR->state=0;
        }
      } break;
    
    case 2: { // burninate
        if (!_adv_squirgoyle_should_burninate(spr)) {
          SPR->state=0;
          SPR->suspend=SUSPEND_TIME;
          adv_sprgrp_kill(SPR->flame);
          spr->tileid-=0x20;
        } else {
          if (_adv_squirgoyle_survey_wreckage(spr)<0) return -1;
        }
      } break;
      
  }
  return 0;
}

/* setup
 *****************************************************************************/
 
static int _adv_squirgoyle_init(struct adv_sprite *spr) {
  if (adv_sprgrp_insert(adv_sprgrp_hazard,spr)<0) return -1;
  if (adv_sprgrp_insert(adv_sprgrp_fragile,spr)<0) return -1;
  SPR->decocounter=BLINK_INTERVAL_MIN+rand()%(BLINK_INTERVAL_MAX-BLINK_INTERVAL_MIN+1);
  if (adv_sprgrp_new(&SPR->flame)<0) return -1;
  return 0;
}

static int _adv_squirgoyle_postinit(struct adv_sprite *spr) {
  if (spr->sprdef) {
    spr->flop=spr->sprdef->argv[0];
  }
  return 0;
}

static void _adv_squirgoyle_del(struct adv_sprite *spr) {
  adv_sprgrp_kill(SPR->flame);
  adv_sprgrp_del(SPR->flame);
}

const struct adv_sprclass adv_sprclass_squirgoyle={
  .name="squirgoyle",
  .objlen=sizeof(struct adv_sprite_squirgoyle),
  .init=_adv_squirgoyle_init,
  .postinit=_adv_squirgoyle_postinit,
  .del=_adv_squirgoyle_del,
  .update=_adv_squirgoyle_update,
  .arg0name="flop",
};
