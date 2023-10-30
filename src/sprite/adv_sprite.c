#include "adv.h"
#include "adv_sprite.h"
#include "adv_sprdef.h"
#include "adv_sprclass.h"
#include "res/adv_res.h"
#include "video/adv_video.h"

/* new
 *****************************************************************************/
 
int adv_sprite_new(struct adv_sprite **dst) {
  if (!dst) return -1;
  if (!(*dst=calloc(1,sizeof(struct adv_sprite)))) return -1;
  (*dst)->refc=1;
  return 0;
}

/* del
 *****************************************************************************/
 
void adv_sprite_del(struct adv_sprite *spr) {
  if (!spr) return;
  if (spr->refc>1) { spr->refc--; return; }
  if (spr->sprclass&&spr->sprclass->del) spr->sprclass->del(spr);
  if (spr->grpv) free(spr->grpv);
  free(spr);
}

/* ref
 *****************************************************************************/
 
int adv_sprite_ref(struct adv_sprite *spr) {
  if (!spr) return -1;
  if (spr->refc<1) return -1;
  if (spr->refc==INT_MAX) return -1;
  spr->refc++;
  return 0;
}

/* high-level construction
 *****************************************************************************/
 
int adv_sprite_create(struct adv_sprite **dst_WEAK,int sprdefid,int x,int y) {
  const struct adv_sprdef *sprdef=adv_res_get_sprdef(sprdefid);
  if (!sprdef) {
    fprintf(stderr,"ERROR: Sprite #%d not found\n",sprdefid);
    return -1;
  }
  struct adv_sprite *spr=0;
  if (adv_sprite_instantiate(&spr,sprdef->sprclass)<0) return -1;
  spr->x=x;
  spr->y=y;
  spr->tileid=sprdef->tileid;
  spr->sprdefid=sprdefid;
  spr->sprdef=sprdef;
  if (spr->sprclass&&spr->sprclass->postinit&&(spr->sprclass->postinit(spr)<0)) { adv_sprite_kill(spr); return -1; }
  if (dst_WEAK) *dst_WEAK=spr;
  return 0;
}

int adv_sprite_instantiate(struct adv_sprite **dst_WEAK,const struct adv_sprclass *sprclass) {
  struct adv_sprite *spr=0;
  if (sprclass) {
    if (sprclass->objlen<(int)sizeof(struct adv_sprite)) return -1;
    if (!(spr=calloc(1,sprclass->objlen))) return -1;
    spr->refc=1;
    spr->sprclass=sprclass;
    spr->collide_edges=sprclass->collide_edges;
    spr->collide_map_positive=sprclass->collide_map_positive;
    spr->collide_map_negative=sprclass->collide_map_negative;
    spr->collide_solids=sprclass->collide_solids;
  } else {
    if (adv_sprite_new(&spr)<0) return -1;
  }
  if (adv_sprgrp_insert(adv_sprgrp_keepalive,spr)<0) { adv_sprite_del(spr); return -1; }
  adv_sprite_del(spr);
  if (dst_WEAK) *dst_WEAK=spr;

  if (adv_sprgrp_insert(adv_sprgrp_visible,spr)<0) { adv_sprite_kill(spr); return -1; }
  if (sprclass) { // Add to more groups depending on class properties...
    if (sprclass->update&&(adv_sprgrp_insert(adv_sprgrp_update,spr)<0)) { adv_sprite_kill(spr); return -1; }
    if (sprclass->hurt&&(adv_sprgrp_insert(adv_sprgrp_fragile,spr)<0)) { adv_sprite_kill(spr); return -1; }
    if (sprclass->collide_solids&&(adv_sprgrp_insert(adv_sprgrp_solid,spr)<0)) { adv_sprite_kill(spr); return -1; }
  }
  
  if (sprclass&&sprclass->init&&(sprclass->init(spr)<0)) { adv_sprite_kill(spr); return -1; }
  return 0;
}
