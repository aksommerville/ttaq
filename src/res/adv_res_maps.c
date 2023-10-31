#include "adv_res_internal.h"
#include "sprite/adv_sprite.h"
#include "sprite/class/adv_sprite_hero.h"
#include "png/png.h"

/* map list primitives
 *****************************************************************************/

static int adv_res_map_search(int mapid) {
  int lo=0,hi=adv_resmgr.mapc;
  while (lo<hi) {
    int ck=(lo+hi)>>1;
         if (mapid<adv_resmgr.mapv[ck]->mapid) hi=ck;
    else if (mapid>adv_resmgr.mapv[ck]->mapid) lo=ck+1;
    else return ck;
  }
  return -lo-1;
}

// handoff
static int adv_res_map_insert(int p,struct adv_map *map) {
  if ((p<0)||(p>adv_resmgr.mapc)||!map) return -1;
  if (p&&(map->mapid<=adv_resmgr.mapv[p-1]->mapid)) return -1;
  if ((p<adv_resmgr.mapc)&&(map->mapid>=adv_resmgr.mapv[p]->mapid)) return -1;
  if (adv_resmgr.mapc>=adv_resmgr.mapa) {
    int na=adv_resmgr.mapa+16;
    if (na>INT_MAX/sizeof(void*)) return -1;
    void *nv=realloc(adv_resmgr.mapv,sizeof(void*)*na);
    if (!nv) return -1;
    adv_resmgr.mapv=nv;
    adv_resmgr.mapa=na;
  }
  memmove(adv_resmgr.mapv+p+1,adv_resmgr.mapv+p,sizeof(void*)*(adv_resmgr.mapc-p));
  adv_resmgr.mapc++;
  adv_resmgr.mapv[p]=map;
  return 0;
}

/* install one map
 *****************************************************************************/

static int adv_res_load_maps_1(int p,int id,const char *path) {
  struct adv_map *map=0;
  if (adv_map_new(&map)<0) return -1;
  map->mapid=id;
  if (adv_map_decode_file(map,path)<0) { adv_map_del(map); return -1; }
  if (adv_res_map_insert(p,map)<0) { adv_map_del(map); return -1; }
  int tsp=adv_res_tilesheet_search(map->bg);
  if (tsp>=0) map->propv=adv_resmgr.tilesheetv[tsp].propv;
  return 0;
}

/* load all maps at startup
 *****************************************************************************/

int adv_res_load_maps() {
  char subpath[1024];
  if (adv_resmgr.rootc>=sizeof(subpath)-4) return -1;
  memcpy(subpath,adv_resmgr.root,adv_resmgr.rootc);
  memcpy(subpath+adv_resmgr.rootc,"/map",4);
  int subpathc=adv_resmgr.rootc+4;
  subpath[subpathc]=0;
  DIR *dir=opendir(subpath);
  if (!dir) {
    fprintf(stderr,"ERROR: Failed to open maps directory '%s'\n",subpath);
    return -1;
  }
  subpath[subpathc++]='/';
  struct dirent *de;
  while (de=readdir(dir)) {
    if (de->d_type!=DT_REG) continue;
    const char *base=de->d_name;
    if ((base[0]<'0')||(base[0]>'9')) continue;
    int basec=0,id=0,done=0; while (base[basec]) {
      if ((base[basec]<'0')||(base[basec]>'9')) done=1;
      else if (!done) {
        int digit=base[basec]-'0';
        if (id>INT_MAX/10) id=INT_MAX; else id*=10;
        if (id>INT_MAX-digit) id=INT_MAX; else id+=digit;
      }
      basec++;
    }
    if (subpathc>=sizeof(subpath)-basec) continue;
    memcpy(subpath+subpathc,base,basec+1);
    int p=adv_res_map_search(id);
    if (p>=0) {
      fprintf(stderr,"WARNING: Duplicate map ID %d. Ignoring file '%s'.\n",id,base);
      continue;
    }
    if (adv_res_load_maps_1(-p-1,id,subpath)<0) { closedir(dir); return -1; }
  }
  closedir(dir);
  if (adv_resmgr.mapc<1) {
    fprintf(stderr,"ERROR: maps directory '%.*s' doesn't contain any maps!\n",subpathc,subpath);
    return -1;
  }
  return 0;
}

/* helper: load PNG file
 *****************************************************************************/

