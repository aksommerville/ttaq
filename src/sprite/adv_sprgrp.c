#include "adv.h"
#include "adv_sprite.h"

/* sprite comparator
 *****************************************************************************/
 
static int _adv_sprcmp(struct adv_sprite *a,struct adv_sprite *b) {
  if (a->depth<b->depth) return -1;
  if (a->depth>b->depth) return 1;
  if (a->y<b->y) return -1;
  if (a->y>b->y) return 1;
  return 0;
}

/* new
 *****************************************************************************/
 
int adv_sprgrp_new(struct adv_sprgrp **dst) {
  if (!dst) return -1;
  if (!(*dst=calloc(1,sizeof(struct adv_sprgrp)))) return -1;
  (*dst)->refc=1;
  return 0;
}

/* del
 *****************************************************************************/
 
void adv_sprgrp_del(struct adv_sprgrp *grp) {
  if (!grp) return;
  if (grp->refc>1) { grp->refc--; return; }
  if (grp->sprv) free(grp->sprv);
  free(grp);
}

/* ref
 *****************************************************************************/
 
int adv_sprgrp_ref(struct adv_sprgrp *grp) {
  if (!grp) return -1;
  if (grp->refc<1) return -1;
  if (grp->refc==INT_MAX) return -1;
  grp->refc++;
  return 0;
}

/* group list in sprite, private
 *****************************************************************************/

static int _adv_sprite_grp_search(struct adv_sprite *spr,struct adv_sprgrp *grp) {
  int lo=0,hi=spr->grpc;
  while (lo<hi) {
    int ck=(lo+hi)>>1;
         if (grp<spr->grpv[ck]) hi=ck;
    else if (grp>spr->grpv[ck]) lo=ck+1;
    else return ck;
  }
  return -lo-1;
}

static int _adv_sprite_grp_insert(struct adv_sprite *spr,int p,struct adv_sprgrp *grp) {
  if ((p<0)||(p>spr->grpc)) return -1;
  if (p&&(grp<=spr->grpv[p-1])) return -1;
  if ((p<spr->grpc)&&(grp>=spr->grpv[p])) return -1;
  if (spr->grpc>=spr->grpa) {
    int na=spr->grpa+4;
    if (na>INT_MAX/sizeof(void*)) return -1;
    void *nv=realloc(spr->grpv,sizeof(void*)*na);
    if (!nv) return -1;
    spr->grpv=nv;
    spr->grpa=na;
  }
  if (adv_sprgrp_ref(grp)<0) return -1;
  memmove(spr->grpv+p+1,spr->grpv+p,sizeof(void*)*(spr->grpc-p));
  spr->grpc++;
  spr->grpv[p]=grp;
  return 0;
}

static void _adv_sprite_grp_remove(struct adv_sprite *spr,int p) {
  if ((p<0)||(p>=spr->grpc)) return;
  adv_sprgrp_del(spr->grpv[p]);
  spr->grpc--;
  memmove(spr->grpv+p,spr->grpv+p+1,sizeof(void*)*(spr->grpc-p));
}

/* sprite list in group, private
 *****************************************************************************/

static int _adv_sprgrp_spr_search(struct adv_sprgrp *grp,struct adv_sprite *spr) {
  int before=-1;
  int i; for (i=0;i<grp->sprc;i++) {
    if (grp->sprv[i]==spr) return i;
    if (grp->sort_insertions&&(before<0)) {
      if (_adv_sprcmp(spr,grp->sprv[i])<0) before=i;
    }
  }
  if (before<0) return -grp->sprc-1;
  return -before-1;
}

static int _adv_sprgrp_spr_insert(struct adv_sprgrp *grp,int p,struct adv_sprite *spr) {
  if ((p<0)||(p>grp->sprc)) return -1;
  if (grp->sprc>=grp->spra) {
    int na=grp->spra+8;
    if (na>INT_MAX/sizeof(void*)) return -1;
    void *nv=realloc(grp->sprv,sizeof(void*)*na);
    if (!nv) return -1;
    grp->sprv=nv;
    grp->spra=na;
  }
  if (adv_sprite_ref(spr)<0) return -1;
  memmove(grp->sprv+p+1,grp->sprv+p,sizeof(void*)*(grp->sprc-p));
  grp->sprc++;
  grp->sprv[p]=spr;
  return 0;
}

static void _adv_sprgrp_spr_remove(struct adv_sprgrp *grp,int p) {
  if ((p<0)||(p>=grp->sprc)) return;
  adv_sprite_del(grp->sprv[p]);
  grp->sprc--;
  memmove(grp->sprv+p,grp->sprv+p+1,sizeof(void*)*(grp->sprc-p));
}

/* simple membership ops, public interface
 *****************************************************************************/

int adv_sprgrp_has(struct adv_sprgrp *grp,struct adv_sprite *spr) {
  if (!grp||!spr) return 0;
  if (_adv_sprite_grp_search(spr,grp)>=0) return 1;
  return 0;
}

