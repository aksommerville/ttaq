#include "adv.h"
#include "sprite/adv_sprite.h"
#include "sprite/adv_sprclass.h"
#include "res/adv_res.h"
#include "game/adv_map.h"
#include "game/adv_game_progress.h"
#include "video/adv_video.h"
#include "input/adv_input.h"
#include "adv_sprite_hero.h"
#include <math.h>

#define SPR ((struct adv_sprite_hero*)spr)
#define ISPLAYER ((SPR->playerid>=1)&&(SPR->playerid<=ADV_PLAYER_LIMIT))

#define ANIM_FRAME_TIME 8 // Count of frames to display each face of walking animtion.

#define FIRE_RADIUS_LIMIT ADV_TILE_W
#define FIRE_ROTATION_SPEED 10

#define HAZARD_GRACE 4 // A little breathing room when checking hazard collisions.

#define GHOST_PREVENTION_TIME 30 // Minimum interval between ghost<~>flesh transitions.

#define ADV_HERO_WALK_SPEED      3
#define ADV_HERO_FOLLOW_SPEED    8

int adv_sprite_fire_set_owner(struct adv_sprite *spr,struct adv_sprite *owner);
int adv_sprite_gadget_set_owner(struct adv_sprite *spr,struct adv_sprite *owner);

/* sword
 *****************************************************************************/
 
static int _adv_hero_create_sword(struct adv_sprite *spr) {
  adv_sound(ADV_SOUND_SWORD);
  SPR->lock_direction=1;
  SPR->lock_movement=1;
  SPR->walking=0;
  struct adv_sprite *sword=0;
  int x=spr->x,y=spr->y,tadj;
  switch (tadj=spr->tileid%3) {
    case 0: y+=ADV_TILE_H; break;
    case 1: y-=ADV_TILE_H; break;
    case 2: if (spr->flop) {
        x+=ADV_TILE_W;
      } else {
        x-=ADV_TILE_W;
      } break;
  }
  if (adv_sprite_create(&sword,5,x,y)<0) return -1;
  sword->tileid+=tadj;
  sword->flop=spr->flop;
  if (adv_sprgrp_insert(SPR->grp_weapon,sword)<0) { adv_sprite_kill(sword); return -1; }
  return 0;
}

static int _adv_hero_drop_sword(struct adv_sprite *spr) {
  adv_sprgrp_kill(SPR->grp_weapon);
  return 0;
}

static int _adv_hero_update_sword(struct adv_sprite *spr) {
  // If it becomes possible for hero to move without "walking", we must update sword position here.
  return 0;
}

/* arrow
 *****************************************************************************/
 
static int _adv_hero_create_arrow(struct adv_sprite *spr) {
  adv_sound(ADV_SOUND_ARROW);
  SPR->lock_direction=1;
  SPR->lock_movement=1;
  SPR->walking=0;
  struct adv_sprite *arrow=0;
  int x=spr->x,y=spr->y,tadj;
  switch (tadj=spr->tileid%3) {
    case 0: y+=ADV_TILE_H; break;
    case 1: y-=ADV_TILE_H; break;
    case 2: if (spr->flop) {
        x+=ADV_TILE_W;
      } else {
        x-=ADV_TILE_W;
      } break;
  }
  if (adv_sprite_create(&arrow,6,x,y)<0) return -1;
  arrow->tileid+=tadj;
  arrow->flop=spr->flop;
  return 0;
}

/* gadget
 *****************************************************************************/
 
static int _adv_hero_create_gadget(struct adv_sprite *spr) {
  adv_sound(ADV_SOUND_GADGET);
  SPR->lock_direction=1;
  SPR->lock_movement=1;
  SPR->walking=0;
  struct adv_sprite *gadget=0;
  int x=spr->x,y=spr->y,tadj;
  switch (tadj=spr->tileid%3) {
    case 0: y+=ADV_TILE_H; break;
    case 1: y-=ADV_TILE_H; break;
    case 2: if (spr->flop) {
        x+=ADV_TILE_W;
      } else {
        x-=ADV_TILE_W;
      } break;
  }
  if (adv_sprite_create(&gadget,7,x,y)<0) return -1;
  gadget->tileid+=tadj;
  gadget->flop=spr->flop;
  gadget->depth=spr->depth;
  if (adv_sprite_gadget_set_owner(gadget,spr)<0) { adv_sprite_kill(gadget); return -1; }
  if (adv_sprgrp_insert(SPR->grp_weapon,gadget)<0) { adv_sprite_kill(gadget); return -1; }
  return 0;
}

