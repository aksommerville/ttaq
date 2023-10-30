#include "adv.h"
#include "sprite/adv_sprite.h"
#include "sprite/adv_sprclass.h"

struct adv_sprite_prize {
  struct adv_sprite hdr;
  int animcounter;
  int survival;
  int launchimmunity; // initial period after construction when launch will not happen
  
  //XXX: Older, quadratic-curve launching. Cute, but this is a puzzle game, so I want it more deterministic...
  int launchp,launchc; // position and duration of launch activity
  int launchx,launchy; // target for launching
  int launchcx,launchcy; // launch control point (path is quadratic bezier curve)
  int launchdx,launchdy; // gross direction of launch (ie direction hero was facing)
  int launchsx,launchsy; // source position of launch
  
  // New, boring old straight-line launching.
  int dx,dy;
  
};

#define SPR ((struct adv_sprite_prize*)spr)

#define ANIM_FRAME_TIME 8

#define SURVIVAL_TIME 900
#define SURVIVAL_BLINK_TIME 160
#define SURVIVAL_BLINK_INTERVAL 3

//XXX Curve launch.
#define LAUNCH_INITIAL_DISTANCE 10
#define LAUNCH_INITIAL_HEIGHT   16
#define LAUNCH_INITIAL_DURATION 60
#define LAUNCH_ADJUST_DISTANCE  20
#define LAUNCH_ADJUST_HEIGHT    20
#define LAUNCH_RANDOMNESS       10
#define LAUNCH_IMMUNITY         60

#define LAUNCH_SPEED 4

/* update
 *****************************************************************************/
 
static int _adv_prize_update(struct adv_sprite *spr) {
  if (--(SPR->survival)<=0) return adv_sprite_kill(spr);
  if (SPR->survival<SURVIVAL_BLINK_TIME) {
    if ((SPR->survival/SURVIVAL_BLINK_INTERVAL)&1) {
      adv_sprgrp_insert(adv_sprgrp_visible,spr);
    } else {
      adv_sprgrp_remove(adv_sprgrp_visible,spr);
    }
  }
  if (--(SPR->animcounter)<=0) {
    SPR->animcounter=ANIM_FRAME_TIME;
    spr->tileid=(spr->tileid&0xfc)+((spr->tileid+1)&3);
  }
  if (SPR->launchimmunity>0) SPR->launchimmunity--;
  else if (SPR->dx||SPR->dy) {
    if (!adv_sprite_move(spr,SPR->dx*LAUNCH_SPEED,SPR->dy*LAUNCH_SPEED,0)) SPR->dx=SPR->dy=0;
  }
  #if 0//XXX curve launch
  else if ((SPR->launchp>=0)&&(SPR->launchp<SPR->launchc)) {
    if (++(SPR->launchp)>=SPR->launchc) {
      spr->x=SPR->launchx;
      spr->y=SPR->launchy;
      SPR->launchp=SPR->launchc=SPR->launchdx=SPR->launchdy=0;
    } else {
      int ax=SPR->launchsx+((SPR->launchcx-SPR->launchsx)*SPR->launchp)/SPR->launchc;
      int ay=SPR->launchsy+((SPR->launchcy-SPR->launchsy)*SPR->launchp)/SPR->launchc;
      int bx=SPR->launchcx+((SPR->launchx-SPR->launchcx)*SPR->launchp)/SPR->launchc;
      int by=SPR->launchcy+((SPR->launchy-SPR->launchcy)*SPR->launchp)/SPR->launchc;
      spr->x=ax+((bx-ax)*SPR->launchp)/SPR->launchc;
      spr->y=ay+((by-ay)*SPR->launchp)/SPR->launchc;
    }
  }
  #endif
  return 0;
}

/* init
 *****************************************************************************/
 
static int _adv_prize_init(struct adv_sprite *spr) {
  if (adv_sprgrp_insert(adv_sprgrp_prize,spr)<0) return -1;
  if (adv_sprgrp_insert(adv_sprgrp_latchable,spr)<0) return -1;
  SPR->survival=SURVIVAL_TIME;
  SPR->launchimmunity=LAUNCH_IMMUNITY;
  return 0;
}

static int _adv_prize_postinit(struct adv_sprite *spr) {
  adv_sprgrp_remove(adv_sprgrp_solid,spr);
  return 0;
}

/* class definition
 *****************************************************************************/
 
const struct adv_sprclass adv_sprclass_prize={
  .name="prize",
  .objlen=sizeof(struct adv_sprite_prize),
  .init=_adv_prize_init,
  .postinit=_adv_prize_postinit,
  .update=_adv_prize_update,
  .collide_edges=1,
  .collide_map_positive=1,
  .collide_map_negative=0,
  .collide_solids=1,
};

/* launch (public but not declared in headers)
 *****************************************************************************/
 
int adv_sprite_prize_launch(struct adv_sprite *spr,int dx,int dy) {
  if (!spr||(spr->sprclass!=&adv_sprclass_prize)) return -1;
  if (dx&&dy) return -1;
  if (SPR->launchimmunity) return 0;
  
  SPR->dx=(dx<0)?-1:(dx>0)?1:0;
  SPR->dy=(dy<0)?-1:(dy>0)?1:0;
  
  #if 0 // curve launch
  /* Start from scratch? */
  if ((SPR->launchc<1)||(SPR->launchdx!=dx)||(SPR->launchdy!=dy)) {
    SPR->launchx=spr->x+dx*LAUNCH_INITIAL_DISTANCE;
    SPR->launchy=spr->y+dy*LAUNCH_INITIAL_DISTANCE;
    SPR->launchcx=(spr->x+SPR->launchx)>>1;
    SPR->launchcy=(spr->y+SPR->launchy)>>1;
    if (dx==-1) SPR->launchcy-=LAUNCH_INITIAL_HEIGHT;
    else if (dx==1) SPR->launchcy-=LAUNCH_INITIAL_HEIGHT;
    else if (dy==-1) ;
    else if (dy==1) ;
    else return -1;
    SPR->launchp=0;
    SPR->launchc=LAUNCH_INITIAL_DURATION;
    SPR->launchsx=spr->x;
    SPR->launchsy=spr->y;
    SPR->launchdx=dx;
    SPR->launchdy=dy;
  
  /* Adjust current launch paramters. */
  } else {
    SPR->launchx+=dx*LAUNCH_ADJUST_DISTANCE;
    SPR->launchy+=dy*LAUNCH_ADJUST_DISTANCE;
    SPR->launchcx+=dx*(LAUNCH_ADJUST_DISTANCE>>1);
    SPR->launchcy+=dy*(LAUNCH_ADJUST_DISTANCE>>1);
    if (dx) SPR->launchcy-=LAUNCH_ADJUST_HEIGHT;
  
  }
  SPR->launchx+=(rand()%LAUNCH_RANDOMNESS)-(LAUNCH_RANDOMNESS>>1);
  SPR->launchy+=(rand()%LAUNCH_RANDOMNESS)-(LAUNCH_RANDOMNESS>>1);
  #endif
  return 0;
}
