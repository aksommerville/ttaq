#include "adv_res_internal.h"

/* private list primitives
 *****************************************************************************/

static int adv_res_sprdef_search(int sprdefid) {
  int lo=0,hi=adv_resmgr.sprdefc;
  while (lo<hi) {
    int ck=(lo+hi)>>1;
         if (sprdefid<adv_resmgr.sprdefv[ck]->sprdefid) hi=ck;
    else if (sprdefid>adv_resmgr.sprdefv[ck]->sprdefid) lo=ck+1;
    else return ck;
  }
  return -lo-1;
}

// handoff
static int adv_res_sprdef_insert(int p,struct adv_sprdef *sprdef) {
  if ((p<0)||(p>adv_resmgr.sprdefc)||!sprdef) return -1;
  if (p&&(sprdef->sprdefid<=adv_resmgr.sprdefv[p-1]->sprdefid)) return -1;
  if ((p<adv_resmgr.sprdefc)&&(sprdef->sprdefid>=adv_resmgr.sprdefv[p]->sprdefid)) return -1;
  if (adv_resmgr.sprdefc>=adv_resmgr.sprdefa) {
    int na=adv_resmgr.sprdefa+8;
    if (na>INT_MAX/sizeof(void*)) return -1;
    void *nv=realloc(adv_resmgr.sprdefv,sizeof(void*)*na);
    if (!nv) return -1;
    adv_resmgr.sprdefv=nv;
    adv_resmgr.sprdefa=na;
  }
  memmove(adv_resmgr.sprdefv+p+1,adv_resmgr.sprdefv+p,sizeof(void*)*(adv_resmgr.sprdefc-p));
  adv_resmgr.sprdefc++;
  adv_resmgr.sprdefv[p]=sprdef;
  return 0;
}

/* load (single)
 *****************************************************************************/

static int adv_res_load_sprdefs_1(int p,int id,const char *path) {
  struct adv_sprdef *sprdef=0;
  if (adv_sprdef_new(&sprdef)<0) return -1;
  sprdef->sprdefid=id;
  if (adv_sprdef_decode_file(sprdef,path)<0) { adv_sprdef_del(sprdef); return -1; }
  if (adv_res_sprdef_insert(p,sprdef)<0) { adv_sprdef_del(sprdef); return -1; }
  return 0;
}

/* load (init)
 *****************************************************************************/

int adv_res_load_sprdefs() {
  char subpath[1024];
  if (adv_resmgr.rootc>=sizeof(subpath)-7) return -1;
  memcpy(subpath,adv_resmgr.root,adv_resmgr.rootc);
  memcpy(subpath+adv_resmgr.rootc,"/sprite",7);
  int subpathc=adv_resmgr.rootc+7;
  subpath[subpathc]=0;
  DIR *dir=opendir(subpath);
  if (!dir) {
    fprintf(stderr,"ERROR: Failed to open sprites directory '%s'\n",subpath);
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
    int p=adv_res_sprdef_search(id);
    if (p>=0) {
      fprintf(stderr,"WARNING: Duplicate sprite ID %d. Ignoring file '%s'.\n",id,base);
      continue;
    }
    if (adv_res_load_sprdefs_1(-p-1,id,subpath)<0) { closedir(dir); return -1; }
  }
  closedir(dir);

  // These sprites are absolutely required, so assert them right away:
  if (adv_res_sprdef_search(1)<0) { fprintf(stderr,"ERROR: sprite #1 (swordsman) not found\n"); return -1; }
  if (adv_res_sprdef_search(2)<0) { fprintf(stderr,"ERROR: sprite #2 (archer) not found\n"); return -1; }
  if (adv_res_sprdef_search(3)<0) { fprintf(stderr,"ERROR: sprite #3 (gadgeteer) not found\n"); return -1; }
  if (adv_res_sprdef_search(4)<0) { fprintf(stderr,"ERROR: sprite #4 (wizard) not found\n"); return -1; }

  return 0;
}

/* public accessor
 *****************************************************************************/
 
struct adv_sprdef *adv_res_get_sprdef(int sprdefid) {
  int p=adv_res_sprdef_search(sprdefid);
  if (p<0) return 0;
  return adv_resmgr.sprdefv[p];
}