static int _adv_hero_drop_gadget(struct adv_sprite *spr) {
  adv_sprgrp_kill(SPR->grp_weapon);
  if (!SPR->ghost) spr->tileid&=0x0f;
  SPR->lock_direction=0;
  SPR->lock_movement=0;
  SPR->input&=~(ADV_BTNID_LEFT|ADV_BTNID_RIGHT|ADV_BTNID_UP|ADV_BTNID_DOWN); // resume walking if pressed
  return 0;
}

static int _adv_hero_update_gadget(struct adv_sprite *spr) {
  if (SPR->grp_weapon->sprc<1) { // gadget deleted itself
    return _adv_hero_drop_gadget(spr);
  }
  return 0;
}

/* fire
 *****************************************************************************/
 
static int _adv_hero_create_fire(struct adv_sprite *spr) {
  SPR->fire_angle=0;
  SPR->fire_radius=0;
  struct adv_sprite *fire=0;
  if (adv_sprite_create(&fire,8,spr->x,spr->y)<0) return -1;
  if (adv_sprgrp_insert(SPR->grp_weapon,fire)<0) { adv_sprite_kill(fire); return -1; }
  adv_sprite_fire_set_owner(fire,spr);
  //if (adv_sprite_create(&fire,8,spr->x,spr->y)<0) return -1;
  //if (adv_sprgrp_insert(SPR->grp_weapon,fire)<0) { adv_sprite_kill(fire); return -1; }
  //adv_sprite_fire_set_owner(fire,spr);
  return 0;
}

static int _adv_hero_drop_fire(struct adv_sprite *spr) {
  adv_sprgrp_kill(SPR->grp_weapon);
  if (adv_map->lights_out) adv_video_lights_out(2,spr->x,spr->y);
  return 0;
}

static int _adv_hero_update_fire(struct adv_sprite *spr) {
  if (SPR->grp_weapon->sprc!=1) return 0;
  if ((SPR->fire_angle+=FIRE_ROTATION_SPEED)>=360) SPR->fire_angle-=360;
  if (++(SPR->fire_radius)>=FIRE_RADIUS_LIMIT) SPR->fire_radius=FIRE_RADIUS_LIMIT;
  double t=(SPR->fire_angle*M_PI)/180.0;
  int dx=cos(t)*SPR->fire_radius;
  int dy=sin(t)*SPR->fire_radius;
  SPR->grp_weapon->sprv[0]->x=spr->x+dx;
  SPR->grp_weapon->sprv[0]->y=spr->y+dy;
  //SPR->grp_weapon->sprv[1]->x=spr->x-dx;
  //SPR->grp_weapon->sprv[1]->y=spr->y-dy;
  if (adv_map->lights_out) adv_video_lights_out(1,SPR->grp_weapon->sprv[0]->x,SPR->grp_weapon->sprv[0]->y);
  return 0;
}

/* look for new keystrokes and update the direction we are facing.
 * call only if playerid is valid
 *****************************************************************************/
 