int adv_sprgrp_insert(struct adv_sprgrp *grp,struct adv_sprite *spr) {
  if (!grp||!spr) return -1;
  int grpp=_adv_sprite_grp_search(spr,grp);
  if (grpp>=0) return 0; grpp=-grpp-1;
  if (_adv_sprite_grp_insert(spr,grpp,grp)<0) return -1;
  int sprp=grp->sprc;
  if (grp->sort_insertions) {
    if ((sprp=_adv_sprgrp_spr_search(grp,spr))>=0) { _adv_sprite_grp_remove(spr,grpp); return -1; }
    sprp=-sprp-1;
  }
  if (_adv_sprgrp_spr_insert(grp,sprp,spr)<0) { _adv_sprite_grp_remove(spr,grpp); return -1; }
  return 1;
}

int adv_sprgrp_remove(struct adv_sprgrp *grp,struct adv_sprite *spr) {
  if (!grp||!spr) return -1;
  int grpp=_adv_sprite_grp_search(spr,grp);
  if (grpp<0) return 0;
  if (adv_sprite_ref(spr)<0) return -1;
  if (adv_sprgrp_ref(grp)<0) { adv_sprite_del(spr); return -1; }
  _adv_sprite_grp_remove(spr,grpp);
  int sprp=_adv_sprgrp_spr_search(grp,spr);
  if (sprp>=0) _adv_sprgrp_spr_remove(grp,sprp);
  adv_sprite_del(spr);
  adv_sprgrp_del(grp);
  return 0;
}

/* compound removal ops, public
 *****************************************************************************/
 
int adv_sprite_kill(struct adv_sprite *spr) {
  if (!spr) return -1;
  if (spr->grpc<1) return 0;
  if (adv_sprite_ref(spr)<0) return -1;
  while (spr->grpc-->0) {
    struct adv_sprgrp *grp=spr->grpv[spr->grpc];
    int sprp=_adv_sprgrp_spr_search(grp,spr);
    if (sprp>=0) _adv_sprgrp_spr_remove(grp,sprp);
    adv_sprgrp_del(grp);
  }
  spr->grpc=0;
  adv_sprite_del(spr);
  return 0;
}

int adv_sprgrp_clear(struct adv_sprgrp *grp) {
  if (!grp) return -1;
  if (grp->sprc<1) return 0;
  if (adv_sprgrp_ref(grp)<0) return -1;
  while (grp->sprc-->0) {
    struct adv_sprite *spr=grp->sprv[grp->sprc];
    int grpp=_adv_sprite_grp_search(spr,grp);
    if (grpp>=0) _adv_sprite_grp_remove(spr,grpp);
    adv_sprite_del(spr);
  }
  grp->sprc=0;
  adv_sprgrp_del(grp);
  return 0;
}

int adv_sprgrp_kill(struct adv_sprgrp *grp) {
  if (!grp) return -1;
  if (grp->sprc<1) return 0;
  if (adv_sprgrp_ref(grp)<0) return -1;
  while (grp->sprc-->0) {
    struct adv_sprite *spr=grp->sprv[grp->sprc];
    int grpp=_adv_sprite_grp_search(spr,grp);
    if (grpp>=0) _adv_sprite_grp_remove(spr,grpp);
    adv_sprite_kill(spr);
    adv_sprite_del(spr);
  }
  grp->sprc=0;
  adv_sprgrp_del(grp);
  return 0;
}

/* sort
 *****************************************************************************/
 
int adv_sprgrp_sort_by_depth(struct adv_sprgrp *grp) {
  if (!grp) return -1;
  if (grp->sprc<2) return 0;
  if (grp->sort_direction==1) grp->sort_direction=-1;
  else grp->sort_direction=1;
  int first,last,i;
  if (grp->sort_direction==1) {
    first=0; last=grp->sprc-1;
  } else {
    first=grp->sprc-1; last=0;
  }
  for (i=first;i!=last;i+=grp->sort_direction) {
    if (_adv_sprcmp(grp->sprv[i],grp->sprv[i+grp->sort_direction])==grp->sort_direction) {
      struct adv_sprite *tmp=grp->sprv[i];
      grp->sprv[i]=grp->sprv[i+grp->sort_direction];
      grp->sprv[i+grp->sort_direction]=tmp;
    }
  }
  return 0;
}

/* global groups
 *****************************************************************************/

#define ADV_GRPACTION(tag) struct adv_sprgrp *adv_sprgrp_##tag=0;
ADV_FOR_EACH_GROUP
#undef ADV_GRPACTION

int adv_sprgrp_init() {
  int err;
  #define ADV_GRPACTION(tag) if ((err=adv_sprgrp_new(&adv_sprgrp_##tag))<0) return err;
  ADV_FOR_EACH_GROUP
  #undef ADV_GRPACTION
  adv_sprgrp_visible->sort_insertions=1;
  return 0;
}

void adv_sprgrp_quit() {
  #define ADV_GRPACTION(tag) adv_sprgrp_clear(adv_sprgrp_##tag); adv_sprgrp_del(adv_sprgrp_##tag); adv_sprgrp_##tag=0;
  ADV_FOR_EACH_GROUP
  #undef ADV_GRPACTION
}
