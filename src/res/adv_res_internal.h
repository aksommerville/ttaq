#ifndef ADV_RES_INTERNAL_H
#define ADV_RES_INTERNAL_H

#include "adv.h"
#include "adv_res.h"
#include "game/adv_map.h"
#include "sprite/adv_sprdef.h"
#include "video/adv_video.h"
#include <sys/stat.h>
#include <dirent.h>

struct adv_tilesheet {
  int id;
  char *path;
  unsigned char propv[256];
};

extern struct adv_resmgr {

  // Path to data.
  char *root; int rootc;

  // Available tilesheets, sorted by id.
  struct adv_tilesheet *tilesheetv; int tilesheetc,tilesheeta;

  // Current loaded tilesheets.
  int bgimageid;
  int sprimageid;
  void *bgimage;

  // Available maps, sorted by id.
  struct adv_map **mapv; int mapc,mapa;

  // Available sprite definitions, sorted by id.
  struct adv_sprdef **sprdefv; int sprdefc,sprdefa;
  
} adv_resmgr;

int adv_res_tilesheet_search(int id);
int adv_res_tilesheet_insert(int p,int id,const char *path,int pathc);

int adv_res_load_maps();
int adv_res_load_sprdefs();
int adv_res_load_songs();

#endif