static int _adv_hero_update_direction(struct adv_sprite *spr) {
  if (SPR->lock_direction) return 0;
  if (SPR->ghost) return 0;
  int tilerow=spr->tileid&0xf0;
  int tilecol=spr->tileid&0x0f;
  int curdir=tilecol%3; // (down,up,left,right)
  if (spr->flop&&(curdir==2)) curdir++;
  int newdir=curdir;
  if ((adv_inputs[SPR->playerid]&ADV_BTNID_LEFT)&&!(SPR->input&ADV_BTNID_LEFT)) newdir=2;
  else if ((adv_inputs[SPR->playerid]&ADV_BTNID_RIGHT)&&!(SPR->input&ADV_BTNID_RIGHT)) newdir=3;
  else if ((adv_inputs[SPR->playerid]&ADV_BTNID_UP)&&!(SPR->input&ADV_BTNID_UP)) newdir=1;
  else if ((adv_inputs[SPR->playerid]&ADV_BTNID_DOWN)&&!(SPR->input&ADV_BTNID_DOWN)) newdir=0;
  else if ((SPR->walkdx<0)&&!SPR->walkdy) newdir=2;
  else if ((SPR->walkdx>0)&&!SPR->walkdy) newdir=3;
  else if ((SPR->walkdy<0)&&!SPR->walkdx) newdir=1;
  else if ((SPR->walkdy>0)&&!SPR->walkdx) newdir=0;
  else if ((SPR->walkdx<0)&&(curdir==3)) newdir=2;
  else if ((SPR->walkdx>0)&&(curdir==2)) newdir=3;
  else if ((SPR->walkdy<0)&&(curdir==0)) newdir=1;
  else if ((SPR->walkdy>0)&&(curdir==1)) newdir=0;
  if (newdir==curdir) return 0;
  if (newdir==3) {
    spr->tileid=tilerow+((tilecol/3)*3)+2;
    spr->flop=1;
  } else {
    spr->tileid=tilerow+((tilecol/3)*3)+newdir;
    spr->flop=0;
  }
  return 0;
}

/* flag sprite
 *****************************************************************************/
 
static int _adv_hero_update_flag(struct adv_sprite *spr) {
  if (SPR->grp_flag->sprc<1) return -1;
  struct adv_sprite *flag=SPR->grp_flag->sprv[0];
  flag->x=spr->x;
  flag->y=spr->y;
  switch (SPR->flagdir) {
    case 0: flag->y-=ADV_TILE_H; break;
    case 1: flag->y+=ADV_TILE_H; break;
    case 2: flag->x+=ADV_TILE_W; break;
    case 3: flag->x-=ADV_TILE_W; break;
  }
  return 0;
}

static int _adv_hero_create_flag(struct adv_sprite *spr) {
  struct adv_sprite *flag=0;
  if (adv_sprite_instantiate(&flag,0)<0) return -1;
  if (adv_sprgrp_insert(SPR->grp_flag,flag)<0) { adv_sprite_kill(flag); return -1; }
  if (spr->x<0) SPR->flagdir=2;
  else if (spr->x>ADV_SCREEN_W) SPR->flagdir=3;
  else if (spr->y<0) SPR->flagdir=1;
  else if (spr->y>ADV_SCREEN_H) SPR->flagdir=0;
  else if (SPR->walkdx<0) SPR->flagdir=2;
  else if (SPR->walkdx>0) SPR->flagdir=3;
  else if (SPR->walkdy>0) SPR->flagdir=0;
  else SPR->flagdir=1;
  flag->tileid=0x48+SPR->flagdir;
  flag->depth=90;
  adv_sprgrp_remove(adv_sprgrp_visible,spr);
  adv_sprgrp_remove(adv_sprgrp_solid,spr);
  adv_sprgrp_remove(adv_sprgrp_fragile,spr);
  return _adv_hero_update_flag(spr);
}

static int _adv_hero_drop_flag(struct adv_sprite *spr) {
  adv_sprgrp_kill(SPR->grp_flag);
  adv_sprgrp_insert(adv_sprgrp_visible,spr);
  if (!SPR->ghost) {
    adv_sprgrp_insert(adv_sprgrp_solid,spr);
    adv_sprgrp_insert(adv_sprgrp_fragile,spr);
  }
  return 0;
}

/* check switches
 *****************************************************************************/
 
// When you step off a deadman switch, don't just blindly clear the flag. 
// There might be multiple linked buttons for it.
// Nonzero if the switch is approved for clearing.
static int _adv_confirm_deadman_switch(int globalid) {
  int si; for (si=0;si<adv_map->switchc;si++) {
    struct adv_switch *sw=adv_map->switchv+si;
    if (sw->method!=ADV_SWITCH_METHOD_DEADMAN) continue;
    if (sw->globalid!=globalid) continue;
    int pi; for (pi=0;pi<adv_sprgrp_hero->sprc;pi++) {
      struct adv_sprite *spr=adv_sprgrp_hero->sprv[pi];
      if (spr->x/ADV_TILE_W!=sw->col) continue;
      if (spr->y/ADV_TILE_H!=sw->row) continue;
      return 0;
    }
  }
  return 1;
}
 