static void *_loadtilesheet(int id,int colortype) {
  int p=adv_res_tilesheet_search(id);
  if (p<0) {
    fprintf(stderr,"ERROR: tilesheet #%d not found\n",id);
    return 0;
  }
  const char *path=adv_resmgr.tilesheetv[p].path;
  void *serial=0;
  int serialc=adv_file_read(&serial,path);
  if (serialc<0) {
    fprintf(stderr,"%s: Failed to read file\n",path);
    return 0;
  }
  struct png_image *image=png_decode(serial,serialc);
  free(serial);
  // TODO: 2023-10-30: As I found this code, it converted to the desired colortype rather than asserting.
  // Does it matter? Is there an image we might load both with and without alpha?
  if (!image||(image->depth!=8)||(image->colortype!=colortype)) {
    if (image) fprintf(stderr,"%s: Expected (depth,colortype)=(8,%d), found (%d,%d)\n",path,colortype,image->depth,image->colortype);
    else fprintf(stderr,"%s: Failed to decode %d bytes of PNG\n",path,serialc);
    png_image_del(image);
    return 0;
  }
  void *rtn=image->pixels;
  image->pixels=0;
  png_image_del(image);
  return rtn;
}

/* load graphics for new map
 *****************************************************************************/
 
static int _adv_res_load_map_graphics(struct adv_map *map) {

  /* The new background image must be pushed no matter what.
   * Avoid loading the palettes if they haven't changed.
   */
  if (map->bg==adv_resmgr.bgimageid) {
    if (!adv_resmgr.bgimage&&!(adv_resmgr.bgimage=calloc((ADV_TILE_W<<4)*3,ADV_TILE_H<<4))) return -1;
  } else {
    void *pixels=_loadtilesheet(map->bg,2);
    if (!pixels) return -1;
    if (adv_resmgr.bgimage) free(adv_resmgr.bgimage);
    adv_resmgr.bgimage=pixels;
  }
  if (adv_video_draw_bg(map->cellv,adv_resmgr.bgimage)<0) return -1;

  /* The new spritesheet only needs to be loaded if it changed. */
  if (map->spr!=adv_resmgr.sprimageid) {
    void *pixels=_loadtilesheet(map->spr,6);
    if (!pixels) return -1;
    int err=adv_video_set_spritesheet(pixels);
    free(pixels);
    if (err<0) return err;
  }
  
  return 0;
}

/* replace sprites for map transition
 *****************************************************************************/
 
static int _adv_res_replace_sprites(struct adv_map *frommap,struct adv_map *tomap) {
  // CAUTION: (frommap) will be NULL the first time through.
  int i;
  
  /* Remove heroes from keepalive, then flush keepalive. */
  if (frommap) {
    for (i=0;i<adv_sprgrp_hero->sprc;i++) {
      adv_sprgrp_remove(adv_sprgrp_keepalive,adv_sprgrp_hero->sprv[i]);
    }
  }
  adv_sprgrp_kill(adv_sprgrp_keepalive);
  
  /* Create all the sprites listed in tomap's static spawn list. */
  for (i=0;i<tomap->spawnc;i++) {
    struct adv_spawn *spawn=tomap->spawnv+i;
    if (adv_sprite_create(
      0,spawn->sprdefid,
      spawn->col*ADV_TILE_W+(ADV_TILE_W>>1),spawn->row*ADV_TILE_H+(ADV_TILE_H>>1
    ))<0) return -1;
  }
  
  /* Put the heroes back in keepalive and adjust their positions, or create them fresh. */
  if (frommap&&(frommap!=tomap)&&(adv_sprgrp_hero->sprc>0)) {
    // All transitions except the first. Move existing hero sprites...
    int dx=0,dy=0,ax=0,ay=0,abscoord=0;
    if (tomap->mapid==frommap->north) dy=ADV_SCREEN_H+ADV_TILE_H;
    else if (tomap->mapid==frommap->south) dy=-ADV_SCREEN_H-ADV_TILE_H;
    else if (tomap->mapid==frommap->east) dx=-ADV_SCREEN_W-ADV_TILE_W;
    else if (tomap->mapid==frommap->west) dx=ADV_SCREEN_W+ADV_TILE_W;
    else {
      abscoord=1;
      int heroi; for (heroi=0;heroi<adv_sprgrp_hero->sprc;heroi++) {
        struct adv_sprite *hero=adv_sprgrp_hero->sprv[heroi];
        int col=hero->x/ADV_TILE_W,row=hero->y/ADV_TILE_H;
        int ok=0;
        int doori; for (doori=0;doori<frommap->doorc;doori++) {
          struct adv_door *door=frommap->doorv+doori;
          if ((col!=door->srccol)||(row!=door->srcrow)) continue;
          hero->x=door->dstcol*ADV_TILE_W+(ADV_TILE_W>>1);
          hero->y=door->dstrow*ADV_TILE_H+(ADV_TILE_H>>1);
          ok=1;
          break;
        }
        if (!ok) fprintf(stderr,"Panic! Transition from map %d to %d, hero was at cell (%d,%d), no destination cell found!\n",frommap->mapid,tomap->mapid,col,row);
      }
    }
    for (i=0;i<adv_sprgrp_hero->sprc;i++) {
      struct adv_sprite *spr=adv_sprgrp_hero->sprv[i];
      struct adv_sprite_hero *SPR=(struct adv_sprite_hero*)spr;
      if (adv_sprgrp_insert(adv_sprgrp_keepalive,spr)<0) return -1;
      adv_sprgrp_insert(adv_sprgrp_visible,spr);
      if (!SPR->ghost) {
        adv_sprgrp_insert(adv_sprgrp_solid,spr);
        adv_sprgrp_insert(adv_sprgrp_fragile,spr);
      }
      if (!abscoord) {
        spr->x+=dx;
        spr->y+=dy;
      }
      SPR->offscreen=1;
      SPR->offscreen_target=frommap->mapid;
      SPR->flagdir^=1;
    }
  } else {
    // Initial load. Create hero sprites...
    struct adv_sprite *sprv[4]={0};
    if (adv_sprite_create(sprv+0,1,(ADV_SCREEN_W>>1)-(ADV_TILE_W>>1),(ADV_SCREEN_H>>1)-(ADV_TILE_W>>1))<0) return -1;
    if (adv_sprite_create(sprv+1,2,(ADV_SCREEN_W>>1)+(ADV_TILE_W>>1),(ADV_SCREEN_H>>1)-(ADV_TILE_W>>1))<0) return -1;
    if (adv_sprite_create(sprv+2,3,(ADV_SCREEN_W>>1)-(ADV_TILE_W>>1),(ADV_SCREEN_H>>1)+(ADV_TILE_W>>1))<0) return -1;
    if (adv_sprite_create(sprv+3,4,(ADV_SCREEN_W>>1)+(ADV_TILE_W>>1),(ADV_SCREEN_H>>1)+(ADV_TILE_W>>1))<0) return -1;
  }
  
  return 0;
}

