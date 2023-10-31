#include "adv_input_internal.h"
#include "sprite/adv_sprite.h"
#include "sprite/class/adv_sprite_hero.h"

/* del
 *****************************************************************************/
 
void adv_controller_del(struct adv_controller *controller) {
  if (!controller) return;
  if (controller->absmapv) free(controller->absmapv);
  if (controller->keymapv) free(controller->keymapv);
  free(controller);
}

/* new
 *****************************************************************************/
 
int adv_controller_new(struct adv_controller **dst) {
  if (!dst) return -1;
  if (!(*dst=calloc(1,sizeof(struct adv_controller)))) return -1;
  return 0;
}

/* map lists
 *****************************************************************************/
 
int adv_controller_absmap_search(struct adv_controller *controller,int code) {
  if (!controller) return -1;
  int lo=0,hi=controller->absmapc;
  while (lo<hi) {
    int ck=(lo+hi)>>1;
         if (code<controller->absmapv[ck].code) hi=ck;
    else if (code>controller->absmapv[ck].code) lo=ck+1;
    else return ck;
  }
  return -lo-1;
}
 
int adv_controller_keymap_search(struct adv_controller *controller,int code) {
  if (!controller) return -1;
  int lo=0,hi=controller->keymapc;
  while (lo<hi) {
    int ck=(lo+hi)>>1;
         if (code<controller->keymapv[ck].code) hi=ck;
    else if (code>controller->keymapv[ck].code) lo=ck+1;
    else return ck;
  }
  return -lo-1;
}

int adv_controller_absmap_insert(struct adv_controller *controller,int p,int code) {
  if (!controller||(p<0)||(p>controller->absmapc)) return -1;
  if (p&&(code<=controller->absmapv[p-1].code)) return -1;
  if ((p<controller->absmapc)&&(code>=controller->absmapv[p].code)) return -1;
  if (controller->absmapc>=controller->absmapa) {
    int na=controller->absmapa+8;
    if (na>INT_MAX/sizeof(struct adv_absmap)) return -1;
    void *nv=realloc(controller->absmapv,sizeof(struct adv_absmap)*na);
    if (!nv) return -1;
    controller->absmapv=nv;
    controller->absmapa=na;
  }
  memmove(controller->absmapv+p+1,controller->absmapv+p,sizeof(struct adv_absmap)*(controller->absmapc-p));
  controller->absmapc++;
  struct adv_absmap *absmap=controller->absmapv+p;
  memset(absmap,0,sizeof(struct adv_absmap));
  absmap->code=code;
  return 0;
}

int adv_controller_keymap_insert(struct adv_controller *controller,int p,int code) {
  if (!controller||(p<0)||(p>controller->keymapc)) return -1;
  if (p&&(code<=controller->keymapv[p-1].code)) return -1;
  if ((p<controller->keymapc)&&(code>=controller->keymapv[p].code)) return -1;
  if (controller->keymapc>=controller->keymapa) {
    int na=controller->keymapa+8;
    if (na>INT_MAX/sizeof(struct adv_keymap)) return -1;
    void *nv=realloc(controller->keymapv,sizeof(struct adv_keymap)*na);
    if (!nv) return -1;
    controller->keymapv=nv;
    controller->keymapa=na;
  }
  memmove(controller->keymapv+p+1,controller->keymapv+p,sizeof(struct adv_keymap)*(controller->keymapc-p));
  controller->keymapc++;
  struct adv_keymap *keymap=controller->keymapv+p;
  memset(keymap,0,sizeof(struct adv_keymap));
  keymap->code=code;
  return 0;
}

/* setup
 *****************************************************************************/
 