static int _adv_hero_check_switches(struct adv_sprite *spr,int pvcol,int pvrow) {
  if (!SPR->ghost&&!SPR->offscreen) {
    int crcol=spr->x/ADV_TILE_W,crrow=spr->y/ADV_TILE_H;
    if ((crcol!=pvcol)||(crrow!=pvrow)) {
      int i; for (i=0;i<adv_map->switchc;i++) {
        struct adv_switch *sw=adv_map->switchv+i;
        if ((sw->col==pvcol)&&(sw->row==pvrow)) {
          if (sw->method==ADV_SWITCH_METHOD_DEADMAN) {
            if (_adv_confirm_deadman_switch(sw->globalid)) {
              adv_sound(ADV_SOUND_SWITCH);
              if (adv_global_set(sw->globalid,0)<0) return -1;
            }
          }
        } else if ((sw->col==crcol)&&(sw->row==crrow)) {
          if (sw->method==ADV_SWITCH_METHOD_DEADMAN) {
            if (!SPR->ghost) {
              adv_sound(ADV_SOUND_SWITCH);
              if (adv_global_set(sw->globalid,1)<0) return -1;
            }
          } else if (sw->method==ADV_SWITCH_METHOD_STOMPBOX) {
            adv_sound(ADV_SOUND_SWITCH);
            if (adv_global_set(sw->globalid,adv_global_get(sw->globalid)?0:1)<0) return -1;
          }
        }
      }
    }
  }
  return 0;
}

/* movement -- call only if playerid is valid
 *****************************************************************************/
 
static int _adv_hero_update_move(struct adv_sprite *spr) {
  if (SPR->lock_movement) return 0;
  
  int pvcol=spr->x/ADV_TILE_W,pvrow=spr->y/ADV_TILE_H;
  
  int dx=0,dy=0;
  switch (adv_inputs[SPR->playerid]&(ADV_BTNID_LEFT|ADV_BTNID_RIGHT)) {
    case ADV_BTNID_LEFT: dx=-1; break;
    case ADV_BTNID_RIGHT: dx=1; break;
  }
  switch (adv_inputs[SPR->playerid]&(ADV_BTNID_UP|ADV_BTNID_DOWN)) {
    case ADV_BTNID_UP: dy=-1; break;
    case ADV_BTNID_DOWN: dy=1; break;
  }
  if (!dx&&!dy) {
    if (SPR->walking&&!SPR->ghost) spr->tileid&=0x0f; // drop to row 0
    SPR->walkdx=SPR->walkdy=SPR->walking=0;
    goto _check_offscreen_;
  }
  if (!SPR->walking&&!SPR->ghost) {
    SPR->animcounter=0;
    SPR->animstep=0;
  }
  SPR->walking=1;
  SPR->walkdx=dx;
  SPR->walkdy=dy;
  int speed=ADV_HERO_WALK_SPEED,moved;
  //if (SPR->drag) speed=1;
  if ((moved=adv_sprite_move(spr,dx*speed,dy*speed,1))<0) return -1;
  if (dx&&dy&&!SPR->drag&&(moved==speed)) SPR->drag=1;
  else SPR->drag=0;
  
  if ((dx||dy)&&!moved) {
    if (!dx) { // nudge horizontally
      int mod=spr->x%ADV_TILE_W;
      if (mod>(ADV_TILE_W>>1)) {
        if (adv_sprite_move(spr,-1,0,1)<0) return -1;
      } else if (mod<(ADV_TILE_W>>1)) {
        if (adv_sprite_move(spr,1,0,1)<0) return -1;
      }
    } else if (!dy) { // nudge vertically
      int mod=spr->y%ADV_TILE_H;
      if (mod>(ADV_TILE_H>>1)) {
        if (adv_sprite_move(spr,0,-1,1)<0) return -1;
      } else if (mod<(ADV_TILE_H>>1)) {
        if (adv_sprite_move(spr,0,1,1)<0) return -1;
      }
    }
  }
  
  /* Check offscreen. */
 _check_offscreen_:
  if (spr->x<=-(ADV_TILE_W>>1)) {
    spr->x=-(ADV_TILE_W>>1); SPR->offscreen=1; SPR->offscreen_target=adv_map->west;
  } else if (spr->y<=-(ADV_TILE_H>>1)) { 
    spr->y=-(ADV_TILE_H>>1); SPR->offscreen=1; SPR->offscreen_target=adv_map->north;
  } else if (spr->x>=ADV_SCREEN_W+(ADV_TILE_W>>1)) { 
    spr->x=ADV_SCREEN_W+(ADV_TILE_W>>1); SPR->offscreen=1; SPR->offscreen_target=adv_map->east;
  } else if (spr->y>=ADV_SCREEN_H+(ADV_TILE_H>>1)) { 
    spr->y=ADV_SCREEN_H+(ADV_TILE_H>>1); SPR->offscreen=1; SPR->offscreen_target=adv_map->south;
  } else {
    SPR->offscreen=0;
    int col=spr->x/ADV_TILE_W,row=spr->y/ADV_TILE_H;
    int i; for (i=0;i<adv_map->doorc;i++) {
      if (adv_map->doorv[i].srccol!=col) continue;
      if (adv_map->doorv[i].srcrow!=row) continue;
      SPR->offscreen=1;
      SPR->offscreen_target=adv_map->doorv[i].dstmapid;
      break;
    }
  }
  if (SPR->offscreen) {
    if (SPR->grp_flag->sprc>0) { if (_adv_hero_update_flag(spr)<0) return -1; }
    else if (_adv_hero_create_flag(spr)<0) return -1;
  } else {
    adv_map->suspend_xfer=0;
    if (SPR->grp_flag->sprc>0) {
      if (_adv_hero_drop_flag(spr)<0) return -1;
    }
  }
  
  /* Check switches. */
  if (_adv_hero_check_switches(spr,pvcol,pvrow)<0) return -1;
  
  return 0;
}