/* miscellaneous new-map setup
 *****************************************************************************/
 
static int _adv_res_load_map_misc(struct adv_map *map) {
 
  /* lights-out effect */
  if (map->lights_out) {
    adv_video_lights_out(2,0,0);
  } else {
    adv_video_lights_out(0,0,0);
  }
  
  return 0;
}

/* change maps (private)
 *****************************************************************************/
 
static int _adv_res_change_maps(struct adv_map *frommap,struct adv_map *tomap) {
  // CAUTION: (frommap) will be NULL the first time through.
  if (adv_map_update_switches(tomap,0)<0) return -1;
  if (_adv_res_load_map_graphics(tomap)<0) return -1;
  if (_adv_res_replace_sprites(frommap,tomap)<0) return -1;
  if (_adv_res_load_map_misc(tomap)<0) return -1;
  if (adv_song(tomap->song)<0) return -1;
  return 0;
}

/* Replace the global shared map.
 * This does all the bells and whistles associated with the change.
 * Loading sprites, moving the heroes around, you name it...
 *****************************************************************************/

int adv_res_load_map(int mapid) {

  /* Find the resource. */
  if (adv_map) {
    if (adv_map->mapid==mapid) return 0;
    if (adv_map->suspend_xfer) return 0;
  }
  int p=adv_res_map_search(mapid);
  if (p<0) {
    fprintf(stderr,"ERROR: map #%d not found\n",mapid);
    return -1;
  }
  if (_adv_res_change_maps(adv_map,adv_resmgr.mapv[p])<0) return -1;
  adv_map=adv_resmgr.mapv[p];
  adv_map->suspend_xfer=1;
  
  return 0;
}

int adv_res_reload_map() {
  if (!adv_map) return -1;
  if (_adv_res_change_maps(adv_map,adv_map)<0) return -1;
  adv_map->suspend_xfer=1;
  return 0;
}

/* XXX Re-encode all maps
 *****************************************************************************/
 
static int adv_res_convert_map_1(struct adv_map *map) {
  if (!map) return -1;
  char path[1024];
  int pathc=snprintf(path,sizeof(path),"encoded-maps/%d",map->mapid);
  if ((pathc<1)||(pathc>=sizeof(path))) return -1;
  char buf[8192];
  int bufc=adv_map_encode_binary(buf,sizeof(buf),map);
  if (bufc<0) return -1;
  if (bufc>sizeof(buf)) {
    fprintf(stderr,"%s: map encodes to %d (larger than our buffer, and this is probably an encoder error)\n",path,bufc);
    return -1;
  }
  return adv_file_write(path,buf,bufc);
}
 
int adv_res_convert_maps() {
  printf("Dumping all maps in binary form...\n");
  int i; for (i=0;i<adv_resmgr.mapc;i++) {
    if (adv_res_convert_map_1(adv_resmgr.mapv[i])<0) return -1;
  }
  printf("...done dumping maps.\n");
  return 0;
}

/* replace spritesheet (public)
 *****************************************************************************/
 
int adv_res_replace_spritesheet(int imgid) {
  if (imgid==adv_resmgr.sprimageid) return 0;
  void *pixels=_loadtilesheet(imgid,6);
  if (!pixels) return -1;
  int err=adv_video_set_spritesheet(pixels);
  free(pixels);
  if (err<0) return err;
  return 0;
}