int adv_controller_setup(struct adv_controller *controller,int devid,int vid,int pid,const char *name,int namec) {
  if (!controller) return -1;
  if (!name||(namec<0)) namec=0;
  controller->devid=devid;
  
  int bestinmap=-1,score=0,i;
  for (i=0;i<adv_input.inmapc;i++) {
    int qscore=adv_inmap_compare(adv_input.inmapv[i],vid,pid,name,namec,devid);
    if (qscore>score) {
      bestinmap=i;
      score=qscore;
      if (score==ADV_INMAP_SCORE_BEST) break; // no sense comparing any others
    }
  }
  if (bestinmap<0) return -1; // no suitable map
  
  /* Apply inmap to controller. */
  struct adv_inmap *inmap=adv_input.inmapv[bestinmap];
  for (i=0;i<inmap->absmapc;i++) {
    //TODO if (!linput_device_layout_has_abs(&layout,inmap->absmapv[i].code)) continue;
    int p=controller->absmapc; // controller and layout both sort by code, so we don't need to search here
    if (adv_controller_absmap_insert(controller,p,inmap->absmapv[i].code)<0) return -1;
    struct adv_absmap *m=controller->absmapv+p;
    *m=inmap->absmapv[i];
    //TODO We probably do need to query input devices; can't just make shit up for the abs ranges.
    int lo=-32767;//layout.absinfo[m->code].minimum;
    int hi=32768;//layout.absinfo[m->code].maximum;
    if (lo<-32768) lo=-32768;
    if (hi>32767) hi=32767;
    if (lo>=hi) { lo=-32768; hi=32767; }
    if (m->btnlo) { // 2-way mapping
      int mid=(lo+hi)>>1;
      m->lo=(lo+mid)>>1;
      m->hi=(mid+hi)>>1;
      if (m->lo>=m->hi-1) { m->lo=lo; m->hi=hi; }
    } else { // 1-way mapping
      if (lo==hi-1) m->hi=hi;
      else m->hi=lo+((hi-lo)>>1);
    }
  }
  for (i=0;i<inmap->keymapc;i++) {
    //TODO if (!linput_device_layout_has_key(&layout,inmap->keymapv[i].code)) continue;
    int p=controller->keymapc;
    if (adv_controller_keymap_insert(controller,p,inmap->keymapv[i].code)<0) return -1;
    controller->keymapv[p]=inmap->keymapv[i];
  }
  
  return 0;
}

/* click-in -- choose a playerid if one is unassigned
 *****************************************************************************/
 
static void adv_controller_clickin(struct adv_controller *controller) {
  int inuse[1+ADV_PLAYER_LIMIT]={0};
  int i; for (i=0;i<adv_input.controllerc;i++) {
    if (adv_input.controllerv[i]->playerid<1) continue;
    if (adv_input.controllerv[i]->playerid>ADV_PLAYER_LIMIT) continue;
    inuse[adv_input.controllerv[i]->playerid]=1;
  }
  for (i=1;i<=ADV_PLAYER_LIMIT;i++) if (!inuse[i]) {
    controller->playerid=i;
    int j; for (j=0;i<adv_sprgrp_hero->sprc;j++) {
      struct adv_sprite_hero *hero=(struct adv_sprite_hero*)adv_sprgrp_hero->sprv[j];
      if (!hero->playerid) { 
        hero->playerid=i; 
        adv_sprite_create(0,9,hero->hdr.x,hero->hdr.y); // xferhiglight
        break; 
      }
    }
    return;
  }
}

/* set button
 *****************************************************************************/
 
static void adv_controller_set_btn(struct adv_controller *controller,unsigned char btn,int value) {
  if (!btn) return;
  if (value) {
    if (btn==ADV_BTNID_DETACH) {
      if ((controller->playerid>=1)&&(controller->playerid<=ADV_PLAYER_LIMIT)) {
        int i; for (i=0;i<adv_sprgrp_hero->sprc;i++) {
          struct adv_sprite_hero *hero=(struct adv_sprite_hero*)adv_sprgrp_hero->sprv[i];
          if (hero->playerid==controller->playerid) {
            adv_sprite_create(0,11,hero->hdr.x,hero->hdr.y); // detachhighlight
            hero->playerid=0;
            break;
          }
        }
        adv_input_remove_player(controller->playerid);
      }
      return;
    }
    if (controller->state&btn) return;
    controller->state|=btn;
    if ((controller->playerid>=1)&&(controller->playerid<=ADV_PLAYER_LIMIT)) {
      adv_inputs[controller->playerid]=controller->state;
    } else {
      adv_controller_clickin(controller);
    }
  } else {
    if (!(controller->state&btn)) return;
    controller->state&=~btn;
    if ((controller->playerid>=1)&&(controller->playerid<=ADV_PLAYER_LIMIT)) {
      adv_inputs[controller->playerid]=controller->state;
    }
  }
}

/* event
 *****************************************************************************/
 
int adv_controller_event(struct adv_controller *controller,int type,int code,int value) {
  if (!controller) return -1;
  switch (type) {
  
    case EV_ABS: {
        int p=adv_controller_absmap_search(controller,code);
        if (p<0) return 0;
        struct adv_absmap *map=controller->absmapv+p;
        if (value>=map->hi) {
          adv_controller_set_btn(controller,map->btnlo,0);
          adv_controller_set_btn(controller,map->btnhi,1);
        } else if (map->btnlo&&(value<=map->lo)) {
          adv_controller_set_btn(controller,map->btnlo,1);
          adv_controller_set_btn(controller,map->btnhi,0);
        } else {
          adv_controller_set_btn(controller,map->btnlo,0);
          adv_controller_set_btn(controller,map->btnhi,0);
        }
      } break;
      
    case EV_KEY: {
        int p=adv_controller_keymap_search(controller,code);
        if (p<0) return 0;
        adv_controller_set_btn(controller,controller->keymapv[p].btn,value);
      } break;
      
  }
  return 0;
}