/* animation
 *****************************************************************************/
 
static int _adv_hero_update_animation(struct adv_sprite *spr) {
  if (SPR->ghost) {
    if (--(SPR->animcounter)<=0) {
      SPR->animcounter=ANIM_FRAME_TIME;
      if (spr->tileid==0x3f) spr->tileid=0x2f;
      else spr->tileid=0x3f;
    }
  } else if (SPR->walking) {
    if (--(SPR->animcounter)<=0) {
      SPR->animcounter=ANIM_FRAME_TIME;
      if (++(SPR->animstep)>=4) SPR->animstep=0;
      switch (SPR->animstep) {
        case 0: case 2: spr->tileid&=0x0f; break;
        case 1: spr->tileid=(spr->tileid&0x0f)|0x10; break;
        case 3: spr->tileid=(spr->tileid&0x0f)|0x20; break;
      }
    }
  }
  return 0;
}

/* weapon
 *****************************************************************************/
 
static int _adv_hero_update_weapon(struct adv_sprite *spr) {
  if (!SPR->weapon) return 0;
  if (SPR->ghost) return 0;
  if (SPR->offscreen) {
    if (SPR->grp_weapon->sprc>0) goto _drop_weapon_;
  } else if ((adv_inputs[SPR->playerid]&ADV_BTNID_ACTION)&&!(SPR->input&ADV_BTNID_ACTION)) {
    spr->tileid=0x30|(spr->tileid&0x0f);
    switch (SPR->weapon) {
      case ADV_WEAPON_SWORD: return _adv_hero_create_sword(spr);
      case ADV_WEAPON_ARROW: return _adv_hero_create_arrow(spr);
      case ADV_WEAPON_GADGET: return _adv_hero_create_gadget(spr);
      case ADV_WEAPON_FIRE: return _adv_hero_create_fire(spr);
    }
  } else if (!(adv_inputs[SPR->playerid]&ADV_BTNID_ACTION)&&(SPR->input&ADV_BTNID_ACTION)) {
   _drop_weapon_:
    spr->tileid&=0x0f;
    SPR->lock_direction=0;
    SPR->lock_movement=0;
    SPR->input&=~(ADV_BTNID_LEFT|ADV_BTNID_RIGHT|ADV_BTNID_UP|ADV_BTNID_DOWN); // resume walking if pressed
    switch (SPR->weapon) {
      case ADV_WEAPON_SWORD: return _adv_hero_drop_sword(spr);
      case ADV_WEAPON_ARROW: break;
      case ADV_WEAPON_GADGET: return _adv_hero_drop_gadget(spr);
      case ADV_WEAPON_FIRE: return _adv_hero_drop_fire(spr);
    }
  } else if (SPR->input&ADV_BTNID_ACTION) {
    switch (SPR->weapon) {
      case ADV_WEAPON_SWORD: return _adv_hero_update_sword(spr);
      case ADV_WEAPON_ARROW: break;
      case ADV_WEAPON_GADGET: return _adv_hero_update_gadget(spr);
      case ADV_WEAPON_FIRE: return _adv_hero_update_fire(spr);
    }
  }
  return 0;
}

/* transfer control to another sprite
 *****************************************************************************/
 
static int _adv_hero_update_xfer(struct adv_sprite *spr) {
  //TODO Permit switching when all heroes have a player. First player holds SWITCH, second player triggers it.
  if (!(adv_inputs[SPR->playerid]&ADV_BTNID_SWITCH)) return 0;
  if (SPR->input&ADV_BTNID_SWITCH) return 0;
  struct adv_sprite *wrap=0,*advance=0;
  int i,tick=0;
  for (i=0;i<adv_sprgrp_hero->sprc;i++) {
    if (adv_sprgrp_hero->sprv[i]->sprclass!=&adv_sprclass_hero) continue;
    if (adv_sprgrp_hero->sprv[i]==spr) tick=1;
    else if (!((struct adv_sprite_hero*)adv_sprgrp_hero->sprv[i])->playerid) {
      if (!tick) {
        if (!wrap) wrap=adv_sprgrp_hero->sprv[i];
      } else {
        if (!advance) advance=adv_sprgrp_hero->sprv[i];
        break;
      }
    }
  }
  if (!advance) advance=wrap;
  if (!advance) return 0;
  adv_sound(ADV_SOUND_CHANGECTL);
  ((struct adv_sprite_hero*)advance)->playerid=SPR->playerid;
  ((struct adv_sprite_hero*)advance)->input=ADV_BTNID_SWITCH;
  SPR->playerid=0;
  adv_sprite_create(0,9,advance->x,advance->y); // xferhiglight
  return 1;
}

/* when ghost, touching another player kills it. Player, not all fragile sprites.
 *****************************************************************************/
 
static int _adv_hero_update_ghost_kill(struct adv_sprite *spr) {
  if (SPR->offscreen) return 0;
  int col=spr->x/ADV_TILE_W,row=spr->y/ADV_TILE_H;
  if (adv_map_cell_is_heal(adv_map,col,row)) return adv_sprite_hero_fleshify(spr,1);
  int i; for (i=0;i<adv_sprgrp_hero->sprc;i++) {
    struct adv_sprite *qspr=adv_sprgrp_hero->sprv[i];
    if (qspr==spr) continue;
    if (qspr->sprclass!=&adv_sprclass_hero) continue;
    if (((struct adv_sprite_hero*)qspr)->offscreen) continue;
    int dx=spr->x-qspr->x; if ((dx<-ADV_TILE_W>>1)||(dx>ADV_TILE_W>>1)) continue;
    int dy=spr->y-qspr->y; if ((dy<-ADV_TILE_H>>1)||(dy>ADV_TILE_H>>1)) continue;
    adv_sprite_hero_ghostify(qspr);
  }
  return 0;
}

/* check hazards
 *****************************************************************************/
 
static int _adv_hero_update_hazards(struct adv_sprite *spr) {
  if (SPR->ghost||SPR->offscreen) return 0;
  int i; for (i=0;i<adv_sprgrp_hazard->sprc;i++) {
    struct adv_sprite *qspr=adv_sprgrp_hazard->sprv[i];
    int dx=spr->x-qspr->x; if ((dx<-ADV_TILE_W+HAZARD_GRACE)||(dx>ADV_TILE_W-HAZARD_GRACE)) continue;
    int dy=spr->y-qspr->y; if ((dy<-ADV_TILE_H+HAZARD_GRACE)||(dy>ADV_TILE_H-HAZARD_GRACE)) continue;
    adv_sprite_hero_ghostify(spr);
    return 1;
  }
  return 0;
}

/* check prizes
 *****************************************************************************/
 
static int _adv_hero_update_prizes(struct adv_sprite *spr) {
  int i; for (i=0;i<adv_sprgrp_prize->sprc;i++) {
    struct adv_sprite *qspr=adv_sprgrp_prize->sprv[i];
    if (qspr==spr) continue;
    int dx=spr->x-qspr->x; if ((dx<-ADV_TILE_W+HAZARD_GRACE)||(dx>ADV_TILE_W-HAZARD_GRACE)) continue;
    int dy=spr->y-qspr->y; if ((dy<-ADV_TILE_H+HAZARD_GRACE)||(dy>ADV_TILE_H-HAZARD_GRACE)) continue;
    adv_sound(ADV_SOUND_REVIVE); // we want the sound effect whether it does anything or not
    adv_sprite_hero_fleshify(spr,0);
    adv_sprite_kill(qspr);
  }
  return 0;
}

/* enagage follow-me
 *****************************************************************************/
 
static int _adv_hero_update_followme(struct adv_sprite *spr) {
  if ((SPR->playerid<1)||(SPR->playerid>ADV_PLAYER_LIMIT)) return 0;
  if (adv_inputs[SPR->playerid]&ADV_BTNID_FOLLOWME) {
    int i; for (i=0;i<adv_sprgrp_hero->sprc;i++) {
      struct adv_sprite *qspr=adv_sprgrp_hero->sprv[i];
      struct adv_sprite_hero *QSPR=(struct adv_sprite_hero*)qspr;
      if (QSPR->playerid) continue;
      int dx=spr->x-qspr->x;
      int dy=spr->y-qspr->y;
      if (dx<-ADV_HERO_FOLLOW_SPEED) dx=-ADV_HERO_FOLLOW_SPEED;
      else if (dx>ADV_HERO_FOLLOW_SPEED) dx=ADV_HERO_FOLLOW_SPEED;
      if (dy<-ADV_HERO_FOLLOW_SPEED) dy=-ADV_HERO_FOLLOW_SPEED;
      else if (dy>ADV_HERO_FOLLOW_SPEED) dy=ADV_HERO_FOLLOW_SPEED;
      if (!dx&&!dy) continue;
      int pvcol=qspr->x/ADV_TILE_W,pvrow=qspr->y/ADV_TILE_H;
      if (adv_sprite_move(qspr,dx,dy,1)<0) return -1;
      QSPR->walkdx=dx;
      QSPR->walkdy=dy;
      if (_adv_hero_check_switches(qspr,pvcol,pvrow)<0) return -1;
    }
  }
  return 0;
}

/* update, dispatch
 *****************************************************************************/

static int _adv_hero_update(struct adv_sprite *spr) {
  int err;
  if (!SPR->weapon) SPR->weapon=spr->sprdefid;//TODO: assign weapon
  if (SPR->ghost_prevention) SPR->ghost_prevention--;
  if (err=_adv_hero_update_xfer(spr)) return err;
  if (_adv_hero_update_direction(spr)<0) return -1;
  if (_adv_hero_update_move(spr)<0) return -1;
  if (_adv_hero_update_animation(spr)<0) return -1;
  if (_adv_hero_update_weapon(spr)<0) return -1;
  if (SPR->ghost) _adv_hero_update_ghost_kill(spr);
  else if (err=_adv_hero_update_hazards(spr)) return err;
  if (_adv_hero_update_prizes(spr)<0) return -1;
  if (_adv_hero_update_followme(spr)<0) return -1;
  SPR->input=adv_inputs[SPR->playerid];
  return 0;
}

/* init/del
 *****************************************************************************/
 
static int _adv_hero_init(struct adv_sprite *spr) {
  if (adv_sprgrp_new(&SPR->grp_weapon)<0) return -1;
  if (adv_sprgrp_new(&SPR->grp_flag)<0) return -1;
  if (adv_sprgrp_insert(adv_sprgrp_hero,spr)<0) return -1;
  if (adv_sprgrp_insert(adv_sprgrp_fragile,spr)<0) return -1;
  return 0;
}

static void _adv_hero_del(struct adv_sprite *spr) {
  adv_sprgrp_clear(SPR->grp_weapon);
  adv_sprgrp_del(SPR->grp_weapon);
  adv_sprgrp_kill(SPR->grp_flag);
  adv_sprgrp_del(SPR->grp_flag);
}

/* sprclass definition
 *****************************************************************************/

const struct adv_sprclass adv_sprclass_hero={
  .name="hero",
  .objlen=sizeof(struct adv_sprite_hero),
  .init=_adv_hero_init,
  .del=_adv_hero_del,
  .update=_adv_hero_update,
  .hurt=adv_sprite_hero_ghostify,
  .collide_edges=0,
  .collide_map_positive=1,
  .collide_map_negative=1,
  .collide_solids=1,
};

/* minor public accessors
 *****************************************************************************/
 
int adv_sprite_hero_set_playerid(struct adv_sprite *spr,int playerid) {
  if (!spr||(spr->sprclass!=&adv_sprclass_hero)) return -1;
  SPR->playerid=playerid;
  return 0;
}

/* ghost
 *****************************************************************************/

int adv_sprite_hero_ghostify(struct adv_sprite *spr) {
  if (!spr||(spr->sprclass!=&adv_sprclass_hero)) return -1;
  if (SPR->ghost) return 0;
  if (SPR->ghost_prevention) return 0;
  adv_sound(ADV_SOUND_STRIKE);
  if (!SPR->base_tileid) SPR->base_tileid=((spr->tileid&0x0f)/3)*3;
  SPR->ghost=1;
  SPR->ghost_prevention=GHOST_PREVENTION_TIME;
  spr->tileid=0x2f;
  spr->flop=0;
  SPR->lock_direction=0;
  SPR->lock_movement=0;
  SPR->input&=~(ADV_BTNID_LEFT|ADV_BTNID_RIGHT|ADV_BTNID_UP|ADV_BTNID_DOWN); // resume walking if pressed
  switch (SPR->weapon) {
    case ADV_WEAPON_SWORD: _adv_hero_drop_sword(spr); break;
    case ADV_WEAPON_ARROW: break;
    case ADV_WEAPON_GADGET: _adv_hero_drop_gadget(spr); break;
    case ADV_WEAPON_FIRE: _adv_hero_drop_fire(spr); break;
  }
  adv_sprgrp_remove(adv_sprgrp_solid,spr);
  adv_sprgrp_remove(adv_sprgrp_fragile,spr);
  spr->collide_solids=0;
  spr->collide_map_negative=0;
  int col=spr->x/ADV_TILE_W,row=spr->y/ADV_TILE_H;
  int i; for (i=0;i<adv_map->switchc;i++) {
    struct adv_switch *sw=adv_map->switchv+i;
    if ((sw->col!=col)||(sw->row!=row)) continue;
    if (sw->method==ADV_SWITCH_METHOD_DEADMAN) {
      if (adv_global_set(sw->globalid,0)<0) return -1;
    }
  }
  return 0;
}

int adv_sprite_hero_fleshify(struct adv_sprite *spr,int with_sound_effect) {
  if (!spr||(spr->sprclass!=&adv_sprclass_hero)) return -1;
  if (!SPR->ghost) return 0;
  if (SPR->ghost_prevention) return 0;
  if (with_sound_effect) adv_sound(ADV_SOUND_REVIVE);
  SPR->ghost=0;
  SPR->ghost_prevention=GHOST_PREVENTION_TIME;
  spr->tileid=SPR->base_tileid;
  if (!SPR->offscreen) {
    adv_sprgrp_insert(adv_sprgrp_solid,spr);
    adv_sprgrp_insert(adv_sprgrp_fragile,spr);
  }
  spr->collide_solids=1;
  spr->collide_map_negative=1;
  //TODO We ought to ensure that no collision exists.
  int col=spr->x/ADV_TILE_W,row=spr->y/ADV_TILE_H;
  int i; for (i=0;i<adv_map->switchc;i++) {
    struct adv_switch *sw=adv_map->switchv+i;
    if ((sw->col!=col)||(sw->row!=row)) continue;
    if (sw->method==ADV_SWITCH_METHOD_DEADMAN) {
      adv_sound(ADV_SOUND_SWITCH);
      if (adv_global_set(sw->globalid,1)<0) return -1;
    } else if (sw->method==ADV_SWITCH_METHOD_STOMPBOX) {
      adv_sound(ADV_SOUND_SWITCH);
      if (adv_global_set(sw->globalid,adv_global_get(sw->globalid)?0:1)<0) return -1;
    }
  }
  return 0;
}
